// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_MESSAGE_PRODUCER_MESSAGE_PRODUCER_HANDLER_H_
#define PUSH_MESSAGE_PRODUCER_MESSAGE_PRODUCER_HANDLER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "onebox/http_handler.h"
#include "util/kafka/kafka_util.h"
#include "util/net/http_server/http_request.h"
#include "util/net/http_server/http_response.h"

namespace message_producer {

class StatusHandler : public serving::HttpRequestHandler {
 public:
  StatusHandler();
  virtual ~StatusHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  DISALLOW_COPY_AND_ASSIGN(StatusHandler);
};

class MessageHandler : public serving::HttpRequestHandler {
 public:
  MessageHandler();
  virtual ~MessageHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  string GetDefaultResponse();
  string GetErrorResponse(const std::string& err_msg);

  std::unique_ptr<recommendation::KafkaProducer> kafka_producer_;
  DISALLOW_COPY_AND_ASSIGN(MessageHandler);
};

}  // namespace message_producer

#endif  // PUSH_MESSAGE_PRODUCER_MESSAGE_PRODUCER_HANDLER_H_
