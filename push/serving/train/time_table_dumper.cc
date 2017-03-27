// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/serving/train/time_table_dumper.h"

#include <stdlib.h>

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

DECLARE_int32(data_source);
DECLARE_string(mysql_config);

DECLARE_string(mysql_config);
DECLARE_string(baidu_search_template_file);
DECLARE_string(baidu_search_url_format);
DECLARE_string(ctrip_template_file);
DECLARE_string(ctrip_url_format);

namespace train {

BaseTimeTableDumper::BaseTimeTableDumper() {
  LOG(INFO) << "Construct BaseTimeTableDumper";
  Init();
}

BaseTimeTableDumper::~BaseTimeTableDumper() {}

void BaseTimeTableDumper::Init() {
  train_data_fetcher_.reset(new TrainDataFetcher());
  mysql_server_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
  LOG(INFO) << "Init mysql config from file:" << FLAGS_mysql_config;
}

bool BaseTimeTableDumper::UpdateTimeTable() {
  vector<string> trainno_vector;
  if (!QueryAllTrainNo(&trainno_vector)) {
    return false;
  }

  size_t batch_size = 50;
  vector<vector<string>> trainno_2d_vector;
  size_t set_count = trainno_vector.size() / batch_size; 
  for (size_t i = 0; i < set_count; ++i) {
    size_t b = batch_size * i;
    size_t e = batch_size * (i + 1);
    if (i == set_count - 1) {
      e = trainno_vector.size();
    }
    vector<string> sub_vector(trainno_vector.begin() + b,
                              trainno_vector.begin() + e);
    trainno_2d_vector.push_back(sub_vector);
  }

  // TODO(xjliu): using concurrency
  bool whole_success = true;
  vector<TimeTable> time_table_vector;
  for (auto it = trainno_2d_vector.begin(); 
      it != trainno_2d_vector.end(); ++it) {
    LOG(INFO) << "current batch size:" << it->size();
    time_table_vector.clear();
    FetchTimeTable(*it, &time_table_vector);
    if (!DumpResultIntoDb(time_table_vector)) {
      whole_success = false;
    }
  }
  LOG(INFO) << "update time table success, whole_success:" << whole_success;
  return whole_success;
}

QueryStatus BaseTimeTableDumper::QueryTimeTable(
    const string& train_no,
    const string& depart_station, 
    vector<TimeTable>* time_table_vector) {
  string table = "train_time_table";
  string query_format = (
      "SELECT station_no,get_in_time,depart_time,stay_time FROM %s "
      "WHERE train_no='%s' AND station_name='%s';"
  );
  string query_sql = StringPrintf(query_format.c_str(),
                                  table.c_str(),
                                  train_no.c_str(),
                                  depart_station.c_str());
  string query_station_format = (
      "SELECT station_no,station_name,get_in_time,depart_time,stay_time "
      "FROM %s WHERE train_no='%s' AND station_no>='%d';"
  );
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
      if (result_set->rowsCount() > 0) {
        int station_no = 0;
        while (result_set->next()) {
          station_no = result_set->getInt("station_no");
          break; // only one record
        }
        string query_station_sql = StringPrintf(query_station_format.c_str(),
                                                table.c_str(),
                                                train_no.c_str(),
                                                station_no);
        result_set.reset(statement->executeQuery(query_station_sql));
        if (result_set->rowsCount() > 0) { 
          while (result_set->next()) {
            TimeTable time_table;
            time_table.set_train_no(train_no);
            time_table.set_station_no(
                atoi(result_set->getString("station_no").c_str()));
            time_table.set_station_name(result_set->getString("station_name"));
            time_table.set_get_in_time(result_set->getString("get_in_time"));
            time_table.set_depart_time(result_set->getString("depart_time"));
            time_table.set_stay_time(result_set->getString("stay_time"));
            time_table_vector->push_back(time_table);
          }
          return kSuccess;
        } else {
          LOG(ERROR) << "Not found next station list. train_no:"
                     << train_no << ",station:" << depart_station;
          return kNotFound;
        }
      } else {
        LOG(ERROR) << "Not found station_no. train_no:" << train_no
                   << ",station:" << depart_station;
        return kNotFound;
      }
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "# ERR: " << e.what()
               << " (MySQL error code: " << e.getErrorCode()
               << ", SQLState: " << e.getSQLState() << ")"; 
    return kFailure;
  } catch (const std::runtime_error &e) {
    LOG(ERROR) << "runtime error:" << e.what();
    return kFailure;
  }
}

