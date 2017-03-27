// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"
#include "util/net/http_server/http_handler.h"
#include "util/net/http_server/http_server.h"
#include "push/message_producer/message_producer_handler.h"

DEFINE_int32(listen_port, 9093, "");
DEFINE_int32(http_server_thread_num, 8, "");
DEFINE_int32(kafka_producer_flush_timeout, 10000, "");

DEFINE_string(kafka_producer_config,
    "config/push/message_producer/kafka_producer_test.conf", "");
DEFINE_string(kafka_consumer_config,
    "config/push/message_producer/kafka_consumer_test.conf", "");

using namespace message_producer;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  mobvoi::SetupBinaryVersion();
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
  
  LOG(INFO) << "message_producer server start...";
  
  util::HttpServer http_server(FLAGS_listen_port,
      static_cast<size_t>(FLAGS_http_server_thread_num));
  message_producer::StatusHandler status_handler;
  message_producer::MessageHandler message_handler;
  auto status_callback = std::bind(
      &message_producer::StatusHandler::HandleRequest, &status_handler, 
      std::placeholders::_1, std::placeholders::_2);
  auto message_callback = std::bind(
      &message_producer::MessageHandler::HandleRequest, &message_handler, 
      std::placeholders::_1, std::placeholders::_2);
  util::DefaultHttpHandler status_http_handler(status_callback);
  util::DefaultHttpHandler message_http_handler(message_callback);
  http_server.RegisterHttpHandler("/message_producer/status", 
                                  &status_http_handler);
  http_server.RegisterHttpHandler("/message_producer/message", 
                                  &message_http_handler);
  http_server.Serv();
  return 0;
}
