// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/log.h"
#include "base/time.h"
#include "third_party/gflags/gflags.h"

#include "push/util/redis_util.h"

DEFINE_int32(redis_expire_time, 48 * 60 * 60, "redis expire time internal");
DEFINE_int32(redis_max_sleep, 128, "redis reconnect max sleep time");

DEFINE_string(redis_conf,
    "config/push/message_receiver/redis_test.conf", "redis config");
DEFINE_string(redis_sub_topic, "push_controller", "redis subscribe topic");

using namespace recommendation;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
  std::shared_ptr<ConcurrentQueue<string>> message_queue;
  message_queue = std::make_shared<ConcurrentQueue<string>>();
  RedisSubThread thread(message_queue, FLAGS_redis_sub_topic);
  thread.Start();
  while (true) {
    LOG(INFO) << "current queue size:" << message_queue->Size();
    mobvoi::Sleep(10);
  }
}
