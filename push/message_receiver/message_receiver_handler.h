// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_MESSAGE_RECEIVER_MESSAGE_RECEIVER_HANDLER_H_
#define PUSH_MESSAGE_RECEIVER_MESSAGE_RECEIVER_HANDLER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "onebox/http_handler.h"
#include "util/net/http_server/http_request.h"
#include "util/net/http_server/http_response.h"

namespace serving {

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

#endif  // PUSH_MESSAGE_RECEIVER_MESSAGE_RECEIVER_HANDLER_H_
