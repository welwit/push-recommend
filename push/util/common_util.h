// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_UTIL_COMMON_UTIL_H_
#define PUSH_UTIL_COMMON_UTIL_H_

#include "base/compat.h"
#include "third_party/jsoncpp/json.h"
#include "third_party/protobuf/include/google/protobuf/message.h"
#include "third_party/re2//re2/re2.h"

namespace push_controller {

struct VersionType {
  int major;
  int minor;
  int revision;
  bool operator==(VersionType& other) {
    return (major == other.major) && (minor == other.minor)
      && (revision == other.revision);
  }
  bool operator>(VersionType& other) {
    return (major > other.major) ||
      (major == other.major && minor > other.minor) ||
      (major == other.major && minor == other.minor && revision > other.revision);
  }
};

bool JsonWrite(const Json::Value& root, string* output);
string JsonToString(const Json::Value& root);
string ProtoToString(const google::protobuf::Message& message);

bool CheckPhoneNumber(const string& phone_number);

bool GetVersion(const string& version, VersionType* version_type);
bool CheckVersionMatch(const string& version_cmp,
                       const string& version_standard);
bool CheckVersionMatch2(const string& version_cmp,
                        const string& version_standard);
bool CheckVersionGreaterThan(const string& version_cmp,
                             const string& version_standard);

bool CheckChannelInternal(const string& version_channel);

bool ExtractMatchedGroupResults(const string& pattern,
    const string& text, map<string, string>* group_value_map);

}  // namespace push_controller

#endif  // PUSH_UTIL_COMMON_UTIL_H_
