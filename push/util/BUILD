# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'redis_util_debug_main',
  srcs = [
    'redis_util_debug_main.cc',
  ],
  deps = [
    ':redis_util',
    '//base:base',
    '//third_party/gflags:gflags',
  ],
)

cc_binary(
  name = 'zookeeper_util_debug_main',
  srcs = [
    'zookeeper_util_debug_main.cc',
  ],
  deps = [
    ':zookeeper_util',
  ]
)

cc_binary(
  name = 'user_info_debug_tool',
  srcs = [
    'user_info_debug_tool.cc',
  ],
  deps = [
    ':user_info_helper',
    '//base:base',
    '//base:binary_version',
  ],
)

cc_library(
  name = 'redis_util',
  srcs = [
    'redis_util.h',
    'redis_util.cc',
  ],
  deps = [
    '//base:base',
    '//base/file:proto_util',
    '//third_party/gflags:gflags',
    '//third_party/redis_client:redis_client',
    '//push/proto:redis_meta_proto',
  ],
)

cc_library(
  name = 'time_util',
  srcs = [
    'time_util.h',
    'time_util.cc',
  ],
  deps = [
    '//base:base',
  ],
)

cc_library(
  name = 'zookeeper_util',
  srcs = [
    'zookeeper_util.h',
    'zookeeper_util.cc',
  ],
  deps = [
    '//base:base',
    '//base/file:file',
    '//util/net:util',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/zookeeper:zookeeper',
  ],
)

cc_library(
  name = 'common_util',
  srcs = [
    'common_util.h',
    'common_util.cc',
  ],
  deps = [
    '//base:base',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/protobuf:protobuf',
    '//third_party/re2/re2:re2',
  ],
)

cc_library(
  name = 'weather_helper',
  srcs = [
    'weather_helper.h',
    'weather_helper.cc',
  ],
  deps = [
    '//base:base',
    '//third_party/gflags:gflags',
    '//third_party/jsoncpp:jsoncpp',
    '//util/net/http_client:http_client',
    '//push/proto:weather_meta_proto',
    '//push/util:common_util',
  ],
)

cc_library(
  name = 'location_helper',
  srcs = [
    'location_helper.h',
    'location_helper.cc',
  ],
  deps = [
    '//base:base',
    '//push/util:common_util',
    '//third_party/jsoncpp:jsoncpp',
    '//util/net/http_client:http_client',
    '//push/proto:user_device_meta_proto',
  ],
)

cc_library(
  name = 'user_info_helper',
  srcs = [
    'user_info_helper.h',
    'user_info_helper.cc',
  ],
  deps = [
    '//base:base',
    '//push/util:common_util',
    '//third_party/jsoncpp:jsoncpp',
    '//util/mysql:mysql_util',
    '//push/proto:user_device_meta_proto',
  ],
)

cc_test(
  name = 'common_util_test',
  srcs = [
    'common_util_test.cc',
  ],
  deps = [
    ':common_util',
    '//third_party/gtest:gtest_main',
  ],
)

cc_test(
  name = 'time_util_test',
  srcs = [
    'time_util_test.cc'
  ],
  deps = [
    ':time_util',
    '//base:base',
    '//base/file:proto_util',
    '//third_party/gtest:gtest_main',
  ],
)

cc_test(
  name = 'weather_helper_test',
  srcs = [
    'weather_helper_test.cc',
  ],
  deps = [
    'weather_helper',
    '//third_party/gtest:gtest_main',
  ],
)
