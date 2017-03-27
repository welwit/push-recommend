// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_TRAIN_PUSH_PROCESSOR_H_
#define PUSH_PUSH_CONTROLLER_TRAIN_PUSH_PROCESSOR_H_

#include "push/push_controller/push_processor.h"

namespace push_controller {

class TrainPushProcessor : public PushProcessor {
 public:
  TrainPushProcessor();
  virtual ~TrainPushProcessor();
  virtual bool Process(PushEventInfo* push_event);
  bool ParseDetail(UserOrderInfo* user_order);
  bool GetTimeTable(const UserOrderInfo& user_order,
                    vector<train::TimeTable>* time_tables);
  bool BuildPushMessage(const UserOrderInfo& user_order,
                        const PushEventInfo& push_event,
                        const vector<train::TimeTable>& time_tables,
                        Json::Value* message);

 private:
  void AppendCommonField(const UserOrderInfo& user_order,
                         const PushEventInfo& push_event,
                         const vector<train::TimeTable>& time_tables,
                         Json::Value* message);
  void AppendEventField(const UserOrderInfo& user_order,
                        const PushEventInfo& push_event,
                        const vector<train::TimeTable>& time_tables,
                        Json::Value* message);
  void HandleLastDay(const UserOrderInfo& user_order,
                     const PushEventInfo& push_event,
                     const vector<train::TimeTable>& time_tables,
                     Json::Value* message);
  void HandleToday(const UserOrderInfo& user_order,
                   const PushEventInfo& push_event,
                   const vector<train::TimeTable>& time_tables,
                   Json::Value* message);
  void Handle3Hour(const UserOrderInfo& user_order,
                   const PushEventInfo& push_event,
                   const vector<train::TimeTable>& time_tables,
                   Json::Value* message);
  void Handle1Hour(const UserOrderInfo& user_order,
                   const PushEventInfo& push_event,
                   const vector<train::TimeTable>& time_tables,
                   Json::Value* message);
  DISALLOW_COPY_AND_ASSIGN(TrainPushProcessor);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_TRAIN_PUSH_PROCESSOR_H_
