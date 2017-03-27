// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include <arpa/inet.h>

#include <sstream>

#include "base/file/file.h"
#include "base/log.h"
#include "base/string_util.h"
#include "base/time.h"
#include "third_party/gflags/gflags.h"
#include "util/net/util.h"
#include "push/util/zookeeper_util.h"

DECLARE_int32(zookeeper_timeout);
DECLARE_int32(zookeeper_reconnect_attempt);
DECLARE_int32(zookeeper_check_interval);
DECLARE_int32(zookeeper_default_node_port);
DECLARE_string(zookeeper_cluster_addr);
DECLARE_string(zookeeper_this_node_prefix);
DECLARE_string(zookeeper_watched_path);

namespace {

static const int32 kDataMaxLength = 200;

static void GetBasePaths(const string& path, vector<string>* paths) {
  vector<string> segs;
  SplitString(path, '/', &segs);
  if (segs.size() < 2) {
    return;
  }
  string cur_path = "/";
  for (size_t i = 0; i < segs.size(); ++i) {
    if (segs[i].empty()) {
      LOG(WARNING) << "Skip empty seg.";
      continue;
    }
    cur_path = file::File::JoinPath(cur_path, segs[i]);
    paths->push_back(cur_path);
  }
  for (auto& path : *paths) {
    VLOG(1) << "BasePath:" << path;
  }
}

static string GetBasePath(const string& path) {
  vector<string> paths;
  GetBasePaths(path, &paths);
  if (paths.empty()) {
    return "";
  }
  return paths[paths.size() - 2];
}

static string GetValue(const string& host, int port) {
  return StringPrintf("{\"host\":\"%s\",\"port\":\"%d\"}",
                      host.c_str(), port);
}

}

namespace recommendation {

static string SetToString(std::set<NodeInfo>& node_info_set) {
  std::stringstream ss;
  ss << "[";
  size_t size = node_info_set.size();
  for (auto it = node_info_set.begin(); it != node_info_set.end(); ++it) {
    ss << it->ToString();
    if (--size > 0) {
      ss << ",";
    }
  }
  ss << "]";
  return ss.str();
}

static string MapToString(
    std::map<std::string, std::set<NodeInfo>>& node_set_by_path) {
  std::stringstream ss;
  ss << "{";
  size_t size = node_set_by_path.size();
  for (auto it = node_set_by_path.begin(); it != node_set_by_path.end(); ++it) {
    ss << "\"" << it->first << "\":" << SetToString(it->second);
    if (--size > 0) {
      ss << ",";
    }
  }
  ss << "}";
  return ss.str();
}

static bool ParseValue(const string& data, string* host, int* port) {
  Json::Value root;
  Json::Reader reader;
  if (reader.parse(data, root)) {
    *host = root["host"].asString();
    StringToInt(root["port"].asString(), port);
    return true;
  }
  return false;
}

static void GlobalWatcher(zhandle_t* zh, int type, int state,
                          const char* path, void* context) {
  LOG(INFO) << "GlobalWatcher() type:" << type << ", state:" << state
            << ", path:" << path;
  ZkManager* zk_manager = static_cast<ZkManager*>(context);
  if (type == ZOO_SESSION_EVENT) {
    if (state == ZOO_CONNECTED_STATE) {
      zk_manager->OnConnected();
      zk_manager->ResetIndexData(path);
    }
  } else if (type == ZOO_CREATED_EVENT) {
    zk_manager->OnNodeCreated(path);
  } else if (type == ZOO_DELETED_EVENT) {
    zk_manager->OnNodeDeleted(path);
  } else if (type == ZOO_CHANGED_EVENT) {
    zk_manager->OnNodeChanged(path);
  } else if (type == ZOO_CHILD_EVENT) {
    zk_manager->OnChildrenChanged(path);
  } else if (ZOO_NOTWATCHING_EVENT) {
    zk_manager->OnWatchRemoved(path);
  } else if (state == ZOO_AUTH_FAILED_STATE) {
    zk_manager->OnAuthFailed();
  }
}

class SessionThread : public mobvoi::Thread {
 public:
  explicit SessionThread(ZkManager* zk_manager) : Thread(false) {
    zk_manager_ = zk_manager;
    CHECK(zk_manager_) << "zk_manager pointer is null";
    zookeeper_handle_ = zk_manager_->zookeeper_handle();
    stop_ = false;
  }

