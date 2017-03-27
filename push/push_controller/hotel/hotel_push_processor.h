// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_HOTEL_PUSH_PROCESSOR_H_
#define PUSH_PUSH_CONTROLLER_HOTEL_PUSH_PROCESSOR_H_

#include "push/push_controller/push_processor.h"

namespace push_controller {

class HotelPushProcessor : public PushProcessor {
 public:
  HotelPushProcessor();
  virtual ~HotelPushProcessor();
  virtual bool Process(PushEventInfo* push_event);
  bool ParseDetail(UserOrderInfo* user_order);
  bool BuildPushMessage(const UserOrderInfo& user_order,
		                    const PushEventInfo& push_event,
						            Json::Value* message);
 private:
  bool AppendCommonField(const UserOrderInfo& user_order,
		                     const PushEventInfo& push_event,
						             Json::Value* message);
  bool GetLocation(const string& address, string* geo);
  DISALLOW_COPY_AND_ASSIGN(HotelPushProcessor);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_HOTEL_PUSH_PROCESSOR_H_
