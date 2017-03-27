// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_UTIL_LOCATION_HELPER_H_
#define PUSH_UTIL_LOCATION_HELPER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/singleton.h"

#include "push/proto/user_device_meta.pb.h"

namespace recommendation {

class LocationHelper {
 public:
  ~LocationHelper();
  bool GetLocationInfo(const string& latitude,
                       const string& longitude,
                       LocationInfo* location_info);
 private:
  friend struct DefaultSingletonTraits<LocationHelper>;
  LocationHelper();
  DISALLOW_COPY_AND_ASSIGN(LocationHelper);
};

}  // namespace recommendation

#endif  // PUSH_UTIL_LOCATION_HELPER_H_