  void StopThread() {
    stop_ = true;
  }

  virtual ~SessionThread() {
    StopThread();
  }

  virtual void Run() {
    LOG(INFO) << "SessionThread::Run()";
    uint32 check_cnt = 0;
    static const uint32 kCheckSize = 500;
    while (!stop_) {
      int state = zoo_state(zookeeper_handle_);
      VLOG(1) << "Zookeeper state:" << state;
      if (state == ZOO_EXPIRED_SESSION_STATE) {
        LOG(ERROR) << "Zookeeper session is expired, try to reconnect";
        zk_manager_->OnSessionExpired();
        zookeeper_handle_ = zk_manager_->zookeeper_handle();
      }
      if (check_cnt % kCheckSize == 1) {
        LOG(INFO) << "ZkManager::ToString() " << zk_manager_->ToString();
      } else {
        VLOG(1) << "ZkManager::ToString() " << zk_manager_->ToString();
      }
      ++check_cnt;
      mobvoi::Sleep(FLAGS_zookeeper_check_interval);
    }
  }

 private:
  volatile bool stop_;
  ZkManager* zk_manager_;
  zhandle_t* zookeeper_handle_;
  DISALLOW_COPY_AND_ASSIGN(SessionThread);
};

ZkManager::ZkManager()
  : is_register_this_node_(false), zookeeper_handle_(nullptr)
{}

ZkManager::~ZkManager() {}

void ZkManager::Start() {
  LOG(INFO) << "ZkManager::Start()";
  ResetConnectHandle();
  session_thread_.reset(new SessionThread(this));
  session_thread_->Start();
}

zhandle_t* ZkManager::zookeeper_handle() {
  return zookeeper_handle_;
}

void ZkManager::set_is_register_this_node(bool is_register_this_node) {
  std::lock_guard<std::mutex> lock(data_mtx_);
  is_register_this_node_ = is_register_this_node;
}

void ZkManager::AddWatchPath(const std::string& watch_path) {
  std::lock_guard<std::mutex> lock(data_mtx_);
  // only add to table
  std::set<NodeInfo> node_set;
  watched_table_[watch_path] = node_set;
}

string ZkManager::ToString() {
  string result = StringPrintf(
     "{\"is_register_this_node\":%d,\"this_node\":%s,"
     "\"watched_table\":%s}",
     is_register_this_node_, this_node_.ToString().c_str(),
     MapToString(watched_table_).c_str());
  return result;
}

void ZkManager::OnNodeCreated(const char* path) {
  LOG(INFO) << "Node (" << path << ") created";
}

void ZkManager::OnNodeDeleted(const char* path) {
  LOG(INFO) << "Node (" << path << ") deleted";
}

void ZkManager::OnNodeChanged(const char* path) {
  LOG(INFO) << "Node (" << path << ") changed";
}

void ZkManager::OnAuthFailed() {
  LOG(FATAL) << "Auth failed";
}

void ZkManager::OnWatchRemoved(const char* path) {
  LOG(FATAL) << "Watch removed, path:" << path;
}

void ZkManager::OnChildrenChanged(const char* path) {
  LOG(INFO) << "ZkManager::OnChildrenChanged(), path:" << path;
  AddChildrenWatchPath(string(path));
}

void ZkManager::OnConnected() {
  LOG(INFO) << "ZkManager::OnConnected()";
  struct sockaddr sock_addr;
  socklen_t sock_len = sizeof(sock_addr);
  if (zookeeper_get_connected_host(
        zookeeper_handle_, &sock_addr, &sock_len) == NULL) {
    LOG(FATAL) << "Cannot get connected host";
  }
  if (sock_addr.sa_family == AF_INET) {
    struct sockaddr_in* sock_addr_in = (struct sockaddr_in *) &sock_addr;
    char host[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sock_addr_in->sin_addr), host, sizeof(host));
    LOG(INFO) << "Connected zookeeper server, host:"
              << string(host) << ", port:"
              << ntohs(sock_addr_in->sin_port);
  }
  LOG(INFO) << "Zookeeper session is connected";
  connect_latch_->CountDown();
}

void ZkManager::OnSessionExpired() {
  LOG(INFO) << "ZkManager::OnSessionExpired()";
  zookeeper_close(zookeeper_handle_);
  ResetConnectHandle();
}

