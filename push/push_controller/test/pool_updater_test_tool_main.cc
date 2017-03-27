// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/compat.h"
#include "base/log.h"
#include "base/singleton.h"
#include "third_party/gflags/gflags.h"

#include "push/push_controller/business_factory.h"
#include "push/push_controller/business_processor.h"
#include "push/push_controller/push_pool_updater.h"

DEFINE_bool(use_cluster_mode, false, "");
DEFINE_bool(enable_update_push_pool, true, "");

DEFINE_int32(pool_update_internal, 120, "push pool update schedule internal");
DEFINE_int32(zookeeper_timeout, 3000, "(In MS)");
DEFINE_int32(zookeeper_reconnect_attempt, 5, "");
DEFINE_int32(zookeeper_check_interval, 5, "(In Seconds)");
DEFINE_int32(zookeeper_default_node_port, 9312, "");

DEFINE_string(mysql_config,
    "config/recommendation/push_controller/mysql_server_test.conf", "");
DEFINE_string(zookeeper_cluster_addr, "ali-hz-dev:2181", "");
DEFINE_string(zookeeper_this_node_prefix,
    "/ns/intelligent_push/push_controller/replica_", "");
DEFINE_string(zookeeper_watched_path,
    "/ns/intelligent_push/push_controller", "");

using namespace push_controller;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  // singleton of BusinessFactory
  BusinessFactory* business_factory = Singleton<BusinessFactory>::get();
  // new and register business processor
  BaseBusinessProcessor* train_business_processor = (
      new TrainBusinessProcessor());
  business_factory->RegisterBusinessProcessor(
      BusinessType::kBusinessTrain, train_business_processor);
  BaseBusinessProcessor* flight_business_processor = (
      new FlightBusinessProcessor());
  business_factory->RegisterBusinessProcessor(
      BusinessType::kBusinessFlight, flight_business_processor);
  BaseBusinessProcessor* movie_business_processor = (
      new MovieBusinessProcessor());
  business_factory->RegisterBusinessProcessor(
      BusinessType::kBusinessMovie, movie_business_processor);

  UserOrderProcessor user_order_processor;
  vector<UserOrderInfo> user_orders;
  CHECK(user_order_processor.QueryUserOrder(&user_orders));
  LOG(INFO) << "The number of user order:" << user_orders.size();
  for(auto it = user_orders.begin();
      it != user_orders.end(); ++it) {
    LOG(INFO) << "user order:\n" << it->Utf8DebugString();
    BusinessType business_type = it->business_type();
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
    LOG(INFO) << "The number of push events:" << push_events.size();
    for (auto iter = push_events.begin();
        iter != push_events.end(); ++iter) {
      LOG(INFO) << "push event:\n" << iter->Utf8DebugString();
    }
  }
  return 0;
}
