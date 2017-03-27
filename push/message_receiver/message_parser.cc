// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/file/proto_util.h"
#include "base/log.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"
#include "third_party/re2//re2/re2.h"

#include "push/message_receiver/message_parser.h"
#include "push/util/common_util.h"

DECLARE_string(extract_rule_conf);

namespace message_receiver {

RegexManager::RegexManager() {
  extract_rule_conf_.reset(new ExtractRuleConf);
  CHECK(LoadRules());
}

RegexManager::~RegexManager() {}

bool RegexManager::LoadRules() {
  if (file::ReadProtoFromTextFile(FLAGS_extract_rule_conf,
                                  extract_rule_conf_.get())) {
    return true;
  } else {
    return false;
  }
}

void RegexManager::DebugRules() {
  LOG(INFO) << "ALL RULES:\n"
            << push_controller::ProtoToString(*extract_rule_conf_);
}

bool RegexManager::SelfCheckRules() {
  bool success = true;
  for (int i = 0; i < extract_rule_conf_->extract_rule_size(); ++i) {
    map<string, string> group_value_map;
    auto& rule = extract_rule_conf_->extract_rule(i);
    auto& regex = rule.regex();
    auto& first_type = rule.first_type();
    auto& second_type = rule.second_type();
    auto& test_message = rule.test_message();
    if (!push_controller::ExtractMatchedGroupResults(regex,
                                                     test_message,
                                                     &group_value_map)) {
      success = false;
      continue;
    }
    LOG(INFO) << "MATCH success, type:(" << first_type
              << "," << second_type << ")";
    for (auto& group_value_pair : group_value_map) {
      LOG(INFO) << "GROUP VALUE: (" << group_value_pair.first
                << "," << group_value_pair.second << ")";
    }
  }
  return success;
}

bool RegexManager::Extract(const MessageMatcher& message_matcher,
                           Json::Value* output) {
  map<string, string> group_value_map;
  ExtractRule extract_rule;
  string message_input = message_matcher.message_input();
  if (!ExtractInfoInternal(message_input, &group_value_map, &extract_rule)) {
    LOG(ERROR) << "Extract info internal failed";
    return false;
  }
  return BuildOutput(message_matcher, group_value_map, extract_rule, output);
}

bool RegexManager::ExtractInfoInternal(const string& message,
                                       map<string, string>* group_value_map,
                                       ExtractRule* extract_rule) {
  bool success = false;
  for (int i = 0; i < extract_rule_conf_->extract_rule_size(); ++i) {
    auto& rule = extract_rule_conf_->extract_rule(i);
    auto& regex = rule.regex();
    auto& first_type = rule.first_type();
    auto& second_type = rule.second_type();
    if (push_controller::ExtractMatchedGroupResults(regex,
                                                    message,
                                                    group_value_map)) {
      VLOG(2) << "match success type:(" << first_type
              << "," << second_type << "), message:" << message;
      for (auto& group_value_pair : *group_value_map) {
        VLOG(2) << "extract group value: (" << group_value_pair.first
               << "," << group_value_pair.second << ")";
      }
      *extract_rule = rule;
      success = true;
      break;
    }
  }
  return success;
}

bool RegexManager::BuildOutput(const MessageMatcher& message_matcher,
                               const map<string, string>& group_value_map,
                               const ExtractRule& extract_rule,
                               Json::Value* output) {
  auto& first_type = extract_rule.first_type();
  Json::Value content;
  for (int i = 0; i < extract_rule.extract_field_size(); ++i) {
    auto& extract_field = extract_rule.extract_field(i);
    const string& key = extract_field.key();
    const string& value = extract_field.value();
    auto it = group_value_map.find(value);
    if (it == group_value_map.end()) {
      string datetime;
      if (ExtractDatetime(value, group_value_map, &datetime)) {
        content[key] = datetime;
        continue;
      } else if (ExtractDate(value, group_value_map, &datetime)) {
        content[key] = datetime;
        continue;
      } else {
        LOG(ERROR) << "extract failed, no field:"
                   << value << " in map, message:"
                   << message_matcher.message_input();
        return false;
      }
    } else {
      content[key] = it->second;
      continue;
    }
  }
  content["user"] = message_matcher.user_id();
  Json::Value content_array;
  content_array.append(content);
  (*output)[first_type] = content_array;
  VLOG(2) << "Extract result: " << push_controller::JsonToString(*output);
  return true;
}

bool RegexManager::ExtractDate(const string& input,
                               const map<string, string>& group_value_map,
                               string* output) {
  output->clear();
  string& result = *output;
  result.clear();
  string format = "([A-Za-z]+?)-([A-Za-z]+?)";
  if (!RE2::FullMatch(input, format)) {
    return false;
  }
  vector<string> month_day;
  SplitString(input, '-', &month_day);
  map<string, string>::const_iterator it;
  it = group_value_map.find(month_day[0]);
  if (it == group_value_map.end()) {
    return false;
  }
  result = it->second;
  it = group_value_map.find(month_day[1]);
  if (it == group_value_map.end()) {
    return false;
  }
  result = result + "-" + it->second;
  return true;
}

bool RegexManager::ExtractDatetime(const string& input,
                                   const map<string, string>& group_value_map,
                                   string* output) {
  output->clear();
  string& result = *output;
  result.clear();
  string format = "([A-Za-z]+?)-([A-Za-z]+?) ([A-Za-z]+?):([A-Za-z]+?)";
  if (!RE2::FullMatch(input, format)) {
    return false;
  }
  vector<string> date_time;
  vector<string> month_day;
  vector<string> hour_minute;
  SplitString(input, ' ', &date_time);
  SplitString(date_time[0], '-', &month_day);
  SplitString(date_time[1], ':', &hour_minute);
  map<string, string>::const_iterator it;
  it = group_value_map.find(month_day[0]);
  if (it == group_value_map.end()) {
    return false;
  }
  result = it->second;
  it = group_value_map.find(month_day[1]);
  if (it == group_value_map.end()) {
    return false;
  }
  result = result + "-" + it->second;
  it = group_value_map.find(hour_minute[0]);
  if (it == group_value_map.end()) {
    return false;
  }
  result = result + " " + it->second;
  it = group_value_map.find(hour_minute[1]);
  if (it == group_value_map.end()) {
    return false;
  }
  result = result + ":" + it->second;
  return true;
}

MessageParser::MessageParser() {
  regex_manager_.reset(new RegexManager);
  regex_manager_->DebugRules();
  CHECK(regex_manager_->SelfCheckRules());
}

MessageParser::~MessageParser() {}

bool MessageParser::Parse(const string& message_input, string* message_output) {
  Json::Value message_input_json;
  if (!ParseRawMessage(message_input, &message_input_json)) {
    return false;
  }
  MessageMatcher message_matcher;
  if (!InitMessageMatcher(message_input_json, &message_matcher)) {
    return false;
  }
  Json::Value output;
  if (!regex_manager_->Extract(message_matcher, &output)) {
    return false;
  }
  *message_output = push_controller::JsonToString(output);
  LOG(INFO) << "MESSAE output:" << *message_output;
  return true;
}

bool MessageParser::ParseRawMessage(const string& message_input,
                                    Json::Value* message_input_json) {
  Json::Reader reader;
  try {
    if (!reader.parse(message_input, *message_input_json)) {
      LOG(ERROR) << "parsed by reader failed, msg:"
                 << message_input;
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "parse raw message exception:" << e.what();
    return false;
  }
  return true;
}

bool MessageParser::InitMessageMatcher(const Json::Value& message_input_json,
                                       MessageMatcher* message_matcher) {
  if (message_input_json.isMember("app_key") &&
      message_input_json.isMember("user_id") &&
      message_input_json.isMember("msg")) {
    if (message_input_json["app_key"].asString() != "com.mobvoi.companion") {
      LOG(INFO) << "INVALID app_key message:"
                << push_controller::JsonToString(message_input_json);
      return false;
    }
    message_matcher->set_user_id(message_input_json["user_id"].asString());
    message_matcher->set_message_input(message_input_json["msg"].asString());
    VLOG(2) << "Init MessageMatcher success, toString:\n"
            << push_controller::ProtoToString(*message_matcher);
    return true;
  } else {
    LOG(INFO) << "NOT supported message:"
              << push_controller::JsonToString(message_input_json);
    return false;
  }
}

}  // namespace message_receiver
