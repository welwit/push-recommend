// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/message_receiver/message_controller.h"

DECLARE_bool(use_raw_msg_processor);
DECLARE_int32(receiver_type);
DECLARE_string(receiver_raw_msg_topic);
DECLARE_string(receiver_json_msg_topic);

namespace message_receiver {

MsgController::MsgController() {
  msg_queue_ = std::make_shared<mobvoi::ConcurrentQueue<string>>();
  CHECK(msg_queue_.get());
  if (FLAGS_receiver_type == kKafkaThreadReceiver) {
    msg_receiver_.reset(new KafkaMsgReceiver());
  } else if (FLAGS_receiver_type == kRedisThreadReceiver) {
    msg_receiver_.reset(new RedisMsgReceiver());
  } else {
    msg_receiver_.reset(new KafkaPoolMsgRecevier());
  }
  CHECK(msg_receiver_.get());
  msg_processor_thread_.reset(
      new MsgProcessorThread(FLAGS_use_raw_msg_processor));
  CHECK(msg_processor_thread_.get());
}

MsgController::~MsgController() {}

void MsgController::Run() {
  string topic;
  if (FLAGS_use_raw_msg_processor) {
    topic = FLAGS_receiver_raw_msg_topic;
  } else {
    topic = FLAGS_receiver_json_msg_topic;
  }
  msg_receiver_->Receive(topic, msg_queue_);
  msg_processor_thread_->set_msg_queue(msg_queue_);
  msg_processor_thread_->Start();
}

}  // namespace message_receiver
