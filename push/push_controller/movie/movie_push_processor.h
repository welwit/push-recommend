// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_MOVIE_PUSH_PROCESSOR_H_
#define PUSH_PUSH_CONTROLLER_MOVIE_PUSH_PROCESSOR_H_

#include "push/push_controller/push_processor.h"

namespace push_controller {

class MoviePushProcessor : public PushProcessor {
 public:
  MoviePushProcessor();
  virtual ~MoviePushProcessor();
  virtual bool Process(PushEventInfo* push_event);
  bool ParseDetail(UserOrderInfo* user_order);
  bool BuildPushMessage(const UserOrderInfo& user_order,
                        const PushEventInfo& push_event,
                        Json::Value* message);

 private:
  void AppendCommonField(const UserOrderInfo& user_order,
                         const PushEventInfo& push_event,
                         Json::Value* message);
  void AppendEventField(const UserOrderInfo& user_order,
                        const PushEventInfo& push_event,
                        Json::Value* message);
  void HandleToday(const UserOrderInfo& user_order,
                   const PushEventInfo& push_event,
                   Json::Value* message);
  void Handle30Min(const UserOrderInfo& user_order,
                   const PushEventInfo& push_event,
                   Json::Value* message);
  DISALLOW_COPY_AND_ASSIGN(MoviePushProcessor);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_MOVIE_PUSH_PROCESSOR_H_
