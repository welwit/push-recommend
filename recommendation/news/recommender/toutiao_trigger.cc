// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/recommender/toutiao_trigger.h"

#include "base/log.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"
#include "util/net/http_client/http_client.h"
#include "util/random/random.h"

#include "push/util/common_util.h"

namespace {

static const int kHotNewsCount = 2;
static const int kLocalNewsCount = 2;
static const int kCategoryNewsCount = 1;
static const int kCategoryNewsTotalCount = 5;

static const char kToutiaoHotNews[] =
  "http://news.mobvoi.com/query?type=hot&count=%d&output=watch";
static const char kToutiaoCategoryNews[] = "http://news.mobvoi.com/query"
  "?type=get_data&count=%d&output=watch&category=%s";
static const char kToutiaoLocalNews[] = "http://news.mobvoi.com/query"
  "?type=get_data&count=%d&output=watch&category=本地&city=%s";
}

namespace recommendation {

ToutiaoTrigger::ToutiaoTrigger() {
  category_helper_ = Singleton<CategoryHelper>::get();
}

ToutiaoTrigger::~ToutiaoTrigger() {}

void ToutiaoTrigger::Fetch(const string& city,
                           vector<StoryDetail>* news_details) {
  news_details->clear();
  Json::Value result;
  int32 seed = static_cast<int32>(pthread_self()) % time(NULL);
  util::ACMRandom random(seed);
  const map<int, ToutiaoCategoryInfo>* category_map =
    category_helper_->GetCategoryMap();
  set<int32_t> category_news_idx_set;
  int32_t category_news_count = 0;
  while (category_news_count < kCategoryNewsTotalCount) {
    int32_t idx = random.Uniform(category_map->size());
    auto it = category_map->find(idx);
    const auto& category_name = it->second.name();
    if (category_name == "本地") continue;
    if (category_news_idx_set.find(it->first) != category_news_idx_set.end()) {
      continue;
    }
    result.clear();
    if (FetchCategoryNews(category_name, &result)) {
      ParseNewsDetails(result, news_details, "CATE_" + category_name);
      ++category_news_count;
    }
  }
  result.clear();
  if (FetchHotNews(&result)) {
    ParseNewsDetails(result, news_details, "HOT");
  }
  if (!city.empty()) {
    result.clear();
    if (FetchLocalNews(city, &result)) {
      ParseNewsDetails(result, news_details, "LOCAL_" + city);
    }
  }
  LOG(INFO) << "Candidates news size:" << news_details->size();
  for (auto it = news_details->begin(); it != news_details->end(); ++it) {
    VLOG(2) << "news detail:" << push_controller::ProtoToString(*it);
  }
}

bool ToutiaoTrigger::FetchHotNews(Json::Value* result) {
  string url = StringPrintf(kToutiaoHotNews, kHotNewsCount);
  util::HttpClient http_client;
  if (!http_client.FetchUrl(url) || http_client.response_code() != 200) {
    LOG(ERROR) << "Fetch hot news failed from url: " << url
               << ", response_code:" << http_client.response_code();
    return false;
  }
  Json::Reader reader;
  reader.parse(http_client.ResponseBody(), *result);
  if ((*result)["status"] != "success") {
    LOG(ERROR) << "Failed to get hot news from url: " << url
               << ", response:" << http_client.ResponseBody();
    return false;
  }
  return true;
}

bool ToutiaoTrigger::FetchLocalNews(const string& city,
                                    Json::Value* result) {
  string url = StringPrintf(kToutiaoLocalNews,
                            kLocalNewsCount, city.c_str());
  util::HttpClient http_client;
  if (!http_client.FetchUrl(url) || http_client.response_code() != 200) {
    LOG(ERROR) << "Fetch local news failed from url: " << url
               << ", response_code:" << http_client.response_code();
    return false;
  }
  Json::Reader reader;
  reader.parse(http_client.ResponseBody(), *result);
  if ((*result)["status"] != "success") {
    LOG(ERROR) << "Failed to get local news from url: " << url
               << ", response:" << http_client.ResponseBody();
    return false;
  }
  return true;
}

bool ToutiaoTrigger::FetchCategoryNews(const string& category,
                                       Json::Value* result) {
  string url = StringPrintf(kToutiaoCategoryNews, kCategoryNewsCount,
                            category.c_str());
  util::HttpClient http_client;
  if (!http_client.FetchUrl(url) || http_client.response_code() != 200) {
    LOG(ERROR) << "Fetch category news failed from url: " << url
               << ", response_code:" << http_client.response_code();
    return false;
  }
  Json::Reader reader;
  reader.parse(http_client.ResponseBody(), *result);
  if ((*result)["status"] != "success") {
    LOG(ERROR) << "Failed to get category from url: " << url
               << ", response:" << http_client.ResponseBody();
    return false;
  }
  return true;
}

void ToutiaoTrigger::ParseNewsDetails(const Json::Value &result,
    vector<StoryDetail> *news_details, const string& type) {
  const Json::Value& details =
    result["content"]["data"][0]["params"]["details"];
  for (Json::Value::ArrayIndex i = 0; i < details.size(); ++i) {
    const Json::Value& detail = details[i];
    StoryDetail story;
    story.set_app_url(detail["appUrl"].asString());
    story.set_background_url(detail["backgroundUrl"].asString());
    story.set_browser_url(detail["browserUrl"].asString());
    story.set_cid(detail["cid"].asInt64());
    story.set_publish_time(detail["publishTime"].asString());
    story.set_source(detail["source"].asString());
    story.set_summary(detail["summary"].asString());
    story.set_tip(detail["tip"].asString());
    story.set_title(detail["title"].asString());
    story.set_type(type);
    if (FilterStory(story)) {
      continue;
    }
    news_details->push_back(story);
  }
}

bool ToutiaoTrigger::FilterStory(const StoryDetail& story) {
  if (story.app_url().empty() ||
      story.background_url().empty() ||
      story.browser_url().empty() ||
      story.publish_time().empty() ||
      story.source().empty() ||
      story.summary().empty() ||
      story.title().empty()) {
    return true;
  }
  return false;
}

}  // namespace recommendation
