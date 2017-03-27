// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_SERVING_TRAIN_TRAIN_DATA_FETCHER_H_
#define PUSH_SERVING_TRAIN_TRAIN_DATA_FETCHER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "third_party/jsoncpp/json.h"

namespace train {

class TrainDataFetcher {
 public:
  TrainDataFetcher();
  ~TrainDataFetcher();
  bool FetchTimeTable(const string& url, 
                      const string& template_file,
                      Json::Value* result);
 private:
  bool ParsePageContent(const string& url, 
                        const string& template_file,
                        const string& page_content, 
                        Json::Value* result); 
  
  DISALLOW_COPY_AND_ASSIGN(TrainDataFetcher);
};

}  // namespace train 

#endif  // PUSH_SERVING_TRAIN_TRAIN_DATA_FETCHER_H_
