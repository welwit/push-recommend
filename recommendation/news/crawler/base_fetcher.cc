// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/crawler/base_fetcher.h"

#include "base/file/proto_util.h"
#include "base/hash.h"
#include "base/log.h"
#include "base/time.h"
#include "push/util/time_util.h"
#include "third_party/gflags/gflags.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/re2//re2/re2.h"
#include "util/protobuf/proto_json_format.h"

#include "recommendation/news/crawler/util.h"

DECLARE_string(link_template);
DECLARE_string(mysql_config_file);

namespace {

static const char kInsertFormat[] =
    "INSERT INTO news_info (id, url, title, content, category, "
    "publish_source, publish_time, summary, keywords, tags, image, "
    "meta_publish_source, meta_publish_time, data_utilization, score, topic) "
    "VALUES('%lu', '%s', '%s', '%s', '%s', '%s', FROM_UNIXTIME('%d'), '%s', "
    "'%s', '%s', '%s', '%s', FROM_UNIXTIME('%d'), '%d', '%s', '%s');";

static const char kSelectFormat[] =
    "SELECT id FROM news_info WHERE id = '%lu';";

static const int kSummaryMinWordCount = 500;

}

namespace recommendation {

BaseFetcher::BaseFetcher() : last_clear_time_(0) {
  mysql_config_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config_file,
                                    mysql_config_.get()));
}
  
bool BaseFetcher::Init() {
  InitSeedVec();
  InitContentTemplateVec();
  InitUrlTopicMap();
  return true;
}

BaseFetcher::~BaseFetcher() {}

bool BaseFetcher::InitSeedVec() {
  return true;
}

bool BaseFetcher::InitContentTemplateVec() {
  return true;
}

bool BaseFetcher::InitUrlTopicMap() {
  return true;
}

bool BaseFetcher::InitChannelTopicMap() {
  return true;
}

bool BaseFetcher::DoClear() {
  LOG(INFO) << "DoClear start, visited urls total:" << visited_set_.size();
  time_t now = time(NULL);
  CHECK(now > last_clear_time_);
  mobvoi::Time time_now = mobvoi::Time::FromTimeT(now);
  mobvoi::Time time_last_clear = mobvoi::Time::FromTimeT(last_clear_time_);
  mobvoi::Time::Exploded exploded_now, exploded_last_clear;
  time_now.LocalExplode(&exploded_now);
  time_last_clear.LocalExplode(&exploded_last_clear);
  if (exploded_now.year == exploded_last_clear.year &&
      exploded_now.month == exploded_last_clear.month &&
      exploded_now.day_of_month == exploded_last_clear.day_of_month) {
    LOG(INFO) << "DoClear do nothing";
    return false;
  }
  mobvoi::WriteLock locker(&mutex_);
  visited_set_.clear();
  last_clear_time_ = now;
  LOG(INFO) << "DoClear ok, visited urls total:" << visited_set_.size();
  return true;
}

bool BaseFetcher::FetchLinkVec(vector<string>* link_vec) {
  for (auto& seed : seed_vec_) {
    Json::Value result;
    if (!FetchToJson(seed, FLAGS_link_template, &result)) {
      continue;
    }
    if (!IsValidLinkResult(result)) {
      continue;
    }
    const Json::Value& data = result["data"];
    for (Json::ArrayIndex i = 0; i < data.size(); ++i) {
      if (!IsValidLink(data[i])) {
        continue;
      }
      string url = data[i]["link"].asString();
      if (FilterURL(url)) continue;
      LOG(INFO) << "Fetch URL ok, url:" << url;
      if (!IsVisited(url)) {
        link_vec->push_back(url);
      } else {
        LOG(INFO) << "FetchLinkVec, URL is visited, url:" << url;
      }
    }
  }
  return true;
}

bool BaseFetcher::FetchContent(const string& link, Json::Value* content) {
  if (IsVisited(link)) {
    LOG(INFO) << "FetchContent, URL is visited, url:" << link;
    return false;
  }
  if (FetchNewsContent(link, content_template_vec_, content)) {
    mobvoi::WriteLock locker(&mutex_);
    visited_set_.insert(link);
    return true;
  } else {
    LOG(WARNING) << "FetchNewsContent failed, url=" << link;
    return false;
  }
}

