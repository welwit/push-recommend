// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/log.h"
#include "third_party/jsoncpp/json.h"
#include "util/net/http_client/http_client.h"
#include "push/util/time_util.h"

int main(int argc, char** argv) {
  string url = "http://flight-info-service/flight/query_flightinfo";
  Json::Value request;
  request["flight_no"] = "CA1831";
  string date;
  recommendation::MakeDate(0, &date);
  request["depart_date"] = date;
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
