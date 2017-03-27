// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_MESSAGE_RECEIVER_MESSAGE_RECEIVER_H_
#define PUSH_MESSAGE_RECEIVER_MESSAGE_RECEIVER_H_

#include <memory>

#include "base/compat.h"
#include "base/concurrent_queue.h"
#include "base/file/proto_util.h"
#include "base/log.h"
#include "base/string_util.h"
#include "base/thread.h"
#include "proto/mysql_config.pb.h"
#include "third_party/gflags/gflags.h"
#include "third_party/jsoncpp/json.h"
#include "util/kafka/kafka_util.h"
#include "util/net/http_client/http_client.h"

#include "push/message_receiver/message_parser.h"
#include "push/proto/push_meta.pb.h"
#include "push/util/redis_util.h"
#include "push/util/time_util.h"

namespace message_receiver {

class MsgReceiver {
 public:
  MsgReceiver();
  virtual ~MsgReceiver();
  virtual void Receive(const std::string& topic,
                       shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue) = 0;
};

class RedisMsgReceiver : public MsgReceiver {
 public:
  RedisMsgReceiver();
  virtual ~RedisMsgReceiver();
  virtual void Receive(const std::string& topic,
                       shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue);

 private:
  unique_ptr<recommendation::RedisSubThread> receiver_thread_;
  DISALLOW_COPY_AND_ASSIGN(RedisMsgReceiver);
};

class KafkaMsgReceiver : public MsgReceiver {
 public:
  KafkaMsgReceiver();
  virtual ~KafkaMsgReceiver();
  virtual void Receive(const std::string& topic,
                       shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue);

 private:
  unique_ptr<recommendation::KafkaConsumerThread> receiver_thread_;
  DISALLOW_COPY_AND_ASSIGN(KafkaMsgReceiver);
};

class KafkaPoolMsgRecevier : public MsgReceiver {
 public:
  KafkaPoolMsgRecevier();
  virtual ~KafkaPoolMsgRecevier();
  virtual void Receive(const std::string& topic,
                       shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue);
 private:
  unique_ptr<recommendation::KafkaConsumerThreadPool> receiver_thread_pool_;
  DISALLOW_COPY_AND_ASSIGN(KafkaPoolMsgRecevier);
};

}  // namespace message_receiver

#endif  // PUSH_MESSAGE_RECEIVER_MESSAGE_RECEIVER_H_
