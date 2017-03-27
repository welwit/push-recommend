package(default_visibility = ['//visibility:public'])

load("//third_party/protobuf:protobuf.bzl", "cc_proto_library")

cc_proto_library(
  name = 'news_meta_proto',
  srcs = ['news_meta.proto'],
)

cc_proto_library(
  name = 'topic_meta_proto',
  srcs = ['topic_meta.proto'],
)

cc_proto_library(
  name = 'crawler_config_meta_proto',
  srcs = ['crawler_config_meta.proto'],
  deps = [':topic_meta_proto'],
  use_grpc_plugin = True,
)
