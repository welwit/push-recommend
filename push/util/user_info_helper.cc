// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/util/user_info_helper.h"

#include "base/hash.h"
#include "base/log.h"
#include "base/string_util.h"
#include "util/mysql/mysql_util.h"

#include "push/util/common_util.h"

DECLARE_int32(mysql_page_size);
DECLARE_string(mysql_config);

namespace {

static const char kQueryAllDeviceSQL[] =
    "SELECT id, device_type, bluetooth_match_id, updated, wear_model, "
    "wear_version, wear_version_channel, wear_os, phone_model, "
    "phone_version, phone_os, latitude, longitude FROM device "
    "WHERE device_type = 'watch' limit %d, %d;";

static const char kQueryAllUserSQL[] = 
    "SELECT name, device_id FROM nickname WHERE "
    "package_name = 'com.mobvoi.ticwear.home' limit %d, %d;";

static const char kQueryDeviceSQL[] =
    "SELECT id, device_type, bluetooth_match_id, updated, wear_model, "
    "wear_version, wear_version_channel, wear_os, phone_model, "
    "phone_version, phone_os, latitude, longitude FROM device "
    "WHERE device_type = 'watch' AND id = '%s';";

static const char kQueryDeviceByUserSQL[] =
    "SELECT device_id FROM nickname WHERE "
    "package_name = 'com.mobvoi.ticwear.home' AND name = '%s';";
}

