# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'load_regex_test_tool_main',
  srcs = [
    'load_regex_test_tool_main.cc',
  ],
  deps = [
    ':message_parser',
    '//base:base',
    '//base/file:proto_util',
  ],
)

cc_binary(
  name = 'message_publish_test_tool_main',
  srcs = [
    'message_publish_test_tool_main.cc',
  ],
  deps = [
    '//base:base',
    '//util/kafka:kafka_util',
    '//push/util:redis_util',
  ],
)

cc_binary(
  name = 'message_receiver_main',
  srcs = [
    'message_receiver_main.cc',
  ],
  deps = [
    ':message_controller',
    ':message_receiver_handler',
    '//base:base',
    '//base:binary_version',
  ],
)

cc_library(
  name = 'message_receiver',
  srcs = [
    'message_receiver.h',
    'message_receiver.cc',
  ],
  deps = [
    ':message_parser',
    '//base:base',
    '//proto:mysql_config_proto',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//util/net/http_client:http_client',
    '//util/kafka:kafka_util',
    '//push/util:redis_util',
    '//push/util:time_util',
    '//push/proto:push_meta_proto',
  ],
)

cc_library(
  name = 'message_receiver_handler',
  srcs = [
    'message_receiver_handler.h',
    'message_receiver_handler.cc',
  ],
  deps = [
    '//base:base',
    '//onebox:http_handler',
    '//third_party/jsoncpp:jsoncpp',
  ],
)

cc_library(
  name = 'message_parser',
  srcs = [
    'message_parser.h',
    'message_parser.cc',
  ],
  deps = [
    '//base:base',
    '//base/file:proto_util',
    '//third_party/jsoncpp:jsoncpp',
    '//push/util:common_util',
    '//push/proto:message_receiver_meta_proto',
  ],
)

cc_library(
  name = 'message_processor',
  srcs = [
    ':message_processor.h',
    ':message_processor.cc',
  ],
  deps = [
    ':message_receiver',
    '//push/util:common_util',
  ],
)

cc_library(
  name = 'message_controller',
  srcs = [
    ':message_controller.h',
    ':message_controller.cc',
  ],
  deps = [
    ':message_processor',
    ':message_receiver',
  ],
)

cc_test(
  name = 'message_parser_test',
  srcs = ['message_parser_test.cc'],
  deps = [
    'message_parser',
    '//base:base',
    '//third_party/gtest:gtest_main',
  ],
)
