// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/serving/flight/flight_data_updater.h"

#include "base/file/proto_util.h"
#include "base/log.h"
#include "base/time.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"

DECLARE_int32(flight_update_duration);
DECLARE_string(mysql_config);

namespace flight {

FlightDataUpdater::FlightDataUpdater() 
  : schedule_internal_(1800) { // default duration: 30min
  Init();
  LOG(INFO) << "Construct FlightDataUpdater success, schedule_internal:" 
            << schedule_internal_;
}

FlightDataUpdater::~FlightDataUpdater() {}

void FlightDataUpdater::Init() {
  flight_db_interface_.reset(new FlightDbInterface);
  mysql_server_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
  LOG(INFO) << "Init mysql config from file:" << FLAGS_mysql_config;
  if (FLAGS_flight_update_duration > 0) {
    schedule_internal_ = FLAGS_flight_update_duration;
  }
}

void FlightDataUpdater::Run() {
  LOG(INFO) << "start thread FlightDataUpdater";
  mobvoi::Sleep(5);
  vector<string> flight_no_vector;
  while (true) {
    flight_no_vector.clear();
    if (QueryAllFlightNo(&flight_no_vector) &&
        UpdateAllFlightInfo(flight_no_vector)) {
      LOG(INFO) << "update flight data successfully";
    } else {
      LOG(INFO) << "update flight data failed";
    }
    mobvoi::Sleep(schedule_internal_);
  }
}

bool FlightDataUpdater::QueryAllFlightNo(vector<string>* flight_no_vector) {
  flight_no_vector->clear();
  string table = "flight_no";
  string query_format = "SELECT flight_no FROM %s;";
  string query_sql = StringPrintf(query_format.c_str(), table.c_str());
  try {
    sql::Driver * driver = sql::mysql::get_driver_instance();
    std::shared_ptr<sql::Connection> connection(
        driver->connect(mysql_server_->host(), 
                        mysql_server_->user(), 
                        mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::shared_ptr<sql::Statement> statement(connection->createStatement());
    std::shared_ptr<sql::ResultSet> result_set(
        statement->executeQuery(query_sql));
    while (result_set->next()) {
      flight_no_vector->push_back(result_set->getString("flight_no"));
    }
    LOG(INFO) << "Get all flightno success, count:"
              << flight_no_vector->size();
    return true;
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "# ERR: " << e.what()
               << " (MySQL error code: " << e.getErrorCode()
               << ", SQLState: " << e.getSQLState() << ")"; 
    return false;
  } catch (const std::runtime_error &e) {
    LOG(ERROR) << "runtime error:" << e.what();
    return false;
  }
}

bool FlightDataUpdater::UpdateAllFlightInfo(
    const vector<string>& flight_no_vector) {
  if (flight_no_vector.empty()) {
    return true;
  }
  int success_cnt = 0;
  for (auto& flight_no : flight_no_vector) {
    if (!flight_db_interface_->UpdateFlightData(flight_no)) {
      LOG(WARNING) << "update flight data failed, no:" << flight_no;
      continue;
    }
    ++success_cnt;
  }
  LOG(INFO) << "all flight count:" << flight_no_vector.size()
            << ",update success count:" << success_cnt;
  return true;
}

}  // namespace flight 