void ZkManager::ResetIndexData(const char* path) {
  std::lock_guard<std::mutex> lock(data_mtx_);
  LOG(INFO) << "ResetIndexData()";
  if (is_register_this_node_) {
    RegisterSelf();
  }
  if (!watched_table_.empty()) {
    std::vector<std::string> paths;
    for (auto it = watched_table_.begin(); it != watched_table_.end(); ++it) {
      paths.push_back(it->first);
    }
    watched_table_.clear();
    for (auto& path : paths) {
      AddChildrenWatchPath(path);
    }
  }
}

int ZkManager::GetNodePos(const std::string& watched_path) {
  std::lock_guard<std::mutex> lock(data_mtx_);
  auto it = watched_table_.find(watched_path);
  if (it == watched_table_.end()) {
    return -1;
  }
  auto node_set = it->second;
  for (auto set_it = node_set.begin(); set_it != node_set.end(); ++set_it) {
    if (!set_it->host_port_json.empty() &&
        set_it->host_port_json == this_node_.host_port_json &&
        set_it->node_pos_in_zk >= 0) {
      return set_it->node_pos_in_zk;
    }
  }
  return -1;
}

int ZkManager::GetTotalNodeCnt(const std::string& watched_path) {
  std::lock_guard<std::mutex> lock(data_mtx_);
  auto it = watched_table_.find(watched_path);
  if (it == watched_table_.end()) {
    return -1;
  }
  return it->second.size();
}

void ZkManager::ResetConnectHandle() {
  LOG(INFO) << "ZkManager::ResetConnectHandle()";
  connect_latch_.reset(new mobvoi::CountDownLatch(1));
  zookeeper_handle_ = zookeeper_init(FLAGS_zookeeper_cluster_addr.c_str(),
                                     &GlobalWatcher,
                                     FLAGS_zookeeper_timeout,
                                     NULL,
                                     this,
                                     0);
  CHECK(zookeeper_handle_) << "Fail to init zookeeper";
  LOG(INFO) << "Reset zookeeper handle ok, will wait for session established";
  connect_latch_->Wait();
}

void ZkManager::AutoMakingThisNode(int port = -1) {
  NodeInfo node_info;
  node_info.node_path = FLAGS_zookeeper_this_node_prefix;
  node_info.base_path = GetBasePath(node_info.node_path);
  node_info.node_pos_in_zk = -1;
  node_info.port = port;
  if (port < 1024 || port > 49151) {
    node_info.port = FLAGS_zookeeper_default_node_port;
  }
  node_info.host = util::GetLocalHostName();
  if (node_info.host.empty()) {
    LOG(FATAL) << "Can not get local host name";
  }
  node_info.host_port_json = GetValue(node_info.host, node_info.port);
  node_info.is_init = true;
  this_node_ = node_info;
  LOG(INFO) << "ZkManager::AutoMakingThisNode success, this_node:"
            << this_node_.ToString();
}

void ZkManager::set_this_node(const NodeInfo& node_info) {
  this_node_ = node_info;
}

bool ZkManager::RegisterSelf() {
  LOG(INFO) << "ZkManager::RegisterSelf";
  if (!is_register_this_node_) {
    return false;
  }
  if (!this_node_.is_init) {
    AutoMakingThisNode(-1);
  }
  RegisterNode(nullptr);
  return true;
}

