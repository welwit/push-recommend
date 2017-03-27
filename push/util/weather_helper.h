// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_WEATHER_HELPER_H_
#define PUSH_PUSH_CONTROLLER_WEATHER_HELPER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "third_party/jsoncpp/json.h"

#include "push/proto/weather_meta.pb.h"

namespace push_controller {

class WeatherHelper {
 public:
  WeatherHelper();
  ~WeatherHelper();
  bool FetchWeather(const string& city,
                    WeatherInfo* weather_info);
  bool FetchWeather(const string& city,
                    Json::Value* weather_response);
  void TomorrowWeather(const WeatherInfo& weather_info,
                       vector<string>* weather_results);
  void TodayWeather(const WeatherInfo& weather_info,
                    vector<string>* weather_results);

 private:
  void SetPageData(const Json::Value& page_data,
                   WeatherPageDataItem* w_page_data);
  DISALLOW_COPY_AND_ASSIGN(WeatherHelper);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_WEATHER_HELPER_H_
