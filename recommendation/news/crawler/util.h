// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_CRAWLER_UTIL_H_
#define RECOMMENDATION_NEWS_CRAWLER_UTIL_H_

#include "base/compat.h"
#include "third_party/jsoncpp/json.h"
#include "recommendation/news/proto/topic_meta.pb.h"

namespace recommendation {

struct UrlTopicType {
  string url;
  PageTopic topic;
};

bool FetchHtml(const string& url, string* html);
bool FetchHtmlSimple(const string& url, string* html);

void ParseHtml(const string& html, const string& templatefile,
               Json::Value* result);

bool FetchToJson(const string& url,
                 const string& templatefile, Json::Value* result);
bool FetchToJsonSimple(const string& url,
                       const string& templatefile, Json::Value* result);

void ParseLinkData(const Json::Value& link_data, vector<string>* link_vec);
void ParseLinkToJson(const Json::Value& link_data,
                     vector<Json::Value>* data_vec);

bool IsValidLinkResult(const Json::Value& link_result);

bool IsValidLink(const Json::Value& link);

bool IsValidNewsContent(const Json::Value& input);

bool FetchNewsContent(const string& url, const string& templatefile,
                      Json::Value* content);

bool FetchNewsContent(const string& url,
                      const vector<string>& templates,
                      Json::Value* content);

template<typename T>
vector<vector<T>> SplitVector(const vector<T>& vec, size_t n) {
  vector<vector<T>> out_vec;
  size_t length = vec.size() / n;
  size_t remain = vec.size() % n;
  size_t begin = 0;
  size_t end = 0;
  for (size_t i = 0; i < std::min(n, vec.size()); ++i) {
    end += (remain > 0) ? (length + !!(remain--)) : (length);
    out_vec.push_back(vector<T>(vec.begin() + begin, vec.begin() + end));
    begin = end;
  }
  return out_vec;
}

template<typename T>
void MergeVector(const vector<vector<T>>& input_vec_vec, vector<T>* out_vec) {
  for (auto& input_vec : input_vec_vec) {
    out_vec->insert(out_vec->end(), input_vec.begin(), input_vec.end());
  }
}

}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_CRAWLER_UTIL_H_
