// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/file/proto_util.h"
#include "base/log.h"
#include "base/string_util.h"
#include "third_party/jsoncpp/json.h"
#include "third_party/zookeeper/zookeeper.h"
#include "util/kafka/kafka_util.h"

DECLARE_int32(kafka_producer_flush_timeout);
DECLARE_string(kafka_producer_config);
DECLARE_string(kafka_consumer_config);

namespace {

static const char kBrokerList[] = "metadata.broker.list";
static const char kDefaultTopicConf[] = "default_topic_conf";
static const char kBrokerPath[] = "/brokers/ids";

struct ZookeeperDeleter {
  void operator()(zhandle_t* zh) const {
    zookeeper_close(zh);
  }
};

static bool GetZookeeperBrokerList(const string& zk_servers, string* brokers) {
  brokers->clear();
  unique_ptr<zhandle_t, ZookeeperDeleter> zhandle(
      zookeeper_init(zk_servers.c_str(), NULL, 3000, NULL, NULL, 0));
  if (!zhandle.get()) {
    LOG(ERROR) << "zookeeper init failed";
    return false;
  }
  struct String_vector node_vec;
  if (zoo_get_children(zhandle.get(), kBrokerPath, 1, &node_vec) != ZOK) {
    LOG(ERROR) << "zoo get children (" << kBrokerPath << ") failed";
    return false;
  }
  for (int i = 0; i < node_vec.count; ++i) {
    string new_path = StringPrintf("/brokers/ids/%s", node_vec.data[i]);
    struct Stat file_stat;
    int buff_size = 0;
    char buff[1];
    int ret = 0;
    ret = zoo_get(
        zhandle.get(), new_path.c_str(), 0, buff, &buff_size, &file_stat);
    if (ret != ZOK) {
      LOG(ERROR) << "zoo get path (" << new_path << ") stat failed";
      return false;
    }
    string data;
    data.resize(file_stat.dataLength);
    buff_size = file_stat.dataLength;
    ret = zoo_get(zhandle.get(), new_path.c_str(), 0, 
                  const_cast<char*>(data.data()), &buff_size, &file_stat);
    if (ret != ZOK) {
      LOG(ERROR) << "zoo get path (" << new_path << ") failed";
      return false;
    }
    if (buff_size > 0) {
      Json::Value value;
      Json::Reader reader;
      if (!reader.parse(data, value)) {
        LOG(ERROR) << "Error zk data (" << data << ")";
        return false;
      }
      LOG(INFO) << "path:" << new_path << ", data:" << data 
                << ", value:" << value.toStyledString();
      string host;
      int port;
      if (value.isMember("host")) {
        host = value["host"].asString();
      } else {
        return false;
      }
      if (value.isMember("port")) {
        port = value["port"].asInt();
      } else {
        return false;
      }
      if (brokers->empty()) {
        *brokers = StringPrintf("%s:%d", host.c_str(), port);
      } else {
        *brokers += "," + StringPrintf("%s:%d", host.c_str(), port);
      }
    }
  }
  return !brokers->empty();
}

}

namespace recommendation {

KafkaConfig::KafkaConfig(const std::string& config_file) 
  : use_zookeeper(false) {
  conf_meta_.reset(new KafkaConfMeta);
  CHECK(conf_meta_.get());
  conf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  CHECK(conf_.get());
  topic_conf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));
  CHECK(topic_conf_.get());
  InitConfigFromFile(config_file);
}

KafkaConfig::~KafkaConfig() {}

void KafkaConfig::InitConfigFromFile(const std::string& config_file) {
  InitConfigMeta(config_file);
  CHECK(InitRdkafkaConfig()) << "Fail to init rdkakfa config";
}

void KafkaConfig::InitConfigMeta(const std::string& config_file) {
  config_file_ = config_file;
  LOG(INFO) << "config_file:" << config_file;
  CHECK(file::ReadProtoFromTextFile(config_file_, conf_meta_.get()))
    << "Read proto conf failed, file:" << config_file_;
  LOG(INFO) << "kafka_conf_meta:\n" << conf_meta_->Utf8DebugString();
  if (conf_meta_->has_zookeeper_servers() 
      && conf_meta_->zookeeper_servers() != "") {
    use_zookeeper = true;
    CHECK(GetZookeeperBrokerList(conf_meta_->zookeeper_servers(), 
                                 &zk_broker_list_)) 
      << "Get broker list from zk failed";
  } else {
    use_zookeeper = false;
  }
}

