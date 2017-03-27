// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include <time.h>

#include <string>

#include "base/at_exit.h"
#include "base/basictypes.h"
#include "base/string_util.h"
#include "util/kafka/kafka_util.h"
#include "push/util/redis_util.h"

DEFINE_bool(use_kafka_queue, true, "");

DEFINE_int32(redis_expire_time,
    48 * 60 * 60, "redis expire time internal");
DEFINE_int32(redis_max_sleep,
    128, "redis reconnect max sleep time");
DEFINE_int32(kafka_producer_flush_timeout, 10000, "");

DEFINE_string(redis_conf,
    "config/push/message_receiver/redis_test.conf", "redis config");
DEFINE_string(redis_sub_topic,
    "push_controller", "redis subscribe topic");
DEFINE_string(kafka_producer_config,
    "config/push/message_receiver/kafka_producer_test.conf", "");
DEFINE_string(kafka_consumer_config,
    "config/push/message_receiver/kafka_consumer_test.conf", "");

namespace {

enum MessageType {
  kFlight = 1,
  kTrain = 2,
  kMovie = 3,
  kHotel = 4,
};

struct TestData {
  MessageType type;
  string message_format;
};

static const TestData kTestCases[] = {
  {
    kFlight,
    u8"{\"timestamp\":1472466132712,\"rule_id\":\"1\",\"app_key\":"
    "\"com.mobvoi.companion\",\"user_id\":"
    "\"%s\",\"number\":\"10690155\",\"msg\":"
    "\"【航旅纵横】您2016-%d-%d乘坐的CA1302次航班预计%d:%d起飞，"
    "前序航班已于18:09到达广州白云，提前6分钟到达。请留意机场稍后的登机通知。"
    "（仅供参考）\"}",
  },
  {
    kTrain,
    u8"{\"timestamp\":1472354804462,\"rule_id\":\"2\",\"app_key\":"
    "\"com.mobvoi.companion\",\"user_id\":"
    "\"%s\",\"number\":\"12306\",\"msg\":"
    "\"【铁路客服】订单EA82336605,明先生您已购%d月%d日Z96次"
    "15车3号上铺、3号中铺万州%d:%d开。\"}",
  },
  {
    kMovie,
    u8"{\"timestamp\":1470982675000,\"rule_id\":\"1\",\"app_key\":"
    "\"com.mobvoi.companion\",\"user_id\":\"%s\",\"number\":"
    "\"106571014800278\",\"msg\":\"【腾讯科技】%d月%d日%d:%d"
    "北京耀莱成龙国际影城西红门店盗墓笔记,9号厅巨幕5排9座已买,"
    "凭序列号305267和验证码098224到影院大厅红色耀莱自助取票机"
    "兑换取票「微票儿」\"}",
  },
  {
    kHotel,
    "{\"timestamp\":1474194100095,\"rule_id\":\"1\",\"number\":\"1069800058582034\",\"user_id\":\"%s\",\"app_key\":\"com.mobvoi.companion\",\"msg\":\"【去哪儿网】预订成功：您预订的%d月%d日入住东莞万达文华酒店豪华大床房-无早特惠1间1晚，已支付529.00元。房间已预留，订单最晚可在2小时后于酒店查询到，入住请出示入住人有效身份证件，服务由五彩旅程提供:010-89677252。酒店地址：东莞市东城区东纵大道208号，-0769-22001888。遇到任何问题请戳以下链接联系客服：http://d.qunar.com/cAzzFt 。\"}",
  },
};

struct tm* GetTimeStructure() {
  time_t now = time(0);
  struct tm* ts = localtime(&now);
  return ts;
}

} // namespace

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  recommendation::RedisReuseClient redis_client;
  recommendation::KafkaProducer kafka_producer;
  string topic = "intelligent_push_raw_msg";
  string user_id = "cda0ec9cd357960c572708ad4aae645a";
  struct tm* ts = GetTimeStructure();

  vector<string> messages;
  for (size_t i = 3; i < arraysize(kTestCases); ++i) {
    int hour, min;
    if (kTestCases[i].type == kMovie) {
      hour = ts->tm_hour;
      min = ts->tm_min + 30;
    } else {
      hour = ts->tm_hour + 3;
      min = ts->tm_min;
    }
    string message;
    if (kTestCases[i].type == kHotel) {
      message = StringPrintf(kTestCases[i].message_format.c_str(),
                             user_id.c_str(),
                             ts->tm_mon + 1,
                             ts->tm_mday);
    } else {
      message = StringPrintf(kTestCases[i].message_format.c_str(),
                                  user_id.c_str(),
                                  ts->tm_mon + 1,
                                  ts->tm_mday,
                                  hour,
                                  min);
    }
    if (!FLAGS_use_kafka_queue) {
      redis_client.Publish(topic, message);
    } else {
      messages.push_back(message);
    }
  }
  if (FLAGS_use_kafka_queue) {
    int cnt = 0;
    kafka_producer.Produce(messages, &cnt);
    LOG(INFO) << "produce finished (" << cnt << " messages)";
  }
  return 0;
}
