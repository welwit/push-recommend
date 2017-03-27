# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'news_push_debug_main',
  srcs = [
    'news_push_debug_main.cc',
  ],
  deps = [
    ':news_push_processor',
    '//base:base',
    '//third_party/gflags:gflags',
  ],
)

cc_library(
  name = 'news_push_processor',
  srcs = [
    'news_push_processor.h',
    'news_push_processor.cc',
  ],
  deps = [
    '//push/push_controller:push_processor',
    '//push/util:redis_util',
    '//push/util:zookeeper_util',
    '//recommendation/news/proto:news_meta_proto',
    '//push/util:user_info_helper',
    '//third_party/re2/re2:re2',
    '//util/protobuf:proto_json_format',
  ]
)