bool BaseFetcher::FetchContentVec(const vector<string>& link_vec,
                                  vector<Json::Value>* content_vec) {
  for (auto& link : link_vec) {
    Json::Value content;
    if (FetchContent(link, &content)) {
      content_vec->push_back(content);
      LOG(INFO) << "Content:" << content.toStyledString();
    }
  }
  LOG(INFO) << "content vector total:" << content_vec->size();
  return true;
}

bool BaseFetcher::Parse(const Json::Value& content, NewsInfo* news_info) {
  if (!IsValidNewsContent(content)) {
    return false;
  }
  const Json::Value& data = content["data"];
  if (!SetId(data, news_info)) {
    LOG(WARNING) << "SetId failed, data:" << data.toStyledString();
    return false;
  }
  if (!SetURL(data, news_info)) {
    LOG(WARNING) << "SetURL failed, data:" << data.toStyledString();
    return false;
  }
  if (!SetTitle(data, news_info)) {
    LOG(WARNING) << "SetTitle failed, data:" << data.toStyledString();
    return false;
  }
  if (!SetContent(data, news_info)) {
    LOG(WARNING) << "SetContent failed, data:" << data.toStyledString();
    return false;
  }
  if (!SetCategory(data, news_info)) {
    LOG(WARNING) << "SetCategory failed, data:" << data.toStyledString();
    return false;
  }
  if (data.isMember("publish_source")) {
    if (!SetPublishSource(data, news_info)) {
      LOG(WARNING) << "SetPublishSource failed, data:" << data.toStyledString();
      return false;
    }
  }
  if (data.isMember("publish_time")) {
    if (!SetPublishTime(data, news_info)) {
      LOG(WARNING) << "SetPublishTime failed, data:" << data.toStyledString();
      return false;
    }
  }
  if (!SetSummary(data, news_info)) {
    LOG(WARNING) << "SetSummary failed, data:" << data.toStyledString();
    return false;
  }
  if (data.isMember("keywords")) {
    if (!SetKeywords(data, news_info)) {
      LOG(WARNING) << "SetKeywords failed, data:" << data.toStyledString();
      return false;
    }
  }
  if (data.isMember("tags")) {
    if (!SetTags(data, news_info)) {
      LOG(WARNING) << "SetTags failed, data:" << data.toStyledString();
      return false;
    }
  }
  if (data.isMember("image")) {
    if (!SetImage(data, news_info)) {
      LOG(WARNING) << "SetImage failed, data:" << data.toStyledString();
      return false;
    }
  }
  if (data.isMember("meta_publish_source")) {
    if (!SetMetaPublishSource(data, news_info)) {
      LOG(WARNING) << "SetMetaPublishSource failed, data:" 
                   << data.toStyledString();
      return false;
    }
  }
  if (data.isMember("meta_publish_time")) {
    if (!SetMetaPublishTime(data, news_info)) {
      LOG(WARNING) << "SetMetaPublishTime failed, data:" 
                   << data.toStyledString();
      return false;
    }
  }
  if (!SetDataUtilization(data, news_info)) {
    LOG(WARNING) << "SetDataUtilization failed, data:" << data.toStyledString();
    return false;
  }
  if (!SetScore(data, news_info)) {
    LOG(WARNING) << "SetScore failed, data:" << data.toStyledString();
    return false;
  }
  if (!SetTopic(data, news_info)) {
    LOG(WARNING) << "SetTopic failed, data:" << data.toStyledString();
    return false;
  }
  if (CustomFilter(data, news_info)) {
    LOG(WARNING) << "CustomFilter work, data:" << data.toStyledString();        
    return false;                                                               
  }
  std::string value;
  util::ProtoJsonFormat::PrintToFastString(*news_info, &value);
  LOG(INFO) << "Good News:" << value;
  return true;
}

bool BaseFetcher::ParseVec(const vector<Json::Value>& content_vec,
                           vector<NewsInfo>* news_info_vec) {
  for (auto& content : content_vec) {
    NewsInfo news_info;
    if (Parse(content, &news_info)) {
      news_info_vec->push_back(news_info);
    }
  }
  LOG(INFO) << "news_info vector total:" << news_info_vec->size();
  return true;
}

