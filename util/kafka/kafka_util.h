// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef UTIL_KAFKA_KAFKA_UTIL_H_
#define UTIL_KAFKA_KAFKA_UTIL_H_ 

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/thread.h"
#include "base/concurrent_queue.h"
#include "third_party/librdkafka/src-cpp/rdkafkacpp.h"
#include "util/kafka/proto/kafka_meta.pb.h"

namespace recommendation {

class KafkaConfig {
 public:
  explicit KafkaConfig(const std::string& config_file);
  ~KafkaConfig();
  void InitConfigFromFile(const std::string& config_file);
  void InitConfigMeta(const std::string& config_file);
  bool InitRdkafkaConfig();
  void DumpConfig();

 protected:
  std::unique_ptr<KafkaConfMeta> conf_meta_; 
  std::unique_ptr<RdKafka::Conf> conf_;
  std::unique_ptr<RdKafka::Conf> topic_conf_;
  std::string config_file_;
  std::string zk_broker_list_;
  bool use_zookeeper;
  DISALLOW_COPY_AND_ASSIGN(KafkaConfig);
};

class KafkaConsumer : public KafkaConfig {
 public:
  KafkaConsumer();
  void set_message_queue(
      std::shared_ptr<mobvoi::ConcurrentQueue<string>> message_queue);
  ~KafkaConsumer();
  void Push(RdKafka::Message* message);
  void Run();
  void ShutDown();

 private:
  bool shut_down_;
  std::unique_ptr<RdKafka::KafkaConsumer> consumer_;
  std::shared_ptr<mobvoi::ConcurrentQueue<string>> message_queue_;
  DISALLOW_COPY_AND_ASSIGN(KafkaConsumer); 
};

class KafkaConsumerThread : public mobvoi::Thread {
 public:
  KafkaConsumerThread();
  void BuildConsumer(
      std::shared_ptr<mobvoi::ConcurrentQueue<string>> message_queue);
  virtual ~KafkaConsumerThread();
  virtual void Run();
  void ShutDown();

 private:
  std::unique_ptr<KafkaConsumer> consumer_;
  DISALLOW_COPY_AND_ASSIGN(KafkaConsumerThread);
};

class KafkaProducer : public KafkaConfig {
 public:
  KafkaProducer();
  ~KafkaProducer();
  bool Produce(const vector<string>& messages, int* success_cnt);

 private:
  DISALLOW_COPY_AND_ASSIGN(KafkaProducer); 
};

class KafkaConsumerThreadPool {
 public:
  KafkaConsumerThreadPool(int pool_size);
  ~KafkaConsumerThreadPool();
  void SetSharedConcurrentQueue(
      std::shared_ptr<mobvoi::ConcurrentQueue<string>> message_queue);
  void Start(); 

 private:
  int pool_size_;
  std::unique_ptr<KafkaConsumerThread[]> thread_pool_;
  DISALLOW_COPY_AND_ASSIGN(KafkaConsumerThreadPool);
};

} // namespace recommendation

#endif // UTIL_KAFKA_KAFKA_UTIL_H_ 
