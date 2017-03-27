# Copyright 2017 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

package(default_visibility = ['//visibility:public'])

cc_binary(
  name = 'news_ranking_server_main',
  srcs = [
    'news_ranking_server_main.cc',
  ],
  deps = [
    ':ranking_handler',
    ':ranking_thread',
    '//base:base',
    '//third_party/gflags:gflags',
  ],
)

cc_binary(
  name = 'doc_manager_debug_tool',
  srcs = [
    'doc_manager_debug_tool.cc',
  ],
  deps = [
    ':doc_manager',
    '//base:base',
    '//third_party/gflags:gflags',
  ],
)

cc_binary(
  name = 'cosine_ranker_debug_tool',
  srcs = [
    'cosine_ranker_debug_tool.cc',
  ],
  deps = [
    ':cosine_ranker',
    '//base:base',
    '//third_party/gflags:gflags',
  ],
)

cc_library(
  name = 'doc_manager',
  srcs = [
    'doc_meta.h',
    'doc_manager.h',
    'doc_manager.cc',
  ],
  deps = [
    '//base:base',
    '//push/util:time_util',
    '//third_party/gflags:gflags',
    '//util/mysql:mysql_util',
    '//recommendation/news/proto:news_meta_proto',
  ],
)

cc_library(
  name = 'cosine_ranker',
  srcs = [
    'doc_meta.h',
    'cosine_ranker.h',
    'cosine_ranker.cc',
  ],
  deps = [
    ':doc_manager',
    '//base:base',
    '//util/nlp/segmenter:segmenter_manager',
    '//util/nlp/keyword:query_key_word_extractor',
  ],
)

cc_library(
  name = 'ranking_handler',
  srcs = [
    'ranking_handler.h',
    'ranking_handler.cc',
  ],
  deps = [
    '//base:base',
    '//onebox:http_handler',
    '//third_party/jsoncpp:jsoncpp',
  ],
)

cc_library(
  name = 'ranking_thread',
  srcs = [
    'ranking_thread.h',
    'ranking_thread.cc',
  ],
  deps = [
    ':cosine_ranker',
    '//base:base',
    '//third_party/gflags:gflags',
  ],
)