bool BaseFetcher::SaveVecToDb(const vector<NewsInfo>& news_info_vec) {
  try {
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection>
      connection(driver->connect(mysql_config_->host(),
                                 mysql_config_->user(),
                                 mysql_config_->password()));
    connection->setSchema(mysql_config_->database());
    sql::Statement* statement = connection->createStatement();
    if (statement) {
      for (auto& news_info : news_info_vec) {
        SaveStatementToMysql(statement, news_info);
      }
      delete statement;
    }
    return true;
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "SQLException:" << e.what();
    return false;
  }
}

bool BaseFetcher::SaveStatementToMysql(sql::Statement* statement,
                                       const NewsInfo& news_info) {
  string select_cmd = StringPrintf(kSelectFormat, news_info.news_id());
  string insert_cmd = StringPrintf(kInsertFormat,
      news_info.news_id(), news_info.url().c_str(),
      news_info.title().c_str(), news_info.content().c_str(),
      news_info.category().c_str(), news_info.publish_source().c_str(),
      news_info.publish_time(), news_info.summary().c_str(),
      news_info.keywords().c_str(), news_info.tags().c_str(),
      news_info.image().c_str(), news_info.meta_publish_source().c_str(),
      news_info.meta_publish_time(), news_info.data_utilization(),
      news_info.score().c_str(), news_info.topic().c_str());
  try {
    std::unique_ptr<sql::ResultSet>
      result_set(statement->executeQuery(select_cmd));
    if (result_set->rowsCount() > 0) {
      CHECK(result_set->rowsCount() == 1);
      LOG(INFO) << "News is existed in db, id:" << news_info.news_id();
      return false;
    } else {
      string value;
      statement->executeUpdate(insert_cmd);
      util::ProtoJsonFormat::PrintToFastString(news_info, &value);
      LOG(INFO) << "Insert into mysql suc, id:"
                << news_info.news_id() << ", news_info:" << value;
      return true;
    }
  } catch (const sql::SQLException& e) {
    LOG(WARNING) << "SQLException:" << e.what();
    return false;
  };
}

bool BaseFetcher::CustomFilter(const Json::Value& data, NewsInfo* news_info) {
  return false;
}

bool BaseFetcher::SetId(const Json::Value& data, NewsInfo* news_info) {
  if (!data.isMember("url")) {
    return false;
  }
  news_info->set_news_id(mobvoi::Fingerprint(data["url"].asString()));
  return true;
}

bool BaseFetcher::SetURL(const Json::Value& data, NewsInfo* news_info) {
  if (!data.isMember("url")) {
    return false;
  }
  news_info->set_url(data["url"].asString());
  return true;
}

bool BaseFetcher::SetTitle(const Json::Value& data, NewsInfo* news_info) {
  if (!data.isMember("title")) {
    return false;
  }
  news_info->set_title(data["title"].asString());
  return true;
}

bool BaseFetcher::SetContent(const Json::Value& data, NewsInfo* news_info) {
  if (!data.isMember("content")) {
    return false;
  }
  const Json::Value& content_list = data["content"];
  string content;
  for (Json::ArrayIndex i = 0; i < content_list.size(); ++i) {
    if (!content_list[i].isNull() &&
        content_list[i].asString().find("\r\n") != string::npos) {
      LOG(INFO) << "BAD content:" << content_list.toStyledString();
      return false;
    }
    if (content_list[i].isNull()) {
      content += "\1"; // 一般是非文字，如图片，视频等, 这里做标识
    } else {
      content += content_list[i].asString() + "\2"; // 换行标识
    }
  }
  news_info->set_content(content);
  return true;
}

bool BaseFetcher::SetCategory(const Json::Value& data, NewsInfo* news_info) {
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
  return false;
}

bool BaseFetcher::SetPublishSource(const Json::Value& data,
                                   NewsInfo* news_info) {
  return true;
}

bool BaseFetcher::SetPublishTime(const Json::Value& data, NewsInfo* news_info) {
  return true;
}

