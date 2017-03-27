// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include <memory>

#include "base/compat.h"
#include "base/file/proto_util.h"
#include "base/log.h"
#include "proto/mysql_config.pb.h"
#include "third_party/gflags/gflags.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"

DEFINE_string(mysql_config,
  "config/recommendation/push_controller/mysql_server.conf", "");

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, false);
  string user_order_info = "user_order_info";
  std::ostringstream ss;
  ss.str("");
  ss << "CREATE TABLE " << user_order_info << " ("
     << "id varchar(200) NOT NULL DEFAULT '' COMMENT '订单编号',"
     << "user_id varchar(200) NOT NULL DEFAULT '' COMMENT '用户ID',"
     << "business_type smallint NULL DEFAULT 0 COMMENT '业务类型',"
     << "business_time timestamp NOT NULL DEFAULT 0 COMMENT '业务启动时间',"
     << "order_detail text NULL DEFAULT '' COMMENT '订单详情',"
     << "updated timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP COMMENT '更新时间',"
     << "order_status smallint NULL DEFAULT 0 COMMENT '订单状态',"
     << "finished_time timestamp NOT NULL DEFAULT 0 COMMENT '订单完成时间',"
     << "PRIMARY KEY (`id`),"
     << "INDEX `user_id` (`user_id` ASC),"
     << "INDEX `business_time` (`business_time` ASC),"
     << "INDEX `updated` (`updated` ASC),"
     << "INDEX `finished_time` (`finished_time` ASC) )"
     << "ENGINE=InnoDB DEFAULT CHARSET=utf8;";
  string create_user_order_sql = ss.str();
  ss.str("");
  string push_event_info = "push_event_info";
  ss << "CREATE TABLE " << push_event_info << " ("
     << "id varchar(200) NOT NULL DEFAULT '' COMMENT '事件编号',"
     << "order_id varchar(200) NOT NULL DEFAULT '' COMMENT '订单编号',"
     << "user_id varchar(200) NOT NULL DEFAULT '' COMMENT '用户ID',"
     << "business_type smallint NULL DEFAULT 0 COMMENT '业务类型',"
     << "event_type smallint NULL DEFAULT 0 COMMENT '事件类型',"
     << "push_detail text NULL DEFAULT '' COMMENT '推送详情',"
     << "push_time timestamp NOT NULL DEFAULT 0 COMMENT '推送时间',"
     << "business_key varchar(200) NOT NULL DEFAULT '' COMMENT '业务关键字',"
     << "is_realtime tinyint(1) NULL DEFAULT 0 COMMENT '是否实时推送',"
     << "push_status smallint NULL DEFAULT 0 COMMENT '推送状态',"
     << "updated timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP COMMENT '更新时间',"
     << "finished_time timestamp NOT NULL DEFAULT 0 COMMENT '事件完成时间',"
     << "PRIMARY KEY (`id`),"
     << "INDEX `order_id` (`order_id` ASC),"
     << "INDEX `user_id` (`user_id` ASC),"
     << "INDEX `push_time` (`push_time` ASC),"
     << "INDEX `business_key` (`business_key` ASC),"
     << "INDEX `updated` (`updated` ASC),"
     << "INDEX `finished_time` (`finished_time` ASC) )"
     << "ENGINE=InnoDB DEFAULT CHARSET=utf8;";
  string create_push_event_sql = ss.str();
  MysqlServer mysql_server;
  file::ReadProtoFromTextFile(FLAGS_mysql_config, &mysql_server);
  try {
    sql::Driver * driver = sql::mysql::get_driver_instance();
    std::shared_ptr< sql::Connection > connection(driver->connect(
          mysql_server.host(),
          mysql_server.user(),
          mysql_server.password()));
    connection->setSchema(mysql_server.database());
    std::shared_ptr< sql::Statement > statement(connection->createStatement());
    statement->execute(create_user_order_sql);
    statement->execute(create_push_event_sql);
    statement.reset();
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "# ERR: " << e.what()
               << " (MySQL error code: " << e.getErrorCode()
               << ", SQLState: " << e.getSQLState() << ")";
    return -1;
  } catch (const std::runtime_error &e) {
    LOG(ERROR) << "# ERR: " << e.what();
    return -1;
  }
  return 0;
}
