// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_CRAWLER_TENCENT_FETCHER_H_
#define RECOMMENDATION_NEWS_CRAWLER_TENCENT_FETCHER_H_

#include "recommendation/news/crawler/base_fetcher.h"

namespace recommendation {

class TencentFetcher : public BaseFetcher {
 public:
  TencentFetcher();
  virtual ~TencentFetcher();
  virtual bool InitSeedVec();
  virtual bool InitContentTemplateVec();
  virtual bool InitUrlTopicMap();
  virtual bool InitChannelTopicMap();
  virtual bool SetCategory(const Json::Value& data, 
                           NewsInfo* news_info);
  virtual bool SetPublishSource(const Json::Value& data,
                                NewsInfo* news_info);
  virtual bool SetPublishTime(const Json::Value& data, 
                              NewsInfo* news_info);
  virtual bool SetDataUtilization(const Json::Value& data, 
                                  NewsInfo* news_info);
  virtual bool CustomFilter(const Json::Value& data, NewsInfo* news_info); 

 private:
  DISALLOW_COPY_AND_ASSIGN(TencentFetcher);
};

}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_CRAWLER_TENCENT_FETCHER_H_
