// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/log.h"
#include "third_party/jsoncpp/json.h"
#include "util/net/http_client/http_client.h"

int main(int argc, char** argv) {
  //string url = "http://train-info-service/train/query_timetable";
  string url = "http://127.0.0.1:9039/train/query_timetable";
  Json::Value request;
  request["train_no"] = "K21";
  char utf8_string[] = u8"北京西";
  std::string depart_station = utf8_string;
  request["depart_station"] = depart_station;
  string post_data = request.toStyledString();
  LOG(INFO) << post_data;
  util::HttpClient http_client;
  http_client.SetHttpMethod(util::HttpMethod::kPost);
  http_client.SetPostData(post_data);
  http_client.AddHeader("Content-Type", "application/json; charset=utf-8");
  http_client.FetchUrl(url);
  LOG(INFO) << http_client.ResponseBody();
  return 0;
}
