// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_MESSAGE_RECEIVER_MESSAGE_PROCESSOR_H_
#define PUSH_MESSAGE_RECEIVER_MESSAGE_PROCESSOR_H_

#include "push/message_receiver/message_receiver.h"

namespace message_receiver {

class MsgProcessor {
 public:
  MsgProcessor();
  virtual ~MsgProcessor();
  virtual bool Process(const std::string& message) = 0;
  void ProcessQueue(shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue);
  void ShutDown();

 protected:
  bool ParseJsonMessage(const string& message);
  bool ParseFlightOrder(const Json::Value& value,
                        push_controller::UserOrderInfo* user_order_info);
  bool ParseMovieOrder(const Json::Value& value,
                       push_controller::UserOrderInfo* user_order_info);
  bool ParseTrainOrder(const Json::Value& value,
                       push_controller::UserOrderInfo* user_order_info);
  bool ParseHotelOrder(const Json::Value& value,
                       push_controller::UserOrderInfo* user_order_info);
  bool UpdateToDb(const push_controller::UserOrderInfo& user_order_info);
  string CreateOrderId(const std::string& user_id,
                       const push_controller::BusinessType business_type,
                       const int32_t business_time);
  void UploadDataToKafka(const push_controller::UserOrderInfo& user_order_info);

 private:
  bool shut_down_;
  unique_ptr<MysqlServer> mysql_server_;
  DISALLOW_COPY_AND_ASSIGN(MsgProcessor);
};

class JsonMsgProcessor : public MsgProcessor {
 public:
  JsonMsgProcessor();
  virtual ~JsonMsgProcessor();
  virtual bool Process(const std::string& message);

 private:
  DISALLOW_COPY_AND_ASSIGN(JsonMsgProcessor);
};

class RawMsgProcessor : public MsgProcessor {
 public:
  RawMsgProcessor();
  virtual ~RawMsgProcessor();
  virtual bool Process(const std::string& message);

 private:
  unique_ptr<MessageParser> message_parser_;
  DISALLOW_COPY_AND_ASSIGN(RawMsgProcessor);
};

class MsgProcessorThread : public mobvoi::Thread {
 public:
  MsgProcessorThread(bool use_raw_msg_processor);
  virtual ~MsgProcessorThread();
  virtual void Run();
  void set_msg_queue(shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue);
  void ShutDown();

 private:
  std::unique_ptr<MsgProcessor> msg_processor_;
  shared_ptr<mobvoi::ConcurrentQueue<string>> msg_queue_;
  DISALLOW_COPY_AND_ASSIGN(MsgProcessorThread);
};

}  // namespace message_receiver

#endif  // PUSH_MESSAGE_RECEIVER_MESSAGE_PROCESSOR_H_
