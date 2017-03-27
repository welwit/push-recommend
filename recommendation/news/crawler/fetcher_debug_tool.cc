// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/crawler/base_fetcher.h"

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/file/proto_util.h"
#include "base/hash.h"
#include "base/log.h"
#include "base/time.h"
#include "push/util/time_util.h"
#include "third_party/gflags/gflags.h"
#include "third_party/re2//re2/re2.h"
#include "util/protobuf/proto_json_format.h"

#include "recommendation/news/crawler/util.h"

DEFINE_int32(fetch_timeout_ms, 5000, "");

DEFINE_string(mysql_config_file,
    "config/recommendation/news/crawler/mysql_test.conf", "mysql config");
DEFINE_string(link_template,
    "config/recommendation/news/crawler/link.tpl", "");

namespace {

static const char* kSeedArray[] = {
  "http://tech.qq.com/",
};

static const char* kContentTpl[] = {
  "config/recommendation/news/crawler/tencent/tencent_content1.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content2.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content3.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content4.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content5.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content6.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content7.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content8.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content9.tpl",
};

using namespace recommendation;

static const UrlTopicType kToFetchedUrlTopicArray[] = {
  {"http://tech.qq.com/", kEnt},

};

}

namespace recommendation {

class TestFetcher : public BaseFetcher {
 public:
  TestFetcher() {}
  virtual ~TestFetcher() {}
  virtual bool InitSeedVec() {
    for (auto& seed : kSeedArray) {
      seed_vec_.push_back(seed);
    }
    return true;
  }
  virtual bool InitContentTemplateVec() {
    for (auto& tpl : kContentTpl) {
      content_template_vec_.push_back(tpl);
    }
    return true;
  }

  virtual bool InitUrlTopicMap() {
    for (auto& url_topic : kToFetchedUrlTopicArray) {
      url_topic_map_.insert(std::make_pair(url_topic.url, url_topic.topic));
    }
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestFetcher);
};

}  // namespace recommendation

using namespace recommendation;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
  BaseFetcher* fetcher = new TestFetcher();
  
  fetcher->Init();
  vector<string> url_vec;
  vector<Json::Value> result_vec;
  fetcher->FetchLinkVec(&url_vec);
  fetcher->FetchContentVec(url_vec, &result_vec);
  return 0;
}
