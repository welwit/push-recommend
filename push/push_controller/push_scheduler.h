// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_PUSH_SCHEDULER_H_
#define PUSH_PUSH_CONTROLLER_PUSH_SCHEDULER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/thread.h"
#include "proto/mysql_config.pb.h"

#include "push/push_controller/filter.h"
#include "push/proto/push_meta.pb.h"

namespace push_controller {

class PushScheduler : public mobvoi::Thread {
 public:
  PushScheduler();
  virtual ~PushScheduler();
  virtual void Run();

 private:
  void FetchPushEvents(vector<PushEventInfo>* push_events);
  void FetchNicknameTable(vector<PushEventInfo>* push_events);
  void FetchDeviceTable(vector<PushEventInfo>* push_events);
  void FilterPushEvents(vector<PushEventInfo>* push_events);
  void PushToQueue(const vector<PushEventInfo>& push_events);

  std::unique_ptr<MysqlServer> mysql_server_;
  std::unique_ptr<FilterManager> filter_manager_;
  DISALLOW_COPY_AND_ASSIGN(PushScheduler);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_PUSH_SCHEDULER_H_
