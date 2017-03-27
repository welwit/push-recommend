// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/util/common_util.h"

#include "base/string_util.h"
#include "base/log.h"
#include "third_party/jsoncpp/json.h"
#include "third_party/re2/re2/re2.h"

namespace push_controller {

bool JsonWrite(const Json::Value& root, string* output) {
  if (!output) {
    return false;
  }
  try {
    Json::FastWriter writer;
    std::string value = writer.write(root);
    const char trim_chars[] =  {'\n'};
    TrimString(value, trim_chars, output);
  } catch (const std::exception& e) {
    LOG(ERROR) << "JsonWrite, Exception what:" << e.what();
    return false;
  }
  return true;
}

string JsonToString(const Json::Value& root) {
  try {
    Json::FastWriter writer;
    string value = writer.write(root);
    return value;
  } catch (const std::exception& e) {
    LOG(ERROR) << "JsonToString, Exception what: " << e.what();
    return "";
  }
}

string ProtoToString(const google::protobuf::Message& message) {
  string result = message.Utf8DebugString();
  ReplaceSubstringsAfterOffset(&result, 0, "\n", "\t");
  return "{" + result + "}";
}

bool CheckPhoneNumber(const string& phone_number) {
  string format = "([0-9-()（）]{7,18})|(0?(13|14|15|18)[0-9]{9})";
  if (!RE2::FullMatch(phone_number, format)) {
    return false;
  }
  return true;
}

bool GetVersion(const string& version, VersionType* version_type) {
  string format = "tic_(\\d{1,})\\.(\\d{1,})\\.(\\d{1,})";
  int major = 0, minor = 0, revision = 0;
  if (!RE2::PartialMatch(version, format, &major, &minor, &revision)) {
    return false;
  }
  version_type->major = major;
  version_type->minor = minor;
  version_type->revision = revision;
  return true;
}

bool CheckVersionMatch(const string& version_cmp,
                       const string& version_standard) {
  VersionType version_standard_type, version_cmp_type;
  if (!GetVersion(version_standard, &version_standard_type) ||
      !GetVersion(version_cmp, &version_cmp_type)) {
    return false;
  }
  return version_cmp_type == version_standard_type;
}

bool CheckVersionMatch2(const string& version_cmp,
                        const string& version_standard) {
  return version_cmp == version_standard;
}

bool CheckVersionGreaterThan(const string& version_cmp,
                             const string& version_standard) {
  VersionType version_standard_type, version_cmp_type;
  if (!GetVersion(version_standard, &version_standard_type) ||
      !GetVersion(version_cmp, &version_cmp_type)) {
    return false;
  }
  return version_cmp_type > version_standard_type;
}

bool CheckChannelInternal(const string& version_channel) {
  return version_channel == "internal";
}

bool ExtractMatchedGroupResults(const string& pattern,
                                const string& text,
                                map<string, string>* group_value_map) {
  map<string, string>& result_map = *group_value_map;
  result_map.clear();
  RE2 re(pattern);
  if (!re.ok()) return false;
  size_t group_size = re.NumberOfCapturingGroups();
  if (group_size > 20) return false;
  RE2::Arg args[20];
  const RE2::Arg* const p2args[20] = {
    &args[0], &args[1], &args[2], &args[3], &args[4], &args[5], &args[6],
    &args[7], &args[8], &args[9], &args[10], &args[11], &args[12], &args[13],
    &args[14], &args[15], &args[16], &args[17], &args[18], &args[19]
  };

  string values[20];
  for (int i = 0; i < 20; ++i) {
    args[i] = &values[i];
  }
  re2::StringPiece input(text);
  bool found = RE2::FindAndConsumeN(&input, re, p2args, group_size);
  if (!found) return false;
  const map<string, int>& group_to_idx = re.NamedCapturingGroups();
  for (auto it = group_to_idx.begin(); it != group_to_idx.end(); ++it) {
    result_map[it->first] = values[it->second - 1];
  }
  return true;
}

}  // namespace push_controller
