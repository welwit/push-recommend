// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_MESSAGE_RECEIVER_MESSAGE_PARSER_H_
#define PUSH_MESSAGE_RECEIVER_MESSAGE_PARSER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "third_party/jsoncpp/json.h"
#include "push/proto/message_receiver_meta.pb.h"

namespace message_receiver {

class RegexManager {
 public:
  RegexManager();
  ~RegexManager();
  bool LoadRules();
  void DebugRules();
  bool SelfCheckRules();
  bool Extract(const MessageMatcher& message_matcher,
               Json::Value* output);

 private:
  bool ExtractInfoInternal(const string& message,
                           map<string, string>* group_value_map,
                           ExtractRule* extract_rule);
  bool BuildOutput(const MessageMatcher& message_matcher,
                   const map<string, string>& group_value_map,
                   const ExtractRule& extract_rule,
                   Json::Value* output);
  bool ExtractDatetime(const string& input,
                       const map<string, string>& group_value_map,
                       string* output);
  bool ExtractDate(const string& input,
                   const map<string, string>& group_value_map,
                   string* output);

  std::unique_ptr<ExtractRuleConf> extract_rule_conf_;
  DISALLOW_COPY_AND_ASSIGN(RegexManager);
};

class MessageParser {
 public:
  MessageParser();
  ~MessageParser();
  bool Parse(const string& message_input, string* message_output);

 private:
  bool ParseRawMessage(const string& message_input,
                       Json::Value* message_input_json);
  bool InitMessageMatcher(const Json::Value& message_input_json,
                          MessageMatcher* message_matcher);
  bool ParseInternal(MessageMatcher* message_matcher);

  std::unique_ptr<RegexManager> regex_manager_;
  DISALLOW_COPY_AND_ASSIGN(MessageParser);
};

}  // namespace message_receiver

#endif  // PUSH_MESSAGE_RECEIVER_MESSAGE_PARSER_H_
