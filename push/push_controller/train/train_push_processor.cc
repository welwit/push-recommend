// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/train/train_push_processor.h"

DECLARE_string(train_info_service);

namespace push_controller {

TrainPushProcessor::TrainPushProcessor() {
  VLOG(1) << "TrainPushProcessor::TrainPushProcessor()";
}

TrainPushProcessor::~TrainPushProcessor() {}

bool TrainPushProcessor::Process(PushEventInfo* push_event) {
  VLOG(2) << "TrainPushProcessor::Process() ...";
  UserOrderInfo user_order;
  if (!GetUserOrder(*push_event, &user_order)) {
    LOG(ERROR) << "GetUserOrder failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "GetUserOrder success, id:" << push_event->id();
  if (!ParseDetail(&user_order)) {
    LOG(ERROR) << "ParseDetail failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "Parse detail success, id:" << push_event->id();
  vector<train::TimeTable> time_tables;
  if (!GetTimeTable(user_order, &time_tables)) {
    LOG(ERROR) << "GetTimeTable failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "GetTimeTable success, id:" << push_event->id();
  Json::Value message;
  if (!BuildPushMessage(user_order, *push_event, time_tables, &message)) {
    LOG(ERROR) << "BuildPushMessage failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "BuildPushMessage success, id:" << push_event->id();
  string message_desc = business_key_map_[push_event->business_type()];
  if (!push_sender_->SendPush(kMessageTrain, message_desc,
                              push_event->user_id(), message)) {
    LOG(ERROR) << "SendPush failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "SendPush success, id:" << push_event->id();
  UploadPushDataToKafka(user_order, *push_event, message);
  return true;
}

bool TrainPushProcessor::ParseDetail(UserOrderInfo* user_order) {
  VLOG(2) << "TrainPushProcessor::ParseDetail(), id:" << user_order->id();
  Json::Value root;
  try {
    Json::Reader reader;
    if (!reader.parse(user_order->order_detail(), root)) {
      LOG(ERROR) << "JSON parse failed: "
                 << reader.getFormattedErrorMessages();
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "JSON parse failed, exception:" << e.what();
    return false;
  }
  if (root.isNull() || !root.isObject()) {
    LOG(ERROR) << "order detail format error, detail:"
               << user_order->order_detail();
    return false;
  }
  BusinessInfo* business_info = user_order->mutable_business_info();
  UserTrainInfo* user_train_info = business_info->mutable_user_train_info();
  user_train_info->set_user_id(root["user"].asString());
  user_train_info->set_train_no(root["train_num"].asString());
  user_train_info->set_depart_station(root["from"].asString());
  user_train_info->set_seat_list(root["seat"].asString());
  string depart_time_string = root["depart"].asString();
  time_t depart_time;
  recommendation::ShortDatetimeToTimestampNew(depart_time_string,
                                           &depart_time);
  user_train_info->set_depart_time(depart_time);
  return true;
}

bool TrainPushProcessor::GetTimeTable(
    const UserOrderInfo& user_order,
    vector<train::TimeTable>* time_tables) {
  Json::Value request;
  request["train_no"] = user_order.business_info().user_train_info().train_no();
  request["depart_station"] = (
      user_order.business_info().user_train_info().depart_station());
  string post_data = JsonToString(request);
  LOG(INFO) << "Train info request json: " << post_data;
  util::HttpClient http_client;
  http_client.SetHttpMethod(util::HttpMethod::kPost);
  http_client.SetPostData(post_data);
  http_client.AddHeader("Content-Type", "application/json; charset=utf-8");
  if (!http_client.FetchUrl(FLAGS_train_info_service)) {
    LOG(ERROR) << "fetch url failed";
    return false;
  }
  LOG(INFO) << "Train info response json: " << http_client.ResponseBody();
  string response_body = http_client.ResponseBody();
  Json::Value response;
  Json::Reader reader;
  try {
    reader.parse(response_body, response);
  } catch (const std::exception& e) {
    LOG(ERROR) << "response parse failed:" << e.what();
    return false;
  }
  if ("success" == response["status"].asString()) {
    Json::Value data_list = response["data"];
    for (Json::ArrayIndex index = 0; index < data_list.size(); ++index) {
      train::TimeTable time_table;
      time_table.set_train_no(data_list[index]["train_no"].asString());
      time_table.set_station_no(data_list[index]["station_no"].asInt());
      time_table.set_station_name(data_list[index]["station_name"].asString());
      time_table.set_get_in_time(data_list[index]["get_in_time"].asString());
      time_table.set_depart_time(data_list[index]["depart_time"].asString());
      time_table.set_stay_time(data_list[index]["stay_time"].asString());
      time_tables->push_back(time_table);
    }
    VLOG(2) << "train_no:"
            << user_order.business_info().user_train_info().train_no()
            << ", table size:" << time_tables->size();
    return true;
  } else {
    LOG(ERROR) << "query train info failed:"
               << response["err_msg"].asString();
    return false;
  }
}

bool TrainPushProcessor::BuildPushMessage(
    const UserOrderInfo& user_order,
    const PushEventInfo& push_event,
    const vector<train::TimeTable>& time_tables,
    Json::Value* message) {
  if (push_event.event_type() != kEventRemindTimelineLastDay &&
      push_event.event_type() != kEventRemindTimelineToday &&
      push_event.event_type() != kEventRemindTimeline3Hour &&
      push_event.event_type() != kEventRemindTimeline1Hour) {
    LOG(ERROR) << "ERR push event type:" << push_event.event_type();
    return false;
  }
  AppendCommonField(user_order, push_event, time_tables, message);
  AppendEventField(user_order, push_event, time_tables, message);
  LOG(INFO) << "MESSAGE:" << JsonToString(*message);
  return true;
}

void TrainPushProcessor::AppendCommonField(
    const UserOrderInfo& user_order,
    const PushEventInfo& push_event,
    const vector<train::TimeTable>& time_tables,
    Json::Value* msg) {
  Json::Value& message = *msg;
  const UserTrainInfo& user_train_info = (
      user_order.business_info().user_train_info());
  message["id"] = user_order.id();
  message["status"] = "success";
  message["product_key"] = business_key_map_[push_event.business_type()];
  message["event_key"] = event_key_map_[push_event.event_type()];
  message["train_no"] = user_train_info.train_no();
  message["station_from"] = user_train_info.depart_station();
  message["station_to"] = "";
  message["city_from"] = "";
  message["city_to"] = "";
  message["city_to_weather"] = "";
  string depart_time;
  recommendation::TimestampToDatetime(
      static_cast<time_t>(user_train_info.depart_time()), &depart_time);
  message["plan_depart"] = depart_time;
  message["plan_arrive"] = "";
  message["carriage"] = "";
  message["seat_no"] = user_train_info.seat_list();
  message["seat_type"] = "";
  message["running_time"] = "";
  Json::Value data_list;
  Json::Value null_obj_value(Json::objectValue);
  time_t now = time(NULL);
  string expire_time;
  recommendation::MakeExpireTime(now, &expire_time, "%Y-%m-%d %H:%M");
  message["sys"]["expire_time"] = expire_time;
  for (auto it = time_tables.begin(); it != time_tables.end(); ++it) {
    Json::Value data;
    data["title"] = it->station_name();
    data["time"] = it->get_in_time();
    data_list.append(data);
  }
  message["stations"] = data_list;
  message["is_push"] = "false";
  message["push_detail"]["des"] = u8"出门问问智能推送";
}

void TrainPushProcessor::AppendEventField(
    const UserOrderInfo& user_order,
    const PushEventInfo& push_event,
    const vector<train::TimeTable>& time_tables,
    Json::Value* message) {
  if (push_event.event_type() == kEventRemindTimelineLastDay) {
    HandleLastDay(user_order, push_event, time_tables, message);
  } else if (push_event.event_type() == kEventRemindTimelineToday) {
    HandleToday(user_order, push_event, time_tables, message);
  } else if (push_event.event_type() == kEventRemindTimeline3Hour) {
    Handle3Hour(user_order, push_event, time_tables, message);
  } else if (push_event.event_type() == kEventRemindTimeline1Hour) {
    Handle1Hour(user_order, push_event, time_tables, message);
  } else {
    LOG(ERROR) << "Should not go to this branch";
  }
}

void TrainPushProcessor::HandleLastDay(
    const UserOrderInfo& user_order,
    const PushEventInfo& push_event,
    const vector<train::TimeTable>& time_tables,
    Json::Value* msg) {
  Json::Value& message = *msg;
  const UserTrainInfo& user_train_info = (
      user_order.business_info().user_train_info());
  string title_format = u8"%s 明天出发";
  string title = StringPrintf(title_format.c_str(),
                              user_train_info.train_no().c_str());
  string ticker_format = u8"%s出发";
  string ticker = StringPrintf(ticker_format.c_str(),
                               user_train_info.depart_station().c_str());
  vector<string> ticker_vec;
  ticker_vec.push_back(ticker);
  SetTimeline(title, ticker_vec, message);
}

void TrainPushProcessor::HandleToday(
    const UserOrderInfo& user_order,
    const PushEventInfo& push_event,
    const vector<train::TimeTable>& time_tables,
    Json::Value* msg) {
  Json::Value& message = *msg;
  const UserTrainInfo& user_train_info = (
      user_order.business_info().user_train_info());
  string depart_time;
  recommendation::TimestampToTimeStr(
      static_cast<time_t>(user_train_info.depart_time()), &depart_time);
  string title_format = u8"%s %s发车";
  string title = StringPrintf(title_format.c_str(),
                              user_train_info.train_no().c_str(),
                              depart_time.c_str());
  string ticker_format = u8"%s出发";
  string ticker = StringPrintf(ticker_format.c_str(),
                               user_train_info.depart_station().c_str());
  vector<string> ticker_vec;
  ticker_vec.push_back(ticker);
  SetTimeline(title, ticker_vec, message);
}

void TrainPushProcessor::Handle3Hour(
    const UserOrderInfo& user_order,
    const PushEventInfo& push_event,
    const vector<train::TimeTable>& time_tables,
    Json::Value* msg) {
  Json::Value& message = *msg;
  const UserTrainInfo& user_train_info = (
      user_order.business_info().user_train_info());
  string depart_time;
  recommendation::TimestampToTimeStr(
      static_cast<time_t>(user_train_info.depart_time()), &depart_time);
  string title_format = u8"%s %s发车";
  string title = StringPrintf(title_format.c_str(),
                              user_train_info.train_no().c_str(),
                              depart_time.c_str());
  string ticker = user_train_info.seat_list();
  vector<string> ticker_vec;
  ticker_vec.push_back(ticker);
  SetTimeline(title, ticker_vec, message);
}

void TrainPushProcessor::Handle1Hour(
    const UserOrderInfo& user_order,
    const PushEventInfo& push_event,
    const vector<train::TimeTable>& time_tables,
    Json::Value* msg) {
  Json::Value& message = *msg;
  const UserTrainInfo& user_train_info = (
      user_order.business_info().user_train_info());
  string depart_time;
  recommendation::TimestampToTimeStr(
      static_cast<time_t>(user_train_info.depart_time()), &depart_time);
  string title_format = u8"%s %s发车";
  string title = StringPrintf(title_format.c_str(),
                              user_train_info.train_no().c_str(),
                              depart_time.c_str());
  string ticker = user_train_info.seat_list();
  vector<string> ticker_vec;
  ticker_vec.push_back(ticker);
  SetTimeline(title, ticker_vec, message);
}

}  // namespace push_controller
