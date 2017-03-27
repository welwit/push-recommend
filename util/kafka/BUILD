# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'kafka_consumer_debug_main',
  srcs = [
    'kafka_consumer_debug_main.cc',
  ],
  deps = [
    ':kafka_util',
    '//base:base',
    '//third_party/gflags:gflags',
  ],
)

cc_binary(
  name = 'kafka_producer_debug_main',
  srcs = [
    'kafka_producer_debug_main.cc',
  ],
  deps = [
    ':kafka_util',
    '//base:base',
    '//third_party/gflags:gflags',
  ],
)

cc_library(
  name = 'kafka_util',
  srcs = [
    'kafka_util.h',
    'kafka_util.cc',
  ],
  deps = [
    '//base:base',
    '//base/file:proto_util',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/zookeeper:zookeeper',
    '//third_party/librdkafka/src-cpp:librdkafka++',
    '//util/kafka/proto:kafka_meta_proto',
  ],
)
