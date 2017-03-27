// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"
#include "util/net/http_server/http_handler.h"
#include "util/net/http_server/http_server.h"

#include "recommendation/news/recommender/recommendation_handler.h"

using namespace recommendation;

DEFINE_bool(enable_load_devices, true, "");

DEFINE_int32(listen_port, 9167, "");
DEFINE_int32(http_server_thread_num, 8, "");
DEFINE_int32(mysql_page_size, 10000, "");

DEFINE_string(mysql_config,
    "config/recommendation/news/recommender/mysql_test.conf", "");
DEFINE_string(category_file,
    "config/recommendation/news/recommender/toutiao_category.txt", "");
DEFINE_string(location_service,
    "http://location-service/geodecoder?lat=%s&lng=%s&high_precision=true", "");

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  mobvoi::SetupBinaryVersion();
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "News recommender server is starting...";

  util::HttpServer http_server(FLAGS_listen_port,
      static_cast<size_t>(FLAGS_http_server_thread_num));
  StatusHandler status_handler;
  RecommendationHandler recommendation_handler;
  auto status_callback = std::bind(
      &StatusHandler::HandleRequest, &status_handler,
      std::placeholders::_1, std::placeholders::_2);
  auto recommendation_callback = std::bind(
      &RecommendationHandler::HandleRequest, &recommendation_handler,
      std::placeholders::_1, std::placeholders::_2);
  util::DefaultHttpHandler status_http_handler(status_callback);
  util::DefaultHttpHandler recommendation_http_handler(recommendation_callback);
  http_server.RegisterHttpHandler("/news/recommender/status", &status_http_handler);
  http_server.RegisterHttpHandler("/news/recommender/recommendation",
                                  &recommendation_http_handler);
  LOG(INFO) << "News recommender server is started";
  http_server.Serv();
  return 0;
}
