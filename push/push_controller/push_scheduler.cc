// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/file/proto_util.h"
#include "base/log.h"
#include "base/string_util.h"
#include "base/time.h"
#include "third_party/gflags/gflags.h"
#include "third_party/jsoncpp/json.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"

#include "push/push_controller/business_factory.h"
#include "push/push_controller/push_processor.h"
#include "push/push_controller/push_scheduler.h"
#include "push/util/common_util.h"
#include "push/util/time_util.h"
#include "push/util/zookeeper_util.h"

DECLARE_bool(use_cluster_mode);
DECLARE_bool(enable_schedule_push);
DECLARE_int32(push_scheduler_internal);
DECLARE_string(mysql_config);
DECLARE_string(zookeeper_watched_path);

namespace {

static const string kTimeFormat = "%Y-%m-%d %H:%M:%S";

static const char kSelectFormat[] =
  "SELECT id, order_id, user_id, business_type, event_type, push_detail, "
  "push_time, business_key, is_realtime, push_status, updated, finished_time "
  "FROM push_event_info WHERE date(push_time) IN ('%s', '%s', '%s');";

static const char kSelectFormatV2[] =
  "SELECT id, order_id, user_id, business_type, event_type, push_detail, "
  "push_time, business_key, is_realtime, push_status, updated, finished_time, "
  "fingerprint_id "
  "FROM push_event_info WHERE date(push_time) IN ('%s', '%s', '%s') "
  "AND mod(fingerprint_id, %d) = %d;";

static const char kFetchPhoneNicknameFormat[] =
  "SELECT package_name, name, device_id FROM nickname WHERE name = '%s' AND "
  "package_name = 'com.mobvoi.companion' ORDER BY updated DESC;";

static const char kFetchWatchNicknameFormat[] =
  "SELECT package_name, name, device_id FROM nickname WHERE name = '%s' AND "
  "package_name = 'com.mobvoi.ticwear.home' ORDER BY updated DESC;";

static const char kFetchDeviceFormat[] =
  "SELECT device_type, bluetooth_match_id, wear_model, wear_version, "
  "wear_version_channel, wear_os, phone_model, phone_version, phone_os, "
  "admin_area, locality, sub_locality, latitude, longitude FROM device "
  "WHERE id = '%s';";

}