bool KafkaConfig::InitRdkafkaConfig() {
  // config global list
  auto& global_item_list = conf_meta_->global_item_list();
  for (auto& item : global_item_list) {
    std::string err_string;
    if (kBrokerList == item.name()) {
      if (use_zookeeper) {
        if (conf_->set(string(kBrokerList), zk_broker_list_, err_string) 
            != RdKafka::Conf::CONF_OK) {
          LOG(FATAL) << "rdkafka conf (" << item.name() 
                     << ") failed, errstr:" << err_string;
          return false;
        }
        continue;
      }
    } 
    if (conf_->set(item.name(), item.value(), err_string) 
        != RdKafka::Conf::CONF_OK) {
      LOG(FATAL) << "rdkafka conf (" << item.name() 
                 << ") failed, errstr:" << err_string;
      return false;
    }
  }
  // config global callback list
  auto& global_callback_list = conf_meta_->global_callback_list();
  for (auto& callback : global_callback_list) {
    if (callback.is_set_callback()) {
      LOG(WARNING) << "Unsupported set global callback";
    }
  }
  // config topic list
  bool use_default_topic_conf = conf_meta_->use_default_topic_conf();
  if (use_default_topic_conf) {
    std::string err_string;
    if (conf_->set(kDefaultTopicConf, topic_conf_.get(), err_string) 
        != RdKafka::Conf::CONF_OK) {
      LOG(FATAL) << "rdkafka conf (" << kDefaultTopicConf
                 << ") failed, errstr:" << err_string;
      return false;
    }
    // config topic callback list
    auto& topic_callback_list = conf_meta_->topic_callback_list();
    for (auto& callback : topic_callback_list) {
      if (callback.is_set_callback()) {
        LOG(WARNING) << "Unsupported set topic callback";
      }
    }
  } else {
    LOG(FATAL) << "Unsupported use non-default topic conf";
    return false;
  }
  DumpConfig();
  return true;
}
  
void KafkaConfig::DumpConfig()  {
  for (int pass = 0 ; pass < 2 ; pass++) {
    std::list<std::string> *dump;
    if (pass == 0) {
      dump = conf_->dump();
      LOG(INFO) << "# Global config";
    } else {
      dump = topic_conf_->dump();
      LOG(INFO) << "# Topic config";
    }
    std::stringstream ss;
    for (std::list<std::string>::iterator it = dump->begin();
        it != dump->end(); ) {
      ss << *it << " = ";
      it++;
      ss << *it << "\n";
      it++;
    }
    LOG(INFO) << ss.str();
  }
}


KafkaConsumer::KafkaConsumer() 
  : KafkaConfig(FLAGS_kafka_consumer_config), shut_down_(false) {}

KafkaConsumer::~KafkaConsumer() {
  consumer_->close();
}

void KafkaConsumer::set_message_queue(
    std::shared_ptr<mobvoi::ConcurrentQueue<string>> message_queue) {
  message_queue_ = message_queue;
}

void KafkaConsumer::Run() {
  CHECK(message_queue_.get()) << "message_queue is null";
  string errstr;
  consumer_.reset(RdKafka::KafkaConsumer::create(conf_.get(), errstr));
  CHECK(consumer_.get()) << "errstr:" << errstr;
  LOG(INFO) << "Create consumer " << consumer_->name() << " success";
  vector<string> topics;
  topics.push_back(conf_meta_->topic());
  RdKafka::ErrorCode err = consumer_->subscribe(topics);
  if (err) {
    LOG(FATAL) << "Failed to subscribe topic:" << topics[0] 
               << " err: " << RdKafka::err2str(err);
  }
  LOG(INFO) << "Subcribe topic " << topics[0] << " success";
  while (true) {
    if (shut_down_) {
      LOG(INFO) << "shut down kafka consumer..";
      break;
    }
    RdKafka::Message* message = consumer_->consume(1000);
    Push(message);
    delete message;
  }
}

