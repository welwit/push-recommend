# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

cc_binary(
  name = 'user_feedback_server_main',
  srcs = [
    'user_feedback_server_main.cc',
  ],
  deps = [
    ':user_feedback_handler',
    '//base:binary_version',
  ],
)

cc_library(
  name = 'user_feedback_handler',
  srcs = [
    'user_feedback_handler.cc',
    'user_feedback_handler.h',
  ],
  deps = [
    ':db_handler',
    '//base:base',
    '//base/file:proto_util',
    '//onebox:http_handler',
    '//proto:mysql_config_proto',
    '//third_party/curl:curl',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//util/net/http_client:http_client',
    '//util/net:util',
    '//push/proto:user_feedback_meta_proto',
    '//push/util:common_util',
  ],
)

cc_library(
  name = 'db_handler',
  srcs = [
    'db_handler.cc',
    'db_handler.h',
  ],
  deps = [
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//push/proto:user_feedback_meta_proto',
  ],
)
