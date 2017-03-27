// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_CRAWLER_FETCHER_THREAD_H_
#define RECOMMENDATION_NEWS_CRAWLER_FETCHER_THREAD_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/thread.h"

#include "recommendation/news/crawler/sina_fetcher.h"
#include "recommendation/news/crawler/xinhua_fetcher.h"
#include "recommendation/news/crawler/tencent_fetcher.h"

namespace recommendation {

class ContentFetcherThread : public mobvoi::Thread {
 public:
  ContentFetcherThread();
  virtual ~ContentFetcherThread();
  virtual void Run();
  void InitFetcher(std::shared_ptr<BaseFetcher> fetcher);
  void InitLinkVec(const vector<string>& link_vec); 
  void GetResultVec(std::vector<Json::Value>* content_vec);

 private:
  std::vector<std::string> link_vec_;
  std::vector<Json::Value> content_vec_;
  std::shared_ptr<BaseFetcher> fetcher_;
  DISALLOW_COPY_AND_ASSIGN(ContentFetcherThread);
};

class FetcherThread : public mobvoi::Thread {
 public:
  explicit FetcherThread(BaseFetcher* base_fetcher);
  virtual ~FetcherThread();
  virtual void Run();
  void set_using_multithreads_content_fetcher(bool using_multithreads_content_fetcher);

 private:
  bool Process();
  bool MultiThreadsFetchContentVec(const vector<string>& link_vec,
                                   vector<Json::Value>* content_vec);
  
  std::shared_ptr<BaseFetcher> fetcher_;
  bool using_multithreads_content_fetcher_;
  DISALLOW_COPY_AND_ASSIGN(FetcherThread);
};

class FetcherThreadPool {
 public:
  FetcherThreadPool();
  ~FetcherThreadPool();
  void Start();
  void Stop();
 
 private:
  std::vector<FetcherThread*> thread_vec_;
  DISALLOW_COPY_AND_ASSIGN(FetcherThreadPool);
};

}  // namespace recommendation

#endif  // RECOMMENDATION_NEWS_CRAWLER_FETCHER_THREAD_H_
