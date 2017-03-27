// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"

#include "recommendation/news/ranker/doc_manager.h"

DEFINE_string(mysql_config_file,
    "config/recommendation/news/crawler/mysql_test.conf", "mysql config");

using namespace recommendation;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
  std::vector<NewsInfo> doc_vec;
  MySQLDocReader doc_reader;
  doc_reader.FetchAllDoc(&doc_vec);
  doc_vec.clear();
  doc_reader.FetchDailyHotDoc(&doc_vec);
  doc_vec.clear();
  doc_reader.FetchDailyRankingDoc(&doc_vec);
  doc_vec.clear();
  return 0;
}
