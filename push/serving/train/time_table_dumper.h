// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_SERVING_TRAIN_TIME_TABLE_DUMPER_H_
#define PUSH_SERVING_TRAIN_TIME_TABLE_DUMPER_H_

#include <memory>

#include "base/basictypes.h"
#include "base/compat.h"
#include "proto/mysql_config.pb.h"

#include "push/proto/train_meta.pb.h"
#include "push/serving/train/train_data_fetcher.h"

namespace train {

class BaseTimeTableDumper {
 public:
  BaseTimeTableDumper();
  ~BaseTimeTableDumper();
  void Init();
  bool UpdateTimeTable();
  QueryStatus QueryTimeTable(const string& train_no, 
                             const string& depart_station,
                             vector<TimeTable>* time_table_vector);
  bool FetchAndDumpTimeTable(const vector<string>& train_no_vector);
  bool QueryAllTrainNo(vector<string>* result_vector);
  void FetchTimeTable(const vector<string>& result_vector,
                      vector<TimeTable>* time_table_vector);
  bool DumpResultIntoDb(const vector<TimeTable>& time_table_vector);
  virtual bool FetchTimeTableByTrainNo(
      const string& train_no, vector<TimeTable>* time_table_vector) = 0;

 protected:
  std::unique_ptr<TrainDataFetcher> train_data_fetcher_;

 private:
  std::unique_ptr<MysqlServer> mysql_server_;
  DISALLOW_COPY_AND_ASSIGN(BaseTimeTableDumper);
};

class CtripTimeTableDumper : public BaseTimeTableDumper {
 public:
  CtripTimeTableDumper();
  ~CtripTimeTableDumper();
  virtual bool FetchTimeTableByTrainNo(
      const string& train_no, vector<TimeTable>* time_table_vector);

 private:
  DISALLOW_COPY_AND_ASSIGN(CtripTimeTableDumper);
};

class BaiduTimeTableDumper : public BaseTimeTableDumper {
 public:
  BaiduTimeTableDumper();
  ~BaiduTimeTableDumper();
  virtual bool FetchTimeTableByTrainNo(
      const string& train_no, vector<TimeTable>* time_table_vector);

 private:
  DISALLOW_COPY_AND_ASSIGN(BaiduTimeTableDumper);
};

}  // namespace train 

#endif  // PUSH_SERVING_TRAIN_TIME_TABLE_DUMPER_H_
