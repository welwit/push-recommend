// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/serving/flight/flight_info_handler.h"

#include "base/file/proto_util.h"
#include "base/log.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"
#include "third_party/jsoncpp/json.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"
#include "util/net/util.h"

#include "push/util/common_util.h"

DECLARE_string(mysql_config);

namespace {
  
static const char kOkText[] = "ok";
static const char kServiceName[] = "flight info service";
static const char kQueryFormat[] = (
    "SELECT flight_no FROM flight_no WHERE flight_no = '%s';"
);
static const char kInsertFormat[] = (
    "INSERT INTO flight_no (flight_no) VALUES('%s');"
);

} // namespace

namespace serving {

FlightInfoQueryHandler::FlightInfoQueryHandler() {
  LOG(INFO) << "Construct FlightInfoQueryHandler";
  Init();
}

FlightInfoQueryHandler::~FlightInfoQueryHandler() {}

void FlightInfoQueryHandler::Init() {
  flight_db_interface_.reset(new FlightDbInterface);
  mysql_server_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
  LOG(INFO) << "Init mysql config from file:" << FLAGS_mysql_config;
}

bool FlightInfoQueryHandler::HandleRequest(util::HttpRequest* request,
                                           util::HttpResponse* response) {
  LOG(INFO) << "Receive FlightInfoQueryHandler request";
  response->SetJsonContentType();
  Json::Value req;
  try {
    Json::Reader reader;
    reader.parse(request->GetRequestData(), req);
  } catch (const std::exception& e) {
    LOG(ERROR) << "Read json resquest data failed:" << e.what();
    response->AppendBuffer(ErrorInfo(e.what()));
    return false;
  }
  Json::FastWriter writer;
  LOG(INFO) << "Flight query request:" << writer.write(req);
  FlightRequest flight_request; 
  FlightResponse flight_response;
  flight_request.set_flight_no(req["flight_no"].asString());
  flight_request.set_depart_date(req["depart_date"].asString());
  if (flight_db_interface_->GetFlightInfo(&flight_request, &flight_response)) {
    response->AppendBuffer(ResponseInfo(flight_response));
    return true;
  } else {
    LOG(ERROR) << "Get flight info failed, flight_no:"
               << flight_request.flight_no();
    if (!UpdateFlightNo(flight_request.flight_no())) {
      LOG(ERROR) << "update flight no to db failed";
    }
    if (!flight_db_interface_->UpdateFlightData(flight_request.flight_no())) {
      LOG(ERROR) << "fetch and update flight data to redis failed";
    }
    response->AppendBuffer(ErrorInfo("Get flight info failed"));
    return false;
  }
}

string FlightInfoQueryHandler::ErrorInfo(const string& error) const {
  Json::Value result;
  result["status"] = "error";
  result["err_msg"] = error;
  result["data"] = Json::Value(Json::arrayValue);
  string res = push_controller::JsonToString(result);
  return res;
}

string FlightInfoQueryHandler::ResponseInfo(const FlightResponse& response) {
  Json::Value result, data, data_list;
  data["flight_no"] = response.flight_no();
  data["airport_from"] = response.airport_from();
  data["airport_to"] = response.airport_to();
  data["city_from"] = response.city_from();
  data["city_to"] = response.city_to();
  data["terminal_from"] = response.terminal_from();
  data["terminal_to"] = response.terminal_to();
  data["plan_takeoff"] = response.plan_takeoff();
  data["plan_arrive"] = response.plan_arrive();
  data["estimated_takeoff"] = response.estimated_takeoff();
  data["estimated_arrive"] = response.estimated_arrive();
  data["actual_takeoff"] = response.actual_takeoff();
  data["actual_arrive"] = response.actual_arrive();
  data["checkin_counter"] = response.checkin_counter();
  data["checkin_stoptime"] = response.checkin_stoptime();
  data["is_delay"] = response.is_delay();
  data["carousel"] = response.carousel();
  data["depart_date"] = response.depart_date();
  data["actual_gate"] = response.actual_gate();
  data["plan_gate"] = response.plan_gate();
  data["gate_is_change"] = response.gate_is_change();
  data_list.append(data);
  result["status"] = "success";
  result["err_msg"] = "";
  result["data"] = data_list;
  string res = push_controller::JsonToString(result);
  LOG(INFO) << "Flight response: " << res;
  return res;
}

bool FlightInfoQueryHandler::UpdateFlightNo(const string& flight_no) {
  string query_sql = StringPrintf(kQueryFormat, flight_no.c_str());
  string insert_sql = StringPrintf(kInsertFormat, flight_no.c_str());
  try {
    sql::Driver * driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection> connection(
        driver->connect(mysql_server_->host(), 
                        mysql_server_->user(), 
                        mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    std::unique_ptr<sql::ResultSet> result_set(
        statement->executeQuery(query_sql));
    if (result_set->rowsCount() == 0) {
      statement->executeUpdate(insert_sql);
      LOG(INFO) << "update flight no to db success, no:" << flight_no;
      return true;
    } else {
      LOG(WARNING) << "flight no is in table, need not update, no:" 
                   << flight_no;
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "#ERR:" << e.what();
    return false;
  }
}

StatusHandler::StatusHandler() {}

StatusHandler::~StatusHandler() {}

bool StatusHandler::HandleRequest(util::HttpRequest* request,
                                  util::HttpResponse* response) {
  LOG(INFO)<< "Receive StatusHandler request";
  response->SetJsonContentType();
  Json::Value result;
  result["status"] = kOkText;
  result["host"] = util::GetLocalHostName();
  result["service"] = kServiceName;
  response->AppendBuffer(result.toStyledString());
  return true;
}

}  // namespace serving
