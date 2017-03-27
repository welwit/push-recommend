// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"

#include "push/util/user_info_helper.h"

using namespace recommendation;

DEFINE_bool(enable_load_devices, true, "");
DEFINE_int32(mysql_page_size, 10000, "");
DEFINE_string(mysql_config,
    "config/recommendation/news/recommender/mysql.conf", "");

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  mobvoi::SetupBinaryVersion();
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  DeviceInfoHelper* d = Singleton<DeviceInfoHelper>::get();
  string latitude, longitude;
  d->GetDeviceCoordinate("446cd65a0b216c8dbdbb33df7de4c02f", &latitude, &longitude);
  return 0;
}
