// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/recommender/news_trigger.h"

#include "base/base64.h"
#include "base/file/proto_util.h"
#include "base/string_util.h"
#include "push/util/time_util.h"
#include "push/util/common_util.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"
#include "util/protobuf/proto_json_format.h"

DECLARE_string(mysql_config);

namespace {

static const char kKeyPattern[] = "NEWSSRC-*";
static const char kSelectFormat[] =
    "SELECT id, url, title, content, category, publish_source, publish_time, "
    "summary, keywords, tags, image, meta_publish_source, meta_publish_time, "
    "data_utilization, score, topic FROM news_info "
    "WHERE publish_time > '%s' and data_utilization = '2';";

static void RandomShuffle(vector<recommendation::StoryDetail>* rec_list) {
  random_shuffle(rec_list->begin(), rec_list->end());
}

static const int kMaxResultNum = 5;
static const int kMinHitWordCount = 5;

}


namespace recommendation {

struct HotScoreCompare {
  bool operator()(const StoryDetail& left, const StoryDetail& right) {
    return left.hot_score() > right.hot_score();
  }
};

struct HitNumCompare {
  bool operator()(const StoryDetail& left, const StoryDetail& right) {
    return left.hit_hot_terms().size() > right.hit_hot_terms().size();
  }
};

NewsTrigger::NewsTrigger() {
  mysql_config_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_config_.get()));
}

NewsTrigger::~NewsTrigger() {}

bool NewsTrigger::Trigger(int64 category, vector<StoryDetail>* result_vec) {
  map<int64, vector<StoryDetail>> category_result_map;
  result_vec->clear();
  vector<NewsInfo> news_info_vec;
  if (!FetchDoc(&news_info_vec)) {
    return false;
  }
  TransformDoc(news_info_vec, result_vec, &category_result_map);
  bool using_sort = true;
  int64 default_category = 0;
  if (using_sort) {
    if (category != default_category) {
      if (category_result_map.find(category) != category_result_map.end()) {
        result_vec->clear();
        result_vec->insert(result_vec->end(), 
            category_result_map[category].begin(), 
            category_result_map[category].end());
      }
    }
    Sort(result_vec);
  } else { 
    RandomShuffle(result_vec);
  }
  return true;
}

bool NewsTrigger::FetchDoc(vector<NewsInfo>* news_info_vec) {
  try {
    string yestorday;
    recommendation::MakeDate(-1, &yestorday);
    yestorday = yestorday + " 20:00:00";
    string select_sql = StringPrintf(kSelectFormat, yestorday.c_str());
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection>
      connection(driver->connect(
            mysql_config_->host(),
            mysql_config_->user(),
            mysql_config_->password()));
    connection->setSchema(mysql_config_->database());
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    std::unique_ptr<sql::ResultSet> result_set(
        statement->executeQuery(select_sql));
    if (result_set->rowsCount() > 0) {
      while (result_set->next()) {
        try {
          NewsInfo news_info;
          string news_id = result_set->getString("id");
          news_info.set_news_id(StringToUint64(news_id));
          news_info.set_url(result_set->getString("url"));
          news_info.set_title(result_set->getString("title"));
          news_info.set_category(result_set->getString("category"));
          news_info.set_publish_source(result_set->getString("publish_source"));
          time_t publish_time;
          recommendation::DatetimeToTimestamp(
              result_set->getString("publish_time"),
              &publish_time, "%Y-%m-%d %H:%M:%S");
          news_info.set_publish_time(publish_time);
          news_info.set_summary(result_set->getString("summary"));
          news_info.set_keywords(result_set->getString("keywords"));
          news_info.set_tags(result_set->getString("tags"));
          news_info.set_image(result_set->getString("image"));
          news_info.set_meta_publish_source(
              result_set->getString("meta_publish_source"));
          time_t meta_publish_time;
          recommendation::DatetimeToTimestamp(
              result_set->getString("meta_publish_time"), 
              &meta_publish_time, "%Y-%m-%d %H:%M:%S");
          news_info.set_meta_publish_time(meta_publish_time);
          news_info.set_data_utilization(result_set->getInt("data_utilization"));
          news_info.set_score(result_set->getString("score"));
          news_info.set_topic(result_set->getString("topic"));
          string output;
          util::ProtoJsonFormat::PrintToFastString(news_info, &output);
          VLOG(1) << "News info:" << output;
          news_info_vec->push_back(news_info);
        } catch (const sql::SQLException &e) {
          LOG(WARNING) << "SQLException: " << e.what();
        }
      }
    }
    LOG(INFO) << "Fetched doc total: " << news_info_vec->size();
    return true;
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "SQLException: " << e.what();
    return false;
  }
}

