// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef RECOMMENDATION_NEWS_RECOMMENDER_CATEGORY_HELPER_H_
#define RECOMMENDATION_NEWS_RECOMMENDER_CATEGORY_HELPER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/singleton.h"

#include "recommendation/news/proto/news_meta.pb.h"

namespace recommendation {

class CategoryHelper {
 public:
  ~CategoryHelper();
  const std::map<int, ToutiaoCategoryInfo>* GetCategoryMap() {
    return &category_map_;
  }

 private:
  CategoryHelper();
  bool LoadFromFile();
  friend struct DefaultSingletonTraits<CategoryHelper>;

  std::map<int, ToutiaoCategoryInfo> category_map_;
  DISALLOW_COPY_AND_ASSIGN(CategoryHelper);
};

}  // namespace recommendation_news

#endif  // RECOMMENDATION_NEWS_RECOMMENDER_CATEGORY_HELPER_H_
