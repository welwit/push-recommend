// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/movie/movie_push_processor.h"

namespace push_controller {

MoviePushProcessor::MoviePushProcessor() {
  VLOG(2) << "MoviePushProcessor::MoviePushProcessor";
}

MoviePushProcessor::~MoviePushProcessor() {}

bool MoviePushProcessor::Process(PushEventInfo* push_event) {
  VLOG(2) << "MoviePushProcessor::process() ...";
  UserOrderInfo user_order;
  if (!GetUserOrder(*push_event, &user_order)) {
    LOG(ERROR) << "GetUserOrder failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "GetUserOrder success, id:" << push_event->id();
  if (!ParseDetail(&user_order)) {
    LOG(ERROR) << "Parse detail failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "ParseDetail success, id:" << push_event->id();
  Json::Value message;
  if (!BuildPushMessage(user_order, *push_event, &message)) {
    LOG(ERROR) << "BuildPushMessage failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "BuildPushMessage success, id:" << push_event->id();
  string message_desc = business_key_map_[push_event->business_type()];
  if (!push_sender_->SendPush(kMessageMovie, message_desc,
                              push_event->user_id(), message)) {
    LOG(ERROR) << "SendPush failed, id:" << push_event->id();
    return false;
  }
  LOG(INFO) << "SendPush success, id:" << push_event->id();
  UploadPushDataToKafka(user_order, *push_event, message);
  return true;
}

bool MoviePushProcessor::ParseDetail(UserOrderInfo* user_order) {
  VLOG(2) << "MoviePushProcessor::ParseDetail(), id:" << user_order->id();
  Json::Value root;
  try {
    Json::Reader reader;
    if (!reader.parse(user_order->order_detail(), root)) {
      LOG(ERROR) << "JSON parse failed:"
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
  UserMovieInfo* user_movie_info = business_info->mutable_user_movie_info();
  user_movie_info->set_user_id(root["user"].asString());
  user_movie_info->set_cinema(root["cinema"].asString());
  user_movie_info->set_movie(root["movie"].asString());
  user_movie_info->set_seat_list(root["seat"].asString());
  user_movie_info->set_verification_code(root["code"].asString());
  user_movie_info->set_serial_number(root["seqno"].asString());
  string show_time_string = root["show_time"].asString();
  time_t show_time;
  recommendation::ShortDatetimeToTimestampNew(show_time_string, &show_time);
  user_movie_info->set_show_time(show_time);
  return true;
}

bool MoviePushProcessor::BuildPushMessage(const UserOrderInfo& user_order,
    const PushEventInfo& push_event, Json::Value* message) {
  if (push_event.event_type() != kEventRemindTimelineToday &&
      push_event.event_type() != kEventRemindTimeline30Min) {
    LOG(ERROR) << "ERR push event type:" << push_event.event_type();
    return false;
  }
  AppendCommonField(user_order, push_event, message);
  AppendEventField(user_order, push_event, message);
  LOG(INFO) << "MESSAGE:" << JsonToString(*message);
  return true;
}

void MoviePushProcessor::AppendCommonField(const UserOrderInfo& user_order,
    const PushEventInfo& push_event, Json::Value* msg) {
  Json::Value& message = *msg;
  const UserMovieInfo& user_movie_info =
      user_order.business_info().user_movie_info();
  Json::Value null_obj_value(Json::objectValue);
  message["id"] = user_order.id();
  message["status"] = "success";
  message["product_key"] = business_key_map_[push_event.business_type()];
  message["event_key"] = event_key_map_[push_event.event_type()];
  message["movie_name"] = user_movie_info.movie();
  time_t show_time = user_movie_info.show_time();
  string show_time_string, show_time_week;
  recommendation::TimestampToDatetime(show_time, &show_time_string);
  recommendation::TimestampToWeekStr(show_time, &show_time_week);
  message["show_time"] = show_time_string;
  message["show_week"] = show_time_week;
  message["cinema"] = user_movie_info.cinema();
  message["hall"] = "";
  message["seat_no"] = user_movie_info.seat_list();
  message["serial_number"] = user_movie_info.serial_number();
  message["verification_code"] = user_movie_info.verification_code() ;
  message["ticket_no"] = "";
  message["is_push"] = "false";
  message["push_detail"]["des"] = u8"出门问问智能推送";
  time_t now = time(NULL);
  string expire_time;
  recommendation::MakeExpireTime(now, &expire_time, "%Y-%m-%d %H:%M");
  message["sys"]["expire_time"] = expire_time;
}

void MoviePushProcessor::AppendEventField(const UserOrderInfo& user_order,
                                          const PushEventInfo& push_event,
                                          Json::Value* message) {
  if (push_event.event_type() == kEventRemindTimelineToday) {
    HandleToday(user_order, push_event, message);
  } else if (push_event.event_type() == kEventRemindTimeline30Min) {
    Handle30Min(user_order, push_event, message);
  } else {
    LOG(ERROR) << "Should not go to this branch";
  }
}

void MoviePushProcessor::HandleToday(const UserOrderInfo& user_order,
                                     const PushEventInfo& push_event,
                                     Json::Value* msg) {
  Json::Value& message = *msg;
  const UserMovieInfo& user_movie_info = (
      user_order.business_info().user_movie_info());
  string title = user_movie_info.movie();
  string ticker_format = u8"%s %s %s";
  time_t show_time = user_movie_info.show_time();
  string show_date, show_week, show_time_string;
  recommendation::TimestampToDatetime(show_time,
                                      &show_date,
                                      "%Y-%m-%d");
  recommendation::TimestampToWeekStr(show_time,
                                     &show_week);
  recommendation::TimestampToDatetime(show_time,
                                      &show_time_string,
                                      "%H:%M");
  string ticker = StringPrintf(ticker_format.c_str(),
                               show_date.c_str(),
                               show_week.c_str(),
                               show_time_string.c_str());
  vector<string> ticker_vec;
  ticker_vec.push_back(ticker);
  SetTimeline(title, ticker_vec, message);
}

void MoviePushProcessor::Handle30Min(const UserOrderInfo& user_order,
                                     const PushEventInfo& push_event,
                                     Json::Value* msg) {
  Json::Value& message = *msg;
  const UserMovieInfo& user_movie_info = (
      user_order.business_info().user_movie_info());
  string title = user_movie_info.movie();
  string ticker = user_movie_info.seat_list();
  vector<string> ticker_vec;
  ticker_vec.push_back(ticker);
  SetTimeline(title, ticker_vec, message);
}

}  // namespace push_controller
