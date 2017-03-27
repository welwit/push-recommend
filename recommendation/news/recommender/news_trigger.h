// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_RECOMMENDER_NEWS_TRIGGER_H_
#define RECOMMENDATION_NEWS_RECOMMENDER_NEWS_TRIGGER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/singleton.h"
#include "proto/mysql_config.pb.h"

#include "recommendation/news/proto/news_meta.pb.h"

namespace recommendation {

class NewsTrigger {
 public:
  ~NewsTrigger();
  bool Trigger(int64 category, vector<StoryDetail>* result_vec);

 private:
  friend struct DefaultSingletonTraits<NewsTrigger>;
  NewsTrigger();
  bool FetchDoc(vector<NewsInfo>* news_info_vec);
  void TransformDoc(const vector<NewsInfo>& news_info_vec,
                    vector<StoryDetail>* result_vec);
  void TransformDoc(const vector<NewsInfo>& news_info_vec, 
      vector<StoryDetail>* result_vec,
      map<int64, vector<StoryDetail>>* category_result_map);
  bool ParseScore(const NewsInfo& news_info, StoryDetail* news_detail);
  void Sort(vector<StoryDetail>* doc_vec);
  
  std::vector<NewsInfo> doc_vec_cache_;
  std::unique_ptr<MysqlServer> mysql_config_;
  DISALLOW_COPY_AND_ASSIGN(NewsTrigger);
};

}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_RECOMMENDER_NEWS_TRIGGER_H_