bool BaseTimeTableDumper::FetchAndDumpTimeTable(
    const vector<string>& train_no_vector) {
  vector<TimeTable> time_table_vector;
  FetchTimeTable(train_no_vector, &time_table_vector);
  if (!DumpResultIntoDb(time_table_vector)) {
    return false;
  }
  LOG(INFO) << "fetch and dump time table success, count:" 
            << train_no_vector.size();
  return true;
}

bool BaseTimeTableDumper::QueryAllTrainNo(vector<string>* result_vector) {
  string table = "train_no"; 
  string query_format = "SELECT train_no FROM %s;";
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
      string train_no = result_set->getString("train_no");
      if (!train_no.empty()) {
        result_vector->push_back(train_no);
      }
    }
    statement.reset();
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "# ERR: " << e.what()
               << " (MySQL error code: " << e.getErrorCode()
               << ", SQLState: " << e.getSQLState() << ")"; 
    return false;
  } catch (const std::runtime_error &e) {
    LOG(ERROR) << "runtime error:" << e.what();
    return false;
  }
  LOG(INFO) << "query train_no result size:" << result_vector->size();
  return true;
}

void BaseTimeTableDumper::FetchTimeTable(
    const vector<string>& result_vector,
    vector<TimeTable>* time_table_vector) {
  string train_no;
  for (auto it = result_vector.begin(); it != result_vector.end(); ++it) {
    train_no = *it;
    if (!FetchTimeTableByTrainNo(train_no, time_table_vector)) {
      LOG(ERROR) << "Fetch time table by no failed, no:" << train_no;
      continue;
    }
  }
}

