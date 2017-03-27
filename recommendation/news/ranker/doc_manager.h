// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_RANKER_DOC_MANAGER_H_
#define RECOMMENDATION_NEWS_RANKER_DOC_MANAGER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "recommendation/news/proto/news_meta.pb.h"
#include "recommendation/news/ranker/doc_meta.h"

namespace recommendation {

class MySQLDocReader {
 public:
  MySQLDocReader();
  ~MySQLDocReader();
  bool FetchDoc(const string& command, std::vector<NewsInfo>* doc_vec);
  bool FetchAllDoc(std::vector<NewsInfo>* doc_vec);
  bool FetchDailyHotDoc(std::vector<NewsInfo>* doc_vec);
  bool FetchDailyRankingDoc(std::vector<NewsInfo>* doc_vec);
  bool FetchUpdateTime(time_t* last_update_time);

 private:
  DISALLOW_COPY_AND_ASSIGN(MySQLDocReader);
};

class MySQLDocWriter {
 public:
  MySQLDocWriter();
  ~MySQLDocWriter();
  bool WriteDoc(const RankerOutput& ranker_output);

 private:
  DISALLOW_COPY_AND_ASSIGN(MySQLDocWriter);
};

}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_RANKER_DOC_MANAGER_H_
