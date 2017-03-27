// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/crawler/sina_fetcher.h"

#include "base/log.h"
#include "base/string_util.h"
#include "recommendation/news/crawler/util.h"
#include "third_party/gflags/gflags.h"
#include "util/parser/rss_parser/rss_parser.h"

//#include "recommendation/news/crawler/xml_parser.h"

namespace {

using namespace recommendation;

static const UrlTopicType kRssArray[] = {
  {"http://rss.sina.com.cn/news/china/focus15.xml", kChina},
  {"http://rss.sina.com.cn/news/world/focus15.xml", kWorld},
  {"http://rss.sina.com.cn/news/society/focus15.xml", kChina},
  {"http://rss.sina.com.cn/news/china/hktaiwan15.xml", kHongkong},
  {"http://rss.sina.com.cn/news/allnews/sports.xml", kSports},
  {"http://rss.sina.com.cn/news/allnews/tech.xml", kTech},
  {"http://rss.sina.com.cn/news/allnews/finance.xml", kFinance},
  {"http://rss.sina.com.cn/jczs/focus.xml", kMilitary},
  {"http://rss.sina.com.cn/news/allnews/eladies.xml", kEnt},
  {"http://rss.sina.com.cn/fashion/all/news.xml", kEnt},
  {"http://rss.sina.com.cn/news/allnews/ent.xml", kEnt},
  {"http://rss.sina.com.cn/edu/focus19.xml", kEdu},
};

static const UrlTopicType kUrlTopicArray[] = {
  {"http://mil.news.sina.com.cn/", kMilitary},
  {"http://finance.sina.com.cn/", kFinance},
  {"http://auto.sina.com.cn/", kAuto},
  {".leju.com/", kHouse},
  {"http://tech.sina.com.cn/", kTech},
  {"http://ent.sina.com.cn/", kEnt},
  {"http://sports.sina.com.cn/", kSports},
  {"http://edu.sina.com.cn/", kEdu},
  {"http://health.sina.com.cn/", kHealth},
  {"http://travel.sina.com.cn/", kTravel},
  {"http://news.sina.com.cn/", kNews},
};

static const char kRssUrlPrefix[] = 
  "http://go.rss.sina.com.cn/redirect.php?url=";

static const char* kSinaContentTpl[] = {
  "config/recommendation/news/crawler/sina/sina_content1.tpl",
  "config/recommendation/news/crawler/sina/sina_content2.tpl",
};

static const char kLinkPattern[] = "//link";

}

namespace recommendation {

RssSinaFetcher::RssSinaFetcher() {}

RssSinaFetcher::~RssSinaFetcher() {}
  
bool RssSinaFetcher::InitSeedVec() {
  for (auto& rss : kRssArray) {
    seed_vec_.push_back(rss.url);
  }
  return true;
}

bool RssSinaFetcher::InitContentTemplateVec() {
  for (auto& tpl : kSinaContentTpl) {
    content_template_vec_.push_back(tpl);
  }
  return true;
}

bool RssSinaFetcher::InitUrlTopicMap() {
  for (auto& url_topic : kUrlTopicArray) {
    url_topic_map_.insert(std::make_pair(url_topic.url, url_topic.topic));
  };
  return true;
}

bool RssSinaFetcher::FetchLinkVec(vector<string>* link_vec) {
  for (auto& seed : seed_vec_) {
    string html;
    if (!FetchHtmlSimple(seed, &html)) {
      continue;
    }
    VLOG(1) << "Html:" << html;
    crawl::RssFile rss;  
    util::RssParser rss_parser;
    rss_parser.Parse(string(html), &rss);
    for (int i = 0; i < rss.items_size(); ++i) {
      const string& url = rss.items(i).link();
      string res_url;
      if (FilterUrl(url, &res_url)) continue;
      LOG(INFO) << "Fetch url ok, url:" << res_url;
      if (!IsVisited(res_url)) {
        link_vec->push_back(res_url);
      } else {
        LOG(INFO) << "FetchLinkVec, URL is visited, url:" << url;
      }
    }
  }
  return true;
}

bool RssSinaFetcher::SetCategory(const Json::Value& data, NewsInfo* news_info) {
  if (!data.isMember("url")) {
    return false;
  }
  const string& url = data["url"].asString();
  for (auto it = url_topic_map_.begin(); it != url_topic_map_.end(); ++it) {
    if (url.find(it->first) != string::npos) {
      news_info->set_category(IntToString(it->second));
      return true;
    }
  }
  LOG(INFO) << "Unknown url:" << data["url"].asString();
  // RssSinaFetcher 即使没有分类也可以视为成功，主要使用做训练数据
  news_info->set_category("");
  return true;
}

bool RssSinaFetcher::SetKeywords(const Json::Value& data, NewsInfo* news_info) {
  if (!data.isMember("keywords")) {
    return false;
  }
  news_info->set_keywords(data["keywords"].asString());
  return true;
}

bool RssSinaFetcher::SetTags(const Json::Value& data, NewsInfo* news_info) {
  if (!data.isMember("tags")) {
    return false;
  }
  news_info->set_tags(data["tags"].asString());
  return true;
}

bool RssSinaFetcher::SetImage(const Json::Value& data, NewsInfo* news_info) {
  if (!data.isMember("image")) {
    return false;
  }
  news_info->set_image(data["image"].asString());
  return true;
}

bool RssSinaFetcher::SetMetaPublishSource(const Json::Value& data,
                                          NewsInfo* news_info) {
  if (!data.isMember("meta_publish_source")) {
    return false;
  }
  news_info->set_meta_publish_source(data["meta_publish_source"].asString());
  return true;
}

bool RssSinaFetcher::SetMetaPublishTime(const Json::Value& data,
                                        NewsInfo* news_info) {
  if (!data.isMember("meta_publish_time")) {
    return false;
  }
  time_t time;
  if (ExtractTime3(data["meta_publish_time"].asString(), &time)) {
    news_info->set_meta_publish_time(static_cast<int32>(time));
    return true;
  }
  return false;
}

bool RssSinaFetcher::SetDataUtilization(const Json::Value& data, 
                                        NewsInfo* news_info) {
  news_info->set_data_utilization(kTraining);
  return true;
}

bool RssSinaFetcher::FilterUrl(const string& url, string* out) {
  size_t pos = 0;
  if ((pos = url.find(kRssUrlPrefix)) != string::npos) {
    *out = url.substr(pos + strlen(kRssUrlPrefix)); 
    if (out->find("http://") != string::npos) {
      return false;
    } 
  }
  VLOG(1) << "Filter url:" << url;
  return true;
}

}  // namespace recommendation
