// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/compat.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"
#include "third_party/gtest/gtest.h"
#include "push/serving/train/time_table_dumper.h"

DEFINE_string(mysql_config,
  "config/push/train/mysql_server.conf", "");
DEFINE_string(baidu_search_template_file,
  "config/push/train/baidu_time_table.template",
  "baidu search template file");
DEFINE_string(baidu_search_url_format,
  "https://www.baidu.com/s?wd=%s", "baidu search url format");
DEFINE_string(ctrip_template_file,
  "config/push/train/time_table.template", 
  "ctrip template file");
DEFINE_string(ctrip_url_format,
  "http://trains.ctrip.com/trainbooking/TrainSchedule/%s/", 
  "ctrip url format");

using namespace train;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  mobvoi::SetupBinaryVersion();
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  BaiduTimeTableDumper baidu_time_table_dumper;
  string train_no = "K21";
  vector<TimeTable> time_table_vector;
  EXPECT_TRUE(baidu_time_table_dumper.FetchTimeTableByTrainNo(
        train_no, &time_table_vector));
  for (auto& time_table : time_table_vector) {
    LOG(INFO) << "time_table:" << time_table.Utf8DebugString();
  }
  return 0;
}