bool ZkManager::RegisterNode(const NodeInfo* node_info) {
  LOG(INFO) << "ZkManager::RegisterNode()";
  NodeInfo new_node;
  if (node_info != nullptr) {
    new_node = *node_info;
  } else {
    new_node = this_node_;
  }
  if (new_node.node_path.empty() || new_node.host.empty()) {
    return false;
  }
  vector<string> paths;
  GetBasePaths(new_node.node_path, &paths);
  if (paths.size() < 2) {
    return false;
  }
  // create root paths
  for (size_t i = 0; i < paths.size() - 1; ++i) {
    int ret = zoo_create(zookeeper_handle_,
                         paths[i].c_str(), NULL, -1,
                         &ZOO_OPEN_ACL_UNSAFE, 0, 0, 0);
    VLOG(1) << "path:" << paths[i];
    if (ret != ZOK && ret != ZNODEEXISTS) {
      LOG(ERROR) << "Fail to register node, error:" << zerror(ret);
      return false;
    }
  }
  // create current path
  if (new_node.host_port_json.empty()) {
    new_node.host_port_json = GetValue(new_node.host, new_node.port);
  }
  // check if need to rewatch
  if (!NeedToReRegister(FLAGS_zookeeper_watched_path,
                        new_node.host_port_json)) {
    LOG(INFO) << "NODE is in zk cluster, need not to reRegister, host_port_json:"
              << new_node.host_port_json;
    return true;
  }
  int flag = ZOO_EPHEMERAL | ZOO_SEQUENCE;
  static const int kBuffSize = 1024;
  char path[kBuffSize];
  int ret = zoo_create(zookeeper_handle_,
                       FLAGS_zookeeper_this_node_prefix.c_str(),
                       new_node.host_port_json.data(),
                       new_node.host_port_json.size(),
                       &ZOO_OPEN_ACL_UNSAFE,
                       flag, path, kBuffSize);
  if (ret != ZOK) {
    LOG(ERROR) << "Fail to register path: "
               << FLAGS_zookeeper_this_node_prefix
               << ", error: " << zerror(ret);
    return false;
  }
  LOG(INFO) << "path: " << FLAGS_zookeeper_this_node_prefix
            << ", ZOO_EPHEMERAL | ZOO_SEQUENCE, result path: " << path;
  new_node.node_path = path;
  new_node.base_path = paths[paths.size() - 2];
  this_node_ = new_node;
  return true;
}

bool ZkManager::GetData(const std::string& path, string* data) {
  LOG(INFO) << "ZkManager::GetData() path:" << path;
  data->clear();
  char buffer[kDataMaxLength];
  int buffer_len = kDataMaxLength - 1;
  int ret = zoo_get(zookeeper_handle_,
                    path.c_str(), ZOO_CHANGED_EVENT,
                    buffer, &buffer_len, NULL);
  if (ret == ZOK && buffer_len > 0) {
    DCHECK_LT(buffer_len, kDataMaxLength);
    data->assign(buffer, buffer_len);
    return true;
  }
  return false;
}

bool ZkManager::WatchNodeChildren(const std::string& node_path,
                                  std::set<NodeInfo>* node_set) {
  LOG(INFO) << "ZkManager::WatchNodeChildren(), node_path:" << node_path;
  struct String_vector string_vector;
  int ret = zoo_get_children(zookeeper_handle_, node_path.c_str(),
                             ZOO_CHILD_EVENT,
                             &string_vector);
  if (ret == ZOK) {
    for (int32 i = 0; i < string_vector.count; ++i) {
      string host_port_json;
      string leaf = node_path + "/" + string(string_vector.data[i]);
      if (!GetData(leaf, &host_port_json)) {
        continue;
      }
      VLOG(1) << "leaf: " << leaf << ", host_port_json: " << host_port_json;
      string host;
      int port;
      if (!ParseValue(host_port_json, &host, &port)) {
        continue;
      }
      NodeInfo node_info;
      node_info.node_path = leaf;
      node_info.host_port_json = host_port_json;
      node_info.host = host;
      node_info.port = port;
      node_info.node_pos_in_zk = i;
      node_info.base_path = GetBasePath(leaf);
      node_info.is_init = true;
      LOG(INFO) << "node_info:" << node_info.ToString();
      node_set->insert(node_info);
    }
    deallocate_String_vector(&string_vector);
    return true;
  } else if (ret == ZNONODE) {
    LOG(ERROR) << "Fail to watch node, invalid path:" << node_path;
    return false;
  }
  return false;
}

void ZkManager::AddChildrenWatchPath(const std::string& watch_path) {
  LOG(INFO) << "ZkManager::AddChildrenWatchPath() watch_path:" << watch_path;
  std::set<NodeInfo> node_set;
  WatchNodeChildren(watch_path, &node_set);
  watched_table_[watch_path] = node_set;
}

bool ZkManager::NeedToReRegister(const std::string& node_path,
                                 const string& data) {
  std::set<NodeInfo> node_set;
  if (!WatchNodeChildren(node_path, &node_set)) {
    return true;
  }
  for (auto it = node_set.begin(); it != node_set.end(); ++it) {
    if (it->host_port_json == data) {
      return false;
    }
  }
  return true;
}

}  // namespace recommendation
