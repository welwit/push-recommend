// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_NEWS_NEWS_PUSH_PROCESSOR_H_
#define PUSH_PUSH_CONTROLLER_NEWS_NEWS_PUSH_PROCESSOR_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/singleton.h"
#include "recommendation/news/proto/news_meta.pb.h"

#include "push/push_controller/push_processor.h"
#include "push/util/common_util.h"
#include "push/util/redis_util.h"
#include "push/util/user_info_helper.h"

namespace push_controller {

class NewsPushProcessor {
 public:
  NewsPushProcessor();
  ~NewsPushProcessor();
  bool Process();

 private:
  friend struct DefaultSingletonTraits<NewsPushProcessor>;

  bool GetRecList(const string& device, Json::Value* rec_results);
  bool BuildPushMessage(const string& device,
                        const recommendation::StoryDetail& rec_content,
                        Json::Value* message);
  bool BuildPushMessageByNotification(const string& device,
      const recommendation::StoryDetail& rec_content, Json::Value* message);
  bool ParseRecVec(const Json::Value& rec_results,
                   vector<recommendation::StoryDetail>* rec_vec);
  bool ParseNewsId(const recommendation::StoryDetail& rec_content,
                   string* id);
  bool ChooseRecContent(const Json::Value& rec_results,
                        recommendation::StoryDetail* rec_content);
  bool SaveRecContentToDb(const string& device,
                          const recommendation::StoryDetail& rec_content);
  bool GetDeviceInfo(const string& device,
                     recommendation::DeviceInfo* device_info);

  recommendation::DeviceInfoHelper* device_info_helper_;
  PushSender push_sender_;
  recommendation::RedisReuseClient redis_client_;
  DISALLOW_COPY_AND_ASSIGN(NewsPushProcessor);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_NEWS_NEWS_PUSH_PROCESSOR_H_