void NewsTrigger::TransformDoc(
    const vector<NewsInfo>& news_info_vec, vector<StoryDetail>* result_vec,
    map<int64, vector<StoryDetail>>* category_result_map) {
  for (auto& news_info : news_info_vec) {
    string input;
    util::ProtoJsonFormat::PrintToFastString(news_info, &input);
    if (news_info.title().empty() ||
        news_info.summary().empty() ||
        news_info.category().empty()) {
      LOG(INFO) << "Filter input news_info:" << input;
      continue;
    }
    StoryDetail story;
    story.set_app_url(news_info.url());
    if (!news_info.image().empty() && 
        StartsWithASCII(news_info.image(), "http://", true)) {
      story.set_background_url(news_info.image());
    } else {
      story.set_background_url("");
    }
    story.set_browser_url(news_info.url());
    int64 category;
    StringToInt64(news_info.category(), &category);
    story.set_cid(category);
    string publish_time;
    if (news_info.publish_time() != 0) {
      recommendation::TimestampToDatetime(news_info.publish_time(),
                                          &publish_time, "%m-%d");
    } else if (news_info.meta_publish_time() != 0) {
      recommendation::TimestampToDatetime(news_info.meta_publish_time(),
                                          &publish_time, "%m-%d");
    } else {
      LOG(INFO) << "Filter invalid publish_time story, id=" 
                << news_info.news_id();
      continue;
    }
    story.set_publish_time(publish_time);
    string publish_source;
    if (!news_info.publish_source().empty()) {
      publish_source = news_info.publish_source();
    } else if (!news_info.meta_publish_source().empty()) {
      publish_source = news_info.meta_publish_source();
    } else {
      LOG(INFO) << "Filter invalid publish_source story, id=" 
                << news_info.news_id();
      continue;
    }
    story.set_source(publish_source);
    story.set_summary(news_info.summary());
    story.set_tip("0");
    story.set_title(news_info.title());
    story.set_type("news");
    story.set_id(StringPrintf("%lu", news_info.news_id()));
    story.set_keywords(news_info.keywords());
    ParseScore(news_info, &story);
    string output;
    util::ProtoJsonFormat::PrintToFastString(story, &output);
    VLOG(1) << "Story:" << output;
    result_vec->push_back(story);
    if (category_result_map->find(story.cid()) == category_result_map->end()) {
      vector<StoryDetail> story_vec;
      story_vec.push_back(story);
      (*category_result_map)[category] = story_vec;
    } else {
      (*category_result_map)[category].push_back(story);
    }
  }
  LOG(INFO) << "result_vec size:" << result_vec->size();
  for (auto& category_result : *category_result_map) {
    LOG(INFO) << "Category:" << category_result.first 
              << ", results_size:" << category_result.second.size();
  }
}


bool NewsTrigger::ParseScore(const NewsInfo& news_info, 
                             StoryDetail* news_detail) {
  const string& score = news_info.score();
  if (score.empty()) {
   return true;
  } 
  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(score, root)) {
    LOG(WARNING) << "Reader parse score failed";
    return false;
  }
  Json::Value& hot = root["hot"];
  const string& hot_score_str = hot["hot_score"].asString();
  news_detail->set_hot_score(StringToDouble(hot_score_str));
  const string& hit_hot_terms = hot["hit_hot_terms"].asString();
  std::vector<std::string> hit_term_vec;
  SplitString(hit_hot_terms, ',', &hit_term_vec);
  for (auto& hit_term : hit_term_vec) {
    news_detail->add_hit_hot_terms(hit_term);    
  }
  return true;
}
  
void NewsTrigger::Sort(vector<StoryDetail>* doc_vec) {
  static const char kTrimChars[] = {' '};
  set<string> publish_source_set;
  set<string> title_set;
  sort(doc_vec->begin(), doc_vec->end(), HitNumCompare());
  if (doc_vec->size() > kMaxResultNum) {
    vector<StoryDetail> result_vec;
    vector<StoryDetail> backup_vec;
    for (auto& doc : *doc_vec) {
      string source, title;
      TrimString(doc.source(), kTrimChars, &source);
      TrimString(doc.title(), kTrimChars, &title);
      if (doc.hit_hot_terms().size() >= kMinHitWordCount &&
          title_set.find(title) == title_set.end() &&
          publish_source_set.find(source) == publish_source_set.end()) {
        result_vec.push_back(doc);
        title_set.insert(title);
        publish_source_set.insert(source);
      } else {
        backup_vec.push_back(doc);
      }
    }
    if (result_vec.size() >= kMaxResultNum) {
      result_vec.resize(kMaxResultNum);
    } else {
      RandomShuffle(&backup_vec);
      for (auto& backup : backup_vec) {
        result_vec.push_back(backup); 
        if (result_vec.size() >= kMaxResultNum) {
          break;
        }
      }
    }
    doc_vec->clear();
    doc_vec->insert(doc_vec->end(), result_vec.begin(), result_vec.end());
  }
}
  
}  // namespace recommendation
