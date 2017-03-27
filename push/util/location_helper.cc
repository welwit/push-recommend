// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/util/location_helper.h"

#include "base/string_util.h"
#include "push/util/common_util.h"
#include "third_party/gflags/gflags.h"
#include "util/net/http_client/http_client.h"

DECLARE_string(location_service);

namespace recommendation {

LocationHelper::LocationHelper() {}

LocationHelper::~LocationHelper() {}


bool LocationHelper::GetLocationInfo(const string& latitude,
                                     const string& longitude,
                                     LocationInfo* location_info) {
  string url = StringPrintf(FLAGS_location_service.c_str(),
                            latitude.c_str(), longitude.c_str());
  util::HttpClient http_client;
  if (!http_client.FetchUrl(url) || http_client.response_code() != 200) {
    LOG(ERROR) << "Fetch location failed, latitude:"
               << latitude << ", longitude:" << longitude;
    return false;
  }
  Json::Value result;
  Json::Reader reader;
  reader.parse(http_client.ResponseBody(), result);
  if (result["country"].empty() || result["province"].empty() ||
      result["city"].empty() || result["sublocality"].empty()) {
    LOG(WARNING) << "BAD result:" << push_controller::JsonToString(result);
    return false;
  }
  location_info->set_country(result["country"].asString());
  location_info->set_province(result["province"].asString());
  location_info->set_city(result["city"].asString());
  location_info->set_sublocality(result["sublocality"].asString());
  location_info->set_street(result["street"].asString());
  location_info->set_distance(result["distance"].asString());
  location_info->set_country_code(result["country_code"].asDouble());
  location_info->set_adcode(result["adcode"].asString());
  location_info->set_street_number(result["street_number"].asString());
  location_info->set_lat(result["lat"].asDouble());
  location_info->set_lng(result["lng"].asDouble());
  return true;
}

}  // namespace recommendation
