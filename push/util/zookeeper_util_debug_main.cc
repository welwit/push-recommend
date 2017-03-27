// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/log.h"
#include "base/time.h"
#include "third_party/gflags/gflags.h"
#include "push/util/zookeeper_util.h"

DEFINE_int32(zookeeper_timeout, 3000, "(In MS)");
DEFINE_int32(zookeeper_reconnect_attempt, 5, "");
DEFINE_int32(zookeeper_check_interval, 5, "(In Seconds)");
DEFINE_int32(zookeeper_default_node_port, 9312, "");

DEFINE_string(zookeeper_cluster_addr, "ali-hz-dev:2181", "");
DEFINE_string(zookeeper_this_node_prefix,
    "/ns/intelligent_push/push_controller/replica_", "");
DEFINE_string(zookeeper_watched_path,
    "/ns/intelligent_push/push_controller", "");

using namespace recommendation;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  ZkManager* zk_manager = Singleton<ZkManager>::get();
  zk_manager->set_is_register_this_node(true);
  zk_manager->AddWatchPath(FLAGS_zookeeper_watched_path);
  zk_manager->Start();
  while (true) {
    mobvoi::Sleep(1000);
  }
  return 0;
}
