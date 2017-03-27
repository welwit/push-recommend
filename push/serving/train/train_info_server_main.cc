// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"
#include "util/net/http_server/http_handler.h"
#include "util/net/http_server/http_server.h"

#include "push/serving/train/train_info_handler.h"

DEFINE_int32(listen_port, 9039, "");
DEFINE_int32(http_server_thread_num, 8, "");
DEFINE_int32(data_source, 1, "time table data source, 1:ctrip, 2:baidu");

DEFINE_string(mysql_config,
  "config/push/train/mysql_server.conf", "");
DEFINE_string(baidu_search_template_file,
  "config/push/train/baidu_time_table.template",
  "baidu search template file");
DEFINE_string(baidu_search_url_format,
  "https://www.baidu.com/s?wd=%s", "baidu search url format");
DEFINE_string(ctrip_template_file,
  "config/push/train/time_table.template", 
  "ctrip template file");
DEFINE_string(ctrip_url_format,
  "http://trains.ctrip.com/trainbooking/TrainSchedule/%s/", 
  "ctrip url format");
DEFINE_string(trainno_template_format,
  "config/push/train/trainno_%s.template","trainno template file");

int main(int argc, char **argv) {
  base::AtExitManager at_exit;
  mobvoi::SetupBinaryVersion();
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
  
  LOG(INFO) << "start train info server ...";

  util::HttpServer http_server(
      FLAGS_listen_port,
      static_cast<size_t>(FLAGS_http_server_thread_num));

  serving::TimeTableQueryHandler time_table_query_handler;
  serving::TimeTableUpdateHandler time_table_update_handler;
  serving::TrainnoUpdateHandler train_no_update_handler;
  serving::StatusHandler status_handler;

  auto time_table_query_callback = std::bind(
      &serving::TimeTableQueryHandler::HandleRequest, 
      &time_table_query_handler, 
      std::placeholders::_1, 
      std::placeholders::_2);
  auto time_table_update_callback = std::bind(
      &serving::TimeTableUpdateHandler::HandleRequest, 
      &time_table_update_handler, 
      std::placeholders::_1, 
      std::placeholders::_2);
  auto train_no_update_callback = std::bind(
      &serving::TrainnoUpdateHandler::HandleRequest, 
      &train_no_update_handler, 
      std::placeholders::_1, 
      std::placeholders::_2);
  auto status_callback = std::bind(
      &serving::StatusHandler::HandleRequest, 
      &status_handler, 
      std::placeholders::_1, 
      std::placeholders::_2);

  util::DefaultHttpHandler time_table_query_http_handler(
      time_table_query_callback);
  util::DefaultHttpHandler time_table_update_http_handler(
      time_table_update_callback);
  util::DefaultHttpHandler train_no_update_http_handler(
      train_no_update_callback);
  util::DefaultHttpHandler status_http_handler(status_callback);

  http_server.RegisterHttpHandler("/train/query_timetable", 
                                  &time_table_query_http_handler);
  http_server.RegisterHttpHandler("/train/update_timetable", 
                                  &time_table_update_http_handler);
  http_server.RegisterHttpHandler("/train/update_trainno", 
                                  &train_no_update_http_handler);
  http_server.RegisterHttpHandler("/train/status", 
                                  &status_http_handler);

  http_server.Serv();
  return 0;
}
