// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include <memory>

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "onebox/http_handler_dispatcher.h"
#include "third_party/gflags/gflags.h"
#include "util/net/http_server/http_handler.h"
#include "util/net/http_server/http_server.h"

#include "push/serving/flight/flight_data_updater.h"
#include "push/serving/flight/flight_info_handler.h"

DEFINE_int32(listen_port, 9040, "");
DEFINE_int32(http_server_thread_num, 8, "");
DEFINE_int32(redis_expire_time,
    48 * 60 * 60, "redis expire time internal");
DEFINE_int32(redis_max_sleep,
    128, "redis reconnect max sleep time");
DEFINE_int32(flight_update_duration, 600, "flight info update duration");

DEFINE_string(redis_conf,
    "config/push/flight/redis_test.conf", "redis config");
DEFINE_string(redis_sub_topic,
    "push_controller", "redis subscribe topic");
DEFINE_string(mysql_config,
  "config/push/flight/mysql_server.conf", "mysql config");

using namespace flight;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  mobvoi::SetupBinaryVersion();
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
  
  LOG(INFO) << "start flight info server ...";

  std::unique_ptr<FlightDataUpdater> flight_data_updater;
  flight_data_updater.reset(new FlightDataUpdater);
  flight_data_updater->Start();

  util::HttpServer http_server(
      FLAGS_listen_port,
      static_cast<size_t>(FLAGS_http_server_thread_num));

  serving::FlightInfoQueryHandler query_handler;
  serving::StatusHandler status_handler;

  auto query_callback = std::bind(
      &serving::FlightInfoQueryHandler::HandleRequest, 
      &query_handler, 
      std::placeholders::_1, 
      std::placeholders::_2);
  auto status_callback = std::bind(
      &serving::StatusHandler::HandleRequest, 
      &status_handler, 
      std::placeholders::_1, 
      std::placeholders::_2);

  util::DefaultHttpHandler query_http_handler(query_callback);
  util::DefaultHttpHandler status_http_handler(status_callback);
  
  http_server.RegisterHttpHandler("/flight/query_flightinfo", 
                                  &query_http_handler);
  http_server.RegisterHttpHandler("/flight/status", 
                                  &status_http_handler);

  http_server.Serv();
  flight_data_updater->Join();
  return 0;
}
