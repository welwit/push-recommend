// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/ranker/ranking_handler.h"

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
  result["service"] = "news ranking service";
  response->AppendBuffer(result.toStyledString());
  return true;
}

}  // namespace serving
