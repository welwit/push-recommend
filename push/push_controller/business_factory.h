// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_BUSINESS_FACTORY_H_
#define PUSH_PUSH_CONTROLLER_BUSINESS_FACTORY_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/singleton.h"

#include "push/push_controller/business_processor.h"
#include "push/push_controller/push_processor.h"
#include "push/proto/push_meta.pb.h"

namespace push_controller {

class BusinessFactory {
 public:
  typedef map<BusinessType, BaseBusinessProcessor*> BusinessFactoryMapType;
  typedef map<BusinessType, PushProcessor*> PushProcessorMapType;

  void RegisterBusinessProcessor(BusinessType business_type,
      BaseBusinessProcessor* base_business_processor);
  BaseBusinessProcessor* GetBusinessProcessor(BusinessType business_type);

  void RegisterPushProcessor(BusinessType business_type,
      PushProcessor* push_processor);
  PushProcessor* GetPushProcessor(BusinessType business_type);

 private:
  BusinessFactory() {}
  friend struct DefaultSingletonTraits<BusinessFactory>;

  BusinessFactoryMapType business_factory_map_;
  PushProcessorMapType push_processor_map_;
};


} // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_BUSINESS_FACTORY_H_
