// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/push_sender.h"

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
#include "util/net/http_client/http_client.h"

#include "push/util/common_util.h"

DECLARE_string(mysql_config);
DECLARE_string(link_server);

namespace {

static const char kPackageName[] = "com.mobvoi.ticwear.home";
static const char kTable[] = "push_event_info";

static const char kUpdateFormat[] =
  "UPDATE %s set push_status='%d' WHERE id='%s';";

}

namespace push_controller {

PushSender::PushSender() {
  VLOG(1) << "PushSender::PushSender()";
  mysql_server_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
  VLOG(1) << "Read MySQLConf from file:" << FLAGS_mysql_config;
}

PushSender::~PushSender() {}

bool PushSender::SendPush(MessageType message_type,
                          const string& message_desc,
                          const string& user_id,
                          const Json::Value& message_content) {
  util::HttpClient http_client;
  http_client.SetHttpMethod(util::HttpMethod::kPost);
  http_client.AddHeader("Content-Type", "application/json; charset=utf-8");
  Json::Value message, push;
  message["type"] = message_type;
  message["desc"] = message_desc;
  message["content"] = message_content;
  push["message"] = message;
  push["user_name"] = user_id;
  push["package_name"] = kPackageName;
  string post_data = JsonToString(push);
  LOG(INFO) << "SEND push request: " << post_data;
  http_client.SetPostData(post_data);
  if (http_client.FetchUrl(FLAGS_link_server)) {
    string result = http_client.ResponseBody();
    try {
      Json::Value value;
      Json::Reader reader;
      if (reader.parse(result, value)) {
        if (value.isMember("status") && value["status"] == true) {
          LOG(INFO) << "SEND push success, response:" << result;
          return true;
        }
      }
    } catch (const std::exception& e) {
      LOG(ERROR) << "send push failed: " << e.what() << ", response: "
        << http_client.ResponseBody();
      return false;
    }
  }
  LOG(ERROR) << "send push failed: response: " << http_client.ResponseBody();
  return false;
}

bool PushSender::UpdatePushStatus(const PushEventInfo& push_event_info,
                                  PushStatus push_status) {
  try {
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection>
        connection(driver->connect(
            mysql_server_->host(),
            mysql_server_->user(),
            mysql_server_->password()));
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    statement->execute("USE " + mysql_server_->database());
    string update_sql = StringPrintf(kUpdateFormat,
                                     kTable,
                                     static_cast<int>(push_status),
                                     push_event_info.id().c_str());
    statement->executeUpdate(update_sql);
    return true;
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "SQLException: " << e.what() << ", event_id:"
               << push_event_info.id();
    return false;
  }
}

}  // namespace push_controller
