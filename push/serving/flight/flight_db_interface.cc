// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/serving/flight/flight_db_interface.h"

#include <map>

#include "base/compat.h"
#include "base/log.h"
#include "base/file/proto_util.h"
#include "third_party/gflags/gflags.h"

#include "push/util/time_util.h"

DECLARE_int32(redis_expire_time);
DECLARE_string(mysql_config);

namespace flight {

FlightDbInterface::FlightDbInterface() {
  LOG(INFO) << "Construct FlightDbInterface";
  flight_data_fetcher_.reset(new FlightDataFetcher);
}

FlightDbInterface::~FlightDbInterface() {}

bool FlightDbInterface::GetFlightInfo(FlightRequest* request,
                                      FlightResponse* response) {
  if (!redis_client_.InitConnection()) {
    LOG(ERROR) << "redis init connection failed";
    return false;
  }
  const string& flight_no = request->flight_no();
  const string& depart_date = request->depart_date();
  string key = flight_no + "&" + depart_date;
  map<string, string> result_map;
  if (redis_client_.Hgetall(key, &result_map)) {
    response->set_flight_no(result_map["flight_no"]);  
    response->set_airport_from(result_map["airport_from"]);  
    response->set_airport_to(result_map["airport_to"]);  
    response->set_city_from(result_map["city_from"]);  
    response->set_city_to(result_map["city_to"]);  
    response->set_terminal_from(result_map["terminal_from"]);  
    response->set_terminal_to(result_map["terminal_to"]);  
    response->set_plan_takeoff(result_map["plan_takeoff"]);  
    response->set_plan_arrive(result_map["plan_arrive"]);  
    response->set_estimated_takeoff(result_map["estimated_takeoff"]);  
    response->set_estimated_arrive(result_map["estimated_arrive"]);  
    response->set_actual_takeoff(result_map["actual_takeoff"]);  
    response->set_actual_arrive(result_map["actual_arrive"]);  
    response->set_checkin_counter(result_map["checkin_counter"]);  
    response->set_checkin_stoptime(result_map["checkin_stoptime"]);  
    response->set_is_delay(result_map["is_delay"]);  
    response->set_carousel(result_map["carousel"]);  
    response->set_depart_date(result_map["depart_date"]);  
    response->set_actual_gate(result_map["actual_gate"]);  
    response->set_plan_gate(result_map["plan_gate"]);
    if (response->plan_gate() != response->actual_gate()) {
      response->set_gate_is_change("true");
      LOG(INFO) << "Gate changed, plan:" << response->plan_gate()
                << ",actual:" << response->actual_gate();
    } else {
      response->set_gate_is_change("false");
    }
    redis_client_.FreeConnection();
    return true;
  } else {
    LOG(ERROR) << "Hgetall failed, key:" << key;
    redis_client_.FreeConnection();
    return false;
  }
}

bool FlightDbInterface::UpdateFlightData(const string& flight_no) {
  bool success = true;
  FlightRequest request;
  FlightResponse response;
  request.Clear();
  response.Clear();
  BuildFlightRequestForToday(flight_no, &request);
  if (!FetchAndUpdateFlight(request, &response)) {
    success = false;
  }
  request.Clear();
  response.Clear();
  BuildFlightRequestForTomorrow(flight_no, &request);
  if (!FetchAndUpdateFlight(request, &response)) {
    success = false;
  }
  return success;
}

bool FlightDbInterface::UpdateFlightDataToRedis(FlightResponse* response) {
  if (!redis_client_.InitConnection()) {
    LOG(ERROR) << "redis init connection failed";
    return false;
  }
  LOG(INFO) << "Redis connect success";
  const string& flight_no = response->flight_no();
  const string& depart_date = response->depart_date();
  string key = flight_no + "&" + depart_date;
  map<string, string> flight_data_map;
  SetFlightDataMap(response, &flight_data_map);
  SetPlanGateToMap(key, response, &flight_data_map);
  int retry_cnt = 3;
  while (retry_cnt-- > 0) {
    if (redis_client_.Hmset(key, flight_data_map)) {
      break;
    }
  }
  if (retry_cnt == 0) {
    LOG(ERROR) << "redis mset failed, key:" << key;
    redis_client_.FreeConnection();
    return false;
  }
  LOG(INFO) << "Redis hmset success, key:" << key;
  retry_cnt = 3;
  while (retry_cnt-- > 0) {
    if (redis_client_.Expire(key, FLAGS_redis_expire_time)) {
      break;
    }
  }
  if (retry_cnt == 0) {
    LOG(ERROR) << "redis set expire failed, key:" << key;
    redis_client_.FreeConnection();
    return false;
  }
  LOG(INFO) << "Redis set expire success, key:" << key;
  redis_client_.FreeConnection();
  return true;
}

void FlightDbInterface::BuildFlightRequestForToday(
    const string& flight_no,
    FlightRequest* flight_request) {
  string date;
  recommendation::MakeDate(0, &date);
  flight_request->set_flight_no(flight_no);
  flight_request->set_depart_date(date);
}

void FlightDbInterface::BuildFlightRequestForTomorrow(
    const string& flight_no,
    FlightRequest* flight_request) {
  string date;
  recommendation::MakeDate(1, &date);
  flight_request->set_flight_no(flight_no);
  flight_request->set_depart_date(date);
}

bool FlightDbInterface::FetchAndUpdateFlight(const FlightRequest& request,
                                             FlightResponse* response) {
  if (!flight_data_fetcher_->FetchFlightData(request, response)) {
    LOG(WARNING) << "fetch failed, flight_no:" << request.flight_no();
    return false;
  }
  LOG(INFO) << "fetch success, flight_no:" << request.flight_no();
  if (!UpdateFlightDataToRedis(response)) {
    LOG(WARNING) << "update to redis failed, flight_no:" 
                 << request.flight_no();
    return false;
  }
  LOG(INFO) << "update to redis success, flight_no:" 
            << request.flight_no();
  return true;
}

void FlightDbInterface::SetFlightDataMap(
    FlightResponse* response,
    map<string, string>* flight_data_map_pointer) {
  map<string, string> &flight_data_map = *flight_data_map_pointer;
  flight_data_map["flight_no"] = response->flight_no();
  flight_data_map["airport_from"] = response->airport_from();
  flight_data_map["airport_to"] = response->airport_to();
  flight_data_map["city_from"] = response->city_from();
  flight_data_map["city_to"] = response->city_to();
  flight_data_map["terminal_from"] = response->terminal_from();
  flight_data_map["terminal_to"] = response->terminal_to();
  flight_data_map["plan_takeoff"] = response->plan_takeoff();
  flight_data_map["plan_arrive"] = response->plan_arrive();
  flight_data_map["estimated_takeoff"] = response->estimated_takeoff();
  flight_data_map["estimated_arrive"] = response->estimated_arrive();
  flight_data_map["actual_takeoff"] = response->actual_takeoff();
  flight_data_map["actual_arrive"] = response->actual_arrive();
  flight_data_map["checkin_counter"] = response->checkin_counter();
  flight_data_map["checkin_stoptime"] = response->checkin_stoptime();
  flight_data_map["actual_gate"] = response->actual_gate();
  flight_data_map["is_delay"] = response->is_delay();
  flight_data_map["carousel"] = response->carousel();
  flight_data_map["depart_date"] = response->depart_date();
}

void FlightDbInterface::SetPlanGateToMap(
    const string& key,
    FlightResponse* response,
    map<string, string>* flight_data_map_pointer) {
  string field = "plan_gate";
  bool is_existed = true;
  if (!redis_client_.Exists(key)) {
    is_existed = false;
  } else {
    if (!redis_client_.Hexists(key, field)) {
      is_existed = false;
    } else {
      string value;
      if (redis_client_.Hget(key, field, &value)) {
        if (value.empty()) {
          is_existed = false;
        }
      }
    }
  }
  // 第一次插入plan_gate作为计划登机口，以后不再更新
  if (!is_existed) {
    map<string, string> &flight_data_map = *flight_data_map_pointer;
    flight_data_map["plan_gate"] = response->plan_gate();
    LOG(INFO) << "plan to insert gate, key:" << key << ",plan_gate:" 
              << flight_data_map["plan_gate"];
  }
}

}  // namespace flight 
