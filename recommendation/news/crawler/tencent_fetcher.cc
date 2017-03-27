// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/crawler/tencent_fetcher.h"

#include "base/file/proto_util.h"
#include "base/hash.h"
#include "base/log.h"
#include "base/time.h"
#include "push/util/time_util.h"
#include "third_party/gflags/gflags.h"
#include "third_party/re2//re2/re2.h"
#include "util/protobuf/proto_json_format.h"

#include "recommendation/news/crawler/util.h"

namespace {

static const char* kSeedArray[] = {
  "http://www.qq.com/",
  "http://news.qq.com/",
  "http://news.qq.com/china_index.shtml",
  "http://news.qq.com/world_index.shtml",
  "http://society.qq.com/",
  "http://mil.qq.com/mil_index.htm",
  "http://news.qq.com/militery.shtml",
  "http://finance.qq.com/",
  "http://stock.qq.com/",
  "http://money.qq.com/",
  "http://sports.qq.com/",
  "http://ent.qq.com/",
  "http://ent.qq.com/star/",
  "http://fashion.qq.com/",
  "http://fashion.qq.com/beauty/beauty_list.htm",
  "http://health.qq.com/",
  "http://baby.qq.com/",
  "http://house.qq.com/",
  "http://auto.qq.com/",
  "http://tech.qq.com/",
  "http://tech.qq.com/en.htm",
  "http://digi.tech.qq.com/"
  "http://dajia.qq.com/",
  "http://edu.qq.com/",
  "http://edu.qq.com/abroad/",
  "http://history.news.qq.com/",
  "http://ly.qq.com/",
};

using namespace recommendation;

static const UrlTopicType kToFetchedUrlTopicArray[] = {
  {"http://news.qq.com/", kNews},
  {"http://society.qq.com/", kSociety},
  {"http://mil.qq.com/", kMilitary},
  {"http://finance.qq.com/", kFinance},
  {"http://stock.qq.com/", kFinance},
  {"http://money.qq.com/", kFinance},
  {"http://sports.qq.com/", kSports},
  {"http://ent.qq.com/", kEnt},
  {"http://fashion.qq.com/", kFashion},
  {"http://health.qq.com/", kHealth},
  {"http://baby.qq.com/", kBaby},
  {"http://house.qq.com/", kHouse},
  {"http://auto.qq.com/", kAuto},
  {"http://tech.qq.com/", kTech},
  {"http://edu.qq.com/", kEdu},
  {"http://view.news.qq.com/", kComments},
  {"http://cul.qq.com/", kCulture},
  {"http://dajia.qq.com/", kDajia},
  {"http://ly.qq.com/", kTravel},
  {"http://digi.tech.qq.com/", kDigital},
};

static const UrlTopicType kTencentChannelTopicArray[] = {
  {"时政新闻", kChina},
  {"各地新闻", kChina},
  {"国际时事", kWorld},
  {"社会万象", kSociety},
  {"军事要闻", kMilitary},
  {"国际财经", kFinance},
  {"市场策略", kFinance},
  {"综艺新闻", kEnt},
  {"明星资讯", kEnt},
  {"电视新闻", kEnt},
  {"移动互联", kTech},
  {"互联网", kTech},
  {"IT业界", kTech},
  {"通信", kTech},
  {"电子商务", kTech},
  {"教育新闻", kEdu},
};

static const char* kTencentContentTpl[] = {
  "config/recommendation/news/crawler/tencent/tencent_content1.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content2.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content3.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content4.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content5.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content6.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content7.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content8.tpl",
  "config/recommendation/news/crawler/tencent/tencent_content9.tpl",
};

}

namespace recommendation {

TencentFetcher::TencentFetcher() {}

TencentFetcher::~TencentFetcher() {}
  
bool TencentFetcher::InitSeedVec() {
  for (auto& seed : kSeedArray) {
    seed_vec_.push_back(seed);
  }
  return true;
}

bool TencentFetcher::InitContentTemplateVec() {
  for (auto& tpl : kTencentContentTpl) {
    content_template_vec_.push_back(tpl);
  }
  return true;
}

bool TencentFetcher::InitUrlTopicMap() {
  for (auto& url_topic : kToFetchedUrlTopicArray) {
    url_topic_map_.insert(std::make_pair(url_topic.url, url_topic.topic));
  }
  return true;
}
  
bool TencentFetcher::InitChannelTopicMap() {
  for (auto& channel_topic : kTencentChannelTopicArray) {
    channel_topic_map_.insert(std::make_pair(channel_topic.url, 
                                             channel_topic.topic));
  }
  return true;
}

bool TencentFetcher::SetCategory(const Json::Value& data,
                                 NewsInfo* news_info) {
  if (data.isMember("category") && !data["category"].empty()) {
    const string& category = data["category"].asString();
    auto it = channel_topic_map_.find(category);
    if (it != channel_topic_map_.end()) {
      news_info->set_category(IntToString(it->second));
      return true;
    } 
  }
  return BaseFetcher::SetCategory(data, news_info);
}

bool TencentFetcher::SetPublishSource(const Json::Value& data,
                                      NewsInfo* news_info) {
  if (!data.isMember("publish_source")) {
    return false;
  }
  news_info->set_publish_source(data["publish_source"].asString());
  return true;
}

bool TencentFetcher::SetPublishTime(const Json::Value& data,
                                    NewsInfo* news_info) {
  if (!data.isMember("publish_time")) {
    return false;
  }
  time_t time;
  if (ExtractTime4(data["publish_time"].asString(), &time)) {
    news_info->set_publish_time(static_cast<int32>(time));
    return true;
  }
  return false;
}

bool TencentFetcher::SetDataUtilization(const Json::Value& data,
                                        NewsInfo* news_info) {
  news_info->set_data_utilization(kRecommend);
  return true;
}

bool TencentFetcher::CustomFilter(const Json::Value& data, NewsInfo* news_info) {
  if (!news_info->has_content()) {
    return true;
  }
  const string& content = news_info->content();
  if (content.find("正在加载...") != string::npos ||
      content.find("自动播放") != string::npos) {
    return true;
  }
  return false;
}

}  // namespace recommendation
