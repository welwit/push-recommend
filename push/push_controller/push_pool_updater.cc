// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/push_pool_updater.h"

#include <ctime>
#include <memory>
#include <sstream>

#include "base/file/proto_util.h"
#include "base/log.h"
#include "base/string_util.h"
#include "base/time.h"
#include "third_party/gflags/gflags.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"
#include "push/util/time_util.h"
#include "push/push_controller/business_factory.h"
#include "push/push_controller/business_processor.h"
#include "push/push_controller/update_time_backup.h"
#include "push/util/common_util.h"
#include "push/util/zookeeper_util.h"

DECLARE_bool(use_cluster_mode);
DECLARE_bool(enable_update_push_pool);

DECLARE_int32(pool_update_internal);

DECLARE_string(mysql_config);
DECLARE_string(zookeeper_watched_path);

namespace {

static const string kTimeFormat = "%Y-%m-%d %H:%M:%S";

string kQueryFormat =
    "SELECT id, user_id, business_type, business_time, order_detail, "
    "updated, order_status, finished_time "
    "FROM %s WHERE updated > '%s';";

string kQueryFormatV2 =
    "SELECT id, user_id, business_type, business_time, order_detail, "
    "updated, order_status, finished_time, fingerprint_id "
    "FROM %s WHERE updated > '%s' AND mod(fingerprint_id, %d) = %d;";

static const char kTable[] = "user_order_info";

}

namespace push_controller {

PushPoolUpdater::PushPoolUpdater() {}

PushPoolUpdater::~PushPoolUpdater() {}

void PushPoolUpdater::Run() {
  PushEventProcessor push_event_processor;
  UserOrderProcessor user_order_processor;
  vector<UserOrderInfo> user_orders;
  while (true) {
    if (FLAGS_enable_update_push_pool) {
      user_orders.clear();
      if (!user_order_processor.QueryUserOrder(&user_orders)) {
        LOG(ERROR) << "query user order failed";
      } else {
        push_event_processor.UpdatePushEvent(&user_orders);
      }
    } else {
      LOG(INFO) << "push pool need not update";
    }
    mobvoi::Sleep(FLAGS_pool_update_internal);
  }
}

UserOrderProcessor::UserOrderProcessor(): last_updated_(0) {
  VLOG(1) << "UserOrderProcessor::UserOrderProcessor()";
  UpdateTimeBackup* update_time_backup = Singleton<UpdateTimeBackup>::get();
  time_t last_updated_backup;
  if (update_time_backup->ReadTimeFromDb(&last_updated_backup)) {
    last_updated_ = last_updated_backup;
  }
  mysql_server_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
  VLOG(1) << "Read mysqlConf success, file:" << FLAGS_mysql_config;
}

UserOrderProcessor::~UserOrderProcessor() {}

bool UserOrderProcessor::QueryUserOrder(
    vector<UserOrderInfo>* user_order_vector) {
  VLOG(2) << "UserOrderProcessor::QueryUserOrder()";
  try {
    time_t now = time(0);
    sql::Driver * driver = sql::mysql::get_driver_instance();
    std::unique_ptr< sql::Connection >
        connection(driver->connect(
            mysql_server_->host(),
            mysql_server_->user(),
            mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::unique_ptr< sql::Statement > statement(connection->createStatement());
    string last_updated_string;
    recommendation::TimestampToDatetime(last_updated_, &last_updated_string,
                                        "%Y-%m-%d %H:%M:%S");
    string query;
    if (FLAGS_use_cluster_mode) {
      recommendation::ZkManager* zk_manager =
        Singleton<recommendation::ZkManager>::get();
      int node_pos = zk_manager->GetNodePos(FLAGS_zookeeper_watched_path);
      int node_cnt = zk_manager->GetTotalNodeCnt(FLAGS_zookeeper_watched_path);
      if (node_pos < 0 || node_cnt <= 0) {
        LOG(ERROR) << "Get node data from zk failed, node_pos=" << node_pos
                   << ", node_cnt=" << node_cnt;
        return false;
      }
      query = StringPrintf(kQueryFormatV2.c_str(), kTable,
                           last_updated_string.c_str(), node_cnt, node_pos);
    } else {
      query = StringPrintf(kQueryFormat.c_str(), kTable,
                           last_updated_string.c_str());
    }
    VLOG(2) << "query order success, updated:" << last_updated_string
            << ", sql:" << query;
    std::unique_ptr<sql::ResultSet> result_set(statement->executeQuery(query));
    while (result_set->next()) {
      UserOrderInfo user_order_info;
      user_order_info.set_id(result_set->getString("id"));
      user_order_info.set_user_id(result_set->getString("user_id"));
      user_order_info.set_business_type(
          static_cast<BusinessType>(result_set->getInt("business_type")));
      user_order_info.set_order_detail(result_set->getString("order_detail"));
      user_order_info.set_order_status(
          static_cast<OrderStatus>(result_set->getInt("order_status")));
      time_t business_time, finished_time, updated_time;
      string business_time_string = (
          result_set->getString("business_time"));
      recommendation::DatetimeToTimestamp(
          business_time_string, &business_time, kTimeFormat);
      user_order_info.set_business_time(business_time);
      string updated_string = result_set->getString("updated");
      recommendation::DatetimeToTimestamp(
          updated_string, &updated_time, kTimeFormat);
      user_order_info.set_updated(updated_time);
      string finished_string = result_set->getString("finished_time");
      recommendation::DatetimeToTimestamp(
          finished_string, &finished_time, kTimeFormat);
      user_order_info.set_finished_time(finished_time);
      user_order_info.set_fingerprint_id(
          result_set->getInt64("fingerprint_id"));
      user_order_vector->push_back(user_order_info);
    }
    LOG(INFO) << "QUERY success, order total count:"
              << user_order_vector->size() << ", time at:" << now;
    last_updated_ = now;
    UpdateTimeBackup* update_time_backup = Singleton<UpdateTimeBackup>::get();
    update_time_backup->BackupTimeToDb(last_updated_);
    statement.reset();
    return true;
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "SQLException: " << e.what();
    return false;
  }
}

PushEventProcessor::PushEventProcessor() {}

PushEventProcessor::~PushEventProcessor() {}

void PushEventProcessor::UpdatePushEvent(
    vector<UserOrderInfo>* user_orders) {
  VLOG(2) << "The number of user orders:" << user_orders->size();
  int success_num = 0;
  int user_orders_number = user_orders->size();
  for(auto it = user_orders->begin();
      it != user_orders->end(); ++it) {
    LOG(INFO) << "READ user order: " << ProtoToString(*it);
    BusinessType business_type = it->business_type();
    BusinessFactory* business_factory = Singleton<BusinessFactory>::get();
    BaseBusinessProcessor* business_processor = (
        business_factory->GetBusinessProcessor(business_type));
    if (!business_processor) {
      LOG(ERROR) << "Can't find processor, business:" << business_type;
      continue;
    }
    vector<PushEventInfo> push_events;
    if (!business_processor->CreatePushEvent(&(*it), &push_events)) {
      LOG(ERROR) << "create push event failed";
      continue;
    }
    for (auto iter = push_events.begin();
        iter != push_events.end(); ++iter) {
      LOG(INFO) << "CREATE push event: " << ProtoToString(*iter);
    }
    if (!business_processor->UpdateEventsToDb(push_events)) {
      LOG(ERROR) << "update events to db failed";
      continue;
    }
    ++success_num;
  }
  if (success_num != user_orders_number) {
    LOG(WARNING) << "some order fail to update push evnets";
  }
}

}  // namespace push_controller
