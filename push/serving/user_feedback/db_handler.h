// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_SERVING_USER_FEEDBACK_DB_HANDLER_H_
#define PUSH_SERVING_USER_FEEDBACK_DB_HANDLER_H_

#include <memory>

#include "base/basictypes.h"
#include "proto/mysql_config.pb.h"

#include "push/proto/user_feedback_meta.pb.h"

namespace feedback {

class DbHandler {
 public:
  DbHandler();
  virtual ~DbHandler();
  virtual void Init();
  virtual bool InsertUserFeedback(UserFeedbackRequest *request,
                                  UserFeedbackResponse *response);
  virtual bool QueryUserFeedback(UserFeedbackQueryResquest *request,
                                 UserFeedbackQueryResponse *response);

 private:
  std::unique_ptr<MysqlServer> mysql_server_;
  
  DISALLOW_COPY_AND_ASSIGN(DbHandler);
};

}  // namespace feedback

#endif  // PUSH_SERVING_USER_FEEDBACK_DB_HANDLER_H_
