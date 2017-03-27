// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_UTIL_REDIS_UTIL_H_
#define PUSH_UTIL_REDIS_UTIL_H_

#include <mutex>

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/concurrent_queue.h"
#include "base/log.h"
#include "base/thread.h"
#include "third_party/redis_client/async.h"
#include "third_party/redis_client/hiredis.h"
#include "push/proto/redis_meta.pb.h"

namespace recommendation {

using namespace mobvoi;

class RedisBase {
 public:
  RedisBase();
  virtual ~RedisBase();
  redisContext* redis_context();
  bool Auth(const string& password);
  bool Expire(const string& key, int expire_time);
  bool Exists(const string& key);
  bool Hmset(const string& key, const map<string, string>& data_map);
  bool Hget(const string& key, const string& field, string* value);
  bool Hgetall(const string& key, map<string, string>* result_map);
  bool Hexists(const string& key, const string& field);
  bool Subscribe(const std::string& topic);
  bool Publish(const std::string& topic, const std::string& message);
  bool Set(const string& key, const string& value, uint32_t expire_seconds);
  bool Get(const string& key, string* value);
  bool Keys(const string& pattern, vector<string>* keys);

  std::unique_ptr<RedisConf> redis_conf_;
  redisContext* redis_context_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RedisBase);
};

class RedisClient : public RedisBase {
 public:
  RedisClient();
  virtual ~RedisClient();
  bool InitConf();
  bool InitConnection();
  void FreeConnection();

 private:
  DISALLOW_COPY_AND_ASSIGN(RedisClient);
};

class RedisReuseClient : public RedisBase {
 public:
  RedisReuseClient();
  virtual ~RedisReuseClient();
  bool ReConnect();

 private:
  bool Init();
  bool InitConf();
  bool Connect();
  void Cleanup();
  DISALLOW_COPY_AND_ASSIGN(RedisReuseClient);
};

class RedisSubThread : public Thread {
 public:
  RedisSubThread(std::shared_ptr<ConcurrentQueue<string>> message_queue,
                 const std::string& topic);
  virtual ~RedisSubThread();
  virtual void Run();
  bool ProcessMessage(void* reply);
  void ShutDown();

 private:
  bool shut_down_;
  std::string topic_;
  std::unique_ptr<RedisReuseClient> redis_client_;
  std::shared_ptr<ConcurrentQueue<string>> message_queue_;
  DISALLOW_COPY_AND_ASSIGN(RedisSubThread);
};

}  // namespace recommendation

#endif  // PUSH_UTIL_REDIS_UTIL_H_
