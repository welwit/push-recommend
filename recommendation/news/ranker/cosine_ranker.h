// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_RANKER_COSINE_RANKER_H_
#define RECOMMENDATION_NEWS_RANKER_COSINE_RANKER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "indexing/util/term_weight.h"
#include "util/nlp/keyword/query_key_word_extractor.h"
#include "util/nlp/keyword/query_key_word_linear_model.h"
#include "util/nlp/segmenter/impl/comb_segment.h"
#include "util/nlp/segmenter/segmenter_manager.h"

#include "recommendation/news/proto/news_meta.pb.h"
#include "recommendation/news/ranker/doc_manager.h"
#include "recommendation/news/ranker/doc_meta.h"

namespace recommendation {

class CosineRanker {
 public:
  CosineRanker();
  virtual ~CosineRanker();
  bool LoadStopwords();
  bool LoadIdfDict();
  bool ReadAllDoc(std::vector<NewsInfo>* doc_vec);
  bool ReadDailyHotDoc(std::vector<NewsInfo>* doc_vec);
  bool ReadDailyRankingDoc(std::vector<NewsInfo>* doc_vec);

  bool GetKeywords(const std::vector<NewsInfo>& doc_vec,
      std::vector<std::pair<string, TfIdfInfo>>* result_vec);
  bool GetTermsFromTitle(const std::vector<NewsInfo>& doc_vec,
      std::vector<std::pair<string, TfIdfInfo>>* result_vec);
  bool GetTermsFromTitleNew(const std::vector<NewsInfo>& doc_vec,
      std::vector<std::pair<string, TfIdfInfo>>* result_vec);
  bool GetTermsFromContent(const std::vector<NewsInfo>& doc_vec,
      std::vector<std::pair<string, TfIdfInfo>>* result_vec);
  bool GetTerms(const std::vector<string>& doc_vec,
      std::vector<std::pair<string, TfIdfInfo>>* result_vec);
  bool GetTermsNew(const std::vector<string>& doc_vec,
      std::vector<std::pair<string, TfIdfInfo>>* result_vec);
  bool GetTopnTerms(size_t topn,
      std::vector<std::pair<string, TfIdfInfo>>* result_vec);

  bool VectorizeDocTitle(const NewsInfo& doc,
                         std::pair<uint64, DocInfo>* doc_info_pair);
  bool VectorizeDocContent(const NewsInfo& doc,
                           std::pair<uint64, DocInfo>* doc_info_pair);
  bool VectorizeDoc(const string& doc,
                    std::map<string, TfIdfInfo>* result_map);
  bool VectorizeDocTitleAll(const vector<NewsInfo>& doc_vec,
      std::map<uint64, DocInfo>* result_map);
  bool VectorizeDocContentAll(const vector<NewsInfo>& doc_vec,
      std::map<uint64, DocInfo>* result_map);

  bool PrepareRankingInput(RankerInput* ranker_input);
  bool ExecuteRanking(const RankerInput& ranker_input, RankerOutput* ranker_output);
  bool ProcessRankingOutput(const RankerOutput& ranker_output);

 private:
  std::unique_ptr<MySQLDocReader> doc_reader_;
  std::unique_ptr<MySQLDocWriter> doc_writer_;
  base::hash_set<string> stop_words_;
  base::hash_map<string, float> word_idf_map_;
  std::unique_ptr<segmenter::Segmenter> segmenter_;
  std::unique_ptr<keyword::QueryKeyWordExtractor> extractor_;
  indexing::TermManager* term_manager_;
  DISALLOW_COPY_AND_ASSIGN(CosineRanker);
};

}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_RANKER_COSINE_RANKER_H_
