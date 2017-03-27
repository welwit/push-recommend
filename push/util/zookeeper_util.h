// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_UTIL_ZOOKEEPER_UTIL_H_
#define PUSH_UTIL_ZOOKEEPER_UTIL_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/count_down_latch.h"
#include "base/mutex.h"
#include "base/singleton.h"
#include "base/string_util.h"
#include "base/thread.h"
#include "third_party/jsoncpp/json.h"
#include "third_party/zookeeper/zookeeper.h"

namespace recommendation {

struct NodeInfo {
  bool is_init;
  int port;
  int node_pos_in_zk;
  string base_path;
  string node_path;
  string host_port_json;
  string host;
  NodeInfo() {
    is_init = false;
    port = -1;
    node_pos_in_zk = -1;
  }
  bool operator < (const NodeInfo& other) const {
    int path_cmp = node_path.compare(other.node_path);
    return path_cmp < 0;
  }
  string ToString() const {
    string result = StringPrintf(
        "{\"is_init\":\"%d\",\"base_path\":\"%s\","
        "\"node_path\":\"%s\","
        "\"host_port_json\":%s,\"node_pos_in_zk\":\"%d\","
        "\"host\":\"%s\",\"port\":\"%d\"}",
        is_init, base_path.c_str(), node_path.c_str(),
        host_port_json.c_str(), node_pos_in_zk, host.c_str(),
        port);
    return result;
  }
};

class ZkManager {
 public:
  ~ZkManager();
  void Start();
  zhandle_t* zookeeper_handle();
  void set_is_register_this_node(bool is_register_this_node);
  void AddWatchPath(const std::string& watch_path);
  string ToString();
  void OnNodeCreated(const char* path);
  void OnNodeDeleted(const char* path);
  void OnNodeChanged(const char* path);
  void OnAuthFailed();
  void OnWatchRemoved(const char* path);
  void OnChildrenChanged(const char* path);
  void OnConnected();
  void OnSessionExpired();
  void ResetIndexData(const char* path);
  int GetNodePos(const std::string& watched_path);
  int GetTotalNodeCnt(const std::string& watched_path);

 private:
  friend struct DefaultSingletonTraits<ZkManager>;
  ZkManager();
  void ResetConnectHandle();
  void AutoMakingThisNode(int port);
  void set_this_node(const NodeInfo& node_info);
  bool RegisterSelf();
  bool RegisterNode(const NodeInfo* node_info);
  bool GetData(const std::string& path, string* data);
  bool WatchNodeChildren(const std::string& node_path,
                         std::set<NodeInfo>* node_set);
  void AddChildrenWatchPath(const std::string& watch_path);
  bool NeedToReRegister(const std::string& node_path, const string& data);

  std::mutex data_mtx_;
  bool is_register_this_node_;
  NodeInfo this_node_;
  std::map<std::string, set<NodeInfo>> watched_table_;
  std::unique_ptr<mobvoi::Thread> session_thread_;
  std::unique_ptr<mobvoi::CountDownLatch> connect_latch_;
  zhandle_t* zookeeper_handle_;
  DISALLOW_COPY_AND_ASSIGN(ZkManager);
};

}  // namespace recommendation

#endif // PUSH_UTIL_ZOOKEEPER_UTIL_H_
