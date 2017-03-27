// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/ranker/ranking_thread.h"

#include "base/log.h"
#include "base/time.h"

DECLARE_int32(news_ranker_execute_interval);

namespace recommendation {

RankingThread::RankingThread(CosineRanker* ranker) : last_ranking_time_(0) {
  CHECK(ranker != nullptr);
  ranker_.reset(ranker);
  doc_reader_.reset(new MySQLDocReader);
}

RankingThread::~RankingThread() {}

void RankingThread::Run() {
  while (true) {
    time_t ranking_time = 0;
    if (IfExecutingTask(&ranking_time)) {
      if (!ExecuteTask(ranking_time)) {
        LOG(ERROR) << "Execute task failed, ranking_time:" << ranking_time;
      }
    }
    mobvoi::Sleep(FLAGS_news_ranker_execute_interval);
  }
}
  
bool RankingThread::IfExecutingTask(time_t* ranking_time) {
  time_t last_update_time;
  if (!doc_reader_->FetchUpdateTime(&last_update_time)) {
    LOG(INFO) << "Failed in FetchUpdateTime, also will Execute task";
    return true;
  }
  if (last_update_time > last_ranking_time_) {
    *ranking_time = time(NULL);
    return true;
  } else {
    return false;
  } 
}

bool RankingThread::ExecuteTask(time_t ranking_time) {
  RankerInput ranker_input;
  RankerOutput ranker_output;
  ranker_->PrepareRankingInput(&ranker_input);
  ranker_->ExecuteRanking(ranker_input, &ranker_output);
  ranker_->ProcessRankingOutput(ranker_output);
  last_ranking_time_ = ranking_time;
  return true;
}

}  // namespace recommendation
