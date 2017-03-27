// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"

#include "recommendation/news/crawler/xinhua_fetcher.h"

DEFINE_int32(fetch_timeout_ms, 5000, "");

DEFINE_string(mysql_config_file,
    "config/recommendation/news/crawler/mysql_test.conf", "mysql config");
DEFINE_string(link_template,
    "config/recommendation/news/crawler/link.tpl", "");

using namespace recommendation;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  BaseFetcher* fetcher = new XinhuaFetcher;
  fetcher->Init();
  vector<string> url_vec;
  vector<Json::Value> result_vec;
  fetcher->FetchLinkVec(&url_vec);
  fetcher->FetchContentVec(url_vec, &result_vec);
  return 0;
}
