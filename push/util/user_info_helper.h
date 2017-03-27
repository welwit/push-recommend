// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_UTIL_USER_INFO_HELPER_H_
#define PUSH_UTIL_USER_INFO_HELPER_H_

#include <mutex>

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/mutex.h"
#include "base/singleton.h"
#include "third_party/jsoncpp/json.h"

#include "push/proto/user_device_meta.pb.h"

namespace recommendation {

class DeviceInfoHelper {
 public:
  ~DeviceInfoHelper();
  bool GetDeviceCoordinate(const string& device_id,
                           string* latitude, string* longitude);
  bool GetDeviceInfo(const string& device, DeviceInfo* device_info);
  bool GetDeviceByUser(const string& user_id, string* device_id);
  void GetClusterDeviceId(uint32 node_pos, uint32 node_cnt,
                          vector<string>* devices);
  void GetClusterUserDeviceVec(uint32 node_pos, uint32 node_cnt,
      vector<std::pair<string, string>>* user_device_vec);
  void GetAllDeviceId(vector<string>* devices);
  void GetAllUserDevicePair(vector<std::pair<string, string>>* user_device_vec);
  void QueryAllDevice(Json::Value* result);
  void QueryAllUser(Json::Value* result);

 private:
  friend struct DefaultSingletonTraits<DeviceInfoHelper>;
  DeviceInfoHelper();
  void QueryAllInternal(const char* query_sql_format,
                        int32 page_size, Json::Value* result);

  DISALLOW_COPY_AND_ASSIGN(DeviceInfoHelper);
};

}  // namespace recommendation

#endif  // PUSH_UTIL_USER_INFO_HELPER_H_
