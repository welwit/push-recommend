// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_PUSH_POOL_UPDATER_H_
#define PUSH_PUSH_CONTROLLER_PUSH_POOL_UPDATER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/thread.h"
#include "proto/mysql_config.pb.h"

#include "push/proto/push_meta.pb.h"

namespace push_controller {

class PushPoolUpdater : public mobvoi::Thread {
 public:
  PushPoolUpdater();
  virtual ~PushPoolUpdater();
  virtual void Run();

 private:
  DISALLOW_COPY_AND_ASSIGN(PushPoolUpdater);
};

class UserOrderProcessor {
 public:
  UserOrderProcessor();
  ~UserOrderProcessor();
  bool QueryUserOrder(vector<UserOrderInfo>* user_order_vector);

 private:
  time_t last_updated_;
  std::unique_ptr<MysqlServer> mysql_server_;
  DISALLOW_COPY_AND_ASSIGN(UserOrderProcessor);
};

class PushEventProcessor {
 public:
  PushEventProcessor();
  virtual ~PushEventProcessor();
  virtual void UpdatePushEvent(vector<UserOrderInfo>* user_order_vector);

 private:
  DISALLOW_COPY_AND_ASSIGN(PushEventProcessor);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_PUSH_POOL_UPDATER_H_
