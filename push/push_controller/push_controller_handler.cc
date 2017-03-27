// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/push_controller_handler.h"

#include "base/log.h"
#include "base/singleton.h"
#include "third_party/jsoncpp/json.h"
#include "util/net/util.h"
#include "push/push_controller/news/news_push_processor.h"

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
  result["service"] = "push controller service";
  response->AppendBuffer(result.toStyledString());
  return true;
}

NewsPushHandler::NewsPushHandler() {}

NewsPushHandler::~NewsPushHandler() {}

bool NewsPushHandler::HandleRequest(util::HttpRequest* request,
                                    util::HttpResponse* response) {
  LOG(INFO)<< "Receive NewsPushHandler request";
  response->SetJsonContentType();
  Json::Value result;
  response->AppendBuffer(result.toStyledString());
  push_controller::NewsPushProcessor* processor =
    Singleton<push_controller::NewsPushProcessor>::get();
  if (processor->Process()) {
    result["status"] = "ok";
    result["msg"] = "Do news push finished";
    response->AppendBuffer(result.toStyledString());
    return true;

  } else {
    result["status"] = "error";
    result["msg"] = "Do news push failed";
    response->AppendBuffer(result.toStyledString());
    return false;
  }
}

}  // namespace serving
