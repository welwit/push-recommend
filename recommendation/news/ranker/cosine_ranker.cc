// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/ranker/cosine_ranker.h"

#include <cmath>

#include "base/file/simple_line_reader.h"
#include "base/log.h"
#include "base/singleton.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"

DEFINE_int32(ranker_max_term_count, 300, "");
DEFINE_string(idf_dict_file, "config/recommendation/news/ranker/idf.txt", "");
DEFINE_string(stopwords_file, "config/util/nlp/segmenter/dict/stop_words.utf8", "");

namespace {

static const char* kNounPosTags[] = {"nr", "nrw", "nt", "nz", "ns", "ng"};

static bool IsGoodPosTag(const string& attr) {
  for (size_t i = 0; i < arraysize(kNounPosTags); ++i) {
    if (attr == kNounPosTags[i]) {
      return true;
    }
  }
  return false;
}

}

namespace recommendation {

struct TermFreqCompare {
  bool operator()(const pair<string, TfIdfInfo>& left,
                  const pair<string, TfIdfInfo>& right) {
    if (left.second.tf == right.second.tf) {
      return left.second.df < right.second.df;
    }
    return left.second.tf > right.second.tf;
  }
};

struct ScoreCompare {
  bool operator()(const pair<string, TfIdfInfo>& left,
                  const pair<string, TfIdfInfo>& right) {
    return left.second.score > right.second.score;
  }
};

struct NaturalTfIdfCompare {
  bool operator()(const pair<string, TfIdfInfo>& left,
                  const pair<string, TfIdfInfo>& right) {
    return left.second.natural_tf_idf > right.second.natural_tf_idf;
  }
};

struct LogTfIdfCompare {
  bool operator()(const pair<string, TfIdfInfo>& left,
                  const pair<string, TfIdfInfo>& right) {
    return left.second.log_tf_idf > right.second.log_tf_idf;
  }
};

struct DocScoreCompare {
  bool operator()(const pair<uint64, DocResult>& left,
                  const pair<uint64, DocResult>& right) {
    return left.second.hot_score > right.second.hot_score;
  }
};

CosineRanker::CosineRanker() {
  doc_reader_.reset(new MySQLDocReader);
  doc_writer_.reset(new MySQLDocWriter);
  segmenter_.reset(segmenter::SegmenterManager::GetSegmenter("comb", ""));
  LoadStopwords();
  LoadIdfDict();
  term_manager_ = Singleton<indexing::TermManager>::get();
  extractor_.reset(new keyword::QueryKeyWordExtractor(term_manager_,
                                                      &stop_words_));
}

CosineRanker::~CosineRanker() {
}

bool CosineRanker::LoadStopwords() {
  file::SimpleLineReader reader(FLAGS_stopwords_file, true);
  std::vector<std::string> stop_words;
  if (!reader.ReadLines(&stop_words)) {
    return false;
  }
  stop_words_.insert(stop_words.begin(), stop_words.end());
  LOG(INFO) << "stop_words total:" << stop_words_.size();
  return true;
}

bool CosineRanker::LoadIdfDict() {
  file::SimpleLineReader reader(FLAGS_idf_dict_file, true);
  std::vector<std::string> idf_line_vec;
  if (!reader.ReadLines(&idf_line_vec)) {
    return false;
  }
  for (auto& idf_line : idf_line_vec) {
    std::vector<std::string> idf_vec;
    SplitString(idf_line, '\t', &idf_vec);
    word_idf_map_[idf_vec[0]] = StringToDouble(idf_vec[1]);
  }
  LOG(INFO) << "word_idf_map total: " << word_idf_map_.size();
  return true;
}

bool CosineRanker::ReadAllDoc(std::vector<NewsInfo>* doc_vec) {
  return doc_reader_->FetchAllDoc(doc_vec);
}

bool CosineRanker::ReadDailyHotDoc(std::vector<NewsInfo>* doc_vec) {
  return doc_reader_->FetchDailyHotDoc(doc_vec);
}

bool CosineRanker::ReadDailyRankingDoc(std::vector<NewsInfo>* doc_vec) {
  return doc_reader_->FetchDailyRankingDoc(doc_vec);
}

bool CosineRanker::GetKeywords(
    const std::vector<NewsInfo>& doc_vec,
    std::vector<std::pair<string, TfIdfInfo>>* result_vec) {
  LOG(INFO) << "GetKeywords runing ...";
  std::map<string, TfIdfInfo> term_info_map;
  std::set<string> term_set;
  int total_doc = 0;
  for (auto& doc : doc_vec) {
    ++total_doc;
    term_set.clear();
    std::vector<std::string> kw_vec;
    SplitString(doc.keywords(), ',', &kw_vec);
    for (auto& kw : kw_vec) {
      if (term_set.find(kw) == term_set.end()) {
        if (term_info_map.find(kw) == term_info_map.end()) {
          TfIdfInfo tfidf;
          term_info_map[kw] = tfidf;
        }
        term_info_map[kw].df += 1;
      }
      term_info_map[kw].tf += 1;
    }
  }
  result_vec->insert(result_vec->end(),
                     term_info_map.begin(), term_info_map.end());
  sort(result_vec->begin(), result_vec->end(), TermFreqCompare());
  LOG(INFO) << "Keywords_Result total:" << result_vec->size();
  for (auto& result : *result_vec) {
    LOG(INFO) << "Result:" << result.first
              << "\t" << result.second.tf
              << "\t" << result.second.df;
  }
  return true;
}

bool CosineRanker::GetTermsFromTitle(
    const std::vector<NewsInfo>& doc_vec,
    std::vector<std::pair<string, TfIdfInfo>>* result_vec) {
  LOG(INFO) << "GetTermsFromTitle runing ...";
  std::vector<std::string> title_vec;
  for (auto& doc : doc_vec) {
    title_vec.push_back(doc.title());
  }
  return GetTerms(title_vec, result_vec);
}

bool CosineRanker::GetTermsFromTitleNew(
    const std::vector<NewsInfo>& doc_vec,
    std::vector<std::pair<string, TfIdfInfo>>* result_vec) {
  LOG(INFO) << "GetTermsFromTitleNew runing ...";
  std::vector<std::string> title_vec;
  for (auto& doc : doc_vec) {
    title_vec.push_back(doc.title());
  }
  return GetTermsNew(title_vec, result_vec);
}

bool CosineRanker::GetTermsFromContent(
    const std::vector<NewsInfo>& doc_vec,
    std::vector<std::pair<string, TfIdfInfo>>* result_vec) {
  LOG(INFO) << "GetTermsFromContent runing ...";
  std::vector<std::string> content_vec;
  for (auto& doc : doc_vec) {
    content_vec.push_back(doc.content());
  }
  return GetTerms(content_vec, result_vec);
}

bool CosineRanker::GetTerms(const std::vector<string>& doc_vec,
    std::vector<std::pair<string, TfIdfInfo>>* result_vec) {
  LOG(INFO) << "GetTerms ...";
  std::map<string, TfIdfInfo> term_info_map;
  std::set<string> term_set;
  int total_doc = 0;
  for (auto& doc : doc_vec) {
    ++total_doc;
    term_set.clear();
    segmenter_->FeedText(doc);
    segmenter::SegmentToken token;
    vector<segmenter::SegmentToken> tokens;
    vector<keyword::KeyWord> key_word_vec;
    while (segmenter_->Next(&token)) {
      tokens.push_back(token);
    }
    keyword::QueryKeyWordExtractInput input;
    input.segmented_tokens = &tokens;
    extractor_->ExtractKeyWordsFromQuery(input, &key_word_vec);
    for (auto it = key_word_vec.begin(); it != key_word_vec.end(); ++it) {
      const keyword::KeyWord& keyword = *it;
      const string& term = keyword.token;
      const string& postag = keyword.feat.postag;
      if (stop_words_.find(term) != stop_words_.end()) continue;
      if (!IsGoodPosTag(postag)) continue;

      if (term_set.find(term) == term_set.end()) {
        if (term_info_map.find(term) == term_info_map.end()) {
          TfIdfInfo tfidf;
          term_info_map[term] = tfidf;
          term_info_map[term].attr = postag;
        }
        term_info_map[term].df += 1;
      }
      term_info_map[term].tf += 1;
      term_info_map[term].score += keyword.score;
    }
  }
  result_vec->insert(result_vec->end(),
                     term_info_map.begin(), term_info_map.end());
  sort(result_vec->begin(), result_vec->end(), ScoreCompare());
  LOG(INFO) << "Result total:" << result_vec->size();
  for (auto& result : *result_vec) {
    LOG(INFO) << "Result:" << result.first
              << "\t" << result.second.tf
              << "\t" << result.second.df
              << "\t" << result.second.attr
              << "\t" << result.second.score;
  }
  return true;
}

bool CosineRanker::GetTermsNew(const std::vector<string>& doc_vec,
    std::vector<std::pair<string, TfIdfInfo>>* result_vec) {
  LOG(INFO) << "GetTermsNew ...";
  std::map<string, TfIdfInfo> term_info_map;
  std::set<string> term_set;
  int total_doc = 0;
  for (auto& doc : doc_vec) {
    ++total_doc;
    term_set.clear();
    segmenter_->FeedText(doc);
    segmenter::SegmentToken token;
    vector<segmenter::SegmentToken> tokens;
    vector<keyword::KeyWord> key_word_vec;
    while (segmenter_->Next(&token)) {
      tokens.push_back(token);
    }
    keyword::QueryKeyWordExtractInput input;
    input.segmented_tokens = &tokens;
    extractor_->ExtractKeyWordsFromQuery(input, &key_word_vec);
    for (auto it = key_word_vec.begin(); it != key_word_vec.end(); ++it) {
      const keyword::KeyWord& keyword = *it;
      const string& term = keyword.token;
      const string& postag = keyword.feat.postag;
      if (stop_words_.find(term) != stop_words_.end()) continue;
      if (!IsGoodPosTag(postag)) continue;
      if (term_set.find(term) == term_set.end()) {
        if (term_info_map.find(term) == term_info_map.end()) {
          TfIdfInfo tfidf;
          term_info_map[term] = tfidf;
          term_info_map[term].attr = postag;
          if (word_idf_map_.find(term) != word_idf_map_.end()) {
            term_info_map[term].idf = word_idf_map_[term];
          } else {
            term_info_map[term].idf = 0;
          }
        }
        term_info_map[term].df += 1;
      }
      term_info_map[term].tf += 1;
      term_info_map[term].natural_tf_idf += term_info_map[term].idf;
    }
  }
  for (auto it = term_info_map.begin(); it != term_info_map.end(); ++it) {
    it->second.log_tf_idf = (1 + log2(it->second.tf)) * (it->second.idf);
  }
  result_vec->insert(result_vec->end(),
                     term_info_map.begin(), term_info_map.end());
  sort(result_vec->begin(), result_vec->end(), NaturalTfIdfCompare());
  LOG(INFO) << "Result total:" << result_vec->size();
  for (auto& result : *result_vec) {
    LOG(INFO) << "Result:" << result.first
              << "\t" << result.second.tf
              << "\t" << result.second.df
              << "\t" << result.second.idf
              << "\t" << result.second.natural_tf_idf
              << "\t" << result.second.log_tf_idf
              << "\t" << result.second.attr;
  }
  return true;
}

bool CosineRanker::GetTopnTerms(
    size_t topn, std::vector<std::pair<string, TfIdfInfo>>* result_vec) {
  LOG(INFO) << "GetTopnTerms ...";
  if (topn < result_vec->size()) {
    result_vec->resize(topn);
  }
  double square_sum_natural_tf_idf = 0;
  double square_sum_log_tf_idf = 0;
  for (auto& result : *result_vec) {
    square_sum_natural_tf_idf +=
      result.second.natural_tf_idf * result.second.natural_tf_idf;
    square_sum_log_tf_idf +=
      result.second.log_tf_idf * result.second.log_tf_idf;
  }
  for (auto& result : *result_vec) {
    result.second.natural_tf_idf =
      result.second.natural_tf_idf / sqrt(square_sum_natural_tf_idf);
    result.second.log_tf_idf =
      result.second.log_tf_idf / sqrt(square_sum_log_tf_idf);
  }
  sort(result_vec->begin(), result_vec->end(), LogTfIdfCompare());
  LOG(INFO) << "Topn_Terms total:" << result_vec->size();
  for (auto& result : *result_vec) {
    LOG(INFO) << "Term_Result:" << result.first
              << "\t" << result.second.tf
              << "\t" << result.second.df
              << "\t" << result.second.idf
              << "\t" << result.second.natural_tf_idf
              << "\t" << result.second.log_tf_idf
              << "\t" << result.second.attr;
  }
  return true;
}

bool CosineRanker::VectorizeDocTitle(const NewsInfo& doc,
    std::pair<uint64, DocInfo>* doc_info_pair) {
  LOG(INFO) << "VectorizeDocTitle runing ...";
  uint64 doc_id = doc.news_id();
  doc_info_pair->first = doc_id;
  VectorizeDoc(doc.title(), &doc_info_pair->second.title_map);
  return true;
}

bool CosineRanker::VectorizeDocContent(const NewsInfo& doc,
    std::pair<uint64, DocInfo>* doc_info_pair) {
  LOG(INFO) << "VectorizeDocContent runing ...";
  uint64 doc_id = doc.news_id();
  doc_info_pair->first = doc_id;
  VectorizeDoc(doc.content(), &doc_info_pair->second.content_map);
  return true;
}

bool CosineRanker::VectorizeDoc(const string& doc,
                                std::map<string, TfIdfInfo>* result_map) {
  LOG(INFO) << "VectorizeDoc ...";
  std::map<string, TfIdfInfo>& term_info_map = *result_map;
  segmenter_->FeedText(doc);
  segmenter::SegmentToken token;
  vector<segmenter::SegmentToken> tokens;
  while (segmenter_->Next(&token)) {
    tokens.push_back(token);
  }
  for (auto & token : tokens) {
    const string& term = token.term();
    if (stop_words_.find(term) != stop_words_.end()) continue;
    if (term_info_map.find(term) == term_info_map.end()) {
      TfIdfInfo tfidf;
      term_info_map[term] = tfidf;
      term_info_map[term].df = 1;
      if (word_idf_map_.find(term) != word_idf_map_.end()) {
        term_info_map[term].idf = word_idf_map_[term];
      } else {
        term_info_map[term].idf = 0;
      }
    }
    term_info_map[term].tf += 1;
  }
  for (auto& term_info : term_info_map) {
    term_info.second.log_tf_idf =
      (1 + log2(term_info.second.tf)) * (term_info.second.idf);
    term_info.second.natural_tf_idf =
      (term_info.second.tf) * (term_info.second.idf);
  }
  double square_sum_natural_tf_idf = 0;
  double square_sum_log_tf_idf = 0;
  for (auto& result : term_info_map) {
    square_sum_natural_tf_idf +=
      result.second.natural_tf_idf * result.second.natural_tf_idf;
    square_sum_log_tf_idf +=
      result.second.log_tf_idf * result.second.log_tf_idf;
  }
  for (auto& result : term_info_map) {
    result.second.natural_tf_idf =
      result.second.natural_tf_idf / sqrt(square_sum_natural_tf_idf);
    result.second.log_tf_idf =
      result.second.log_tf_idf / sqrt(square_sum_log_tf_idf);
  }
  for (auto& result : term_info_map) {
    LOG(INFO) << "Doc_Result:" << result.first
              << "\t" << result.second.tf
              << "\t" << result.second.df
              << "\t" << result.second.idf
              << "\t" << result.second.natural_tf_idf
              << "\t" << result.second.log_tf_idf;
  }
  return true;
}

bool CosineRanker::VectorizeDocTitleAll(const vector<NewsInfo>& doc_vec,
    std::map<uint64, DocInfo>* result_map) {
  for (auto& doc : doc_vec) {
    auto it = result_map->find(doc.news_id());
    if (it == result_map->end()) {
      std::pair<uint64, DocInfo> doc_info_pair;
      VectorizeDocTitle(doc, &doc_info_pair);
      result_map->insert(doc_info_pair);
    } else {
      LOG(INFO) << "Can't support modify map";
    }
  }
  return true;
}

bool CosineRanker::VectorizeDocContentAll(const vector<NewsInfo>& doc_vec,
    std::map<uint64, DocInfo>* result_map) {
  int total = 0;
  for (auto& doc : doc_vec) {
    auto it = result_map->find(doc.news_id());
    if (it == result_map->end()) {
      std::pair<uint64, DocInfo> doc_info_pair;
      VectorizeDocContent(doc, &doc_info_pair);
      result_map->insert(doc_info_pair);
      total += 1;
    } else {
      LOG(INFO) << "Can't support modify map";
    }
  }
  LOG(INFO) << "VectorizeDocContentAll total:" << total 
            << ", doc_total:" << doc_vec.size();
  return true;
}

bool CosineRanker::PrepareRankingInput(RankerInput* ranker_input) {
  LOG(INFO) << "PrepareRankingInput ...";
  std::vector<NewsInfo> doc_vec;
  ReadDailyHotDoc(&doc_vec);
  GetTermsFromTitleNew(doc_vec, &ranker_input->term_vec);
  GetTopnTerms(FLAGS_ranker_max_term_count, &ranker_input->term_vec);
  doc_vec.clear();
  ReadDailyRankingDoc(&doc_vec);
  for (auto& doc : doc_vec) {
    ranker_input->news_info_map.insert(std::make_pair(doc.news_id(), doc));
  }
  VectorizeDocContentAll(doc_vec, &ranker_input->doc_info_map);
  return true;
}

bool CosineRanker::ExecuteRanking(
    const RankerInput& ranker_input, RankerOutput* ranker_output) {
  LOG(INFO) << "ExecuteRanking ...";
  std::map<uint64, DocResult> doc_score_map;
  for (auto& term_pair : ranker_input.term_vec) {
    const string& term = term_pair.first;
    const TfIdfInfo& tfidf_info = term_pair.second;
    for (auto& doc_info : ranker_input.doc_info_map) {
      uint64 doc_id = doc_info.first;
      auto it = doc_info.second.content_map.find(term);
      if (it != doc_info.second.content_map.end()) {
        if (doc_score_map.find(doc_id) == doc_score_map.end()) {
          DocResult doc_result;
          doc_result.hot_score =
            it->second.log_tf_idf * tfidf_info.log_tf_idf;
          doc_result.hit_hot_terms.push_back(term);
          doc_score_map[doc_id] = doc_result;
        } else {
          DocResult& doc_result = doc_score_map[doc_id];
          doc_result.hot_score +=
            it->second.log_tf_idf * tfidf_info.log_tf_idf;
          doc_result.hit_hot_terms.push_back(term);
          doc_score_map[doc_id] = doc_result;
        }
      }
    }
  }
  ranker_output->doc_hot_score_vec.insert(
      ranker_output->doc_hot_score_vec.end(),
      doc_score_map.begin(), doc_score_map.end());
  sort(ranker_output->doc_hot_score_vec.begin(),
       ranker_output->doc_hot_score_vec.end(), DocScoreCompare());
  for (auto& doc : ranker_output->doc_hot_score_vec) {
    auto it = ranker_input.news_info_map.find(doc.first);
    if (it == ranker_input.news_info_map.end()) continue;
    string hit_list = "[";
    for (auto& hit : doc.second.hit_hot_terms) {
      hit_list += hit + ",";
    }
    hit_list = hit_list.substr(0, hit_list.size() - 1) + "]";
    LOG(INFO) << "HIT doc:" << doc.first
              << ", score:" << doc.second.hot_score
              << ", term_list:" << hit_list;
    LOG(INFO) << "HIT doc detail: " << "url=" << it->second.url()
              << ", title=" << it->second.title()
              << ", summary=" << it->second.summary();
  }
  LOG(INFO) << "Ranked doc total:" << ranker_output->doc_hot_score_vec.size();
  return true;
}

bool CosineRanker::ProcessRankingOutput(const RankerOutput& ranker_output) {
  LOG(INFO) << "ProcessRankingOutput ...";
  doc_writer_->WriteDoc(ranker_output);
  return true;
}

}  // namespace recommendation
