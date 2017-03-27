// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/recommender/recommendation_handler.h"

#include "base/log.h"
#include "push/util/common_util.h"
#include "util/net/util.h"
#include "util/protobuf/proto_json_format.h"
#include "util/url/parser/url_parser.h"

namespace recommendation {

StatusHandler::StatusHandler() {}

StatusHandler::~StatusHandler() {}

bool StatusHandler::HandleRequest(util::HttpRequest* request,
                                  util::HttpResponse* response) {
  LOG(INFO) << "Receive StatusHandler request";
  response->SetJsonContentType();
  Json::Value result;
  result["status"] = "ok";
  result["host"] = util::GetLocalHostName();
  result["service"] = "news personalization service";
  response->AppendBuffer(result.toStyledString());
  return true;
}

RecommendationHandler::RecommendationHandler() {
  toutiao_trigger_ = Singleton<ToutiaoTrigger>::get();
  news_trigger_ = Singleton<NewsTrigger>::get();
  device_info_helper_ = Singleton<DeviceInfoHelper>::get();
  location_helper_ = Singleton<LocationHelper>::get();
}

RecommendationHandler::~RecommendationHandler() {}

bool RecommendationHandler::HandleRequest(util::HttpRequest* request,
                                          util::HttpResponse* response) {
  response->SetJsonContentType();
  string request_data = request->GetRequestData();
  string url = request->Url();
  LOG(INFO) << "Receive RecommendationHandler request, url:"
            << url << ", data=" << request_data;
  map<string, string> params;
  if (!ParseQuery(url, &params)) {
    response->AppendBuffer(GetErrorResponse("Get url params failed"));
    return false;
  }
  if (params.empty() || params.find("user_id") == params.end()) {
    response->AppendBuffer(GetErrorResponse("Params must contain user_id"));
    return false;
  }
  string user_id = params["user_id"];
  string latitude, longitude;
  if (!device_info_helper_->GetDeviceCoordinate(user_id, &latitude, &longitude)) {
    response->AppendBuffer(GetErrorResponse("Get lat and lng failed"));
    return false;
  }
  VLOG(2) << "user_id:" << user_id << ", lat:" << latitude
          << ", lng:" << longitude;
  LocationInfo location_info;
  if (!location_helper_->GetLocationInfo(latitude, longitude, &location_info)) {
    response->AppendBuffer(GetErrorResponse("Get location info failed"));
    return false;
  }
  vector<StoryDetail> news_details;
  int64 default_category = 0;
  if (!news_trigger_->Trigger(default_category, &news_details) ||
      news_details.empty()) {
    LOG(WARNING) << "Fetch news from portal failed, using toutiao instead";
    toutiao_trigger_->Fetch(location_info.city(), &news_details);
  }
  if (news_details.empty()) {
    response->AppendBuffer(GetErrorResponse("Candidates empty"));
    return false;
  }
  response->AppendBuffer(GetDefaultResponse(user_id, news_details));
  return true;
}

bool RecommendationHandler::ParseQuery(const std::string& url,
                                       map<string, string>* params) {
  string::size_type index = url.find('?');
  if (index == string::npos) {
    LOG(ERROR) << "Bad url:" << url;
    return false;
  }
  util::ParseUrlParams(url.substr(index + 1), params);
  return true;
}

string RecommendationHandler::GetDefaultResponse(
    const string& user_id, const vector<StoryDetail>& doc_vec) {
  Json::Value result;
  Json::Value data;
  result["status"] = "success";
  for (auto& doc : doc_vec) {
    Json::Value result;
    util::ProtoJsonFormat::WriteToValue(doc, &result);
    LOG(INFO) << "Recommed Candidate for user:" << user_id << ", doc:" 
            << push_controller::JsonToString(result);
    data.append(result);
  }
  result["data"] = data;
  return result.toStyledString();
}

string RecommendationHandler::GetErrorResponse(const std::string& err_msg) {
  Json::Value result;
  result["status"] = "error";
  result["err_msg"] = err_msg;
  return result.toStyledString();
}

}  // namespace recommendation
