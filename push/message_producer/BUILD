# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'message_producer_server_main',
  srcs = [
    'message_producer_server_main.cc',
  ],
  deps = [
    ':message_producer_handler',
    '//base:base',
    '//base:binary_version',
  ],
)

cc_library(
  name = 'message_producer_handler',
  srcs = [
    'message_producer_handler.h',
    'message_producer_handler.cc',
  ],
  deps = [
    '//base:base',
    '//onebox:http_handler',
    '//third_party/jsoncpp:jsoncpp', 
    '//util/kafka:kafka_util',
  ],
)
