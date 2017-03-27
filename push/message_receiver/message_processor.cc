// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/message_receiver/message_processor.h"

#include "base/hash.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"

#include "push/util/common_util.h"

DECLARE_bool(enabled_hotel);
DECLARE_bool(is_use_cluster_mode);
DECLARE_string(burypoint_kafka_topic);
DECLARE_string(burypoint_kafka_log_server);
DECLARE_string(burypoint_upload_log_event_type);
DECLARE_string(mysql_config);

namespace {

static const char kFlightTypePrefix[] = "flight";
static const char kMovieTypePrefix[] = "movie";
static const char kTrainTypePrefix[] = "train";
static const char kHotelTypePrefix[] = "hotel";

static const char kInsertFormat[] =
    "INSERT INTO %s (id, user_id, business_type, business_time, "
    "order_detail, order_status) "
    "VALUES('%s', '%s', '%d', FROM_UNIXTIME('%d'), '%s', '%d');";

static const char kInsertFormatV2[] =
    "INSERT INTO %s (id, user_id, business_type, business_time, "
    "order_detail, order_status, fingerprint_id) "
    "VALUES('%s', '%s', '%d', FROM_UNIXTIME('%d'), '%s', '%d', '%lu');";

static const char user_order_table[] = "user_order_info";

} // namespace

