// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"

#include "push/push_controller/news/news_push_processor.h"

using namespace push_controller;

DEFINE_bool(is_test, true, "");
DEFINE_bool(use_cluster_mode, false, "");
DEFINE_bool(use_version_filter, false, "");
DEFINE_bool(use_channel_filter, false, "");
DEFINE_bool(enable_load_devices, false, "");

DEFINE_int32(mysql_page_size, 10000, "");
DEFINE_int32(recommendation_content_expire_seconds, 3 * 24 * 3600, "");
DEFINE_int32(redis_expire_time,
    48 * 60 * 60, "redis expire time internal");
DEFINE_int32(redis_max_sleep,
    128, "redis reconnect max sleep time");
DEFINE_int32(zookeeper_timeout, 3000, "(In MS)");
DEFINE_int32(zookeeper_reconnect_attempt, 5, "");
DEFINE_int32(zookeeper_check_interval, 5, "(In Seconds)");
DEFINE_int32(zookeeper_default_node_port, 9048, "");

DEFINE_string(mysql_config,
    "config/push/push_controller/mysql_server_test.conf", "");
DEFINE_string(link_server,
    "http://link-server/api/push_message/to_user_device", "link server addr");
DEFINE_string(recommender_server,
    "http://news-recommender-server-main/news/recommender/recommendation?user_id=%s", "");
DEFINE_string(device_test, "a960d251aa89828eb49c8ec701efb6bd", "");
DEFINE_string(redis_conf,
    "config/onebox/news/redis.conf", "redis config");
DEFINE_string(redis_sub_topic,
    "push_controller", "redis subscribe topic");
DEFINE_string(zookeeper_cluster_addr, "ali-hz-dev:2181", "");
DEFINE_string(zookeeper_this_node_prefix,
    "/ns/intelligent_push/push_controller/replica_", "");
DEFINE_string(zookeeper_watched_path,
    "/ns/intelligent_push/push_controller", "");
DEFINE_string(news_version_greater, "tic_4.9.0", "");
DEFINE_string(news_version_equal, "tic_4.9.0", "");
DEFINE_string(burypoint_kafka_log_server, "http://heartbeat-server/log/realtime", "");
DEFINE_string(burypoint_kafka_topic, "intelligent-push", "");
DEFINE_string(burypoint_upload_push_log_event_type, "test_push_controller_push_suc", "");
DEFINE_string(burypoint_upload_fail_log_event_type, "test_push_controller_push_fail", "");

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
  NewsPushProcessor n;
  n.Process();
  return 0;
}
