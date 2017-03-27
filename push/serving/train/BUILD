# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'train_info_server_main',
  srcs = [
    'train_info_server_main.cc',
  ],
  deps = [
    ':train_info_handler',
    '//base:binary_version',
  ],
)

cc_library(
  name = 'time_table_dumper',
  srcs = [
    'time_table_dumper.h',
    'time_table_dumper.cc',
  ],
  deps = [
    ':train_data_fetcher',
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//third_party/jsoncpp',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//push/proto:train_meta_proto',
  ],
)

cc_library(
  name = 'train_data_fetcher',
  srcs = [
    'train_data_fetcher.h',
    'train_data_fetcher.cc',
  ],
  deps = [
    '//base:base',
    '//crawler/doc_handler:doc_processor',
    '//crawler/proto:crawldoc_proto',
    '//util/net/http_client:http_client',
    '//util/parser/template_parser:template_parser',
  ]
)

cc_library(
  name = 'trainno_dumper',
  srcs = [
    'trainno_dumper.h',
    'trainno_dumper.cc',
  ],
  deps = [
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//third_party/jsoncpp',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//push/proto:train_meta_proto',
  ],
)

cc_library(
  name = 'train_info_handler',
  srcs = [
    'train_info_handler.h',
    'train_info_handler.cc',
  ],
  deps = [
    ':time_table_dumper',
    ':train_data_fetcher',
    ':train_data_updater',
    ':trainno_dumper',
    '//base:base',
    '//onebox:http_handler',
    '//third_party/jsoncpp:jsoncpp',
    '//push/proto:train_meta_proto',
    '//push/util:common_util',
  ],
)

cc_library(
  name = 'train_data_updater',
  srcs = [
    'train_data_updater.h',
    'train_data_updater.cc',
  ],
  deps = [
    ':time_table_dumper',
    '//base:base',
  ],
)
