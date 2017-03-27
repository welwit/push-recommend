// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "third_party/gflags/gflags.h"

#include "push/serving/train/time_table_dumper.h"

DEFINE_string(mysql_config,
  "config/recommendation/train/mysql_server.conf", "");
DEFINE_string(baidu_search_template_file,
  "config/recommendation/train/baidu_time_table.template",
  "baidu search template file");
DEFINE_string(baidu_search_url_format,
  "https://www.baidu.com/s?wd=%s", "baidu search url format");
DEFINE_string(ctrip_template_file,
  "config/recommendation/train/time_table.template", 
  "ctrip template file");
DEFINE_string(ctrip_url_format,
  "http://trains.ctrip.com/trainbooking/TrainSchedule/%s/", 
  "ctrip url format");

using namespace train;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  BaiduTimeTableDumper time_table_dumper;
  time_table_dumper.UpdateTimeTable();
  return 0;
}
