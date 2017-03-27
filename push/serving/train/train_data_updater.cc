// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/serving/train/train_data_updater.h"

#include "base/log.h"

DECLARE_int32(data_source);

namespace train {

TrainDataUpdater::TrainDataUpdater(
    std::shared_ptr<ConcurrentQueue<string>> queue) 
  : Thread(false) {
  LOG(INFO) << "construct TrainDataUpdater";
  train_no_queue_ = queue;
  if (FLAGS_data_source == kSourceCtrip) {
    time_table_dumper_.reset(new CtripTimeTableDumper());
  } else {
    time_table_dumper_.reset(new BaiduTimeTableDumper());
  }
}

TrainDataUpdater::~TrainDataUpdater() {}

void TrainDataUpdater::Run() {
  LOG(INFO) << "start thread TrainDataUpdater";
  while (true) {
    string train_no;
    train_no_queue_->Pop(train_no);
    if (!UpdateTrainInfoByTrainNo(train_no)) {
      LOG(ERROR) << "update train info by train_no failed, train_no:"
                  << train_no;
    } else {
      LOG(INFO) << "update train info success, train_no:" << train_no;
    }
  }
}

bool TrainDataUpdater::UpdateTrainInfoByTrainNo(const string& train_no) {
  LOG(INFO) << "start update train info, train_no:" << train_no;
  vector<string> train_no_vector;
  train_no_vector.push_back(train_no);
  return time_table_dumper_->FetchAndDumpTimeTable(train_no_vector);
}

}  // namespace train 
