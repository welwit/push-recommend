// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/log.h"
#include "base/time.h"
#include "third_party/gflags/gflags.h"

#include "util/kafka/kafka_util.h"

DEFINE_int32(message_cnt, 100000, "");
DEFINE_int32(batch_size, 1000, "");
DEFINE_int32(kafka_producer_flush_timeout, 10000, "");

DEFINE_string(kafka_producer_config,
    "config/util/kafka/kafka_producer_test.conf", "");
DEFINE_string(kafka_consumer_config,
    "config/util/kafka/kafka_consumer_test.conf", "");

using namespace recommendation;

static const char kTestFormat[] =
    "-----------------------------------------------------------------------"
    "-----------------------------------------------------------------------"
    "-----------------------------------------------------------------------"
    "-----------------------------------------------------------------------"
    "-----------------------------------------------------------------------"
    "-------------------------------------------------------MobvoiMessage-%d";

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);
  
  KafkaProducer kafka_producer;
  vector<string> messages;
  int id = 0;
  int loop_start = 0;
  while (true) {
    loop_start = id; 
    messages.clear();
    for (; 
        (id < FLAGS_batch_size + loop_start) && (id < FLAGS_message_cnt); 
        ++id) {
      messages.push_back(StringPrintf(kTestFormat, id));
    };
    int cnt = 0;
    kafka_producer.Produce(messages, &cnt);
    LOG(INFO) << "Produced msgCnt:" << cnt;
    if (id >= FLAGS_message_cnt) {
      break;
    }
  }
  LOG(INFO) << "Produced finish";
  return 0;
}
