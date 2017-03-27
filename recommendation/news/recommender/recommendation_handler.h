// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_RECOMMENDER_RECOMMENDATION_HANDLER_H_
#define RECOMMENDATION_NEWS_RECOMMENDER_RECOMMENDATION_HANDLER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "onebox/http_handler.h"
#include "push/util/location_helper.h"
#include "push/util/user_info_helper.h"
#include "third_party/jsoncpp/json.h"
#include "util/net/http_server/http_request.h"
#include "util/net/http_server/http_response.h"

#include "recommendation/news/recommender/news_trigger.h"
#include "recommendation/news/recommender/toutiao_trigger.h"

namespace recommendation {

class StatusHandler : public serving::HttpRequestHandler {
 public:
  StatusHandler();
  virtual ~StatusHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  DISALLOW_COPY_AND_ASSIGN(StatusHandler);
};

class RecommendationHandler : public serving::HttpRequestHandler {
 public:
  RecommendationHandler();
  virtual ~RecommendationHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  bool ParseQuery(const std::string& url, map<string, string>* params);
  string GetDefaultResponse(const string& user_id,
                            const vector<StoryDetail>& doc_vec);
  string GetErrorResponse(const std::string& err_msg);

  ToutiaoTrigger* toutiao_trigger_;
  NewsTrigger* news_trigger_;
  DeviceInfoHelper* device_info_helper_;
  LocationHelper* location_helper_;
  DISALLOW_COPY_AND_ASSIGN(RecommendationHandler);
};

}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_RECOMMENDER_RECOMMENDATION_HANDLER_H_
