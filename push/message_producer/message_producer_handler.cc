// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/message_producer/message_producer_handler.h"

#include "base/log.h"
#include "third_party/jsoncpp/json.h"
#include "util/net/util.h"

namespace message_producer {

StatusHandler::StatusHandler() {}

StatusHandler::~StatusHandler() {}

bool StatusHandler::HandleRequest(util::HttpRequest* request,
                                  util::HttpResponse* response) {
  LOG(INFO) << "Receive StatusHandler request";
  response->SetJsonContentType();
  Json::Value result;
  result["status"] = "ok";
  result["host"] = util::GetLocalHostName();
  result["service"] = "message producer service";
  response->AppendBuffer(result.toStyledString());
  return true;
}

MessageHandler::MessageHandler() {
  kafka_producer_.reset(new recommendation::KafkaProducer());
  CHECK(kafka_producer_.get());
}

MessageHandler::~MessageHandler() {}

bool MessageHandler::HandleRequest(util::HttpRequest* request,
                                   util::HttpResponse* response) {
  string request_data = request->GetRequestData();
  LOG(INFO) << "Receive MessageHandler request, data=" << request_data;
  vector<string> messages;
  int cnt = 0;
  messages.push_back(request_data);
  kafka_producer_->Produce(messages, &cnt);
  response->SetJsonContentType();
  response->AppendBuffer(GetDefaultResponse());
  return true;
}

string MessageHandler::GetDefaultResponse() {
  Json::Value result;
  result["status"] = "success";
  return result.toStyledString();
}

string MessageHandler::GetErrorResponse(const std::string& err_msg) {
  Json::Value result;
  result["status"] = "error";
  result["err_msg"] = err_msg;
  return result.toStyledString();
}

}  // namespace message_producer
