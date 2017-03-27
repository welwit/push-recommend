// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/update_time_backup.h"

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

#include "push/util/time_util.h"

DECLARE_string(mysql_config);

namespace {

static const string kTimeFormat = "%Y-%m-%d %H:%M:%S";

static const char kQuerySql[] =
    "SELECT time_backup FROM push_controller_backup "
    "WHERE id = 'ORDER_UPDATE_TIME';";

static const char kInsertFormat[] =
    "INSERT INTO push_controller_backup (id, time_backup) "
    "VALUES('ORDER_UPDATE_TIME', FROM_UNIXTIME('%d'));";

static const char kUpdateFormat[] =
    "UPDATE push_controller_backup SET time_backup = FROM_UNIXTIME('%d') "
    "WHERE id = 'ORDER_UPDATE_TIME';";

}

namespace push_controller {

UpdateTimeBackup::UpdateTimeBackup() {
  VLOG(1) << "UpdateTimeBackup::UpdateTimeBackup()";
  mysql_server_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
}

UpdateTimeBackup::~UpdateTimeBackup() {}

bool UpdateTimeBackup::ReadTimeFromDb(time_t* time_backup) {
  try {
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection>
        connection(driver->connect(
            mysql_server_->host(),
            mysql_server_->user(),
            mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    std::unique_ptr<sql::ResultSet> result_set(
        statement->executeQuery(kQuerySql));
    if (result_set->rowsCount() > 0) {
      string time_backup_string;
      while (result_set->next()) {
        time_backup_string = result_set->getString("time_backup");
        break;
      }
      recommendation::DatetimeToTimestamp(time_backup_string,
                                          time_backup,
                                          kTimeFormat);
      VLOG(1) << "Read time from db success, time_backup_string:"
              << time_backup_string;
      return true;
    } else {
      LOG(WARNING) << "Read time from db failed";
      return false;
    }
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "SQLException: " << e.what();
    return false;
  }
}

bool UpdateTimeBackup::BackupTimeToDb(time_t time_backup) {
  try {
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection>
        connection(driver->connect(
            mysql_server_->host(),
            mysql_server_->user(),
            mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    string select_sql = kQuerySql;
    string insert_sql = StringPrintf(kInsertFormat,
                                     static_cast<int>(time_backup));
    string update_sql = StringPrintf(kUpdateFormat,
                                     static_cast<int>(time_backup));
    std::unique_ptr<sql::ResultSet> result_set(
        statement->executeQuery(select_sql));
    if (result_set->rowsCount() > 0) {
      statement->executeUpdate(update_sql);
      VLOG(1) << "Update backup time to db, time_backup:" << time_backup;
    } else {
      statement->executeUpdate(insert_sql);
      VLOG(1) << "Insert backup time to db, time_backup:" << time_backup;
    }
    return true;
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "SQLException: " << e.what();
    return false;
  }
}

}  // namespace push_controller
