# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'news_crawler_server_main',
  srcs = [
    'news_crawler_server_main.cc',
  ],
  deps = [
    ':fetcher_thread',
    ':news_crawler_handler',
    '//base:base',
    '//base/file:proto_util',
    '//crawler/base:http_status',
    '//third_party/gflags:gflags',
  ],
)

cc_binary(
  name = 'xinhua_fetcher_debug_tool',
  srcs = [
    'xinhua_fetcher_debug_tool.cc',
  ],
  deps = [
    ':xinhua_fetcher',
    ':news_crawler_handler',
    '//base:base',
    '//base/file:proto_util',
    '//crawler/base:http_status',
    '//third_party/gflags:gflags',
  ],
)

cc_binary(
  name = 'tencent_fetcher_debug_tool',
  srcs = [
    'tencent_fetcher_debug_tool.cc',
  ],
  deps = [
    ':tencent_fetcher',
    ':news_crawler_handler',
    '//base:base',
    '//base/file:proto_util',
    '//crawler/base:http_status',
    '//third_party/gflags:gflags',
  ],
)

cc_binary(
  name = 'sina_fetcher_debug_tool',
  srcs = [
    'sina_fetcher_debug_tool.cc',
  ],
  deps = [
    ':sina_fetcher',
    '//base:base',
  ],
)

cc_binary(
  name = 'fetcher_debug_tool',
  srcs = [
    'fetcher_debug_tool.cc',
  ],
  deps = [
    ':base_fetcher',
    '//base:base',
    '//base/file:proto_util',
    '//crawler/base:http_status',
    '//third_party/gflags:gflags',
    '//recommendation/news/proto:news_meta_proto',
    '//recommendation/news/proto:topic_meta_proto',
  ],
)

cc_binary(
  name = 'util_debug_tool',
  srcs = [
    'util_debug_tool.cc',
  ],
  deps = [
    ':util',
    '//base:base',
    '//third_party/gflags:gflags',
  ],
)

cc_library(
  name = 'news_crawler_handler',
  srcs = [
    'news_crawler_handler.h',
    'news_crawler_handler.cc',
  ],
  deps = [
    '//base:base',
    '//onebox:http_handler',
    '//third_party/jsoncpp:jsoncpp',
  ],
)

cc_library(
  name = 'util',
  srcs = [
    'util.h',
    'util.cc',
  ],
  deps = [
    '//base:base',
    '//crawler/doc_handler:doc_processor',
    '//recommendation/news/proto:topic_meta_proto',
    '//third_party/jsoncpp:jsoncpp',
    '//util/parser/template_parser:template_parser',
  ],
)

cc_library(
  name = 'xinhua_fetcher',
  srcs = [
    'xinhua_fetcher.h',
    'xinhua_fetcher.cc',
  ],
  deps = [
    ':base_fetcher',
    ':util',
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//push/util:time_util',
    '//recommendation/news/proto:news_meta_proto',
    '//recommendation/news/proto:topic_meta_proto',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/re2/re2:re2',
  ],
)

cc_library(
  name = 'tencent_fetcher',
  srcs = [
    'tencent_fetcher.h',
    'tencent_fetcher.cc',
  ],
  deps = [
    ':base_fetcher',
    ':util',
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//push/util:time_util',
    '//recommendation/news/proto:news_meta_proto',
    '//recommendation/news/proto:topic_meta_proto',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/re2/re2:re2',
  ],
)

cc_library(
  name = 'sina_fetcher',
  srcs = [
    'sina_fetcher.h',
    'sina_fetcher.cc',
  ],
  deps = [
    ':base_fetcher',
    ':util',
    '//base:base',
    '//third_party/gflags:gflags',
    '//third_party/jsoncpp:jsoncpp',
    '//util/parser/rss_parser:rss_parser',
  ],
)

cc_library(
  name = 'fetcher_thread',
  srcs = [
    'fetcher_thread.h',
    'fetcher_thread.cc',
  ],
  deps = [
    ':sina_fetcher',
    ':tencent_fetcher',
    ':xinhua_fetcher',
    ':util',
    '//base:base',
    '//third_party/gflags:gflags',
  ],
)

cc_library(
  name = 'base_fetcher',
  srcs = [
    'base_fetcher.h',
    'base_fetcher.cc',
  ],
  deps = [
    ':util',
    '//base:base',
    '//base/file:proto_util',
    '//proto:mysql_config_proto',
    '//push/util:time_util',
    '//recommendation/news/proto:news_meta_proto',
    '//recommendation/news/proto:topic_meta_proto',
    '//third_party/jsoncpp:jsoncpp',
    '//third_party/mysql_client_cpp:mysqlcppconn',
    '//third_party/re2/re2:re2',
  ],
)

cc_test(
  name = 'util_test',
  srcs = [
    'util_test.cc',
  ],
  deps = [
    ':util',
    '//base:base',
    '//base/file:proto_util',
    '//third_party/gtest:gtest_main',
  ],
)
