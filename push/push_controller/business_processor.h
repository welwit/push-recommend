// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_BASE_BUSINESS_PROCESSOR_H_
#define PUSH_PUSH_CONTROLLER_BASE_BUSINESS_PROCESSOR_H_

#include <memory>

#include "base/basictypes.h"
#include "base/compat.h"
#include "proto/mysql_config.pb.h"

#include "push/proto/push_meta.pb.h"

namespace push_controller {

class BaseBusinessProcessor {
 public:
  BaseBusinessProcessor();
  virtual ~BaseBusinessProcessor();
  virtual bool CreatePushEvent(UserOrderInfo* user_order_info,
                               vector<PushEventInfo>* push_event_vector) = 0;
  bool UpdateEventsToDb(const vector<PushEventInfo>& push_events);
  void CreateId(PushEventInfo* push_event_info);

 private:
  bool CheckAttribute(const PushEventInfo& push_event_left,
                      const PushEventInfo& push_event_right);

  std::unique_ptr<MysqlServer> mysql_server_;
  DISALLOW_COPY_AND_ASSIGN(BaseBusinessProcessor);
};

class TrainBusinessProcessor : public BaseBusinessProcessor {
 public:
  TrainBusinessProcessor();
  virtual ~TrainBusinessProcessor();
  virtual bool CreatePushEvent(UserOrderInfo* user_order_info,
                               vector<PushEventInfo>* push_event_vector);

 private:
  bool ParseDetail(const string& detail, UserOrderInfo* user_order_info);

  DISALLOW_COPY_AND_ASSIGN(TrainBusinessProcessor);
};

class FlightBusinessProcessor : public BaseBusinessProcessor {
 public:
  FlightBusinessProcessor();
  virtual ~FlightBusinessProcessor();
  virtual bool CreatePushEvent(UserOrderInfo* user_order_info,
                               vector<PushEventInfo>* push_event_vector);

 private:
  bool ParseDetail(const string& detail, UserOrderInfo* user_order_info);
  DISALLOW_COPY_AND_ASSIGN(FlightBusinessProcessor);
};

class MovieBusinessProcessor : public BaseBusinessProcessor {
 public:
  MovieBusinessProcessor();
  virtual ~MovieBusinessProcessor();
  virtual bool CreatePushEvent(UserOrderInfo* user_order_info,
                               vector<PushEventInfo>* push_event_vector);

 private:
  bool ParseDetail(const string& detail, UserOrderInfo* user_order_info);
  DISALLOW_COPY_AND_ASSIGN(MovieBusinessProcessor);
};

class HotelBusinessProcessor : public BaseBusinessProcessor {
 public:
  HotelBusinessProcessor();
  virtual ~HotelBusinessProcessor();
  virtual bool CreatePushEvent(UserOrderInfo* user_order_info,
                               vector<PushEventInfo>* push_event_vector);
 private:
  bool ParseDetail(const string& detail, UserOrderInfo* user_order_info);
  DISALLOW_COPY_AND_ASSIGN(HotelBusinessProcessor);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_BASE_BUSINESS_PROCESSOR_H_
