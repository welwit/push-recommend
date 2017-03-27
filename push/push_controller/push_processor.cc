// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/push_processor.h"

#include "base/file/proto_util.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"

DECLARE_bool(use_cluster_mode);
DECLARE_string(mysql_config);
DECLARE_string(burypoint_kafka_log_server);
DECLARE_string(burypoint_kafka_topic);
DECLARE_string(burypoint_upload_push_log_event_type);
DECLARE_string(burypoint_upload_fail_log_event_type);

namespace {

static const string kTimeFormat = "%Y-%m-%d %H:%M:%S";

static const char kQueryFormat[] =
    "SELECT id, user_id, business_type, business_time, order_detail, "
    "updated, order_status, finished_time "
    "FROM user_order_info WHERE id = '%s';";

static const char kQueryFormatV2[] =
    "SELECT id, user_id, business_type, business_time, order_detail, "
    "updated, order_status, finished_time, fingerprint_id "
    "FROM user_order_info WHERE id = '%s';";

static const char kNameBusinessFlight[] = "flight";
static const char kNameBusinessMovie[] = "movie";
static const char kNameBusinessTrain[] = "train";
static const char kNameBusinessHotel[] = "hotel";

static const char kNameEventLastday[] = "lastday";
static const char kNameEventToday[] = "today";
static const char kNameEvent3Hour[] = "3hour";
static const char kNameEvent1Hour[] = "1hour";
static const char kNameEventDepart[] = "depart";
static const char kNameEventShowtime[] = "showtime";
static const char kNameEvent30Min[] = "30min";
static const char kNameEventTakeoff[] = "takeoff";
static const char kNameEventArrive[] = "arrive";

}

