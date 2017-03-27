// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_SERVING_TRAIN_TRAIN_INFO_HANDLER_H_
#define PUSH_SERVING_TRAIN_TRAIN_INFO_HANDLER_H_

#include "base/compat.h"
#include "base/concurrent_queue.h"
#include "onebox/http_handler.h"
#include "util/net/http_server/http_request.h"
#include "util/net/http_server/http_response.h"

#include "push/proto/train_meta.pb.h"
#include "push/serving/train/time_table_dumper.h"
#include "push/serving/train/train_data_updater.h"
#include "push/serving/train/trainno_dumper.h"

namespace serving {

using namespace train;

class TimeTableQueryHandler : public HttpRequestHandler {
 public:
  TimeTableQueryHandler();
  virtual ~TimeTableQueryHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);
 
 private:
  string ErrorInfo(QueryStatus status) const;
  string ResponseInfo(const vector<TimeTable>& time_table_vector);
 
  std::unique_ptr<BaseTimeTableDumper> time_table_dumper_;
  std::unique_ptr<TrainDataUpdater> train_data_updater_;
  std::shared_ptr<ConcurrentQueue<string>> train_no_queue_;
  DISALLOW_COPY_AND_ASSIGN(TimeTableQueryHandler);
};

class TimeTableUpdateHandler : public HttpRequestHandler {
 public:
  TimeTableUpdateHandler();
  virtual ~TimeTableUpdateHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  std::unique_ptr<BaseTimeTableDumper> time_table_dumper_;
  DISALLOW_COPY_AND_ASSIGN(TimeTableUpdateHandler);
};

class TrainnoUpdateHandler : public HttpRequestHandler {
 public:
  TrainnoUpdateHandler();
  virtual ~TrainnoUpdateHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  std::unique_ptr<TrainnoDumper> trainno_dumper_;
  std::unique_ptr<TrainDataFetcher> train_data_fetcher_;
  DISALLOW_COPY_AND_ASSIGN(TrainnoUpdateHandler);
};

class StatusHandler : public HttpRequestHandler {
 public:
  StatusHandler();
  virtual ~StatusHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  DISALLOW_COPY_AND_ASSIGN(StatusHandler);
};

}  // namespace serving

#endif  // PUSH_SERVING_TRAIN_TRAIN_INFO_HANDLER_H_
