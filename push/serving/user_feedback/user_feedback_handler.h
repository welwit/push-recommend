// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_SERVING_USER_FEEDBACK_USER_FEEDBACK_HANDLER_H_
#define PUSH_SERVING_USER_FEEDBACK_USER_FEEDBACK_HANDLER_H_

#include <string>

#include "onebox/http_handler.h"
#include "util/net/http_server/http_request.h"
#include "util/net/http_server/http_response.h"

#include "push/serving/user_feedback/db_handler.h"
#include "push/proto/user_feedback_meta.pb.h"

namespace serving {

using namespace feedback;

class UserFeedbackHandler : public HttpRequestHandler {
 public:
  UserFeedbackHandler();
  virtual void Init();
  virtual ~UserFeedbackHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  string ErrorInfo(const UserFeedbackRequest& request,
                   const std::string& error_info);
  string ResponseInfo(const UserFeedbackResponse& response);

  DbHandler db_handler_;
  map<BusinessType, string> business_type_name_map_;
  map<string, BusinessType> business_name_type_map_;
  map<Status, string> total_status_text_map_;
  map<string, Status> total_text_status_map_;
  
  DISALLOW_COPY_AND_ASSIGN(UserFeedbackHandler);
};

class UserFeedbackQueryHandler : public HttpRequestHandler {
 public:
  UserFeedbackQueryHandler();
  virtual void Init();
  virtual ~UserFeedbackQueryHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  string ErrorInfo(const UserFeedbackQueryResquest& request, 
                   const std::string& error_info);
  string ResponseInfo(const UserFeedbackQueryResponse& response);
  
  DbHandler db_handler_;
  map<BusinessType, string> business_type_name_map_;
  map<string, BusinessType> business_name_type_map_;
  map<Status, string> total_status_text_map_;
  map<string, Status> total_text_status_map_;
  
  DISALLOW_COPY_AND_ASSIGN(UserFeedbackQueryHandler);
};

class StatusHandler : public HttpRequestHandler {
 public:
  StatusHandler();
  virtual ~StatusHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  DISALLOW_COPY_AND_ASSIGN(StatusHandler);
};

}  // namespace serving

#endif  // PUSH_SERVING_USER_FEEDBACK_USER_FEEDBACK_HANDLER_H_
