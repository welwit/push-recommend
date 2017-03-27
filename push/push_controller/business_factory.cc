// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/business_factory.h"

#include "base/log.h"

namespace push_controller {

void BusinessFactory::RegisterBusinessProcessor(BusinessType business_type,
    BaseBusinessProcessor* base_business_processor) {
  auto it = business_factory_map_.find(business_type);
  CHECK(it == business_factory_map_.end());
  business_factory_map_.insert(
      std::make_pair(business_type, base_business_processor));
}

BaseBusinessProcessor*
BusinessFactory::GetBusinessProcessor(BusinessType business_type) {
  auto it = business_factory_map_.find(business_type);
  if (it == business_factory_map_.end()) {
    return NULL;
  } else {
    return it->second;
  }
}

void BusinessFactory::RegisterPushProcessor(BusinessType business_type,
    PushProcessor* push_processor) {
  auto it = push_processor_map_.find(business_type);
  CHECK(it == push_processor_map_.end());
  push_processor_map_.insert(std::make_pair(business_type, push_processor));
}

PushProcessor* BusinessFactory::GetPushProcessor(BusinessType business_type) {
  auto it = push_processor_map_.find(business_type);
  if (it == push_processor_map_.end()) {
    return NULL;
  } else {
    return it->second;
  }
}

}  // namespace push_controller
