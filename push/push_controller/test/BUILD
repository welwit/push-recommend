# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'create_table_tool_main',
  srcs = [
    'create_table_tool_main.cc',
  ],
  deps = [
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//third_party/gflags:gflags',
    '//third_party/mysql_client_cpp:mysqlcppconn',
  ],
)

cc_binary(
  name = 'pool_updater_test_tool_main',
  srcs = [
    'pool_updater_test_tool_main.cc',
  ],
  deps = [
    '//base:base',
    '//push/push_controller:business_factory',
    '//push/push_controller:business_processor',
    '//push/push_controller:push_pool_updater',
  ],
)

cc_binary(
  name = 'push_test_tool_main',
  srcs = [
    'push_test_tool_main.cc',
  ],
  deps = [
    '//push/push_controller/flight:flight_push_processor',
    '//push/push_controller/movie:movie_push_processor',
    '//push/push_controller/train:train_push_processor',
    '//push/push_controller/hotel:hotel_push_processor',
    '//push/push_controller:push_processor',
    '//push/util:common_util',
  ],
)