namespace push_controller {

PushProcessor::PushProcessor() {
  VLOG(2) << "PushProcessor::PushProcessor()";
  Init();
}

PushProcessor::~PushProcessor() {}

void PushProcessor::PushToQueue(const PushEventInfo& push_event) {
  push_event_queue_->Push(push_event);
}

void PushProcessor::Run() {
  while (true) {
    PushEventInfo push_event;
    push_event_queue_->Pop(push_event);
    VLOG(2) << "Pop push event, event_id:" << push_event.id();
    if (!Process(&push_event)) {
      push_sender_->UpdatePushStatus(push_event, kPushFailed);
      UploadFailedPushDataToKafka(push_event);
      LOG(ERROR) << "Push Process failed, event_id:" << push_event.id();
      continue;
    }
    push_sender_->UpdatePushStatus(push_event, kPushSuccess);
    VLOG(2) << "Push Process success, event_id:" << push_event.id();
  }
}

void PushProcessor::Init() {
  business_key_map_ = {
    {kBusinessFlight, kNameBusinessFlight},
    {kBusinessMovie, kNameBusinessMovie},
    {kBusinessTrain, kNameBusinessTrain},
    {kBusinessHotel, kNameBusinessHotel}
  };
  event_key_map_ = {
    {kEventRemindTimelineLastDay, kNameEventLastday},
    {kEventRemindTimelineToday, kNameEventToday},
    {kEventRemindTimeline3Hour, kNameEvent3Hour},
    {kEventRemindTimeline1Hour, kNameEvent1Hour},
    {kEventRemindTimelineDepart, kNameEventDepart},
    {kEventRemindTimelineShowtime, kNameEventShowtime},
    {kEventRemindTimeline30Min, kNameEvent30Min},
    {kEventRemindTimelineTakeoff, kNameEventTakeoff},
    {kEventRemindTimelineArrive, kNameEventArrive}
  };
  mysql_server_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
  VLOG(2) << "Read mysqlConf success, file:" << FLAGS_mysql_config;
  push_sender_.reset(new PushSender());
  push_event_queue_.reset(new mobvoi::ConcurrentQueue<PushEventInfo>());
}

bool PushProcessor::GetUserOrder(const PushEventInfo& push_event,
                                 UserOrderInfo* user_order) {
  VLOG(2) << "PushProcessor::GetUserOrder(), event_id:" << push_event.id();
  try {
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection>
        connection(driver->connect(
            mysql_server_->host(),
            mysql_server_->user(),
            mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    string query;
    if (FLAGS_use_cluster_mode) {
      query = StringPrintf(kQueryFormatV2, push_event.order_id().c_str());
    } else {
      query = StringPrintf(kQueryFormat, push_event.order_id().c_str());
    }
    VLOG(2) << "Query command:" << query;
    std::unique_ptr<sql::ResultSet> result_set(statement->executeQuery(query));
    int result_cnt = 0;
    while (result_set->next()) {
      user_order->set_id(result_set->getString("id"));
      user_order->set_user_id(result_set->getString("user_id"));
      user_order->set_business_type(
          static_cast<BusinessType>(result_set->getInt("business_type")));
      user_order->set_order_detail(result_set->getString("order_detail"));
      user_order->set_order_status(
          static_cast<OrderStatus>(result_set->getInt("order_status")));
      time_t business_time, finished_time, updated_time;
      string business_time_string = (
          result_set->getString("business_time"));
      recommendation::DatetimeToTimestamp(
          business_time_string, &business_time, kTimeFormat);
      user_order->set_business_time(business_time);
      string updated_string = result_set->getString("updated");
      recommendation::DatetimeToTimestamp(
          updated_string, &updated_time, kTimeFormat);
      user_order->set_updated(updated_time);
      string finished_string = result_set->getString("finished_time");
      recommendation::DatetimeToTimestamp(
          finished_string, &finished_time, kTimeFormat);
      user_order->set_finished_time(finished_time);
      user_order->set_fingerprint_id(result_set->getInt64("fingerprint_id"));
      ++result_cnt;
      break;
    }
    if (result_cnt > 0) {
      return true;
    } else {
      LOG(WARNING) << "No user order info for event";
      return false;
    }
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "SQLException: " << e.what();
    return false;
  }
}

void PushProcessor::SetTimeline(const string& title,
                                const vector<string>& ticker_vec,
                                Json::Value& message) {
  message["timeline_detail"]["title"] = title;
  Json::Value ticker_list;
  for (auto& ticker : ticker_vec) {
    Json::Value ticker_value(ticker);
    ticker_list.append(ticker_value);
  }
  message["timeline_detail"]["ticker"] = ticker_list;
  message["timeline_detail"]["des"] = u8"出门问问智能推送";
}

void PushProcessor::SetPushDetail(const string& des,
                                  const string& position1,
                                  const string& position2,
                                  const string& position3,
                                  const string& position4,
                                  const string& ui_type,
                                  Json::Value& message) {
  time_t push_time = time(NULL);
  string push_time_string;
  recommendation::TimestampToDatetime(push_time, &push_time_string);
  message["is_push"] = "true";
  message["push_detail"]["push_time"] = push_time_string;
  message["push_detail"]["des"] = des;
  message["push_detail"]["position1"] = position1;
  message["push_detail"]["position2"] = position2;
  message["push_detail"]["position3"] = position3;
  message["push_detail"]["position4"] = position4;
  message["push_detail"]["ui_type"] = ui_type;
}

void PushProcessor::UploadPushDataToKafka(const UserOrderInfo& user_order,
                                          const PushEventInfo& push_event,
                                          const Json::Value& push_message) {
  time_t now = time(NULL);
  Json::Value data;
  data["timestamp"] = static_cast<uint32_t>(now);
  data["event"] = FLAGS_burypoint_upload_push_log_event_type;
  data["user_id"] = user_order.user_id();
  data["business_type"] = user_order.business_type();
  if (FLAGS_use_cluster_mode) {
    data["fingerprint_id"] =
      static_cast<Json::Int64>(user_order.fingerprint_id());
  }
  Json::Value properties;
  properties["order_id"] = user_order.id();
  properties["order_detail"] = user_order.order_detail();
  properties["business_time"] = user_order.business_time();
  properties["event_id"] = push_event.id();
  properties["event_type"] = push_event.event_type();
  properties["push_time"] = push_event.push_time();
  properties["business_key"] = push_event.business_key();
  properties["push_message"] = push_message;
  data["properties"] = properties;
  Json::Value post_request;
  post_request["topic"] = FLAGS_burypoint_kafka_topic;
  post_request["value"] = data;
  util::HttpClient http_client;
  http_client.SetHttpMethod(util::HttpMethod::kPost);
  string post_data = JsonToString(post_request);
  http_client.SetPostData(post_data);
  if (!http_client.FetchUrl(FLAGS_burypoint_kafka_log_server)) {
    LOG(WARNING) << "Failed to upload to kafka, response code:"
                 << http_client.response_code();
    return;
  }
  LOG(INFO) << "UPLOAD TO KAFKA, data:" << post_data;
}

void PushProcessor::UploadFailedPushDataToKafka(
    const PushEventInfo& push_event) {
  time_t now = time(NULL);
  Json::Value data;
  data["timestamp"] = static_cast<uint32_t>(now);
  data["event"] = FLAGS_burypoint_upload_fail_log_event_type;
  data["user_id"] = push_event.user_id();
  data["business_type"] = push_event.business_type();
  Json::Value properties;
  properties["order_id"] = push_event.order_id();
  properties["event_id"] = push_event.id();
  properties["event_type"] = push_event.event_type();
  properties["push_time"] = push_event.push_time();
  properties["business_key"] = push_event.business_key();
  properties["error_type"] = 0;
  properties["error_reason"] = "";
  data["properties"] = properties;
  Json::Value post_request;
  post_request["topic"] = FLAGS_burypoint_kafka_topic;
  post_request["value"] = data;
  util::HttpClient http_client;
  http_client.SetHttpMethod(util::HttpMethod::kPost);
  Json::FastWriter writer;
  string post_data = JsonToString(post_request);
  http_client.SetPostData(post_data);
  if (!http_client.FetchUrl(FLAGS_burypoint_kafka_log_server)) {
    LOG(WARNING) << "Failed to upload to kafka, response code: "
                 << http_client.response_code();
    return;
  }
  VLOG(2) << "UPLOAD ERROR TO KAFKA, data:" << post_data;
}

void PushProcessor::SetGeneralPushContent(const PushContent& content,
                                          Json::Value& message) {
  message["status"] = content.status;
  message["id"] = content.id;
  message["event_key"] = content.event_key;
  message["product_key"] = content.product_key;
  message["is_push"] = content.is_push;
  SetGeneralPushDetail(content.is_push, content.push_detail, message);
  SetGeneralSys(content.sys, message);
  SetGeneralTimeline(content.timeline_detail, message);
  SetGeneralDetail(content.detail, message);
}

void PushProcessor::SetGeneralPushDetail(bool is_push,
                                         const PushFields& push,
                                         Json::Value& message) {
  if (!is_push) {
    Json::Value null_val(Json::objectValue);
    message["push_detail"] = null_val;
    return;
  }
  message["push_detail"]["push_time"] = push.push_time;
  message["push_detail"]["position1"] = push.position1;
  message["push_detail"]["position2"] = push.position2;
  message["push_detail"]["position3"] = push.position3;
  message["push_detail"]["position4"] = push.position4;
  message["push_detail"]["des"] = push.des;
  message["push_detail"]["ui_type"] = push.ui_type;
}

void PushProcessor::SetGeneralSys(const SysFields& sys,
                                  Json::Value& message) {
  message["sys"]["expire_time"] = sys.expire_time;
}

void PushProcessor::SetGeneralTimeline(const TimelineFields& timeline,
                                       Json::Value& message) {
  message["timeline_detail"]["title"] = timeline.title;
  message["timeline_detail"]["des"] = timeline.des;
  Json::Value ticker;
  for (auto t : timeline.ticker) {
    ticker.append(t);
  }
  message["timeline_detail"]["ticker"] = ticker;
}

void PushProcessor::SetGeneralDetail(const DetailFields& detail,
                                     Json::Value& message) {
  message["detail"]["icon"] = detail.icon;
  message["detail"]["icon_code"] = detail.icon_code;
  message["detail"]["title"]["text"] = detail.title.text;
  message["detail"]["title"]["color"] = detail.title.color;
  message["detail"]["title"]["font"] = detail.title.font;
  message["detail"]["title"]["word_wrap"] = detail.title.word_wrap;
  message["detail"]["title"]["icon"] = detail.title.icon;
  message["detail"]["title"]["icon_code"] = detail.title.icon_code;
  message["detail"]["title"]["top_margin"] = detail.title.top_margin;
  message["detail"]["title"]["bottom_margin"] =
    detail.title.bottom_margin;
  message["detail"]["des"]["text"] = detail.des.text;
  message["detail"]["des"]["color"] = detail.des.color;
  message["detail"]["des"]["font"] = detail.des.font;
  message["detail"]["des"]["word_wrap"] = detail.des.word_wrap;
  message["detail"]["des"]["icon"] = detail.des.icon;
  message["detail"]["des"]["icon_code"] = detail.des.icon_code;
  message["detail"]["des"]["top_margin"] = detail.des.top_margin;
  message["detail"]["des"]["bottom_margin"] = detail.des.bottom_margin;
  for (auto text : detail.text_list) {
    Json::Value val;
    val["text"] = text.text;
    val["color"] = text.color;
    val["font"] = text.font;
    val["word_wrap"] = text.word_wrap;
    val["icon"] = text.icon;
    val["icon_code"] = text.icon_code;
    val["top_margin"] = text.top_margin;
    val["bottom_margin"] = text.bottom_margin;
    message["detail"]["text_list"].append(val);
  }
  for (auto link : detail.link_list) {
    Json::Value val;
    val["text"] = link.text;
    val["color"] = link.color;
    val["font"] = link.font;
    val["bg_color"] = link.bg_color;
    val["data"] = link.data;
    val["geo"] = link.geo;
    val["type"] = static_cast<int32>(link.type);
    val["id"] = link.id;
    message["detail"]["link_list"].append(val);
  }
}

bool PushProcessor::FilterVersion(const string& wear_version,
    const string& version_equal, const string& version_greater) {
  if (CheckVersionGreaterThan(wear_version, version_greater)) {
    return false;
  }
  vector<string> version_equal_to_vec;
  SplitString(version_equal, ',', &version_equal_to_vec);
  for (auto& version : version_equal_to_vec) {
    if (CheckVersionMatch2(wear_version, version)) {
      return false;
    }
  }
  return true;
}

}  // namespace push_controller
