// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_RECOMMENDER_TOUTIAO_TRIGGER_H_
#define RECOMMENDATION_NEWS_RECOMMENDER_TOUTIAO_TRIGGER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/singleton.h"
#include "third_party/jsoncpp/json.h"

#include "recommendation/news/recommender/category_helper.h"

namespace recommendation {

class ToutiaoTrigger {
 public:
  ~ToutiaoTrigger();
  void Fetch(const string& city, vector<StoryDetail>* news_details);

 private:
  friend struct DefaultSingletonTraits<ToutiaoTrigger>;

  ToutiaoTrigger();
  bool FetchHotNews(Json::Value* result);
  bool FetchLocalNews(const string& city, Json::Value* result);
  bool FetchCategoryNews(const string& category, Json::Value* result);
  void ParseNewsDetails(const Json::Value &result,
                        vector<StoryDetail> *news_details,
                        const string& type);
  bool FilterStory(const StoryDetail& story);

  CategoryHelper* category_helper_;
  DISALLOW_COPY_AND_ASSIGN(ToutiaoTrigger);
};

}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_RECOMMENDER_TOUTIAO_TRIGGER_H_
