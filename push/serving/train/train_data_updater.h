// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_SERVING_TRAIN_TRAIN_DATA_UPDATER_H_
#define PUSH_SERVING_TRAIN_TRAIN_DATA_UPDATER_H_

#include "base/compat.h"
#include "base/thread.h"
#include "base/concurrent_queue.h"

#include "push/serving/train/time_table_dumper.h"

namespace train {

using namespace mobvoi;

class TrainDataUpdater : public Thread {
 public:
  TrainDataUpdater(std::shared_ptr<ConcurrentQueue<string>> queue);
  virtual ~TrainDataUpdater();
  virtual void Run();

 private:
  bool UpdateTrainInfoByTrainNo(const string& train_no);
  
  std::unique_ptr<BaseTimeTableDumper> time_table_dumper_;
  std::shared_ptr<ConcurrentQueue<string>> train_no_queue_;
  DISALLOW_COPY_AND_ASSIGN(TrainDataUpdater);
};

}  // namespace train 

#endif  // PUSH_SERVING_TRAIN_TRAIN_DATA_UPDATER_H_
