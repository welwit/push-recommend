// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_CRAWLER_SINA_FETCHER_H_
#define RECOMMENDATION_NEWS_CRAWLER_SINA_FETCHER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "third_party/jsoncpp/json.h"

#include "recommendation/news/crawler/base_fetcher.h"

namespace recommendation {

class RssSinaFetcher : public BaseFetcher {
 public:
  RssSinaFetcher();
  virtual ~RssSinaFetcher();
  virtual bool InitSeedVec();
  virtual bool InitContentTemplateVec();
  virtual bool InitUrlTopicMap();
  virtual bool FetchLinkVec(vector<string>* link_vec);
  virtual bool SetCategory(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetKeywords(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetTags(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetImage(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetMetaPublishSource(const Json::Value& data, 
                                    NewsInfo* news_info);
  virtual bool SetMetaPublishTime(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetDataUtilization(const Json::Value& data, NewsInfo* news_info);

 private:
  bool FilterUrl(const string& url, string* out);
  DISALLOW_COPY_AND_ASSIGN(RssSinaFetcher);
};

}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_CRAWLER_SINA_FETCHER_H_
