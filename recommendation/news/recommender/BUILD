# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'news_recommender_server_main',
  srcs = [
    'news_recommender_server_main.cc',
  ],
  deps = [
    ':recommendation_handler',
    '//base:base',
    '//base:binary_version',
  ],
)

cc_binary(
  name = 'toutiao_trigger_debug_tool',
  srcs = [
    'toutiao_trigger_debug_tool.cc',
  ],
  deps = [
    ':toutiao_trigger',
    '//base:base',
    '//base:binary_version',
    '//push/util:common_util',
  ],
)

cc_library(
  name = 'recommendation_handler',
  srcs = [
    'recommendation_handler.h',
    'recommendation_handler.cc',
  ],
  deps = [
    ':toutiao_trigger',
    ':news_trigger',
    '//push/util:location_helper',
    '//push/util:user_info_helper',
    '//base:base',
    '//onebox:http_handler',
    '//push/util:common_util',
    '//third_party/jsoncpp:jsoncpp',
    '//util/url/parser:url_parser',
    '//util/protobuf:proto_json_format',
  ],
)

cc_library(
  name = 'toutiao_trigger',
  srcs = [
    'toutiao_trigger.h',
    'toutiao_trigger.cc',
  ],
  deps = [
    ':category_helper',
    '//base:base',
    '//third_party/jsoncpp:jsoncpp',
    '//util/net/http_client:http_client',
    '//util/random:random',
  ],
)

cc_library(
  name = 'news_trigger',
  srcs = [
    'news_trigger.h',
    'news_trigger.cc',
  ],
  deps = [
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//push/util:common_util',
    '//push/util:time_util',
    '//recommendation/news/proto:news_meta_proto',
    '//util/protobuf:proto_json_format',
    '//third_party/mysql_client_cpp:mysqlcppconn',
  ],
)

cc_library(
  name = 'category_helper',
  srcs = [
    'category_helper.h',
    'category_helper.cc',
  ],
  deps = [
    '//base:base',
    '//base/file:simple_line_reader',
    '//push/util:common_util',
    '//recommendation/news/proto:news_meta_proto',
  ],
)
