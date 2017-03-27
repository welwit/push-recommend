// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/compat.h"
#include "base/log.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"

#include "push/proto/train_meta.pb.h"
#include "push/serving/train/train_data_fetcher.h"
#include "push/serving/train/trainno_dumper.h"

using namespace train;

DEFINE_string(url_format, "http://trains.ctrip.com/checi/%s/", "url format");
DEFINE_string(template_format,
  "config/push/train/trainno_%s.template", "template file");
DEFINE_string(mysql_config,
  "config/push/train/mysql_server.conf", "");

namespace {
static const char* kTypelist[] = {
  "zhida", "gaotie", "dongche", "tekuai", "kuaiche", "other"};
}

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);

  TrainDataFetcher* train_data_fetcher = new TrainDataFetcher();
  TrainnoDumper trainno_dumper;
  Json::Value result;
  for (size_t i = 0; i < arraysize(kTypelist); ++i) {
    string url = StringPrintf(FLAGS_url_format.c_str(), kTypelist[i]);
    string template_file = StringPrintf(FLAGS_template_format.c_str(), 
                                        kTypelist[i]);
    train_data_fetcher->FetchTimeTable(url, 
                                       template_file, 
                                       &result);
    trainno_dumper.DumpResultIntoDb(result, TrainnoType(i+1));
  }
  return 0;
}
