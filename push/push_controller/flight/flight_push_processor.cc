// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/flight/flight_push_processor.h"

DECLARE_string(flight_info_service);

namespace push_controller {

FlightPushProcessor::FlightPushProcessor() {
  VLOG(2) << "FlightPushProcessor::FlightPushProcessor()";
  weather_helper_.reset(new WeatherHelper());
}

FlightPushProcessor::~FlightPushProcessor() {}

bool FlightPushProcessor::Process(PushEventInfo* push_event) {
  VLOG(2) << "FlightPushProcessor::Process() ...";
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
  VLOG(2) << "ParseDetail success, id:" << push_event->id();
  flight::FlightResponse flight_response;
  if (!GetFlightResponse(user_order, &flight_response)) {
    LOG(ERROR) << "GetFlightResponse failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "GetFlightResponse success, id:" << push_event->id();
  WeatherInfo weather_info;
  if (!FetchWeather(flight_response, &weather_info)) {
    LOG(ERROR) << "FetchWeather failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "FetchWeather success, id:" << push_event->id();
  Json::Value message;
  if (!BuildPushMessage(user_order, *push_event, flight_response,
                        weather_info, &message)) {
    LOG(ERROR) << "BuildPushMessage failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "BuildPushMessage success, id:" << push_event->id();
  string message_desc = business_key_map_[push_event->business_type()];
  if (!push_sender_->SendPush(kMessageFlight, message_desc,
                              push_event->user_id(), message)) {
    LOG(ERROR) << "SendPush failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "SendPush success, id:" << push_event->id();
  UploadPushDataToKafka(user_order, *push_event, message);
  return true;
}

bool FlightPushProcessor::ParseDetail(UserOrderInfo* user_order) {
  VLOG(2) << "FlightPushProcessor::ParseDetail(), id:" << user_order->id();
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
  UserFlightInfo* user_flight_info = business_info->mutable_user_flight_info();
  user_flight_info->set_user_id(root["user"].asString());
  user_flight_info->set_flight_no(root["flightno"].asString());
  string takeoff_time_string = root["takeoff"].asString();
  time_t takeoff_time;
  recommendation::ShortDatetimeToTimestampNew(takeoff_time_string,
                                           &takeoff_time);
  user_flight_info->set_takeoff_time(takeoff_time);
  return true;
}

bool FlightPushProcessor::GetFlightResponse(
    const UserOrderInfo& user_order,
    flight::FlightResponse* flight_response) {
  const UserFlightInfo& user_flight_info = (
      user_order.business_info().user_flight_info());
  time_t takeoff_time = user_flight_info.takeoff_time();
  string depart_date;
  recommendation::TimestampToDatetime(takeoff_time,
                                      &depart_date,
                                      "%Y-%m-%d");
  Json::Value request;
  request["depart_date"] = depart_date;
  request["flight_no"] = user_flight_info.flight_no();
  string post_data = JsonToString(request);
  LOG(INFO) << "Flight info request: " << post_data;
  util::HttpClient http_client;
  http_client.SetHttpMethod(util::HttpMethod::kPost);
  http_client.SetPostData(post_data);
  http_client.AddHeader("Content-Type", "application/json; charset=utf-8");
  if (!http_client.FetchUrl(FLAGS_flight_info_service)) {
    LOG(ERROR) << "fetch url failed";
    return false;
  }
  LOG(INFO) << "Flight info response: " << http_client.ResponseBody();
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
    Json::Value data = data_list[static_cast<Json::ArrayIndex>(0)];
    flight_response->set_flight_no(data["flight_no"].asString());
    flight_response->set_airport_from(data["airport_from"].asString());
    flight_response->set_airport_to(data["airport_to"].asString());
    flight_response->set_city_from(data["city_from"].asString());
    flight_response->set_city_to(data["city_to"].asString());
    flight_response->set_terminal_from(data["terminal_from"].asString());
    flight_response->set_terminal_to(data["terminal_to"].asString());
    flight_response->set_plan_takeoff(data["plan_takeoff"].asString());
    flight_response->set_plan_arrive(data["plan_arrive"].asString());
    flight_response->set_estimated_takeoff(data["estimated_takeoff"].asString());
    flight_response->set_estimated_arrive(data["estimated_arrive"].asString());
    flight_response->set_actual_takeoff(data["actual_takeoff"].asString());
    flight_response->set_actual_arrive(data["actual_arrive"].asString());
    flight_response->set_checkin_counter(data["checkin_counter"].asString());
    flight_response->set_checkin_stoptime(data["checkin_stoptime"].asString());
    flight_response->set_plan_gate(data["plan_gate"].asString());
    flight_response->set_actual_gate(data["actual_gate"].asString());
    flight_response->set_gate_is_change(data["gate_is_change"].asString());
    flight_response->set_is_delay(data["is_delay"].asString());
    flight_response->set_carousel(data["carousel"].asString());
    flight_response->set_depart_date(data["depart_date"].asString());
    return true;
  } else {
    LOG(ERROR) << "query flight info failed, error:"
               << response["err_msg"].asString();
    return false;
  }
}

bool FlightPushProcessor::FetchWeather(
    const flight::FlightResponse& flight_response, WeatherInfo* weather_info) {
  return weather_helper_->FetchWeather(flight_response.city_to(), weather_info);
}

bool FlightPushProcessor::BuildPushMessage(
    const UserOrderInfo& user_order,
    const PushEventInfo& push_event,
    const flight::FlightResponse& flight_response,
    const WeatherInfo& weather_info,
    Json::Value* message) {
  if (push_event.event_type() != kEventRemindTimelineLastDay &&
      push_event.event_type() != kEventRemindTimelineToday &&
      push_event.event_type() != kEventRemindTimeline3Hour &&
      push_event.event_type() != kEventRemindTimeline1Hour &&
      push_event.event_type() != kEventRemindTimelineTakeoff &&
      push_event.event_type() != kEventRemindTimelineArrive) {
    LOG(ERROR) << "ERR push event type:" << push_event.event_type();
    return false;
  }
  if (!AppendCommonField(user_order, push_event, flight_response,
                         weather_info, message)) {
    return false;
  }
  if (!AppendEventField(user_order, push_event, flight_response,
                        weather_info, message)) {
    return false;
  }
  LOG(INFO) << "MESSAGE:" << JsonToString(*message);
  return true;
}

bool FlightPushProcessor::AppendCommonField(
    const UserOrderInfo& user_order, const PushEventInfo& push_event,
    const flight::FlightResponse& flight_response,
    const WeatherInfo& weather_info, Json::Value* msg) {
  vector<string> weather_results;
  weather_helper_->TodayWeather(weather_info, &weather_results);
  if (weather_results.size() != 3) {
    LOG(ERROR) << "ERR weather results size:" << weather_results.size();
    return false;
  }
  string weather = StringPrintf("%s\t%s\t%s",
                                 weather_results[0].c_str(),
                                 weather_results[1].c_str(),
                                 weather_results[2].c_str());
  Json::Value& message = *msg;
  Json::Value null_obj_value(Json::objectValue);
  message["id"] = user_order.id();
  message["status"] = "success";
  message["product_key"] = business_key_map_[push_event.business_type()];
  message["event_key"] = event_key_map_[push_event.event_type()];
  message["flight_no"] = flight_response.flight_no();
  message["airport_from"] = flight_response.airport_from();
  message["airport_to"] = flight_response.airport_to();
  message["city_from"] = flight_response.city_from();
  message["city_to"] = flight_response.city_to();
  message["terminal_from"] = flight_response.terminal_from();
  message["terminal_to"] = flight_response.terminal_to();
  message["city_to_detail"]["weather"] = weather;
  time_t now = time(NULL);
  string expire_time;
  recommendation::MakeExpireTime(now, &expire_time, "%Y-%m-%d %H:%M");
  message["sys"]["expire_time"] = expire_time;
  string plan_takeoff, plan_arrive; // use actual time for show
  if (!recommendation::GetTakeoff(flight_response.plan_takeoff(),
                                  flight_response.estimated_takeoff(),
                                  flight_response.actual_takeoff(),
                                  &plan_takeoff)) {
    return false;
  }
  if (!recommendation::GetArrive(flight_response.plan_arrive(),
                                 flight_response.estimated_arrive(),
                                 flight_response.actual_arrive(),
                                 &plan_arrive)) {
    return false;
  }
  message["flight_detail"]["plan_takeoff"] = plan_takeoff;
  message["flight_detail"]["plan_arrive"] = plan_arrive;
  message["flight_detail"]["estimated_takeoff"] = (
      flight_response.estimated_takeoff());
  message["flight_detail"]["estimated_arrive"] = (
      flight_response.estimated_arrive());
  message["flight_detail"]["actual_takeoff"] = flight_response.actual_takeoff();
  message["flight_detail"]["actual_arrive"] = flight_response.actual_arrive();
  message["flight_detail"]["checkin_counter"] = (
      flight_response.checkin_counter());
  message["flight_detail"]["checkin_stoptime"] = (
      flight_response.checkin_stoptime());
  message["flight_detail"]["plan_gate"] = flight_response.plan_gate();
  message["flight_detail"]["actual_gate"] = flight_response.actual_gate();
  message["flight_detail"]["gate_is_change"] = flight_response.gate_is_change();
  message["flight_detail"]["is_delay"] = flight_response.is_delay();
  message["flight_detail"]["carousel"] = flight_response.carousel();
  return true;
}

bool FlightPushProcessor::AppendEventField(
    const UserOrderInfo& user_order, const PushEventInfo& push_event,
    const flight::FlightResponse& flight_response,
    const WeatherInfo& weather_info, Json::Value* message) {
  if (push_event.event_type() == kEventRemindTimelineLastDay) {
    return HandleLastDay(
        user_order, push_event, flight_response, weather_info, message);
  } else if (push_event.event_type() == kEventRemindTimelineToday) {
    return HandleToday(
        user_order, push_event, flight_response, weather_info, message);
  } else if (push_event.event_type() == kEventRemindTimeline3Hour) {
    return Handle3Hour(
        user_order, push_event, flight_response, weather_info, message);
  } else if (push_event.event_type() == kEventRemindTimeline1Hour) {
    return Handle1Hour(
        user_order, push_event, flight_response, weather_info, message);
  } else if (push_event.event_type() == kEventRemindTimelineTakeoff) {
    return HandleTakeoff(
        user_order, push_event, flight_response, weather_info, message);
  } else if (push_event.event_type() == kEventRemindTimelineArrive) {
    return HandleArrival(
        user_order, push_event, flight_response, weather_info, message);
  } else {
    LOG(ERROR) << "Should not go to this branch";
    return false;
  }
}

bool FlightPushProcessor::HandleLastDay(
    const UserOrderInfo& user_order, const PushEventInfo& push_event,
    const flight::FlightResponse& flight_response,
    const WeatherInfo& weather_info, Json::Value* msg) {
  vector<string> weather_results;
  weather_helper_->TomorrowWeather(weather_info, &weather_results);
  if (weather_results.size() != 3) {
    LOG(ERROR) << "ERR weather results size:" << weather_results.size();
    return false;
  }
  Json::Value& message = *msg;
  Json::Value null_obj_value(Json::objectValue);
  message["is_push"] = "false";
  message["push_detail"] = null_obj_value;
  string title_format = u8"%s 明天出发";
  string title = StringPrintf(title_format.c_str(),
                              flight_response.flight_no().c_str());
  string ticker_format = u8"%s %s°~%s°";
  string ticker = StringPrintf(ticker_format.c_str(),
                               weather_results[0].c_str(),
                               weather_results[1].c_str(),
                               weather_results[2].c_str());
  vector<string> ticker_vec;
  ticker_vec.push_back(ticker);
  SetTimeline(title, ticker_vec, message);
  return true;
}

bool FlightPushProcessor::HandleToday(
    const UserOrderInfo& user_order,
    const PushEventInfo& push_event,
    const flight::FlightResponse& flight_response,
    const WeatherInfo& weather_info,
    Json::Value* msg) {
  string takeoff_time;
  if (!GetTakeoff(flight_response, &takeoff_time)) {
    return false;
  }
  Json::Value& message = *msg;
  Json::Value null_obj_value(Json::objectValue);
  message["is_push"] = "false";
  message["push_detail"] = null_obj_value;
  string title_format = u8"%s %s起飞";
  string title = StringPrintf(title_format.c_str(),
                              flight_response.flight_no().c_str(),
                              takeoff_time.c_str());
  string ticker = flight_response.airport_from()
                + flight_response.terminal_from();
  vector<string> ticker_vec;
  ticker_vec.push_back(ticker);
  SetTimeline(title, ticker_vec, message);
  return true;
}

bool FlightPushProcessor::Handle3Hour(
    const UserOrderInfo& user_order,
    const PushEventInfo& push_event,
    const flight::FlightResponse& flight_response,
    const WeatherInfo& weather_info,
    Json::Value* msg) {
  string takeoff_time;
  if (!GetTakeoff(flight_response, &takeoff_time)) {
    return false;
  }
  vector<string> stoptime_vec;
  SplitString(flight_response.checkin_stoptime(), ' ', &stoptime_vec);
  if (stoptime_vec.size() != 2) {
    return false;
  }
  string stoptime_value = stoptime_vec[1];
  Json::Value& message = *msg;
  Json::Value null_obj_value(Json::objectValue);
  string title_format = u8"%s %s起飞";

  string title = StringPrintf(title_format.c_str(),
                              flight_response.flight_no().c_str(),
                              takeoff_time.c_str());
  string gate_format = u8"登机口%s";
  string counter_format = u8"柜台%s";
  string stoptime_format = u8"%s停止值机";
  string gate = StringPrintf(gate_format.c_str(),
                             flight_response.actual_gate().c_str());
  string counter = StringPrintf(counter_format.c_str(),
                                flight_response.checkin_counter().c_str());
  string stoptime = StringPrintf(stoptime_format.c_str(),
                                 stoptime_value.c_str());
  vector<string> ticker_vec;
  ticker_vec.push_back(counter);
  ticker_vec.push_back(stoptime);
  SetTimeline(title, ticker_vec, message);
  string des = u8"请提前前往机场";
  string position1 = flight_response.flight_no();
  string position2 = gate;
  string position3 = counter;
  string position4 = stoptime;
  string ui_type = "1";
  SetPushDetail(
      des, position1, position2, position3, position4, ui_type, message);
  return true;
}

bool FlightPushProcessor::Handle1Hour(
    const UserOrderInfo& user_order, const PushEventInfo& push_event,
    const flight::FlightResponse& flight_response,
    const WeatherInfo& weather_info, Json::Value* msg) {
  if (flight_response.gate_is_change() != "true") {
    VLOG(2) << "flight gate is not changed, id:" << push_event.id();
    return false;
  }
  if (flight_response.plan_gate().empty() ||
      flight_response.plan_gate() == "--" ||
      flight_response.actual_gate().empty() ||
      flight_response.actual_gate() == "--") {
    LOG(ERROR) << "Invalid flight gate change, id:" << push_event.id();
    return false;
  }
  string takeoff_time;
  if (!GetTakeoff(flight_response, &takeoff_time)) {
    return false;
  }
  Json::Value& message = *msg;
  Json::Value null_obj_value(Json::objectValue);
  string title_format = u8"%s %s起飞";
  string title = StringPrintf(title_format.c_str(),
                              flight_response.flight_no().c_str(),
                              takeoff_time.c_str());
  string gate_change_text_format = u8"登机口变更为%s";
  string gate_change_text = StringPrintf(
      gate_change_text_format.c_str(), flight_response.actual_gate().c_str());
  vector<string> ticker_vec;
  ticker_vec.push_back(gate_change_text);
  SetTimeline(title, ticker_vec, message);
  string des = u8"出门问问智能推送";
  string gate_change_text2 = u8"登机口变更为";
  string position1 = flight_response.flight_no();
  string position2 = gate_change_text2;
  string position3 = flight_response.actual_gate();
  string position4 = flight_response.plan_gate();
  string ui_type = "2";
  SetPushDetail(
      des, position1, position2, position3, position4, ui_type, message);
  return true;
}

bool FlightPushProcessor::HandleTakeoff(
    const UserOrderInfo& user_order, const PushEventInfo& push_event,
    const flight::FlightResponse& flight_response,
    const WeatherInfo& weather_info, Json::Value* msg) {
  if (flight_response.is_delay() != "true") {
    VLOG(2) << "flight is not delayed, id:" << push_event.id();
    return false;
  }
  string takeoff_time;
  if (!GetTakeoff(flight_response, &takeoff_time)) {
    return false;
  }
  vector<string> plan_vec;
  SplitString(flight_response.plan_takeoff(), ' ', &plan_vec);
  if (plan_vec.size() != 2) {
    return false;
  }
  string plan_takeoff_time = plan_vec[1];
  Json::Value& message = *msg;
  Json::Value null_obj_value(Json::objectValue);
  string title_format = u8"%s 起飞延误";
  string title = StringPrintf(title_format.c_str(),
                              flight_response.flight_no().c_str());
  string delay_text_format = u8"起飞延误至%s";
  string delay_text = StringPrintf(delay_text_format.c_str(),
                                   takeoff_time.c_str());
  vector<string> ticker_vec;
  ticker_vec.push_back(delay_text);
  SetTimeline(title, ticker_vec, message);
  string des = u8"出门问问智能推送";
  string delay_text2 = u8"起飞延误至";
  string position1 = flight_response.flight_no();
  string position2 = delay_text2;
  string position3 = takeoff_time;
  string position4 = plan_takeoff_time;
  string ui_type = "2";
  SetPushDetail(
      des, position1, position2, position3, position4, ui_type, message);
  return true;
}

bool FlightPushProcessor::HandleArrival(
    const UserOrderInfo& user_order, const PushEventInfo& push_event,
    const flight::FlightResponse& flight_response,
    const WeatherInfo& weather_info, Json::Value* msg) {
  string arrive_time;
  if (!recommendation::GetArrive(flight_response.estimated_arrive(),
                                flight_response.actual_arrive(),
                                &arrive_time)) {
    return false;
  }
  vector<string> weather_results;
  weather_helper_->TodayWeather(weather_info, &weather_results);
  if (weather_results.size() != 3) {
    LOG(ERROR) << "ERR weather results size:" << weather_results.size();
    return false;
  }
  string weather_format = u8"%s %s°~%s°";
  string weather = StringPrintf(weather_format.c_str(),
                                 weather_results[0].c_str(),
                                 weather_results[1].c_str(),
                                 weather_results[2].c_str());
  Json::Value& message = *msg;
  Json::Value null_obj_value(Json::objectValue);
  string title_format = u8"%s %s抵达";
  string title = StringPrintf(title_format.c_str(),
                              flight_response.flight_no().c_str(),
                              arrive_time.c_str());
  string carousel_text_format = u8"行李转盘%s";
  string carousel_text = StringPrintf(carousel_text_format.c_str(),
                                      flight_response.carousel().c_str());
  vector<string> ticker_vec;
  ticker_vec.push_back(carousel_text);
  SetTimeline(title, ticker_vec, message);
  string des = u8"出门问问智能推送";
  string carousel_text2 = u8"行李转盘";
  string position1 = flight_response.flight_no();
  string position2 = carousel_text2;
  string position3 = flight_response.carousel();
  string position4 = weather;
  string ui_type = "3";
  SetPushDetail(
      des, position1, position2, position3, position4, ui_type, message);
  return true;
}

bool FlightPushProcessor::GetTakeoff(
    const flight::FlightResponse& flight_response, string* takeoff) {
  vector<string> takeoff_vec;
  if (!recommendation::GetTakeoffVec(
        flight_response.plan_takeoff(), flight_response.estimated_takeoff(),
        flight_response.actual_takeoff(), &takeoff_vec)) {
    return false;
  }
  if (takeoff_vec.size() != 2) {
    return false;
  }
  *takeoff = takeoff_vec[1];
  return true;
}

}  // namespace push_controller
