// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/file/proto_util.h"
#include "base/log.h"
#include "crawler/base/http_status.h"
#include "util/net/http_server/http_handler.h"
#include "util/net/http_server/http_server.h"

#include "recommendation/news/crawler/fetcher_thread.h"
#include "recommendation/news/crawler/news_crawler_handler.h"
#include "recommendation/news/crawler/xinhua_fetcher.h"

DEFINE_int32(listen_port, 9166, "");
DEFINE_int32(http_server_thread_num, 8, "");

DEFINE_int32(fetch_timeout_ms, 5000, "");
DEFINE_int32(news_crawler_process_inteval, 3600, "");

DEFINE_string(mysql_config_file,
    "config/recommendation/news/crawler/mysql_test.conf", "mysql config");
DEFINE_string(link_template,
    "config/recommendation/news/crawler/link.tpl", "");

using namespace recommendation;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  FetcherThreadPool thread_pool;
  thread_pool.Start();
  
  LOG(INFO) << "start news crawler server ...";
  util::HttpServer http_server(
      FLAGS_listen_port,
      static_cast<size_t>(FLAGS_http_server_thread_num));
  serving::StatusHandler status_handler;
  auto status_callback = std::bind(
      &serving::StatusHandler::HandleRequest, &status_handler,
      std::placeholders::_1, std::placeholders::_2);
  util::DefaultHttpHandler status_http_handler(status_callback);
  http_server.RegisterHttpHandler("/news_crawler/status",
                                  &status_http_handler);
  http_server.Serv();
  return 0;
}
