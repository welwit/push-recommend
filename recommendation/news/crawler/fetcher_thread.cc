// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/crawler/fetcher_thread.h"

#include "base/log.h"
#include "base/time.h"
#include "recommendation/news/crawler/util.h"


DECLARE_int32(news_crawler_process_inteval);

DEFINE_int32(content_fetcher_thread_number, 15, "");

namespace recommendation {
  
ContentFetcherThread::ContentFetcherThread() {}

ContentFetcherThread::~ContentFetcherThread() {}

void ContentFetcherThread::Run() {
  if (link_vec_.empty()) {
    LOG(INFO) << "link_vec is empty";
    return;
  }
  LOG(INFO) << "FetchContentVec start, tid=" << GetThreadId();
  content_vec_.clear();
  fetcher_->FetchContentVec(link_vec_, &content_vec_);
  LOG(INFO) << "FetchContentVec end, tid=" << GetThreadId();
}

  
void ContentFetcherThread::InitFetcher(std::shared_ptr<BaseFetcher> fetcher) {
  fetcher_ = fetcher;
}

void ContentFetcherThread::InitLinkVec(const vector<string>& link_vec) {
  link_vec_ = link_vec;
}

void ContentFetcherThread::GetResultVec(std::vector<Json::Value>* content_vec) {
  content_vec->insert(content_vec->end(), 
                      content_vec_.begin(), content_vec_.end());
}

FetcherThread::FetcherThread(BaseFetcher* base_fetcher) {
  CHECK(base_fetcher != nullptr);
  fetcher_.reset(base_fetcher);
  fetcher_->Init();
}

FetcherThread::~FetcherThread() {}

void FetcherThread::Run() {
  while (true) {
    Process();
    mobvoi::Sleep(FLAGS_news_crawler_process_inteval);
  }
}

void FetcherThread::set_using_multithreads_content_fetcher(
    bool using_multithreads_content_fetcher) {
  using_multithreads_content_fetcher_ = using_multithreads_content_fetcher;
}

bool FetcherThread::Process() {
  vector<string> link_vec;
  vector<Json::Value> content_vec;
  vector<NewsInfo> news_info_vec;
  fetcher_->DoClear();
  if (!fetcher_->FetchLinkVec(&link_vec)) {
    LOG(WARNING) << "Fetch link vec failed";
    return false;
  }
  LOG(INFO) << "Fetched link_vec total: " << link_vec.size();
  if (using_multithreads_content_fetcher_) {
    if (!MultiThreadsFetchContentVec(link_vec, &content_vec)) {
      LOG(WARNING) << "MultiThreadsFetchContentVec failed";
      return false;
    }
  } else {
    if (!fetcher_->FetchContentVec(link_vec, &content_vec)) {
      LOG(WARNING) << "FetchContentVec failed";
      return false;
    }
  }
  LOG(INFO) << "Fetched content_vec total: " << content_vec.size();
  if (!fetcher_->ParseVec(content_vec, &news_info_vec)) {
    LOG(WARNING) << "ParseVec failed";
    return false;
  }
  LOG(INFO) << "Parse news_info_vec total: " << news_info_vec.size();
  if (!fetcher_->SaveVecToDb(news_info_vec)) {
    LOG(WARNING) << "SaveVecToDb failed";
    return false;
  }
  return true;
}

bool FetcherThread::MultiThreadsFetchContentVec(
    const vector<string>& link_vec, vector<Json::Value>* content_vec) {
  vector<vector<string>> link_vec_vec;
  link_vec_vec = SplitVector(link_vec, FLAGS_content_fetcher_thread_number);
  vector<vector<Json::Value>> result_vec_vec;
  ContentFetcherThread thread_array[link_vec_vec.size()];
  for (size_t i = 0; i < link_vec_vec.size(); ++i) {
    LOG(INFO) << "link_vector input total:" << link_vec_vec[i].size();
    thread_array[i].InitFetcher(fetcher_);
    thread_array[i].InitLinkVec(link_vec_vec[i]);
    thread_array[i].Start();
  }
  for (size_t i = 0; i < link_vec_vec.size(); ++i) {
    thread_array[i].Join();
    std::vector<Json::Value> result_vec;
    thread_array[i].GetResultVec(&result_vec);
    LOG(INFO) << "Sub result_vec total:" << result_vec.size();
    if (!result_vec.empty()) {
      result_vec_vec.push_back(result_vec);
    }
  }
  MergeVector(result_vec_vec, content_vec);
  LOG(INFO) << "Content_vec total:" << content_vec->size();
  return true;
}

FetcherThreadPool::FetcherThreadPool() {
  XinhuaFetcher* xinhua_fetcher = new XinhuaFetcher();
  FetcherThread* xinhua_fetcher_thread = new FetcherThread(xinhua_fetcher);
  xinhua_fetcher_thread->set_using_multithreads_content_fetcher(false);
  thread_vec_.push_back(xinhua_fetcher_thread);
  
  RssSinaFetcher* sina_fetcher = new RssSinaFetcher();
  FetcherThread* sina_fetcher_thread = new FetcherThread(sina_fetcher);
  sina_fetcher_thread->set_using_multithreads_content_fetcher(true);
  thread_vec_.push_back(sina_fetcher_thread);

  TencentFetcher* tencent_fetcher = new TencentFetcher();
  FetcherThread* tencent_fetcher_thread = new FetcherThread(tencent_fetcher);
  thread_vec_.push_back(tencent_fetcher_thread);
  tencent_fetcher_thread->set_using_multithreads_content_fetcher(true);
  LOG(INFO) << "Thread_vec size:" << thread_vec_.size();
}

FetcherThreadPool::~FetcherThreadPool() {
  Stop();
}

void FetcherThreadPool::Start() {
  for (auto& thread : thread_vec_) {
    if (thread != nullptr) {
      thread->Start();
    }
  }
}

void FetcherThreadPool::Stop() {
  for (auto& thread : thread_vec_) {
    if (thread != nullptr) {
      thread->Join();
      delete thread;
    }
  }
}

}  // namespace recommendation
