// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"

#include "recommendation/news/ranker/cosine_ranker.h"

DEFINE_string(mysql_config_file,
    "config/recommendation/news/crawler/mysql_test.conf", "mysql config");

using namespace recommendation;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
  CosineRanker ranker;
  RankerInput ranker_input;
  RankerOutput ranker_output;
  ranker.PrepareRankingInput(&ranker_input);
  ranker.ExecuteRanking(ranker_input, &ranker_output);
  ranker.ProcessRankingOutput(ranker_output);
  return 0;
}
