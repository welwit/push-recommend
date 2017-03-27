// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/serving/train/trainno_dumper.h"

#include <exception>

#include "base/compat.h"
#include "base/file/proto_util.h"
#include "base/log.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"

DECLARE_string(mysql_config);

namespace train {

TrainnoDumper::TrainnoDumper() {
  LOG(INFO) << "Construct TrainnoDumper";
  Init();
}

TrainnoDumper::~TrainnoDumper() {}

void TrainnoDumper::Init() {
  mysql_server_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
  LOG(INFO) << "Init mysql config from file:" << FLAGS_mysql_config;
}

bool TrainnoDumper::DumpResultIntoDb(const Json::Value& result,
                                     TrainnoType type) {
  map<string, TrainnoType> train_no_map;
  if (!ParseResult(result, type, &train_no_map)) {
    LOG(ERROR) << "fail to excute ParseResult";
    return false;  
  }
  if (!SaveMapIntoDb(train_no_map)) {
    LOG(ERROR) << "fail to excute SaveMapIntoDb";
    return false;
  }
  LOG(INFO) << "dump result to db success";
  return true;
}

bool TrainnoDumper::ParseResult(const Json::Value& result,
                               TrainnoType type,
                               map<string, TrainnoType>* train_no_map) {
  Json::Value value;
  try {
    value = result["train_no"]["train_no_list"];
  } catch (const std::exception& e) {
    LOG(ERROR) << "read json value failed:" << e.what();
    return false;
  } 
  string train_no;
  for (size_t index = 0; index < value.size(); ++index) {
    train_no = value[index].asString();
    train_no_map->insert(make_pair(train_no, type));
  }
  LOG(INFO) << "parse result to map, size=" << train_no_map->size() 
            << ",type=" << type;
  return true;
}

bool TrainnoDumper::SaveMapIntoDb(
    const map<string, TrainnoType>& train_no_map) {
  string table = "train_no"; 
  string insert_format = (
      "INSERT INTO %s (train_no, type) VALUES('%s','%d');"
  );
  int count = 0;
  try {
    sql::Driver * driver = sql::mysql::get_driver_instance();
    std::shared_ptr<sql::Connection> connection(
        driver->connect(mysql_server_->host(), 
                        mysql_server_->user(),
                        mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::shared_ptr<sql::Statement> statement(connection->createStatement());
    string train_no, insert_sql; 
    for (auto it = train_no_map.begin(); 
        it != train_no_map.end(); ++it) {
      train_no = it->first;
      TrainnoType type = it->second;
      insert_sql = StringPrintf(insert_format.c_str(), 
                                table.c_str(),
                                train_no.c_str(), int(type));
      try {
        statement->executeUpdate(insert_sql);
        count += 1;
      } catch (const sql::SQLException &e) {
        LOG(ERROR) << "# ERR: " << e.what()
                   << " (MySQL error code: " << e.getErrorCode()
                   << ", SQLState: " << e.getSQLState() << ")"; 
        continue;
      } catch (const std::runtime_error &e) {
        LOG(ERROR) << "runtime error:" << e.what();
        continue;
      }
    }
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "# ERR: " << e.what()
               << " (MySQL error code: " << e.getErrorCode()
               << ", SQLState: " << e.getSQLState() << ")"; 
    return false;
  } catch (const std::runtime_error& e) {
    LOG(ERROR) << "runtime error:" << e.what();
    return false;
  }
  LOG(INFO) << "save to db success, count:" << count; 
  return true;
}

}  // namespace train 
