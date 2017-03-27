// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_RANKER_RANKING_THREAD_H_
#define RECOMMENDATION_NEWS_RANKER_RANKING_THREAD_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/thread.h"

#include "recommendation/news/ranker/cosine_ranker.h"

namespace recommendation {

class RankingThread : public mobvoi::Thread {
 public:
  explicit RankingThread(CosineRanker* ranker);
  virtual ~RankingThread();
  virtual void Run();

 private:
  bool IfExecutingTask(time_t* ranking_time);
  bool ExecuteTask(time_t ranking_time);

  time_t last_ranking_time_;
  std::unique_ptr<MySQLDocReader> doc_reader_;
  std::unique_ptr<CosineRanker> ranker_;
  DISALLOW_COPY_AND_ASSIGN(RankingThread);
};

}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_RANKER_RANKING_THREAD_H_
