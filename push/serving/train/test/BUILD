# Copyright 2016 Mobvoi Inc. All Rights Reserved.
# Author: xjliu@mobvoi.com (Xiaojia Liu)

cc_binary(
  name = 'train_query_test_main',
  srcs = [
    'train_query_test_main.cc',
  ],
  deps = [
    '//base:base',
    '//third_party/jsoncpp',
    '//util/net/http_client:http_client',
  ],
)

cc_binary(
  name = 'fetch_trainno_tool_main',
  srcs = [
    'fetch_trainno_tool_main.cc'
  ],
  deps = [
    '//base:base',                                                         
    '//push/serving/train:train_data_fetcher',                                                         
    '//push/serving/train:trainno_dumper',                                                         
  ],
)

cc_binary(
  name = 'time_table_test_tool_main',
  srcs = [
    'time_table_test_tool_main.cc',
  ],
  deps = [
    '//base:binary_version',
    '//push/serving/train:time_table_dumper',
  ],
)

cc_binary(
  name = 'update_timetable_tool_main',
  srcs = [
    'update_timetable_tool_main.cc'
  ],
  deps = [
    '//push/serving/train:time_table_dumper',
  ],
)