namespace message_receiver {

MsgProcessor::MsgProcessor() : shut_down_(false) {
  mysql_server_.reset(new MysqlServer());
  CHECK(mysql_server_.get());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
  VLOG(2) << "Init mysql conf from:" << FLAGS_mysql_config;
}

MsgProcessor::~MsgProcessor() {}

void MsgProcessor::ProcessQueue(
    shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue) {
  while (true) {
    if (shut_down_) {
      LOG(INFO) << "shut down msg processor";
      break;
    }
    string message;
    msg_queue->Pop(message);
    LOG(INFO) << "Received message:" << message;
    if (!Process(message)) {
      LOG(ERROR) << "Process message:" << message << " failed";
    }
    LOG(INFO) << "Process succeed message: " << message;
  }
}

void MsgProcessor::ShutDown() {
  shut_down_ = true;
}

bool MsgProcessor::ParseJsonMessage(const string& message) {
  VLOG(2) << "MsgProcessor::ParseJsonMessage()";
  Json::Value root;
  Json::Reader reader;
  try {
    if (!reader.parse(message, root)) {
      LOG(ERROR) << "Reader parse failed: "
                 << reader.getFormattedErrorMessages();
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "Reader parse exception: " << e.what();
    return false;
  }
  CHECK(!root.isNull() && root.isObject());
  const vector<string>& keys = root.getMemberNames();
  const string& key = keys[0];
  Json::Value value_list = root[key];
  Json::Value value;
  CHECK(value_list.isArray() && value_list.size() == 1);
  value = value_list[static_cast<Json::Value::ArrayIndex>(0)];
  LOG(INFO) << "DATA: " << push_controller::JsonToString(value);
  push_controller::UserOrderInfo user_order_info;
  if (StartsWithASCII(key, kFlightTypePrefix, true)) {
    if (!ParseFlightOrder(value, &user_order_info)) {
      return false;
    }
  } else if (StartsWithASCII(key, kMovieTypePrefix, true)) {
    if (!ParseMovieOrder(value, &user_order_info)) {
      return false;
    }
  } else if (StartsWithASCII(key, kTrainTypePrefix, true)) {
    if (!ParseTrainOrder(value, &user_order_info)) {
      return false;
    }
  } else if (StartsWithASCII(key, kHotelTypePrefix, true)) {
    if (FLAGS_enabled_hotel) {
      if (!ParseHotelOrder(value, &user_order_info)) {
        return false;
      }
    } else {
      LOG(ERROR) << "unsupported hotel type, message: " << message;
      return false;
    }
  } else {
    LOG(ERROR) << "unsupport type, key:" << key << ", message: " << message;
    return false;
  }
  UpdateToDb(user_order_info);
  UploadDataToKafka(user_order_info);
  return true;
}

bool MsgProcessor::ParseFlightOrder(
    const Json::Value& value, push_controller::UserOrderInfo* user_order_info) {
  try {
    string order_detail;
    if (!push_controller::JsonWrite(value, &order_detail)) {
      return false;
    }
    time_t business_time = 0;
    user_order_info->set_user_id(value["user"].asString());
    user_order_info->set_business_type(push_controller::kBusinessFlight);
    string raw_business_time = value["takeoff"].asString();
    recommendation::ShortDatetimeToTimestampNew(raw_business_time,
                                                &business_time);
    user_order_info->set_business_time(static_cast<int32_t>(business_time));
    user_order_info->set_order_detail(order_detail);
    user_order_info->set_order_status(push_controller::kOrderInitialization);
    string id = CreateOrderId(user_order_info->user_id(),
                              user_order_info->business_type(),
                              user_order_info->business_time());
    user_order_info->set_id(id);
    if (FLAGS_is_use_cluster_mode) {
      int64 fingerprint_id = mobvoi::Fingerprint(value["user"].asString());
      user_order_info->set_fingerprint_id(fingerprint_id);
    }
    LOG(INFO) << "PARSE filght success, user_order_info: "
              << push_controller::ProtoToString(*user_order_info);
    return true;
  } catch (const std::exception& e) {
    LOG(ERROR) << "parse flight failed, id:" << user_order_info->id()
               << ", exception: " << e.what();
    return false;
  }
}

bool MsgProcessor::ParseMovieOrder(
    const Json::Value& value,
    push_controller::UserOrderInfo* user_order_info) {
  try {
    string order_detail;
    if (!push_controller::JsonWrite(value, &order_detail)) {
      return false;
    }
    time_t business_time = 0;
    string user = value["user"].asString();
    user_order_info->set_user_id(user);
    user_order_info->set_business_type(push_controller::kBusinessMovie);
    string raw_business_time = value["show_time"].asString();
    recommendation::ShortDatetimeToTimestampNew(raw_business_time,
                                                &business_time);
    user_order_info->set_business_time(static_cast<int32_t>(business_time));
    user_order_info->set_order_detail(order_detail);
    user_order_info->set_order_status(push_controller::kOrderInitialization);
    string id = CreateOrderId(user_order_info->user_id(),
                              user_order_info->business_type(),
                              user_order_info->business_time());
    user_order_info->set_id(id);
    if (FLAGS_is_use_cluster_mode) {
      int64 fingerprint_id = mobvoi::Fingerprint(value["user"].asString());
      user_order_info->set_fingerprint_id(fingerprint_id);
    }
    LOG(INFO) << "PARSE movie success, user_order_info: "
              << push_controller::ProtoToString(*user_order_info);
    return true;
  } catch (const std::exception& e) {
    LOG(ERROR) << "parse movie failed, id:" << user_order_info->id()
               << ", exception:" << e.what();
    return false;
  }
}

bool MsgProcessor::ParseTrainOrder(
    const Json::Value& value,
    push_controller::UserOrderInfo* user_order_info) {
  try {
    string order_detail;
    if (!push_controller::JsonWrite(value, &order_detail)) {
      return false;
    }
    time_t business_time = 0;
    user_order_info->set_user_id(value["user"].asString());
    user_order_info->set_business_type(push_controller::kBusinessTrain);
    string raw_business_time = value["depart"].asString();
    recommendation::ShortDatetimeToTimestampNew(raw_business_time,
                                                &business_time);
    user_order_info->set_business_time(static_cast<int32_t>(business_time));
    user_order_info->set_order_detail(order_detail);
    user_order_info->set_order_status(push_controller::kOrderInitialization);
    string id = CreateOrderId(user_order_info->user_id(),
                              user_order_info->business_type(),
                              user_order_info->business_time());
    user_order_info->set_id(id);
    if (FLAGS_is_use_cluster_mode) {
      int64 fingerprint_id = mobvoi::Fingerprint(value["user"].asString());
      user_order_info->set_fingerprint_id(fingerprint_id);
    }
    LOG(INFO) << "PARSE train success, user_order_info: "
              << push_controller::ProtoToString(*user_order_info);
    return true;
  } catch (const std::exception& e) {
    LOG(ERROR) << "parse train failed, id:" << user_order_info->id()
               << ", exception:" << e.what();
    return false;
  }
}

bool MsgProcessor::ParseHotelOrder(
    const Json::Value& value,
    push_controller::UserOrderInfo* user_order_info) {
  try {
    string order_detail;
    if (!push_controller::JsonWrite(value, &order_detail)) {
      return false;
    }
    time_t business_time = 0;
    string user = value["user"].asString();
    user_order_info->set_user_id(user);
    user_order_info->set_business_type(push_controller::kBusinessHotel);
    string raw_business_time = value["arrive_time"].asString();
    recommendation::ShortDatetimeToTimestamp(raw_business_time,
        &business_time);
    user_order_info->set_business_time(static_cast<int32_t>(business_time));
    user_order_info->set_order_detail(order_detail);
    user_order_info->set_order_status(push_controller::kOrderInitialization);
    string id = CreateOrderId(user_order_info->user_id(),
        user_order_info->business_type(),
        user_order_info->business_time());
    user_order_info->set_id(id);
    if (FLAGS_is_use_cluster_mode) {
      int64 fingerprint_id = mobvoi::Fingerprint(value["user"].asString());
      user_order_info->set_fingerprint_id(fingerprint_id);
    }
    LOG(INFO) << "PARSE hotel success, user_order_info: "
              << push_controller::ProtoToString(*user_order_info);
    return true;
  } catch (const std::exception& e) {
    LOG(ERROR) << "Parse hotel failed, id:" << user_order_info->id()
               << ", exception:" << e.what();
    return false;
  }
}

bool MsgProcessor::UpdateToDb(
    const push_controller::UserOrderInfo& user_order_info) {
  try {
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection>
        connection(driver->connect(
            mysql_server_->host(),
            mysql_server_->user(),
            mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    string insert_sql;
    if (FLAGS_is_use_cluster_mode) {
      insert_sql = StringPrintf(
          kInsertFormatV2,
          user_order_table,
          user_order_info.id().c_str(),
          user_order_info.user_id().c_str(),
          static_cast<int32_t>(user_order_info.business_type()),
          user_order_info.business_time(),
          user_order_info.order_detail().c_str(),
          static_cast<int32_t>(user_order_info.order_status()),
          user_order_info.fingerprint_id());
    } else {
      insert_sql = StringPrintf(
          kInsertFormat,
          user_order_table,
          user_order_info.id().c_str(),
          user_order_info.user_id().c_str(),
          static_cast<int32_t>(user_order_info.business_type()),
          user_order_info.business_time(),
          user_order_info.order_detail().c_str(),
          static_cast<int32_t>(user_order_info.order_status()));
    }
    statement->executeUpdate(insert_sql);
    VLOG(2) << "Insert success, sql: " << insert_sql;
    return true;
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "# ERR: " << e.what();
    return false;
  }
}

string MsgProcessor::CreateOrderId(
    const std::string& user_id,
    const push_controller::BusinessType business_type,
    const int32_t business_time) {
  return StringPrintf("%s-%d-%d", user_id.c_str(),
      static_cast<int32_t>(business_type), business_time);
}

void MsgProcessor::UploadDataToKafka(
    const push_controller::UserOrderInfo& user_order_info) {
  time_t now = time(NULL);
  Json::Value data;
  data["timestamp"] = static_cast<uint32_t>(now);
  data["event"] = FLAGS_burypoint_upload_log_event_type;
  data["user_id"] = user_order_info.user_id();
  data["business_type"] = user_order_info.business_type();
  if (FLAGS_is_use_cluster_mode) {
    data["fingerprint_id"] =
      static_cast<Json::Int64>(user_order_info.fingerprint_id());
  }
  Json::Value properties;
  properties["order_id"] = user_order_info.id();
  properties["order_detail"] = user_order_info.order_detail();
  properties["business_time"] = user_order_info.business_time();
  data["properties"] = properties;
  Json::Value post_request;
  post_request["topic"] = FLAGS_burypoint_kafka_topic;
  post_request["value"] = data;
  util::HttpClient http_client;
  http_client.SetHttpMethod(util::HttpMethod::kPost);
  string post_data = push_controller::JsonToString(post_request);
  http_client.SetPostData(post_data);
  if (!http_client.FetchUrl(FLAGS_burypoint_kafka_log_server)) {
    LOG(WARNING) << "Failed to upload to kafka, response code: "
                 << http_client.response_code();
    return;
  }
  LOG(INFO) << "UPLOAD TO KAFKA, data:" << post_data;
}

JsonMsgProcessor::JsonMsgProcessor() {}

JsonMsgProcessor::~JsonMsgProcessor() {}

bool JsonMsgProcessor::Process(const std::string& message) {
  return ParseJsonMessage(message);
}

RawMsgProcessor::RawMsgProcessor() {
  message_parser_.reset(new MessageParser());
  CHECK(message_parser_.get());
}

RawMsgProcessor::~RawMsgProcessor() {}

bool RawMsgProcessor::Process(const std::string& message) {
  string json_result;
  if (!message_parser_->Parse(message, &json_result)) {
    LOG(ERROR) << "message_parser Parse() failed, message:" << message;
    return false;
  }
  return ParseJsonMessage(json_result);
}

MsgProcessorThread::MsgProcessorThread(bool use_raw_msg_processor)
  : mobvoi::Thread(true) {
  if (use_raw_msg_processor) {
    msg_processor_.reset(new RawMsgProcessor());
  } else {
    msg_processor_.reset(new JsonMsgProcessor());
  }
}

MsgProcessorThread::~MsgProcessorThread() {
  this->ShutDown();
  this->Join();
}

void MsgProcessorThread::Run() {
  msg_processor_->ProcessQueue(msg_queue_);
}

void MsgProcessorThread::set_msg_queue(
    shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue) {
  msg_queue_ = msg_queue;
}

void MsgProcessorThread::ShutDown() {
  msg_processor_->ShutDown();
}

}  // namespace message_receiver
