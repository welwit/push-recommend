// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/recommender/category_helper.h"

#include "base/file/simple_line_reader.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"

#include "push/util/common_util.h"

DECLARE_string(category_file);

namespace recommendation {

CategoryHelper::CategoryHelper() {
  CHECK(LoadFromFile());
}

CategoryHelper::~CategoryHelper() {}

bool CategoryHelper::LoadFromFile() {
  vector<string> lines;
  file::SimpleLineReader reader(FLAGS_category_file);
  if (!reader.ReadLines(&lines)) {
    return false;
  }
  for (size_t i = 0; i < lines.size(); ++i) {
    vector<string> columns;
    SplitString(lines[i], '\t', &columns);
    ToutiaoCategoryInfo info;
    info.set_id(StringToInt(columns[0]));
    info.set_name(columns[1]);
    info.set_english_name(columns[2]);
    category_map_[info.id()] = info;
  }
  for (auto it = category_map_.begin(); it != category_map_.end(); ++it) {
    VLOG(2) << "category_info, index:" << it->first << ", info:"
            << push_controller::ProtoToString(it->second);
  }
  return true;
}

}  // namespace recommendation
