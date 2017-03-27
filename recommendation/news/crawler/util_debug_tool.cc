// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"

#include "recommendation/news/crawler/util.h"

DEFINE_int32(fetch_timeout_ms, 5000, "");

DEFINE_string(url, "", "");

DEFINE_string(mysql_config_file,
    "config/recommendation/news/crawler/mysql_test.conf", "mysql config");
DEFINE_string(link_template,
    "config/recommendation/news/crawler/link.tpl", "");

static const char* kContentTpl[] = {
  "config/recommendation/news/crawler/tencent/tencent_content1.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content2.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content3.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content4.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content5.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content6.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content7.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content8.tpl",
};

using namespace recommendation;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
 
  std::vector<std::string> tpl_vec; 
  for (auto& tpl : kContentTpl) {
    tpl_vec.push_back(tpl);
  }
  Json::Value content;
  if (FetchNewsContent(FLAGS_url, tpl_vec, &content)) {
    LOG(INFO) << "Success, content:" << content.toStyledString();
  } else {
    LOG(INFO) << "Failed";
  }
  return 0;
}
