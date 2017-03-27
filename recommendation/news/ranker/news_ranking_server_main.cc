// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/file/proto_util.h"
#include "base/log.h"
#include "util/net/http_server/http_handler.h"
#include "util/net/http_server/http_server.h"

#include "recommendation/news/ranker/ranking_handler.h"
#include "recommendation/news/ranker/ranking_thread.h"

DEFINE_int32(listen_port, 9169, "");
DEFINE_int32(http_server_thread_num, 8, "");
DEFINE_int32(news_ranker_execute_interval, 1800, "");
DEFINE_string(mysql_config_file,
    "config/recommendation/news/crawler/mysql_test.conf", "mysql config");

using namespace recommendation;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  CosineRanker* ranker = new CosineRanker;
  RankingThread ranking_thread(ranker);
  ranking_thread.Start();

  LOG(INFO) << "start news ranking server ...";
  util::HttpServer http_server(
      FLAGS_listen_port,
      static_cast<size_t>(FLAGS_http_server_thread_num));
  serving::StatusHandler status_handler;
  auto status_callback = std::bind(
      &serving::StatusHandler::HandleRequest, &status_handler,
      std::placeholders::_1, std::placeholders::_2);
  util::DefaultHttpHandler status_http_handler(status_callback);
  http_server.RegisterHttpHandler("/news_ranking/status",
                                  &status_http_handler);
  http_server.Serv();
  return 0;
}