bool BaseTimeTableDumper::DumpResultIntoDb(
    const vector<TimeTable>& time_table_vector) {
  string table = "train_time_table"; 
  string query_format = (
      "SELECT train_no,station_no,station_name,get_in_time,"
      "depart_time,stay_time FROM %s "
      "WHERE train_no='%s' AND station_no='%d';"
  );
  string insert_format = (
      "INSERT INTO %s (train_no,station_no,station_name,"
      "get_in_time,depart_time,stay_time) "
      "VALUES('%s','%d','%s','%s','%s','%s');"
  );
  string update_format = (
      "UPDATE %s SET "
      "station_name='%s',get_in_time='%s',depart_time='%s',"
      "stay_time='%s' WHERE train_no='%s' AND station_no='%d';"
  );
  try {
    sql::Driver * driver = sql::mysql::get_driver_instance();
    std::shared_ptr<sql::Connection> connection(
      driver->connect(mysql_server_->host(), 
                      mysql_server_->user(), 
                      mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::shared_ptr<sql::Statement> statement(connection->createStatement());
    string query_sql, insert_sql, update_sql;
    for (auto it = time_table_vector.begin();
        it != time_table_vector.end(); ++it) {
      try {
        query_sql = StringPrintf(query_format.c_str(),
                                 table.c_str(),
                                 it->train_no().c_str(),
                                 it->station_no());
        std::shared_ptr<sql::ResultSet> result_set(
          statement->executeQuery(query_sql));
        if (result_set->rowsCount() > 0) {
          update_sql = StringPrintf(update_format.c_str(),
                                    table.c_str(),
                                    it->station_name().c_str(),
                                    it->get_in_time().c_str(),
                                    it->depart_time().c_str(),
                                    it->stay_time().c_str(),
                                    it->train_no().c_str(),
                                    it->station_no());
          statement->executeUpdate(update_sql); 
        } else {
          insert_sql = StringPrintf(insert_format.c_str(),
                                    table.c_str(),
                                    it->train_no().c_str(),
                                    it->station_no(),
                                    it->station_name().c_str(),
                                    it->get_in_time().c_str(),
                                    it->depart_time().c_str(),
                                    it->stay_time().c_str());
          statement->executeUpdate(insert_sql); 
        }
        continue;
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
  } catch (const std::runtime_error &e) {
    LOG(ERROR) << "runtime error:" << e.what();
    return false;
  }
  LOG(INFO) << "succeed in updating to db";
  return true;
}

CtripTimeTableDumper::CtripTimeTableDumper() {
  LOG(INFO) << "Construct CtripTimeTableDumper";
}

CtripTimeTableDumper::~CtripTimeTableDumper() {}

bool CtripTimeTableDumper::FetchTimeTableByTrainNo(
    const string& train_no, vector<TimeTable>* time_table_vector) {
  static const size_t kMinReturnItemCount = 7;
  Json::Value result, data_list, item_list;
  string url = StringPrintf(FLAGS_ctrip_url_format.c_str(), 
                            train_no.c_str());
  bool ret = train_data_fetcher_->FetchTimeTable(url, 
                                                 FLAGS_ctrip_template_file, 
                                                 &result);
  if (!ret) {
    return false;
  }
  try {
    data_list = result["time_table"]["data_list"]; 
  } catch (const std::exception& e) {
    LOG(ERROR) << "parse data_list failed:" << e.what();
    return false;
  } 
  // VLOG only for debug
  VLOG(1) << "data_list:" << data_list.toStyledString();
  for (size_t index = 0; index < data_list.size(); ++index) {
    try {
      item_list = data_list[index]["item_list"];
      VLOG(1) << "item_list:" << item_list.toStyledString();
    } catch (const std::exception& e) {
      LOG(ERROR) << "parse item_list failed:" << e.what();
      return false;
    }
    if (item_list.size() < kMinReturnItemCount) {
      LOG(ERROR) << "invalid item_list size:" << item_list.size()
                 << ",at least:" << kMinReturnItemCount;
      return false;
    }
    TimeTable time_table;
    try {
      time_table.set_train_no(train_no);
      time_table.set_station_no(atoi(item_list[1].asString().c_str()));
      time_table.set_station_name(item_list[2].asString());
      time_table.set_get_in_time(item_list[3].asString());
      time_table.set_depart_time(item_list[4].asString());
      time_table.set_stay_time(item_list[5].asString());
    } catch (const std::exception &e) {
      LOG(ERROR) << "set time table, failed:" << e.what();
      return false;
    }
    time_table_vector->push_back(time_table);
  }
  LOG(INFO) << "time_table_vector size:" << time_table_vector->size();
  return true;
}


BaiduTimeTableDumper::BaiduTimeTableDumper() {
  LOG(INFO) << "Construct BaiduTimeTableDumper";
}

BaiduTimeTableDumper::~BaiduTimeTableDumper() {}

bool BaiduTimeTableDumper::FetchTimeTableByTrainNo(
    const string& train_no, vector<TimeTable>* time_table_vector) {
  static const size_t kBaiduMinReturnItemCount = 6;
  Json::Value result, data_list, item_list;
  string url = StringPrintf(FLAGS_baidu_search_url_format.c_str(),
                            train_no.c_str());
  bool ret = train_data_fetcher_->FetchTimeTable(
      url, FLAGS_baidu_search_template_file, &result);
  if (!ret) {
    return false;
  }
  try {
    data_list = result["time_table"]["data_list"];
  } catch (const std::exception& e) {
    LOG(ERROR) << "parse to data_list failed:" << e.what();
    return false;
  }
  LOG(INFO) << "data_list:" << data_list.toStyledString();
  for (size_t index = 0; index < data_list.size(); ++index) {
    try {
      item_list = data_list[index]["item_list"];
    } catch (const std::exception& e) {
      LOG(ERROR) << "parse to item_list failed:" << e.what();
      return false;
    }
    LOG(INFO) << "item_list:" << item_list.toStyledString();
    if (item_list.size() < kBaiduMinReturnItemCount) {
      LOG(ERROR) << "invalid item_list size:" << item_list.size()
                 << ",baidu at least:" << kBaiduMinReturnItemCount;
      return false;
    }
    TimeTable time_table;
    try {
      time_table.set_train_no(train_no);
      time_table.set_station_no(atoi(
            item_list[static_cast<Json::ArrayIndex>(0)].asString().c_str()));
      time_table.set_station_name(item_list[1].asString());
      time_table.set_get_in_time(item_list[2].asString());
      time_table.set_depart_time(item_list[3].asString());
      time_table.set_stay_time(item_list[4].asString());
      time_table.set_running_time(item_list[5].asString());
    } catch (const std::exception &e) {
      LOG(ERROR) << "set time table, failed:" << e.what();
      return false;
    }
    time_table_vector->push_back(time_table);
  }
  LOG(INFO) << "time_table_vector size:" << time_table_vector->size();
  return true;
}

}  // namespace train 
