// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/log.h"
#include "third_party/jsoncpp/json.h"
#include "util/net/http_client/http_client.h"

int main(int argc, char** argv) {
  string url = "http://user-feedback-service/feedback";
  Json::Value request, data_list, data;

  request["user_id"] = "69322dfbad6c90d9d008095ff3967227";
  data["business_type"] = "schedule";
  data["user_feedback_status"] = true;
  data_list.append(data);
  data["business_type"] = "weather";
  data["user_feedback_status"] = false;
  data_list.append(data);
  data["business_type"] = "movie";
  data["user_feedback_status"] = true;
  data_list.append(data);
  request["data"] = data_list;
  string post_data = request.toStyledString();
  LOG(INFO) << post_data;
  util::HttpClient http_client;
  http_client.SetHttpMethod(util::HttpMethod::kPost);
  http_client.SetPostData(post_data);
  http_client.AddHeader("Content-Type", "application/json; charset=utf-8");
  http_client.FetchUrl(url);
  LOG(INFO) << http_client.ResponseBody();
  
  url = "http://user-feedback-service/query_feedback";
  request.clear();
  request["user_id"] = "69322dfbad6c90d9d008095ff3967227";
  post_data = request.toStyledString();
  LOG(INFO) << post_data;
  http_client.Reset();
  http_client.SetHttpMethod(util::HttpMethod::kPost);
  http_client.SetPostData(post_data);
  http_client.AddHeader("Content-Type", "application/json; charset=utf-8");
  http_client.FetchUrl(url);
  LOG(INFO) << http_client.ResponseBody();
  return 0;
}
