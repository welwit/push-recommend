# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'flight_info_server_main',
  srcs = [
    'flight_info_server_main.cc',
  ],
  deps = [
    ':flight_data_updater',
    ':flight_info_handler',
    '//base:binary_version',
  ],
)

cc_library(
  name = 'flight_data_fetcher',
  srcs = [
    'flight_data_fetcher.cc',
    'flight_data_fetcher.h',
  ],
  deps = [
    '//base:base',
    '//third_party/jsoncpp:jsoncpp',
    '//util/net/http_client:http_client',
    '//push/proto:flight_meta_proto',
    '//push/util:time_util',
  ],
)

cc_library(
  name = 'flight_data_updater',
  srcs = [
    'flight_data_updater.h',
    'flight_data_updater.cc',
  ],
  deps = [
    ":flight_db_interface",
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//third_party/gflags:gflags',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//push/proto:flight_meta_proto',
  ],
)

cc_library(
  name = 'flight_db_interface',
  srcs = [
    'flight_db_interface.cc',
    'flight_db_interface.h',
  ],
  deps = [
    ":flight_data_fetcher",
    '//base/file:proto_util',
    '//third_party/gflags:gflags',
    '//push/proto:flight_meta_proto',
    '//push/util:redis_util',
  ],
)

cc_library(
  name = 'flight_info_handler',
  srcs = [
    'flight_info_handler.h',
    'flight_info_handler.cc',
  ],
  deps = [
    ':flight_db_interface',
    '//base:base',
    '//base/file:proto_util',
    '//onebox:http_handler',
    '//proto:mysql_config_proto',
    '//third_party/gflags:gflags',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//push/proto:flight_meta_proto',
    '//push/util:common_util',
  ],
)
