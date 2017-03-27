// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/util/weather_helper.h"
#include "third_party/gtest/gtest.h"
#include "third_party/gflags/gflags.h"

DEFINE_string(weather_service,
    "https://m.mobvoi.com/search/pc", "weather server addr");

using namespace push_controller;

TEST(WeatherHelper, FetchWeather) {
  WeatherHelper weather_helper;
  std::string city = u8"北京";
  WeatherInfo weather_info;
  EXPECT_TRUE(weather_helper.FetchWeather(city, &weather_info));
}
