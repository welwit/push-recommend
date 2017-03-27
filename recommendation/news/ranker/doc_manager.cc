// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/ranker/doc_manager.h"

#include "base/log.h"
#include "base/string_util.h"
#include "push/util/time_util.h"
#include "third_party/jsoncpp/json.h"
#include "util/mysql/mysql_util.h"
#include "recommendation/news/proto/news_meta.pb.h"

DECLARE_string(mysql_config_file);

namespace {

static const string kTimeFormat = "%Y-%m-%d %H:%M:%S";

static const char kQueryDailyHotDocSQL[] =
    "SELECT id, url, title, content, category, publish_source, "
    "publish_time, summary, keywords, tags, image, meta_publish_source, "
    "meta_publish_time, data_utilization, score, topic FROM news_info "
    "WHERE date(updated) = '%s';";

static const char kQueryDailyRankingDocSQL[] =
    "SELECT id, url, title, content, category, publish_source, "
    "publish_time, summary, keywords, tags, image, meta_publish_source, "
    "meta_publish_time, data_utilization, score, topic FROM news_info "
    "WHERE date(updated) = '%s';";

static const char kQueryALLDocSQL[] =
    "SELECT id, url, title, content, category, publish_source, "
    "publish_time, summary, keywords, tags, image, meta_publish_source, "
    "meta_publish_time, data_utilization, score, topic FROM news_info;";

static const char kQueryUpdateTimeSQL[] = 
    "SELECT updated FROM news_info ORDER BY updated DESC LIMIT 1;";

static const char kUpdateDocScoreSQL[] = 
    "INSERT INTO news_info (id, score) VALUES('%lu', '%s') ON DUPLICATE KEY "
    "UPDATE score = '%s';";

}

namespace recommendation {

MySQLDocReader::MySQLDocReader() {}

MySQLDocReader::~MySQLDocReader() {}

bool MySQLDocReader::FetchDoc(const string& command,
                              std::vector<NewsInfo>* doc_vec) {
  Json::Value result_array;
  bool ret = util::GetMysqlResult(FLAGS_mysql_config_file,
                                  command, &result_array, true);
  if (!ret) {
    LOG(WARNING) << "Fetch training data failed";
    return false;
  }
  VLOG(3) << "Result_array:" << result_array.toStyledString();
  for (size_t i = 0; i < result_array.size(); ++i) {
    NewsInfo news_info;
    const string& id = result_array[i]["id"].asString();
    news_info.set_news_id(StringToUint64(id));
    news_info.set_url(result_array[i]["url"].asString());
    news_info.set_title(result_array[i]["title"].asString());
    news_info.set_content(result_array[i]["content"].asString());
    news_info.set_category(result_array[i]["category"].asString());
    news_info.set_publish_source(result_array[i]["publish_source"].asString());
    const string& publish_time_str = result_array[i]["publish_time"].asString();
    time_t publish_time, meta_publish_time;
    recommendation::DatetimeToTimestamp(publish_time_str,
                                        &publish_time, kTimeFormat);
    news_info.set_publish_time(publish_time);
    news_info.set_summary(result_array[i]["summary"].asString());
    news_info.set_keywords(result_array[i]["keywords"].asString());
    news_info.set_tags(result_array[i]["tags"].asString());
    news_info.set_image(result_array[i]["image"].asString());
    news_info.set_meta_publish_source(result_array[i]["meta_publish_source"].asString());
    const string& meta_publish_time_str = result_array[i]["meta_publish_time"].asString();
    recommendation::DatetimeToTimestamp(meta_publish_time_str,
                                        &meta_publish_time, kTimeFormat);
    news_info.set_meta_publish_time(meta_publish_time);
    const string& data_utilization = result_array[i]["data_utilization"].asString();
    news_info.set_data_utilization(StringToInt(data_utilization));
    news_info.set_score(result_array[i]["score"].asString());
    news_info.set_topic(result_array[i]["topic"].asString());
    LOG(INFO) << "News_info:" << news_info.Utf8DebugString();
    doc_vec->push_back(news_info);
  }
  LOG(INFO) << "Fetched Result doc_vec size:" << doc_vec->size();
  return true;
}

bool MySQLDocReader::FetchAllDoc(std::vector<NewsInfo>* doc_vec) {
  string command = kQueryALLDocSQL;
  return FetchDoc(command, doc_vec);
}

bool MySQLDocReader::FetchDailyHotDoc(std::vector<NewsInfo>* doc_vec) {
  string today;
  MakeDate(0, &today);
  string command = StringPrintf(kQueryDailyHotDocSQL, today.c_str());
  return FetchDoc(command, doc_vec);
}

bool MySQLDocReader::FetchDailyRankingDoc(std::vector<NewsInfo>* doc_vec) {
  string today;
  MakeDate(0, &today);
  string command = StringPrintf(kQueryDailyRankingDocSQL, today.c_str());
  return FetchDoc(command, doc_vec);
}
  
bool MySQLDocReader::FetchUpdateTime(time_t* last_update_time) {
  string command = kQueryUpdateTimeSQL;
  Json::Value result;
  bool ret = util::GetMysqlResult(FLAGS_mysql_config_file,
                                  command, &result, false);
  if (!ret) {
    LOG(WARNING) << "Fetch update time failed";
    return false;
  }
  recommendation::DatetimeToTimestamp(result["updated"].asString(),
                                      last_update_time, kTimeFormat);
  return true;
}

MySQLDocWriter::MySQLDocWriter() {}

MySQLDocWriter::~MySQLDocWriter() {}

bool MySQLDocWriter::WriteDoc(const RankerOutput& ranker_output) {
  int total = 0;
  for (auto& result : ranker_output.doc_hot_score_vec) {
    uint64 doc_id = result.first;
    const DocResult& doc_result = result.second;
    Json::Value hot(Json::objectValue);
    hot["hot_score"] = doc_result.hot_score;
    string hit_hot_terms = JoinString(doc_result.hit_hot_terms, ',');
    hot["hit_hot_terms"] = hit_hot_terms; 
    Json::Value value;
    value["hot"] = hot;
    Json::FastWriter writer;
    string score = writer.write(value);
    string command = StringPrintf(kUpdateDocScoreSQL, doc_id, 
                                  score.c_str(), score.c_str());
    if (!util::RunMysqlCommand(FLAGS_mysql_config_file, command)) {
      LOG(WARNING) << "Failed when Run command:" << command;
      continue;
    }
    total += 1;
  }
  LOG(INFO) << "WriteDoc total:" << total;
  return true;
}

}  // namespace recommendation
