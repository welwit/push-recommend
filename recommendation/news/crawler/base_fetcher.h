// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_CRAWLER_BASE_FETCHER_H_
#define RECOMMENDATION_NEWS_CRAWLER_BASE_FETCHER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/mutex.h"
#include "proto/mysql_config.pb.h"
#include "third_party/jsoncpp/json.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"

#include "recommendation/news/proto/news_meta.pb.h"
#include "recommendation/news/proto/topic_meta.pb.h"

namespace recommendation {

enum DataUtilization {
  kTraining = 1,
  kRecommend = 2,
};

class BaseFetcher {
 public:
  BaseFetcher();
  virtual ~BaseFetcher();
  virtual bool Init();
  virtual bool InitSeedVec();
  virtual bool InitContentTemplateVec();
  virtual bool InitUrlTopicMap();
  virtual bool InitChannelTopicMap();
  virtual bool DoClear();

  virtual bool FetchLinkVec(vector<string>* link_vec);
  virtual bool FetchContent(const string& link, Json::Value* content);
  virtual bool FetchContentVec(const vector<string>& link_vec,
                               vector<Json::Value>* content_vec);
  virtual bool Parse(const Json::Value& content, NewsInfo* news_info);
  virtual bool ParseVec(const vector<Json::Value>& content_vec,
                        vector<NewsInfo>* news_info_vec);
  virtual bool SaveVecToDb(const vector<NewsInfo>& news_info_vec);
  virtual bool SaveStatementToMysql(sql::Statement* statement,
                                    const NewsInfo& news_info);
  virtual bool CustomFilter(const Json::Value& data, NewsInfo* news_info);

  virtual bool SetId(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetURL(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetTitle(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetContent(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetCategory(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetPublishSource(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetPublishTime(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetSummary(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetKeywords(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetTags(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetImage(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetMetaPublishSource(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetMetaPublishTime(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetDataUtilization(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetScore(const Json::Value& data, NewsInfo* news_info);
  virtual bool SetTopic(const Json::Value& data, NewsInfo* news_info);

 protected:
  string CleanContent(const string& content);
  bool FilterURL(const string& url);
  bool IsVisited(const string& url);
  bool ExtractTime1(const string& input, time_t* output);
  bool ExtractTime2(const string& input, time_t* output);
  bool ExtractTime3(const string& input, time_t* output);
  bool ExtractTime4(const string& input, time_t* output);

  std::unique_ptr<MysqlServer> mysql_config_;
  vector<string> content_template_vec_;
  map<string, PageTopic> url_topic_map_;
  vector<string> seed_vec_;
  set<string> visited_set_;
  map<string, PageTopic> channel_topic_map_;
  time_t last_clear_time_;
  mobvoi::SharedMutex mutex_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BaseFetcher);
};

}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_CRAWLER_BASE_FETCHER_H_