void KafkaConsumer::Push(RdKafka::Message* message) {
  int len = static_cast<int>(message->len());
  const char* payload = static_cast<const char*>(message->payload());
  if ((len > 0) && (payload != NULL)) {
    VLOG(1) << "Message received, payload len:" << len << ", payload:" 
            << payload;
    message_queue_->Push(string(payload));
  } 
}

void KafkaConsumer::ShutDown() {
  shut_down_ = true;
}

KafkaConsumerThread::KafkaConsumerThread() : Thread(true) {}

void KafkaConsumerThread::BuildConsumer(
    std::shared_ptr<mobvoi::ConcurrentQueue<string>> message_queue) {
  consumer_.reset(new KafkaConsumer());
  CHECK(consumer_.get()) << "Build consumer failed";
  consumer_->set_message_queue(message_queue);
  LOG(INFO) << "New consumer and set message queue success";
}

KafkaConsumerThread::~KafkaConsumerThread() {
  this->ShutDown();
  this->Join();
}

void KafkaConsumerThread::Run() {
  CHECK(consumer_.get());
  consumer_->Run();
}

void KafkaConsumerThread::ShutDown() {
  consumer_->ShutDown();
  LOG(INFO) << "shut down kafka consumer thread, id:" << this->GetThreadId();
}

KafkaProducer::KafkaProducer() : KafkaConfig(FLAGS_kafka_producer_config) {}

KafkaProducer::~KafkaProducer() {}

bool KafkaProducer::Produce(const vector<string>& messages, int* success_cnt) {
  std::string errstr;
  std::unique_ptr<RdKafka::Producer> producer;
  std::unique_ptr<RdKafka::Topic> topic;
  producer.reset(RdKafka::Producer::create(conf_.get(), errstr));
  if (!producer.get()) {
    LOG(ERROR) << "Create producer failed: " << errstr;
    return false;
  }
  topic.reset(RdKafka::Topic::create(producer.get(), 
                                     conf_meta_->topic(), 
                                     topic_conf_.get(),
                                     errstr)); 
  if (!topic.get()) {
    LOG(ERROR) << "Create producer topic failed: " << errstr;
    return false;
  }
  int cnt = 0;
  int32_t partition = RdKafka::Topic::PARTITION_UA;
  for (auto& message : messages) {
    RdKafka::ErrorCode resp = 
      producer->produce(topic.get(), 
                        partition,
                        RdKafka::Producer::RK_MSG_COPY,
                        const_cast<char *>(message.c_str()), 
                        message.size(),
                        NULL, 
                        NULL);
    if (resp != RdKafka::ERR_NO_ERROR) {
      LOG(ERROR) << "Produce failed: " << RdKafka::err2str(resp);
    } else {
      ++cnt;
      VLOG(1) << "Produced message (" << message.size() << " bytes)";
      producer->poll(0);
    }
  }
  producer->poll(0);
  RdKafka::ErrorCode resp = producer->flush(FLAGS_kafka_producer_flush_timeout);
  if (RdKafka::ERR_NO_ERROR != resp) {
    LOG(WARNING) << "producer flush timeout";
  }  
  *success_cnt = cnt;
  return true;
}

KafkaConsumerThreadPool::KafkaConsumerThreadPool(int pool_size) 
  : pool_size_(pool_size), thread_pool_(nullptr) {
  thread_pool_.reset(new KafkaConsumerThread[pool_size_]);
}

KafkaConsumerThreadPool::~KafkaConsumerThreadPool() {
  for (int i = 0; i < pool_size_; ++i) {
    thread_pool_[i].ShutDown();
    thread_pool_[i].Join();
  }
}
  
void KafkaConsumerThreadPool::SetSharedConcurrentQueue(
      std::shared_ptr<mobvoi::ConcurrentQueue<string>> message_queue) {
  for (int i = 0; i < pool_size_; ++i) {
    thread_pool_[i].BuildConsumer(message_queue);
  }
}

void KafkaConsumerThreadPool::Start() {
  for (int i = 0; i < pool_size_; ++i) {
    thread_pool_[i].Start(); 
  }
}

}
