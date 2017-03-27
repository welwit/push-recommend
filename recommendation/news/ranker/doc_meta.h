// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_RANKER_DOC_META_H_
#define RECOMMENDATION_NEWS_RANKER_DOC_META_H_

#include "base/compat.h"
#include "recommendation/news/proto/news_meta.pb.h"

namespace recommendation {

struct TfIdfInfo {
  int tf;
  int df;
  double idf;
  double natural_tf_idf;
  double log_tf_idf;
  double score;
  string attr;
  TfIdfInfo() :
    tf(0), df(0), idf(0), natural_tf_idf(0),
    log_tf_idf(0), score(0), attr("") {}
};

struct DocInfo {
  std::map<string, TfIdfInfo> title_map;
  std::map<string, TfIdfInfo> content_map;
  std::map<string, TfIdfInfo> keyword_map;
};

struct DocResult {
  double hot_score;
  vector<string> hit_hot_terms;
};

struct RankerInput {
  vector<std::pair<string, TfIdfInfo>> term_vec;
  std::map<uint64, NewsInfo> news_info_map;
  std::map<uint64, DocInfo> doc_info_map;
};

struct RankerOutput {
  vector<std::pair<uint64, DocResult>> doc_hot_score_vec;
};


}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_RANKER_DOC_META_H_
