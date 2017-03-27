// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/util/redis_util.h"

#include <stdlib.h>

#include "base/compat.h"
#include "base/file/proto_util.h"
#include "base/time.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"

DECLARE_int32(redis_expire_time);
DECLARE_int32(redis_max_sleep);
DECLARE_string(redis_conf);
DECLARE_string(redis_sub_topic);

namespace {
static const int kTotalFieldCnt = 20;
}

namespace recommendation {

RedisBase::RedisBase() : redis_context_(nullptr) {}

RedisBase::~RedisBase() {}

redisContext* RedisBase::redis_context() {
  return redis_context_;
}

bool RedisBase::Auth(const string& password) {
  string cmd = "AUTH " + password;
  redisReply* reply = static_cast<redisReply*>(
      redisCommand(redis_context_, cmd.c_str()));
  if (!reply) {
    LOG(ERROR) << "socket invalid, send auth command failed";
    return false;
  }
  bool success = false;
  if (reply->type == REDIS_REPLY_STATUS) {
    CHECK_EQ(string(reply->str), "OK") << "error reply str";
    LOG(INFO) << "password auth success";
    success = true;
  } else {
    CHECK_EQ(reply->type, REDIS_REPLY_ERROR) << "error reply type";
    LOG(ERROR) << "auth reply error";
    success = false;
  }
  freeReplyObject(reply);
  return success;
}

bool RedisBase::Expire(const string& key, int expire_time) {
  string cmd = StringPrintf("EXPIRE %s %d",
                            key.c_str(),
                            expire_time);
  redisReply* reply = static_cast<redisReply*>(
      redisCommand(redis_context_, cmd.c_str()));
  if (!reply) {
    LOG(ERROR) << "reply null";
    return false;
  }
  bool success = false;
  CHECK_EQ(reply->type, REDIS_REPLY_INTEGER) << "error reply type";
  if (reply->integer == 1) {
    LOG(INFO) << "expire success, key:" << key;
    success = true;
  } else {
    CHECK_EQ(reply->integer, 0) << "error reply integer";
    LOG(ERROR) << "expire failed, key:" << key;
    success = false;
  }
  freeReplyObject(reply);
  return success;
}

bool RedisBase::Exists(const string& key) {
  string cmd = StringPrintf("EXISTS %s", key.c_str());
  redisReply* reply = static_cast<redisReply*>(
      redisCommand(redis_context_, cmd.c_str()));
  if (!reply) {
    LOG(ERROR) << "reply null";
    return false;
  }
  bool success = false;
  CHECK_EQ(reply->type, REDIS_REPLY_INTEGER) << "error reply type";
  if (reply->integer == 1) {
    LOG(INFO) << "existed, key:" << key;
    success = true;
  } else {
    CHECK_EQ(reply->integer, 0) << "error reply integer";
    LOG(INFO) << "not existed, key:" << key;
    success = false;
  }
  freeReplyObject(reply);
  return success;
}

bool RedisBase::Hmset(const string& key,
                        const map<string, string>& data_map) {
  vector<const char*> argv;
  vector<size_t> argvlen;
  string cmd = "HMSET";
  argv.push_back(cmd.c_str());
  argvlen.push_back(cmd.size());
  argv.push_back(key.c_str());
  argvlen.push_back(key.size());
  for (auto it = data_map.begin(); it != data_map.end(); ++it) {
    argv.push_back(it->first.c_str());
    argvlen.push_back(it->first.size());
    argv.push_back(it->second.c_str());
    argvlen.push_back(it->second.size());
  }
  redisReply* reply = static_cast<redisReply*>(
      redisCommandArgv(redis_context_,
                       argv.size(),
                       &(argv[0]),
                       &(argvlen[0])));
  if (!reply) {
    LOG(ERROR) << "reply null";
    return false;
  }
  bool success = false;
  if (reply->type == REDIS_REPLY_STATUS) {
    CHECK_EQ(string(reply->str), "OK") << "error reply str";
    success = true;
  } else {
    CHECK_EQ(reply->type, REDIS_REPLY_ERROR);
    LOG(ERROR) << "HMSET failed:" << reply->str;
    success = false;
  }
  freeReplyObject(reply);
  return success;
}

bool RedisBase::Hget(const string& key,
                       const string& field,
                       string* value) {
  string cmd = StringPrintf("HGET %s %s",
                            key.c_str(),
                            field.c_str());
  redisReply* reply = static_cast<redisReply*>(
      redisCommand(redis_context_, cmd.c_str()));
  if (!reply) {
    LOG(ERROR) << "reply null";
    return false;
  }
  bool success = false;
  if (reply->type == REDIS_REPLY_NIL) {
    LOG(INFO) << "key or field not existed";
    success = false;
  } else {
    CHECK_EQ(reply->type, REDIS_REPLY_STRING) << "error reply type";
    *value = string(reply->str);
    LOG(INFO) << "hget success, value:" << *value;
    success = true;
  }
  freeReplyObject(reply);
  return success;
}

bool RedisBase::Hgetall(const string& key,
                          map<string, string>* result_map) {
  string cmd = StringPrintf("HGETALL %s", key.c_str());
  redisReply* reply = static_cast<redisReply*>(
      redisCommand(redis_context_, cmd.c_str()));
  if (!reply) {
    LOG(ERROR) << "reply null";
    return false;
  }
  bool success = false;
  CHECK_EQ(reply->type, REDIS_REPLY_ARRAY) << "error reply type";
  if (reply->elements == 0) {
    LOG(ERROR) << "reply empty array";
    success = false;
  } else {
    string k, v;
    for (size_t i = 0; i < reply->elements; ++i) {
      redisReply* sub_reply = reply->element[i];
      if (i % 2 == 0) {
        k = sub_reply->str;
      } else {
        v = sub_reply->str;
      }
      if (i % 2 == 1) {
        result_map->insert(std::make_pair(k, v));
      }
    }
    LOG(INFO) << "reply array size:" << result_map->size();
    if (result_map->size() == kTotalFieldCnt) {
      success = true;
    } else {
      success = false;
    }
  }
  freeReplyObject(reply);
  return success;
}

bool RedisBase::Hexists(const string& key, const string& field) {
  string cmd = StringPrintf("HEXISTS %s %s",
                            key.c_str(),
                            field.c_str());
  redisReply* reply = static_cast<redisReply*>(
      redisCommand(redis_context_, cmd.c_str()));
  if (!reply) {
    LOG(ERROR) << "reply null";
    return false;
  }
  bool success = false;
  CHECK_EQ(reply->type, REDIS_REPLY_INTEGER) << "error reply type";
  if (reply->integer == 1) {
    LOG(INFO) << "hexisted, key:" << key << ", field:" << field;
    success = true;
  } else {
    CHECK_EQ(reply->integer, 0) << "error reply integer";
    LOG(INFO) << "not hexisted, key:" << key << ", field:" << field;
    success = false;
  }
  freeReplyObject(reply);
  return success;
}

bool RedisBase::Subscribe(const std::string& topic) {
  string cmd = "SUBSCRIBE " + topic;
  redisReply* reply = nullptr;
  reply = static_cast<redisReply*>(
      redisCommand(redis_context_, cmd.c_str()));
  if (!reply) {
    LOG(FATAL) << "sub failed, command:" << cmd;
    return false;
  }
  freeReplyObject(reply);
  LOG(INFO) << "sub success, command:" << cmd;
  return true;
}

bool RedisBase::Publish(const std::string& topic,
                               const std::string& message) {
  string cmd = "PUBLISH " + topic + " " + message;
  redisReply* reply = nullptr;
  reply = static_cast<redisReply*>(
      redisCommand(redis_context_, cmd.c_str()));
  if (!reply) {
    LOG(FATAL) << "publish failed, command:" << cmd;
    return false;
  }
  int subscriber_cnt = reply->integer;
  LOG(INFO) << "subscriber count:" << subscriber_cnt;
  freeReplyObject(reply);
  LOG(INFO) << "publish success, command:" << cmd;
  return true;
}

bool RedisBase::Set(const string& key, const string& value,
                    uint32_t expire_seconds) {
  string cmd = StringPrintf("SET %s %s", key.c_str(), value.c_str());
  VLOG(2) << "Command:" << cmd;
  redisReply* reply = nullptr;
  reply = static_cast<redisReply*>(
      redisCommand(redis_context_, cmd.c_str()));
  if (!reply) {
    LOG(FATAL) << "set failed, command:" << cmd;
    return false;
  }
  bool success = false;
  if (reply->type == REDIS_REPLY_NIL) {
    LOG(ERROR) << "Set Return nil";
    success = false;
  } else if (reply->type == REDIS_REPLY_ERROR) {
    LOG(ERROR) << "Redis set Return err, errstr:" << reply->str;
    success = false;
  } else if (reply->type == REDIS_REPLY_STATUS) {
    CHECK_EQ(string(reply->str), "OK") << "Set reply str is not ok";
  } else {
    LOG(ERROR) << "Redis unknown status, Ret:" << reply->type;
    success = false;
  }
  if (expire_seconds > 0) {
    success = Expire(key, expire_seconds);
  } else {
    success = true;
  }
  freeReplyObject(reply);
  return success;
}

bool RedisBase::Get(const string& key, string* value) {
  string cmd = "GET " + key;
  redisReply* reply = nullptr;
  reply = static_cast<redisReply*>(
      redisCommand(redis_context_, cmd.c_str()));
  if (!reply) {
    LOG(FATAL) << "set failed, command:" << cmd;
    return false;
  }
  bool success = false;
  if (reply->type == REDIS_REPLY_NIL) {
    LOG(WARNING) << "Key: " << key << " is not existed";
    success = false;
  } else if (reply->type == REDIS_REPLY_ERROR) {
    LOG(ERROR) << "Value is not string, key:" << key;
    success = false;
  } else {
    CHECK_EQ(reply->type, REDIS_REPLY_STRING)
      << "Get reply type is not string, key:" << key;
    *value = string(reply->str);
    success = true;
  }
  freeReplyObject(reply);
  return success;
}

bool RedisBase::Keys(const string& pattern, vector<string>* keys) {
  string cmd = StringPrintf("KEYS %s", pattern.c_str());
  redisReply* reply = nullptr;
  reply = static_cast<redisReply*>(
      redisCommand(redis_context_, cmd.c_str()));
  if (!reply) {
    LOG(FATAL) << "set failed, command:" << cmd;
    return false;
  }
  bool success = false;
  if (reply->elements == 0) {
    LOG(WARNING) << "reply array is empty";
    success = false;
  } else {
    success = true;
  }
  for (size_t i = 0 ; i < reply->elements; ++i) {
    redisReply* sub_reply = reply->element[i];
    keys->push_back(sub_reply->str);
  }
  VLOG(1) << "keys size:" << keys->size();
  freeReplyObject(reply);
  return success;
}

RedisClient::RedisClient() {
  bool ret = InitConf();
  CHECK_EQ(ret, true);
  LOG(INFO) << "Construct RedisClient";
}

RedisClient::~RedisClient() {}

bool RedisClient::InitConf() {
  redis_conf_.reset(new RedisConf());
  if (!file::ReadProtoFromTextFile(FLAGS_redis_conf,
                                   redis_conf_.get())) {
    LOG(ERROR) << "read redis conf failed";
    return false;
  }
  LOG(INFO) << "redis init config success";
  return true;
}

bool RedisClient::InitConnection() {
  redis_context_ = redisConnect(redis_conf_->host().c_str(),
                                redis_conf_->port());
  if (!redis_context_ || redis_context_->err) {
    LOG(ERROR) << "redis connect failed";
    return false;
  }
  if (redis_conf_->is_use_password()) {
    if (!Auth(redis_conf_->password())) {
      LOG(ERROR) << "auth failed";
    }
  }
  LOG(INFO) << "Redis init success";
  return true;
}

void RedisClient::FreeConnection() {
  redisFree(redis_context_);
}

RedisReuseClient::RedisReuseClient() {
  CHECK(InitConf()) << "Init conf failed";
  CHECK(Init()) << "Init failed";
  LOG(INFO) << "Construct RedisReuseClient";
}

RedisReuseClient::~RedisReuseClient() {}

bool RedisReuseClient::ReConnect() {
  // try to connect with exponential backoff
  for (int sec = 1; sec <= FLAGS_redis_max_sleep; sec = sec << 1) {
    if (Connect()) {
      LOG(INFO) << "ReConnect success";
      return true;
    }
    if (sec <= FLAGS_redis_max_sleep / 2) {
      Sleep(sec);
    }
  }
  LOG(FATAL) << "ReConnect fatal failed";
  return false;
}

bool RedisReuseClient::Init() {
  return ReConnect();
}

bool RedisReuseClient::InitConf() {
  redis_conf_.reset(new RedisConf());
  if (!file::ReadProtoFromTextFile(FLAGS_redis_conf, redis_conf_.get())) {
    LOG(ERROR) << "read redis conf failed";
    return false;
  }
  LOG(INFO) << "redis init config success";
  return true;
}

bool RedisReuseClient::Connect() {
  Cleanup();
  redis_context_ = redisConnect(redis_conf_->host().c_str(),
                                redis_conf_->port());
  if (!redis_context_ || redis_context_->err) {
    LOG(ERROR) << "redis connect socket failed";
    return false;
  }
  if (redis_conf_->is_use_password()) {
    if (!Auth(redis_conf_->password())) {
      LOG(ERROR) << "password auth failed";
      return false;
    }
  }
  LOG(INFO) << "Connect success";
  return true;
}

void RedisReuseClient::Cleanup() {
  if (redis_context_) {
    redisFree(redis_context_);
    redis_context_ = nullptr;
    LOG(INFO) << "redis clean old socket";
  }
}

RedisSubThread::RedisSubThread(
    std::shared_ptr<ConcurrentQueue<string>> message_queue,
    const std::string& topic)
  : Thread(true), shut_down_(false) {
  message_queue_ = message_queue;
  redis_client_.reset(new RedisReuseClient());
  // using constructor input topic as sub topic
  topic_ = topic;
  LOG(INFO) << "Construct RedisSubThread";
}

RedisSubThread::~RedisSubThread() {
  this->ShutDown();
  this->Join();
}

void RedisSubThread::Run() {
  redisContext* redis_context = redis_client_->redis_context();
  if (!redis_context || redis_context->err) {
    LOG(FATAL) << "Init subscribe socket fatal failed";
    abort();
  }
  if (!redis_client_->Subscribe(topic_)) {
    LOG(FATAL) << "Init subscribe command fatal failed";
    abort();
  }
  while (true) {
    if (shut_down_) {
      LOG(INFO) << "shut down redis sub thread, id:" << this->GetThreadId();
      break;
    }
    if (!redis_context->err) {
      void* reply = nullptr;
      if (redisGetReply(redis_context, &reply) == REDIS_OK) {
        LOG(INFO) << "Sub a new message";
        ProcessMessage(reply);
        freeReplyObject(reply);
      }
    } else {
      if (!redis_client_->ReConnect() || !redis_client_->Subscribe(topic_)) {
        LOG(FATAL) << "Resubscribe fatal failed";
        abort();
      } else {
        LOG(INFO) << "Resubscribe success";
      }
    }
  }
}

bool RedisSubThread::ProcessMessage(void* reply) {
  LOG(INFO) << "start to process new message";
  redisReply* redis_reply = static_cast<redisReply*>(reply);
  if (!redis_reply) {
    LOG(ERROR) << "reply invalid, socket maybe not ok";
    return false;
  }
  if (redis_reply->type == REDIS_REPLY_ARRAY) {
    CHECK_EQ(3, redis_reply->elements);
    CHECK_EQ(string(redis_reply->element[0]->str), "message");
    CHECK_EQ(string(redis_reply->element[1]->str), topic_);
    string message = redis_reply->element[2]->str;
    message_queue_->Push(message);
    LOG(INFO) << "Push message:\n" << message;
    return true;
  }
  LOG(ERROR) << "reply type error, some other error occurred";
  return false;
}

void RedisSubThread::ShutDown() {
  shut_down_ = true;
}

}  // namespace recommendation
