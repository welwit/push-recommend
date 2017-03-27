# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'push_controller_main',
  srcs = [
    'push_controller_main.cc',
  ],
  deps = [
    ':business_factory',
    ':business_processor',
    '//push/push_controller/flight:flight_push_processor',
    '//push/push_controller/hotel:hotel_push_processor',
    '//push/push_controller/movie:movie_push_processor',
    '//push/push_controller/train:train_push_processor',
    ':push_controller_handler',
    ':push_pool_updater',
    ':push_processor',
    ':push_scheduler',
    '//base:base',
    '//base:binary_version',
    '//push/util:zookeeper_util',
  ],
)

cc_library(
  name = 'business_factory',
  srcs = [
    'business_factory.h',
    'business_factory.cc',
  ],
  deps = [
    ':business_processor',
    ':push_processor',
    '//base:base',
    '//push/proto:push_meta_proto',
  ],
)

cc_library(
  name = 'push_controller_handler',
  srcs = [
    'push_controller_handler.h',
    'push_controller_handler.cc',
  ],
  deps = [
    '//base:base',
    '//onebox:http_handler',
    '//third_party/jsoncpp:jsoncpp',
    '//push/push_controller/news:news_push_processor',
  ],
)

cc_library(
  name = 'push_pool_updater',
  srcs = [
    'push_pool_updater.h',
    'push_pool_updater.cc',
  ],
  deps = [
    ':business_factory',
    ':update_time_backup',
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//third_party/gflags:gflags',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//push/util:time_util',
    '//push/proto:push_meta_proto',
    '//push/util:zookeeper_util',
    '//push/util:common_util',
  ],
)

cc_library(
  name = 'push_scheduler',
  srcs = [
    'push_scheduler.h',
    'push_scheduler.cc',
  ],
  deps = [
    ':business_factory',
    ':filter',
    ':push_processor',
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//third_party/gflags:gflags',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//push/util:time_util',
    '//push/proto:push_meta_proto',
    '//push/util:zookeeper_util',
    '//push/util:common_util',
  ],
)

cc_library(
  name = 'business_processor',
  srcs = [
    'business_processor.h',
    'business_processor.cc',
  ],
  deps = [
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//push/util:time_util',
    '//push/proto:push_meta_proto',
    '//push/util:common_util',
  ],
)

cc_library(
  name = 'filter',
  srcs = [
    'filter.h',
    'filter.cc',
  ],
  deps = [
    ':push_sender',
    '//base:base',
    '//push/proto:push_meta_proto',
    '//push/proto:user_feedback_meta_proto',
    '//push/util:common_util',
  ],
)

cc_library(
  name = 'push_processor',
  srcs = [
    'push_processor.h',
    'push_processor.cc',
  ],
  deps = [
    ':push_sender',
    '//push/util:weather_helper',
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//push/util:time_util',
    '//push/proto:flight_meta_proto',
    '//push/proto:train_meta_proto',
    '//push/proto:push_meta_proto',
    '//push/util:common_util',
    '//push/util:user_info_helper',
  ],
)

cc_library(
  name = 'push_sender',
  srcs = [
    'push_sender.h',
    'push_sender.cc',
  ],
  deps = [
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//third_party/gflags:gflags',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//util/net/http_client:http_client',
    '//push/proto:push_meta_proto',
    '//push/util:common_util',
  ],
)

cc_library(
  name = 'update_time_backup',
  srcs = [
    'update_time_backup.h',
    'update_time_backup.cc',
  ],
  deps = [
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//third_party/gflags:gflags',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//push/util:time_util',
  ]
)
