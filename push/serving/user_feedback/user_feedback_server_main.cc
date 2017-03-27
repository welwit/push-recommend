// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "onebox/http_handler_dispatcher.h"
#include "third_party/gflags/gflags.h"
#include "util/net/http_server/http_handler.h"
#include "util/net/http_server/http_server.h"

#include "push/serving/user_feedback/user_feedback_handler.h"

DEFINE_int32(listen_port, 9033, "");
DEFINE_int32(http_server_thread_num, 8, "");

DEFINE_string(mysql_config,
  "config/push/user_feedback/mysql_server.conf", "");

using namespace feedback;

int main(int argc, char **argv) {
  base::AtExitManager at_exit;
  mobvoi::SetupBinaryVersion();
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "start user feedback server ...";

  util::HttpServer http_server(
      FLAGS_listen_port,
      static_cast<size_t>(FLAGS_http_server_thread_num));
  
  serving::UserFeedbackHandler user_feedback_handler;
  serving::UserFeedbackQueryHandler user_feedback_query_handler;
  serving::StatusHandler status_handler;
  
  auto user_feedback_callback = std::bind(
      &serving::UserFeedbackHandler::HandleRequest, 
      &user_feedback_handler, 
      std::placeholders::_1, 
      std::placeholders::_2);
  auto user_feedback_query_callback = std::bind(
      &serving::UserFeedbackQueryHandler::HandleRequest, 
      &user_feedback_query_handler, 
      std::placeholders::_1, 
      std::placeholders::_2);
  auto status_callback = std::bind(
      &serving::StatusHandler::HandleRequest, 
      &status_handler, 
      std::placeholders::_1, 
      std::placeholders::_2);

  util::DefaultHttpHandler user_feedback_http_handler(user_feedback_callback);
  util::DefaultHttpHandler user_feedback_query_http_handler(
      user_feedback_query_callback);
  util::DefaultHttpHandler status_http_handler(status_callback);

  http_server.RegisterHttpHandler("/feedback", 
                                  &user_feedback_http_handler);
  http_server.RegisterHttpHandler("/query_feedback", 
                                  &user_feedback_query_http_handler);
  http_server.RegisterHttpHandler("/status", 
                                  &status_http_handler);

  http_server.Serv();
  return 0;
}
