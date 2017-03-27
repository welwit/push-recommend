// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_PUSH_SENDER_H_
#define PUSH_PUSH_CONTROLLER_PUSH_SENDER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "proto/mysql_config.pb.h"
#include "third_party/jsoncpp/json.h"
#include "push/proto/push_meta.pb.h"

namespace push_controller {

class PushSender {
 public:
  PushSender();
  virtual ~PushSender();
  bool SendPush(MessageType message_type,
                const string& message_desc,
                const string& user_id,
                const Json::Value& message_content);
  bool UpdatePushStatus(const PushEventInfo& push_event_info,
                        PushStatus push_status);

 private:
  std::unique_ptr<MysqlServer> mysql_server_;
  DISALLOW_COPY_AND_ASSIGN(PushSender);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_PUSH_SENDER_H_
