// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"
#include "util/net/http_server/http_handler.h"
#include "util/net/http_server/http_server.h"
#include "push/message_receiver/message_controller.h"
#include "push/message_receiver/message_receiver_handler.h"

using namespace message_receiver;

DEFINE_bool(use_raw_msg_processor, true, "");
DEFINE_bool(enabled_hotel, true, "");
DEFINE_bool(is_use_cluster_mode, false, "");

DEFINE_int32(listen_port, 9047, "");
DEFINE_int32(http_server_thread_num, 8, "");
DEFINE_int32(receiver_type, 3, "");
DEFINE_int32(recevicer_thread_pool_size, 2, "");
DEFINE_int32(redis_expire_time,
    48 * 60 * 60, "redis expire time internal");
DEFINE_int32(redis_max_sleep,
    128, "redis reconnect max sleep time");
DEFINE_int32(kafka_producer_flush_timeout, 10000, "");

DEFINE_string(burypoint_upload_log_event_type, "test_message_receiver_parse", "");
DEFINE_string(burypoint_kafka_log_server, "http://heartbeat-server/log/realtime", "");
DEFINE_string(burypoint_kafka_topic, "intelligent-push", "");
DEFINE_string(receiver_raw_msg_topic, "test_intelligent_push_raw_msg", "");
DEFINE_string(receiver_json_msg_topic, "test_intelligent_push_json_msg", "");
DEFINE_string(mysql_config,
    "config/push/message_receiver/mysql_server_test.conf", "");
DEFINE_string(extract_rule_conf,
    "config/push/message_receiver/extract_rule.conf",
    "extract rule file conf");
DEFINE_string(redis_conf,
    "config/push/message_receiver/redis_test.conf", "redis config");
DEFINE_string(redis_sub_topic,
    "push_controller", "redis subscribe topic");
DEFINE_string(kafka_producer_config,
    "config/push/message_receiver/kafka_producer_test.conf", "");
DEFINE_string(kafka_consumer_config,
    "config/push/message_receiver/kafka_consumer_test.conf", "");

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  mobvoi::SetupBinaryVersion();
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  unique_ptr<MsgController> msg_controller;
  msg_controller.reset(new MsgController());
  msg_controller->Run();

  LOG(INFO) << "start message receiver server ...";
  util::HttpServer http_server(
      FLAGS_listen_port,
      static_cast<size_t>(FLAGS_http_server_thread_num));
  serving::StatusHandler status_handler;
  auto status_callback = std::bind(
      &serving::StatusHandler::HandleRequest, &status_handler,
      std::placeholders::_1, std::placeholders::_2);
  util::DefaultHttpHandler status_http_handler(status_callback);
  http_server.RegisterHttpHandler("/message_receiver/status",
                                  &status_http_handler);
  http_server.Serv();
  return 0;
}
