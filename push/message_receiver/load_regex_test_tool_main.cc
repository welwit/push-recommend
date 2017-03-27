// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "base/at_exit.h"
#include "base/file/proto_util.h"
#include "base/log.h"
#include "third_party/gflags/gflags.h"
#include "push/message_receiver/message_parser.h"

using namespace message_receiver;

DEFINE_string(extract_rule_conf,
    "config/recommendation/message_receiver/extract_rule.conf",
    "extract rule file conf");

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  ExtractRuleConf extract_rule_conf;
  CHECK(file::ReadProtoFromTextFile(FLAGS_extract_rule_conf, &extract_rule_conf));
  LOG(INFO) << extract_rule_conf.Utf8DebugString();
  return 0;
}