namespace recommendation {

DeviceInfoHelper::DeviceInfoHelper() {}

DeviceInfoHelper::~DeviceInfoHelper() {}

bool DeviceInfoHelper::GetDeviceCoordinate(const string& device,
    string* latitude, string* longitude) {
  DeviceInfo device_info;
  if (!GetDeviceInfo(device, &device_info)) {
    return false;
  }
  *latitude = DoubleToString(device_info.latitude());
  *longitude = DoubleToString(device_info.longitude());
  return true;
}

bool DeviceInfoHelper::GetDeviceInfo(const string& device_id, 
                                     DeviceInfo* device_info) {
  string command = StringPrintf(kQueryDeviceSQL, device_id.c_str());
  VLOG(2) << "Command:" << command;
  Json::Value data;
  bool ret = util::GetMysqlResult(FLAGS_mysql_config, command, &data, false);
  if (!ret) {
    LOG(ERROR) << "Get result failed, device_id:" << device_id;
    return false;
  }
  VLOG(2) << "Data:" << push_controller::JsonToString(data);
  const Json::Value& d = data;
  const string& id = d["id"].asString();
  device_info->set_id(id);
  device_info->set_device_type(d["device_type"].asString());
  device_info->set_bluetooth_match_id(d["bluetooth_match_id"].asString());
  device_info->set_updated(d["updated"].asString());
  device_info->set_wear_model(d["wear_model"].asString());
  device_info->set_wear_version(d["wear_version"].asString());
  device_info->set_wear_version_channel(d["wear_version_channel"].asString());
  device_info->set_wear_os(d["wear_os"].asString());
  device_info->set_phone_model(d["phone_model"].asString());
  device_info->set_phone_version(d["phone_version"].asString());
  device_info->set_phone_os(d["phone_os"].asString());
  device_info->set_latitude(d["latitude"].asDouble());
  device_info->set_longitude(d["longitude"].asDouble());
  VLOG(2) << "device_info:" << push_controller::ProtoToString(*device_info);
  return true;
}

bool DeviceInfoHelper::GetDeviceByUser(const string& user_id, 
                                       string* device_id) {
  string command = StringPrintf(kQueryDeviceByUserSQL, user_id.c_str());
  VLOG(2) << "Command:" << command;
  Json::Value data;
  bool ret = util::GetMysqlResult(FLAGS_mysql_config, command, &data, false);
  if (!ret) {
    LOG(ERROR) << "Get result failed, user_id:" << user_id;
    return false;
  }
  VLOG(2) << "Data:" << push_controller::JsonToString(data);
  const Json::Value& d = data;
  *device_id = d["device_id"].asString();
  VLOG(1) << "User_id: " << user_id << ", Device_id:" << *device_id;
  return true;
}

void DeviceInfoHelper::GetClusterDeviceId(uint32 node_pos, uint32 node_cnt,
                                          vector<string>* devices) {
  if (node_pos < 0 || node_cnt < 0 || node_pos >= node_cnt) {
    return;
  }
  GetAllDeviceId(devices);
  for (auto it = devices->begin(); it != devices->end(); ) {
    uint64 fingerprint_id = static_cast<uint64>(mobvoi::Fingerprint(*it));
    if (fingerprint_id % node_cnt != node_pos) {
      it = devices->erase(it);
    } else {
      ++it;
    }
  }
  LOG(INFO) << "Cluster devices size:" << devices->size();
}

void DeviceInfoHelper::GetClusterUserDeviceVec(
    uint32 node_pos, uint32 node_cnt, 
    vector<std::pair<string, string>>* user_device_vec) {
  if (node_pos < 0 || node_cnt < 0 || node_pos >= node_cnt) {
    return;
  }
  GetAllUserDevicePair(user_device_vec);
  for (auto it = user_device_vec->begin(); it != user_device_vec->end(); ) {
    uint64 fingerprint = static_cast<uint64>(mobvoi::Fingerprint(it->first));
    if (fingerprint % node_cnt != node_pos) {
      it = user_device_vec->erase(it);
    } else {
      ++it;
    }
  }
  LOG(INFO) << "Cluster User_device vec size:" << user_device_vec->size();
}

void DeviceInfoHelper::GetAllDeviceId(vector<string>* devices) {
  Json::Value result;
  QueryAllDevice(&result);
  devices->clear();
  for (Json::Value::ArrayIndex i = 0; i < result.size(); ++i) {
    const Json::Value& data = result[i];
    for (Json::Value::ArrayIndex j = 0; j < data.size(); ++j) {
      const Json::Value& d = data[j];
      const string& id = d["id"].asString();
      devices->push_back(id);
    }
  }
  LOG(INFO) << "Device vec size:" << devices->size();
}
  
void DeviceInfoHelper::GetAllUserDevicePair(
    vector<std::pair<string, string>>* user_device_vec) {
  Json::Value result;
  QueryAllUser(&result);
  user_device_vec->clear();
  for (Json::Value::ArrayIndex i = 0; i < result.size(); ++i) {
    const Json::Value& data = result[i];
    for (Json::Value::ArrayIndex j = 0; j < data.size(); ++j) {
      const Json::Value& d = data[j];
      const string& user = d["name"].asString();
      const string& device = d["device_id"].asString();
      user_device_vec->push_back(std::make_pair(user, device));
    }
  }
  LOG(INFO) << "User_device vec size:" << user_device_vec->size();
}

void DeviceInfoHelper::QueryAllDevice(Json::Value* result) {
  return QueryAllInternal(kQueryAllDeviceSQL, FLAGS_mysql_page_size, result);
}

void DeviceInfoHelper::QueryAllUser(Json::Value* result) {
  return QueryAllInternal(kQueryAllUserSQL, FLAGS_mysql_page_size, result);
}

void DeviceInfoHelper::QueryAllInternal(const char* query_sql_format,
                                        int32 page_size, Json::Value* result) {
  LOG(INFO) << "Batch Query start...";
  result->clear();
  int start = 0;
  while (true) {
    string command = StringPrintf(query_sql_format, start * page_size, page_size);
    VLOG(2) << "Command:" << command;
    Json::Value data;
    bool ret = util::GetMysqlResult(FLAGS_mysql_config, command, &data, true);
    if (!ret) {
      LOG(ERROR) << "Get result failed, try again, page_start:"
                 << start * page_size << ", page_size:" << page_size;
      continue;
    }
    VLOG(2) << "Data:" << push_controller::JsonToString(data);
    if (data.empty()) {
      break;
    }
    result->append(data);
    start += 1;
  }
  LOG(INFO) << "Finish to batch query";
  return;
}

}  // namespace recommendation
