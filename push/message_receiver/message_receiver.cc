// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/message_receiver/message_receiver.h"

DECLARE_int32(recevicer_thread_pool_size);

namespace message_receiver {

MsgReceiver::MsgReceiver() {}

MsgReceiver::~MsgReceiver() {}

RedisMsgReceiver::RedisMsgReceiver() {}

RedisMsgReceiver::~RedisMsgReceiver() {}

void RedisMsgReceiver::Receive(
      const std::string& topic,
      std::shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue) {
  receiver_thread_.reset(new recommendation::RedisSubThread(msg_queue, topic));
  CHECK(receiver_thread_.get());
  receiver_thread_->Start();
}

KafkaMsgReceiver::KafkaMsgReceiver() {}

KafkaMsgReceiver::~KafkaMsgReceiver() {}

void KafkaMsgReceiver::Receive(
      const std::string& topic,
      std::shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue) {
  receiver_thread_.reset(new recommendation::KafkaConsumerThread());
  CHECK(receiver_thread_.get());
  receiver_thread_->BuildConsumer(msg_queue);
  receiver_thread_->Start();
}

KafkaPoolMsgRecevier::KafkaPoolMsgRecevier() {}

KafkaPoolMsgRecevier::~KafkaPoolMsgRecevier() {}

void KafkaPoolMsgRecevier::Receive(
      const std::string& topic,
      std::shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue) {
  receiver_thread_pool_.reset(
      new recommendation::KafkaConsumerThreadPool(
        FLAGS_recevicer_thread_pool_size));
  CHECK(receiver_thread_pool_.get());
  receiver_thread_pool_->SetSharedConcurrentQueue(msg_queue);
  receiver_thread_pool_->Start();
}

}  // namespace message_receiver