bool BaseFetcher::SetSummary(const Json::Value& data, NewsInfo* news_info) {
  if (!news_info->has_content()) {
    return false;
  }
  string input = CleanContent(news_info->content());
  if (input[input.size() - 1] == '\2') {
    input = input.substr(0, input.size() - 1);
  }
  vector<string> vec;
  SplitString(input, '\2', &vec);
  string summary;
  int32 total = 0;
  for (auto& line : vec) {
    total += line.size();
    summary += line + "\n";
    if (summary.size() >= kSummaryMinWordCount) {
      break;
    }
  }
  if (summary.size() < kSummaryMinWordCount) {
    return false;
  }
  news_info->set_summary(summary);
  return true;
}

bool BaseFetcher::SetKeywords(const Json::Value& data, NewsInfo* news_info) {
  return true;
}

bool BaseFetcher::SetTags(const Json::Value& data, NewsInfo* news_info) {
  return true;
}

bool BaseFetcher::SetImage(const Json::Value& data, NewsInfo* news_info) {
  return true;
}

bool BaseFetcher::SetMetaPublishSource(const Json::Value& data,
                                       NewsInfo* news_info) {
  return true;
}

bool BaseFetcher::SetMetaPublishTime(const Json::Value& data,
                                     NewsInfo* news_info) {
  return true;
}

bool BaseFetcher::SetDataUtilization(const Json::Value& data, 
                                     NewsInfo* news_info) {
  return true;
}

bool BaseFetcher::SetScore(const Json::Value& data, NewsInfo* news_info) {
  return true;
}

bool BaseFetcher::SetTopic(const Json::Value& data, NewsInfo* news_info) {
  return true;
}

string BaseFetcher::CleanContent(const string& content) {
  size_t idx = 0;
  size_t size = content.size();
  string result = "";
  while (idx < size) {
    size_t pos_start = content.find_first_of("\1", idx);
    if (pos_start == string::npos) {
      result += content.substr(idx);
      break;
    } else {
      size_t pos_end = content.find_first_of("\2", pos_start);
      if (pos_end == string::npos) {
        result += content.substr(idx, pos_start - idx);
        break;
      } else {
        result += content.substr(idx, pos_start - idx);
        idx = pos_end + 1;
      }
    }
  }
  return result;
}

bool BaseFetcher::FilterURL(const string& url) {
  for (auto& url_topic : url_topic_map_) {
    if (url.find(url_topic.first) != string::npos) {
      return false;
    }
  }
  LOG(INFO) << "url is filter, url:" << url;
  return true;
}

bool BaseFetcher::IsVisited(const string& url) {
  mobvoi::ReadLock locker(&mutex_);
  return visited_set_.find(url) != visited_set_.end();
}

bool BaseFetcher::ExtractTime1(const string& input, time_t* output) {
  RE2 re("(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2})");
  string time;
  if (RE2::FullMatch(input, re, &time)) {
    DatetimeToTimestamp(time, output, "%Y-%m-%d %H:%M:%S");
    return true;
  }
  return false;
}

bool BaseFetcher::ExtractTime2(const string& input, time_t* output) {
  RE2 re("(\\d{4})年(\\d{2})月(\\d{2})日 (\\d{2}:\\d{2}:\\d{2})");
  string year, month, day, time;
  if (RE2::FullMatch(input, re, &year, &month, &day, &time)) {
    string time_str = StringPrintf("%s-%s-%s %s",
        year.c_str(), month.c_str(), day.c_str(), time.c_str());
    DatetimeToTimestamp(time_str, output, "%Y-%m-%d %H:%M:%S");
    return true;
  }
  return false;
}
  
bool BaseFetcher::ExtractTime3(const string& input, time_t* output) {
  RE2 re("(\\d{4}-\\d{2}-\\d{2})T(\\d{2}:\\d{2}:\\d{2})");
  string date, time;
  if (RE2::PartialMatch(input, re, &date, &time)) {
    string datetime = date + " " + time;
    DatetimeToTimestamp(datetime, output, "%Y-%m-%d %H:%M:%S");
    return true;
  }
  return false;
}

bool BaseFetcher::ExtractTime4(const string& input, time_t* output) {
  RE2 re("(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2})");
  string time;
  if (RE2::FullMatch(input, re, &time)) {
    DatetimeToTimestamp(time, output, "%Y-%m-%d %H:%M");
    return true;
  }
  return false;
}

}  // namespace recommendation
