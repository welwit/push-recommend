// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/message_receiver/message_receiver_handler.h"

#include "base/log.h"
#include "third_party/jsoncpp/json.h"
#include "util/net/util.h"

namespace serving {

StatusHandler::StatusHandler() {}

StatusHandler::~StatusHandler() {}

bool StatusHandler::HandleRequest(util::HttpRequest* request,
                                  util::HttpResponse* response) {
  LOG(INFO)<< "Receive StatusHandler request";
  response->SetJsonContentType();
  Json::Value result;
  result["status"] = "ok";
  result["host"] = util::GetLocalHostName();
  result["service"] = "message receiver service";
  response->AppendBuffer(result.toStyledString());
  return true;
}

}  // namespace serving
