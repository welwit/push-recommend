// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include <iostream>
#include <string>

#include "base/basictypes.h"
#include "base/compat.h"
#include "third_party/gtest/gtest.h"
#include "third_party/gflags/gflags.h"
#include "recommendation/news/crawler/util.h"

DEFINE_int32(fetch_timeout_ms, 5000, "");

namespace recommendation {

TEST(SplitVectorTest, NormalInput) {
  vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 7, 8, 8, 9, 10, 20, 21, 22, 23, 24, 
                     25, 26, 27, 28, 29, 30, 31, 50, 100, 40, 39,
  };
  vector<vector<int>> result_vec_vec;
  result_vec_vec = SplitVector(vec, 5);
  EXPECT_TRUE(result_vec_vec.size() == 5);
  vector<int> out_vec;
  MergeVector(result_vec_vec, &out_vec);
  EXPECT_EQ(vec.size(), out_vec.size());
}

}  // namespace recommendation
