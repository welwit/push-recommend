// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/crawler/xinhua_fetcher.h"

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
  "http://www.xinhuanet.com",
  "http://www.news.cn/politics/",
  "http://www.news.cn/mil/index.htm",
  "http://www.news.cn/money/index.htm",
  "http://www.news.cn/fortune/",
  "http://www.news.cn/legal/index.htm",
  "http://www.news.cn/auto/index.htm",
  "http://www.news.cn/house/index.htm",
  "http://www.news.cn/tech/index.htm",
  "http://ent.news.cn",
  "http://www.news.cn/sports/",
  "http://education.news.cn",
  "http://www.news.cn/health/",
  "http://travel.news.cn",
  "http://www.news.cn/food/index.htm",
  "http://www.news.cn/world/index.htm",
  "http://www.news.cn/gangao/index.htm",
  "http://www.news.cn/tw/index.htm",
};

using namespace recommendation;

static const UrlTopicType kToFetchedUrlTopicArray[] = {
  {"http://news.xinhuanet.com/politics/", kPolitics},
  {"http://news.xinhuanet.com/mil/", kMilitary},
  {"http://news.xinhuanet.com/fortune/", kFinance},
  {"http://news.xinhuanet.com/money/", kFinance},
  {"http://news.xinhuanet.com/legal/", kLegal},
  {"http://news.xinhuanet.com/auto/", kAuto},
  {"http://news.xinhuanet.com/house/", kHouse},
  {"http://news.xinhuanet.com/tech/", kTech},
  {"http://ent.news.cn/", kEnt},
  {"http://news.xinhuanet.com/sports/", kSports},
  {"http://education.news.cn/", kEdu},
  {"http://news.xinhuanet.com/health/", kHealth},
  {"http://travel.news.cn/", kTravel},
  {"http://news.xinhuanet.com/food/", kFood},
  {"http://news.xinhuanet.com/world/", kWorld},
  {"http://news.xinhuanet.com/gangao/", kHongkong},
  {"http://news.xinhuanet.com/tw/", kTaiwan},
};

static const char* kXinhuaContentTpl[] = {
  "config/recommendation/news/crawler/xinhua/xinhua_content1.tpl",
  "config/recommendation/news/crawler/xinhua/xinhua_content2.tpl",
  "config/recommendation/news/crawler/xinhua/xinhua_content3.tpl",
};

static string TrimLeadingStr(const string& leading, const string& newline,
                             const string &input) {
  string output;
  size_t pos = input.find(leading);
  if (pos != string::npos) {
    output = input.substr(pos + leading.size());
  } else {
    output = input;
  }
  pos = output.find_first_not_of(newline);
  if (pos == string::npos) {
    return "";
  } else if (pos == 0) {
    return output;
  } else {
    return output.substr(pos);
  }
}

static const char kLeadingStr[] = "来源：";
static const char kNewlineStr[] = "\r\n";

}

namespace recommendation {

XinhuaFetcher::XinhuaFetcher() {}

XinhuaFetcher::~XinhuaFetcher() {}
  
bool XinhuaFetcher::InitSeedVec() {
  for (auto& seed : kSeedArray) {
    seed_vec_.push_back(seed);
  }
  return true;
}
  
bool XinhuaFetcher::InitContentTemplateVec() {
  for (auto& tpl : kXinhuaContentTpl) {
    content_template_vec_.push_back(tpl);
  }
  return true;
}

bool XinhuaFetcher::InitUrlTopicMap() {
  for (auto& url_topic : kToFetchedUrlTopicArray) {
    url_topic_map_.insert(std::make_pair(url_topic.url, url_topic.topic));
  }
  return true;
}

bool XinhuaFetcher::SetPublishSource(const Json::Value& data,
                                     NewsInfo* news_info) {
  if (!data.isMember("publish_source")) {
    return false;
  }
  const Json::Value& sources = data["publish_source"];
  if (sources.size() == 2) {
    if (sources[1].empty()) 
      return false;
    string source;
    source = TrimLeadingStr(kLeadingStr, kNewlineStr, sources[1].asString());
    news_info->set_publish_source(source);
    return true;
  }
  if (sources.size() == 1) {
    if (sources[0].empty())
      return false;
    string source;
    source = TrimLeadingStr(kLeadingStr, kNewlineStr, sources[0].asString());
    news_info->set_publish_source(source);
    return true;
  }
  return false;
}

bool XinhuaFetcher::SetPublishTime(const Json::Value& data, 
                                   NewsInfo* news_info) {
  if (!data.isMember("publish_time")) {
    return false;
  }
  time_t time;
  bool ret =  ExtractTime1(data["publish_time"].asString(), &time);
  if (ret) {
    news_info->set_publish_time(static_cast<int32>(time));
    return true;
  }
  ret =  ExtractTime2(data["publish_time"].asString(), &time);
  if (ret) {
    news_info->set_publish_time(static_cast<int32>(time));
    return true;
  }
  return false;
}
  
bool XinhuaFetcher::SetDataUtilization(const Json::Value& data, 
                                       NewsInfo* news_info) {
  news_info->set_data_utilization(kRecommend);
  return true;
}

}  // namespace recommendation
