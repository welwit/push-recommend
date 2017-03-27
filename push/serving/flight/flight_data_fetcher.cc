// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/serving/flight/flight_data_fetcher.h"

#include <sstream>
#include <iostream>

#include "base/basictypes.h"
#include "base/log.h"
#include "base/string_util.h"
#include "third_party/jsoncpp/json.h"
#include "util/net/http_client/http_client.h"

#include "push/util/time_util.h"

namespace {
static const char kUrlForamt[] = (
    "http://coop.flight.qunar.com/api/flightstatus"
    "/getByFnoDate?date=%s&flightno=%s&token=UxzB3okMPlj12hnx&client=sougou");
}

namespace flight {

FlightDataFetcher::FlightDataFetcher() {
  LOG(INFO) << "Construct FlightDataFetcher";
}

FlightDataFetcher::~FlightDataFetcher() {}

bool FlightDataFetcher::FetchFlightData(const FlightRequest& request, 
                                        FlightResponse* response) {
  LOG(INFO) << "Request ==>" << "\n" << request.Utf8DebugString();
  const string& flight_no = request.flight_no();
  string depart_date = request.depart_date();
  string response_body;
  if (!GetRepsonseBody(flight_no, depart_date, &response_body)) {
    LOG(ERROR) << "Call GetRepsonseBody failed";
    return false;
  }
  if (!BuildResponse(response_body, depart_date, response)) {
    LOG(WARNING) << "Call BuildResponse failed";
    return false;
  }
  LOG(INFO) << "fetch flight data success";
  return true;
}

bool FlightDataFetcher::GetRepsonseBody(const string& flight_no, 
                                        const string& depart_date, 
                                        string* response_body) {
  string url = StringPrintf(
      kUrlForamt, depart_date.c_str(), flight_no.c_str());
  LOG(INFO) << "request url:" << url;
  util::HttpClient http_client;
  if (!http_client.FetchUrl(url)) {
    return false;
  }
  *response_body = http_client.ResponseBody();
  LOG(INFO) << "response code:" << http_client.response_code() << ","
            << "curl code:" << http_client.curl_code();
  LOG(INFO) << "page size:" << http_client.ResponseBody().size();
  LOG(INFO) << "header size:" << http_client.ResponseHeader().size();
  LOG(INFO) << "header info:" << http_client.ResponseHeader();
  VLOG(1) << "response body:" << http_client.ResponseBody();
  LOG(INFO) << "fetch url success";
  return true;
}

bool FlightDataFetcher::BuildResponse(const string& response_body,
                                      const string& depart_date, 
                                      FlightResponse* response) {
  Json::Value root;
  string datetime_format = "%Y-%m-%d %H:%M";
  try {
    Json::Reader reader;
    if (!reader.parse(response_body, root)) {
      LOG(WARNING) << "Failed to parse responseBody:" 
                 << reader.getFormattedErrorMessages();
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "json parse failed:" << e.what();
    return false;
  }
  string ret = root.get("ret", "false").asString();
  Json::Value &data_array = root["data"];
  if (data_array.size() > 0) {
    Json::Value::ArrayIndex index = 0;
    Json::Value &data = data_array[index];
    string flight_no = data["flightno"].asString();
    string airport_from = data["dptairport_name"].asString();
    string airport_to = data["arrairport_name"].asString();
    string city_from = data["dptcity_name"].asString();
    string city_to = data["arrcity_name"].asString();
    string terminal_from = data["dpttower"].asString();
    string terminal_to = data["arrtower"].asString();
    string plan_takeoff = recommendation::ToInternalDatetime(
        data["plan_local_dep_time"].asString());
    string plan_arrive = recommendation::ToInternalDatetime(
        data["plan_local_arr_time"].asString());
    string estimated_takeoff = recommendation::ToInternalDatetime(
        data["expect_local_dep_time"].asString());
    string estimated_arrive = recommendation::ToInternalDatetime(
        data["expect_local_arr_time"].asString());
    string actual_takeoff = recommendation::ToInternalDatetime(
        data["actual_local_dep_time"].asString());
    string actual_arrive = recommendation::ToInternalDatetime(
        data["actual_local_arr_time"].asString());
    string checkin_counter = data["zhiji"].asString();
    string checkin_stoptime;
    recommendation::HalfHourBeforeDatetime(plan_takeoff, 
                                           datetime_format, 
                                           &checkin_stoptime);
    string plan_gate = data["dengjikou"].asString();
    string actual_gate = data["dengjikou"].asString();
    string gate_is_change = "false"; //是否变更还需要和数据库做比较
    bool ret = recommendation::IsDelay(
        plan_takeoff, estimated_takeoff, actual_takeoff, datetime_format);
    string is_delay;
    if (ret) {
      is_delay = "true";
    } else {
      is_delay = "false";
    }
    string carousel = data["xingli"].asString();
    response->set_flight_no(flight_no);
    response->set_airport_from(airport_from);
    response->set_airport_to(airport_to);
    response->set_city_from(city_from);
    response->set_city_to(city_to);
    response->set_terminal_from(terminal_from);
    response->set_terminal_to(terminal_to);
    response->set_plan_takeoff(plan_takeoff);
    response->set_plan_arrive(plan_arrive);
    response->set_estimated_takeoff(estimated_takeoff);
    response->set_estimated_arrive(estimated_arrive);
    response->set_actual_takeoff(actual_takeoff);
    response->set_actual_arrive(actual_arrive);
    response->set_checkin_counter(checkin_counter);
    response->set_checkin_stoptime(checkin_stoptime);
    response->set_plan_gate(plan_gate);
    response->set_actual_gate(actual_gate);
    response->set_gate_is_change(gate_is_change);
    response->set_is_delay(is_delay);
    response->set_carousel(carousel);
    response->set_depart_date(depart_date);
    LOG(INFO) << "Response ==>" << "\n" << response->Utf8DebugString();
    return true;
  } else {
    LOG(WARNING) << "reponse data array empty";
    return false;
  }
}

}  // namespace flight 
