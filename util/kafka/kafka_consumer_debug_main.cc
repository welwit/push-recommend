// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/log.h"
#include "base/time.h"
#include "third_party/gflags/gflags.h"

#include "util/kafka/kafka_util.h"

DEFINE_int32(kafka_producer_flush_timeout, 10000, "");

DEFINE_string(kafka_producer_config,
    "config/util/kafka/kafka_producer_test.conf", "");
DEFINE_string(kafka_consumer_config,
    "config/util/kafka/kafka_consumer_test.conf", "");

using namespace recommendation;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
  std::shared_ptr<mobvoi::ConcurrentQueue<string>> message_queue;
  message_queue = std::make_shared<mobvoi::ConcurrentQueue<string>>();
  KafkaConsumerThread kafka_consumer_thread;
  kafka_consumer_thread.BuildConsumer(message_queue);
  kafka_consumer_thread.Start();
  while (true) {
    LOG(INFO) << "current queue size:" << message_queue->Size();
    mobvoi::Sleep(10);
  }
  return 0;
}