namespace push_controller {

PushScheduler::PushScheduler() {
  VLOG(1) << "PushScheduler::PushScheduler()";
  mysql_server_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
  VLOG(1) << "Read MySQLConf from file:" << FLAGS_mysql_config;
  filter_manager_.reset(new FilterManager());
}

PushScheduler::~PushScheduler() {}

void PushScheduler::Run() {
  LOG(INFO) << "PushScheduler::Run() ...";
  while (true) {
    if (FLAGS_enable_schedule_push) {
      vector<PushEventInfo> push_events;
      FetchPushEvents(&push_events);
      if (!push_events.empty()) {
        FetchNicknameTable(&push_events);
        FetchDeviceTable(&push_events);
        LOG(INFO) << "Finish to fetch push events, size: "
          << push_events.size();
        FilterPushEvents(&push_events);
        LOG(INFO) << "Finish to filter push events, size: " << push_events.size();
      }
      if (!push_events.empty()) {
        PushToQueue(push_events);
        VLOG(2) << "Push to queue finished";
      }
    } else {
      LOG(INFO) << "push need not schedule";
    }
    mobvoi::Sleep(FLAGS_push_scheduler_internal);
  }
}

void PushScheduler::FetchPushEvents(vector<PushEventInfo>* push_events) {
  VLOG(2) << "PushScheduler::FetchPushEvents() ...";
  string today, tomorrow, yesterday;
  recommendation::MakeDate(0, &today);
  recommendation::MakeDate(1, &tomorrow);
  recommendation::MakeDate(-1, &yesterday);
  try {
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection>
        connection(driver->connect(
            mysql_server_->host(),
            mysql_server_->user(),
            mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    string select_sql;
    if (FLAGS_use_cluster_mode) {
      recommendation::ZkManager* zk_manager =
        Singleton<recommendation::ZkManager>::get();
      int node_pos = zk_manager->GetNodePos(FLAGS_zookeeper_watched_path);
      int node_cnt = zk_manager->GetTotalNodeCnt(FLAGS_zookeeper_watched_path);
      if (node_pos < 0 || node_cnt <= 0) {
        LOG(ERROR) << "Get node data from zk failed, node_pos=" << node_pos
                   << ", node_cnt=" << node_cnt;
        return;
      }
      select_sql = StringPrintf(kSelectFormatV2, yesterday.c_str(),
                                today.c_str(), tomorrow.c_str(),
                                node_cnt, node_pos);
    } else {
      select_sql = StringPrintf(kSelectFormat, yesterday.c_str(),
                                today.c_str(), tomorrow.c_str());
    }
    VLOG(2) << "SELECT command: " << select_sql;
    std::unique_ptr<sql::ResultSet> result_set(
        statement->executeQuery(select_sql));
    while (result_set->next()) {
      PushEventInfo push_event;
      push_event.set_id(result_set->getString("id"));
      push_event.set_order_id(result_set->getString("order_id"));
      push_event.set_user_id(result_set->getString("user_id"));
      push_event.set_business_type(
          static_cast<BusinessType>(result_set->getInt("business_type")));
      push_event.set_event_type(
          static_cast<EventType>(result_set->getInt("event_type")));
      push_event.set_push_detail(result_set->getString("push_detail"));
      push_event.set_business_key(result_set->getString("business_key"));
      push_event.set_is_realtime(
          static_cast<bool>(result_set->getInt("is_realtime")));
      push_event.set_push_status(
          static_cast<PushStatus>(result_set->getInt("push_status")));
      string push_time_string = result_set->getString("push_time");
      string updated_string = result_set->getString("updated");
      string finished_time_string = result_set->getString("finished_time");
      time_t push_time, updated_time, finished_time;
      recommendation::DatetimeToTimestamp(push_time_string,
                                          &push_time,
                                          kTimeFormat);
      recommendation::DatetimeToTimestamp(updated_string,
                                          &updated_time,
                                          kTimeFormat);
      recommendation::DatetimeToTimestamp(finished_time_string,
                                          &finished_time,
                                          kTimeFormat);
      push_event.set_push_time(push_time);
      push_event.set_updated(updated_time);
      push_event.set_finished_time(finished_time);
      push_event.set_fingerprint_id(result_set->getInt64("fingerprint_id"));
      push_events->push_back(push_event);
    }
    VLOG(2) << "finish to query, size:" << push_events->size();
  } catch (const sql::SQLException& e) {
    LOG(ERROR) << "SQLException: " << e.what();
  }
}

void PushScheduler::FetchNicknameTable(vector<PushEventInfo>* push_events) {
  try {
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection>
        connection(driver->connect(
            mysql_server_->host(),
            mysql_server_->user(),
            mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    for (auto it = push_events->begin(); it != push_events->end(); ++it) {
      string query_watch = StringPrintf(kFetchWatchNicknameFormat,
                                        it->user_id().c_str());
      string query_phone = StringPrintf(kFetchPhoneNicknameFormat,
                                        it->user_id().c_str());
      VLOG(2) << "query watch nickname command:" << query_watch;
      VLOG(2) << "query phone nickname command:" << query_phone;
      std::unique_ptr<sql::ResultSet> result_set(
          statement->executeQuery(query_watch));
      auto watch_nickname = it->mutable_watch_nickname();
      if (result_set->rowsCount() > 0) {
        while (result_set->next()) {
          watch_nickname->set_package_name(
              result_set->getString("package_name"));
          watch_nickname->set_name(result_set->getString("name"));
          watch_nickname->set_device_id(result_set->getString("device_id"));
          break;
        }
      } else {
        LOG(WARNING) << "query watch nickname no result, user:"
                     << it->user_id();
      }
      result_set.reset(statement->executeQuery(query_phone));
      auto phone_nickname = it->mutable_phone_nickname();
      if (result_set->rowsCount() > 0) {
        while (result_set->next()) {
          phone_nickname->set_package_name(
              result_set->getString("package_name"));
          phone_nickname->set_name(result_set->getString("name"));
          phone_nickname->set_device_id(result_set->getString("device_id"));
          break;
        }
      } else {
        LOG(WARNING) << "query phone nickname no result, user:"
                     << it->user_id();
      }
    }
    VLOG(2) << "finish to FetchNicknameTable";
  } catch (const sql::SQLException& e) {
    LOG(ERROR) << "SQLException: " << e.what();
  }
}

void PushScheduler::FetchDeviceTable(vector<PushEventInfo>* push_events) {
  try {
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection>
        connection(driver->connect(
            mysql_server_->host(),
            mysql_server_->user(),
            mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    for (auto it = push_events->begin(); it != push_events->end(); ++it) {
      string watch_device_id = it->watch_nickname().device_id();
      string phone_device_id = it->phone_nickname().device_id();
      std::unique_ptr<sql::ResultSet> result_set;

      string query_watch;
      string query_phone;
      auto watch_device = it->mutable_watch_device();
      auto phone_device = it->mutable_phone_device();
      if (!watch_device_id.empty()) {
        string query_watch = StringPrintf(kFetchDeviceFormat,
            watch_device_id.c_str());
        VLOG(2) << "query watch device command: " << query_watch;
        result_set.reset(statement->executeQuery(query_watch));
        if (result_set->rowsCount() > 0) {
          while (result_set->next()) {
            watch_device->set_id(watch_device_id);
            watch_device->set_device_type(result_set->getString("device_type"));
            watch_device->set_bluetooth_match_id(
                result_set->getString("bluetooth_match_id"));
            watch_device->set_wear_model(result_set->getString("wear_model"));
            watch_device->set_wear_version(
                result_set->getString("wear_version"));
            watch_device->set_wear_version_channel(
                result_set->getString("wear_version_channel"));
            watch_device->set_wear_os(result_set->getString("wear_os"));
            watch_device->set_phone_model(result_set->getString("phone_model"));
            watch_device->set_phone_version(
                result_set->getString("phone_version"));
            watch_device->set_phone_os(result_set->getString("phone_os"));
            watch_device->set_admin_area(result_set->getString("admin_area"));
            watch_device->set_locality(result_set->getString("locality"));
            watch_device->set_sub_locality(
                result_set->getString("sub_locality"));
            watch_device->set_latitude(result_set->getDouble("latitude"));
            watch_device->set_longitude(result_set->getDouble("longitude"));
            break;
          }
        } else {
          LOG(WARNING) << "query watch device no result, user:"
                       << it->user_id();
        }
      } else {
        LOG(WARNING) << "watch_device_id is null, user:" << it->user_id();
      }

      if (!phone_device_id.empty()) {
        string query_phone = StringPrintf(kFetchDeviceFormat,
            phone_device_id.c_str());
        VLOG(2) << "query phone device command:" << query_phone;
        result_set.reset(statement->executeQuery(query_phone));
        if (result_set->rowsCount() > 0) {
          while (result_set->next()) {
            phone_device->set_id(phone_device_id);
            phone_device->set_device_type(result_set->getString("device_type"));
            phone_device->set_bluetooth_match_id(
                result_set->getString("bluetooth_match_id"));
            phone_device->set_wear_model(result_set->getString("wear_model"));
            phone_device->set_wear_version(
                result_set->getString("wear_version"));
            phone_device->set_wear_version_channel(
                result_set->getString("wear_version_channel"));
            phone_device->set_wear_os(result_set->getString("wear_os"));
            phone_device->set_phone_model(result_set->getString("phone_model"));
            phone_device->set_phone_version(
                result_set->getString("phone_version"));
            phone_device->set_phone_os(result_set->getString("phone_os"));
            phone_device->set_admin_area(result_set->getString("admin_area"));
            phone_device->set_locality(result_set->getString("locality"));
            phone_device->set_sub_locality(
                result_set->getString("sub_locality"));
            phone_device->set_latitude(result_set->getDouble("latitude"));
            phone_device->set_longitude(result_set->getDouble("longitude"));
            break;
          }
        } else {
          LOG(WARNING) << "query watch device no result, user:"
                       << it->user_id();
        }
      } else {
        LOG(WARNING) << "phone_device_id is null, user:" << it->user_id();
      }
      VLOG(2) << "push event: " << ProtoToString(*it);
    }
    VLOG(2) << "finish to FetchDeviceTable";
  } catch (const sql::SQLException& e) {
    LOG(ERROR) << "# ERR: " << e.what();
  }
}

void PushScheduler::FilterPushEvents(vector<PushEventInfo>* push_events) {
  VLOG(2) << "Before all filters, size:" << push_events->size();
  filter_manager_->Filtering(push_events);
  VLOG(2) << "After all filters, size:" << push_events->size();
}

void PushScheduler::PushToQueue(const vector<PushEventInfo>& push_events) {
  VLOG(2) << "PushScheduler::PushToQueue, event size:" << push_events.size();
  for (auto it = push_events.begin(); it != push_events.end(); ++it) {
    BusinessType business_type = it->business_type();
    BusinessFactory* business_factory = Singleton<BusinessFactory>::get();
    PushProcessor* push_processor =
      business_factory->GetPushProcessor(business_type);
    if (!push_processor) {
      LOG(ERROR) << "Get push processor failed, business_type:"
                 << business_type;
      continue;
    }
    push_processor->PushToQueue(*it);
    VLOG(2) << "finish to push one to queue, business_type:" << business_type;
  }
}

}  // namespace push_controller
