// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"

#include "recommendation/news/recommender/toutiao_trigger.h"
#include "push/util/common_util.h"

using namespace recommendation;

DEFINE_string(category_file,
    "config/recommendation/news/recommender/toutiao_category.txt", "");

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  mobvoi::SetupBinaryVersion();
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  ToutiaoTrigger* fetcher = Singleton<ToutiaoTrigger>::get();
  vector<StoryDetail> news_details;
  fetcher->Fetch("北京", &news_details);
  return 0;
}
