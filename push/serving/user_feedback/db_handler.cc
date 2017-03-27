// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/serving/user_feedback/db_handler.h"

#include <sstream>
#include <string>
#include <vector>
#include <memory>

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

#include "push/proto/user_feedback_meta.pb.h"

DECLARE_string(mysql_config);

namespace {

static const char kTable[] = "user_feedback";
static const char kQueryFormat[] = (
    "SELECT user_feedback_status FROM %s "
    "WHERE user_id='%s' AND business_type='%d';"
);
static const char kInsertFormat[] = (
    "INSERT INTO %s (user_id,business_type,user_feedback_status) "
    "VALUES('%s','%d','%d');"
);
static const char kUpdateFormat[] = (
    "UPDATE %s SET user_feedback_status='%d' "
    "WHERE user_id='%s' AND business_type='%d';"
);
static const char kQueryFeedbackFormat[] = (
    "SELECT business_type,user_feedback_status FROM %s "
    "WHERE user_id='%s';"
);

}

namespace feedback {

DbHandler::DbHandler() {
  LOG(INFO) << "Construct DbHandler";
  Init();
}

DbHandler::~DbHandler() {}

void DbHandler::Init() {
  mysql_server_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
  LOG(INFO) << "init mysql config from file:" << FLAGS_mysql_config;
}

bool DbHandler::InsertUserFeedback(UserFeedbackRequest *request,
                                   UserFeedbackResponse *response) {
  LOG(INFO) << "InsertUserFeedback";
  try {
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::shared_ptr< sql::Connection > connection(
        driver->connect(mysql_server_->host(), 
                        mysql_server_->user(), 
                        mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::shared_ptr< sql::Statement > statement(connection->createStatement());
   
    const string& user_id = request->user_id();
    for (int index = 0; index < request->data_list_size(); ++index) {
      BusinessType business_type = request->data_list(index).business_type();
      bool user_feedback_status = (
          request->data_list(index).user_feedback_status());
      string query_sql = StringPrintf(kQueryFormat, 
                                      kTable, 
                                      user_id.c_str(),
                                      static_cast<int>(business_type));
      string insert_sql = StringPrintf(kInsertFormat,
                                       kTable,
                                       user_id.c_str(),
                                       business_type,
                                       static_cast<int>(user_feedback_status));
      string update_sql = StringPrintf(kUpdateFormat,
                                       kTable,
                                       static_cast<int>(user_feedback_status),
                                       user_id.c_str(),
                                       static_cast<int>(business_type));
      LOG(INFO) << "query_sql:" << query_sql;
      LOG(INFO) << "insert_sql:" << insert_sql;
      LOG(INFO) << "update_sql:" << update_sql;
      std::shared_ptr< sql::ResultSet > result_set(
          statement->executeQuery(query_sql));
      vector<string> result_vec; 
      while (result_set->next()) {
        string result = result_set->getString("user_feedback_status");
        if (!result.empty()) {
          result_vec.push_back(result);
        }
      }  
      if (!result_vec.empty()) {
        if (result_vec.size() == 1) {
          statement->executeUpdate(update_sql);
          LOG(INFO) << "do update (user_id: " << user_id 
                    << ", business_type: " << business_type << ")";
        } else {
          LOG(ERROR) << "query result cnt should not exceed 1";
          response->set_response_reason("too much result");
          return false;
        } 
      } else {
        statement->executeUpdate(insert_sql);
        LOG(INFO) << "do insert (user_id: " << user_id 
                  << ", business_type: " << business_type << ")";
      }
      UserFeedbackResponse_DataList* data_list = response->add_data_list();
      data_list->set_business_type(business_type);
      data_list->set_response_status(true);
    }
    statement.reset();
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "# ERR: " << e.what()
               << " (MySQL error code: " << e.getErrorCode()
               << ", SQLState: " << e.getSQLState() << ")"; 
    response->set_response_reason(e.what());
    return false;
  } catch (const std::runtime_error &e) {
    LOG(ERROR) << "# ERR: " << e.what();
    response->set_response_reason(e.what());
    return false;
  }
  response->set_user_id(request->user_id());
  response->set_status(kSuccess);
  response->set_err_msg("");
  response->set_response_reason("update ok");
  LOG(INFO) << "InsertUserFeedback update to db success";
  return true;
}

bool DbHandler::QueryUserFeedback(UserFeedbackQueryResquest *request,
                                  UserFeedbackQueryResponse *response) {
  LOG(INFO) << "QueryUserFeedback";
  const string& user_id = request->user_id();
  string query_sql = StringPrintf(kQueryFeedbackFormat,
                                  kTable,
                                  user_id.c_str());
  LOG(INFO) << "query_sql:" << query_sql;
  try {
    sql::Driver * driver = sql::mysql::get_driver_instance();
    std::shared_ptr< sql::Connection > connection(
        driver->connect(mysql_server_->host(), 
                        mysql_server_->user(), 
                        mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::shared_ptr< sql::Statement > statement(connection->createStatement());
    std::shared_ptr< sql::ResultSet > result_set(
        statement->executeQuery(query_sql));
    
    map<int, bool> result_map; 
    while (result_set->next()) {
      try {
        int type = result_set->getInt("business_type");
        bool status = result_set->getBoolean("user_feedback_status");
        result_map.insert(std::make_pair(type, status));
      } catch (const std::exception& e) {
        LOG(INFO) << "result_set parse failed:" << e.what();
        continue;
      }
    } 
    if (!result_map.empty()) {
      for (const auto& result : result_map) {
        BusinessType business_type = (BusinessType)(result.first);
        bool status = result.second;
        UserFeedbackQueryResponse_DataList* data_list = (
          response->add_data_list());
        data_list->set_business_type(business_type);
        data_list->set_user_feedback_status(status);
        data_list->set_response_status(true);
      }
    } else {
      LOG(INFO) << "no record user_id: " << user_id;
      response->set_user_id(user_id);
      response->set_response_reason("no record");
      return false;
    }
    statement.reset(); /* free the object inside  */
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "# ERR: " << e.what()
               << " (MySQL error code: " << e.getErrorCode()
               << ", SQLState: " << e.getSQLState() << ")"; 
    response->set_response_reason(e.what());
    return false;
  } catch (const std::runtime_error &e) {
    LOG(ERROR) << "# ERR: " << e.what();
    response->set_response_reason(e.what());
    return false;
  }
  response->set_user_id(user_id);
  response->set_status(kSuccess);
  response->set_err_msg("");
  response->set_response_reason("query ok");
  LOG(INFO) << "QueryUserFeedback update to db success";
  return true;
}

}  // namespace feedback 
