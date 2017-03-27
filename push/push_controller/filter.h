// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_FILTER_H_
#define PUSH_PUSH_CONTROLLER_FILTER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "push/push_controller/push_sender.h"
#include "push/proto/push_meta.pb.h"
#include "push/proto/user_feedback_meta.pb.h"

namespace push_controller {

class Filter {
 public:
  Filter() {}
  virtual ~Filter() {}
  virtual void Filtering(vector<PushEventInfo>* push_events) = 0;
 private:
  DISALLOW_COPY_AND_ASSIGN(Filter);
};

class UserFeedbackFilter : public Filter {
 public:
  UserFeedbackFilter();
  virtual ~UserFeedbackFilter();
  void Init();
  virtual void Filtering(vector<PushEventInfo>* push_events);

 private:
  bool FetchFeedback(const PushEventInfo& push_event,
                     Json::Value* feedback_result);
  bool IsUserClosedPush(const PushEventInfo& push_event,
                        const Json::Value& feedback_result);

  map<BusinessType, string> business_type_string_map_;
  DISALLOW_COPY_AND_ASSIGN(UserFeedbackFilter);
};

class PushStatusFilter : public Filter {
 public:
  PushStatusFilter();
  virtual ~PushStatusFilter();
  virtual void Filtering(vector<PushEventInfo>* push_events);
 private:
  std::unique_ptr<PushSender> push_sender_;
  DISALLOW_COPY_AND_ASSIGN(PushStatusFilter);
};

class PushTimeFilter : public Filter {
 public:
  PushTimeFilter();
  virtual ~PushTimeFilter();
  virtual void Filtering(vector<PushEventInfo>* push_events);
 private:
  bool IsValidPushTime(time_t push_time);
  DISALLOW_COPY_AND_ASSIGN(PushTimeFilter);
};

class WearModelFilter : public Filter {
 public:
  WearModelFilter();
  virtual ~WearModelFilter();
  virtual void Filtering(vector<PushEventInfo>* push_events);
 private:
  void FilterInternal(vector<PushEventInfo>* push_events);
  DISALLOW_COPY_AND_ASSIGN(WearModelFilter);
};

class FilterManager : public Filter {
 public:
  FilterManager();
  virtual ~FilterManager();
  virtual void Filtering(vector<PushEventInfo>* push_events);
 private:
  std::vector<Filter*> filters_;
  DISALLOW_COPY_AND_ASSIGN(FilterManager);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_FILTER_H_
