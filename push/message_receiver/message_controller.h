// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_MESSAGE_RECEIVER_MESSAGE_CONTROLLER_H_
#define PUSH_MESSAGE_RECEIVER_MESSAGE_CONTROLLER_H_

#include "push/message_receiver/message_processor.h"
#include "push/message_receiver/message_receiver.h"

namespace message_receiver {

class MsgController {
 public:
  MsgController();
  virtual ~MsgController();
  void Run();

 private:
  shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue_;
  unique_ptr<MsgReceiver> msg_receiver_;
  unique_ptr<MsgProcessorThread> msg_processor_thread_;
  DISALLOW_COPY_AND_ASSIGN(MsgController);
};

}  // namespace message_receiver

#endif  // PUSH_MESSAGE_RECEIVER_MESSAGE_CONTROLLER_H_
