// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_SERVING_TRAIN_TRAINNO_DUMPER_H_
#define PUSH_SERVING_TRAIN_TRAINNO_DUMPER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "proto/mysql_config.pb.h"
#include "third_party/jsoncpp/json.h"

#include "push/proto/train_meta.pb.h"

namespace train {

class TrainnoDumper {
 public:
  TrainnoDumper();
  ~TrainnoDumper();
  void Init();
  bool DumpResultIntoDb(const Json::Value& result, TrainnoType type);

 private:
  bool ParseResult(const Json::Value& result,
                   TrainnoType type,
                   map<string, TrainnoType>* train_no_map);
  bool SaveMapIntoDb(const map<string, TrainnoType>& train_no_map);
  
  std::unique_ptr<MysqlServer> mysql_server_;
  DISALLOW_COPY_AND_ASSIGN(TrainnoDumper); 
};

}  // namespace train 

#endif  // PUSH_SERVING_TRAIN_TRAINNO_DUMPER_H_
