// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/util/weather_helper.h"

#include "base/log.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"
#include "util/net/http_client/http_client.h"

#include "push/util/common_util.h"

DECLARE_string(weather_service);

namespace push_controller {

WeatherHelper::WeatherHelper() {}

WeatherHelper::~WeatherHelper() {}

bool WeatherHelper::FetchWeather(const string& city,
                                  WeatherInfo* weather_info) {
  Json::Value response;
  if (!FetchWeather(city, &response)) {
    return false;
  }
  if (response.isMember("status")) {
    if (response["status"] != "success") {
      LOG(ERROR) << "fetch weather failed, status:" << response["status"];
      return false;
    }
    weather_info->set_status(response["status"].asString());
  }
  if (response.isMember("content")) {
    WeatherContent* w_content = weather_info->mutable_content();
    Json::Value& content = response["content"];
    if (content.isMember("confidence")) {
      w_content->set_confidence(content["confidence"].asFloat());
    }
    if (content.isMember("msgId")) {
      w_content->set_msg_id(content["msgId"].asString());
    }
    if (content.isMember("query")) {
      w_content->set_query(content["query"].asString());
    }
    if (content.isMember("searchQuery")) {
      w_content->set_search_query(content["searchQuery"].asString());
    }
    if (content.isMember("task")) {
      w_content->set_task(content["task"].asString());
    }
    if (content.isMember("taskName")) {
      w_content->set_task_name(content["taskName"].asString());
    }
    if (content.isMember("semantic")) {
      Json::Value& semantic = content["semantic"];
      WeatherSemantic* w_semantic = w_content->mutable_semantic();
      if (semantic.isMember("action")) {
        w_semantic->set_action(semantic["action"].asString());
      }
    }
    if (content.isMember("dataSummary")) {
      Json::Value& summary = content["dataSummary"];
      WeatherDataSummary* w_summary = w_content->mutable_data_summary();
      if (summary.isMember("hint")) {
        w_summary->set_hint(summary["hint"].asString());
      }
      if (summary.isMember("title")) {
        w_summary->set_title(summary["title"].asString());
      }
      if (summary.isMember("type")) {
        w_summary->set_type(summary["type"].asString());
      }
    }
    if (content.isMember("data")) {
      Json::Value& data = content["data"];
      if (data.size() == 0) {
        LOG(WARNING) << "No data";
        return false;
      }
      Json::ArrayIndex data_index = 0;
      Json::Value& dataitem = data[data_index];
      WeatherDataItem* w_dataitem = w_content->add_data();
      if (dataitem.isMember("source")) {
        w_dataitem->set_source(dataitem["source"].asString());
      }
      if (dataitem.isMember("type")) {
        w_dataitem->set_type(dataitem["type"].asString());
      }
      if (dataitem.isMember("params")) {
        Json::Value& params = dataitem["params"];
        WeatherParams* w_params = w_dataitem->mutable_params();
        if (params.isMember("tts")) {
          w_params->set_tts(params["tts"].asString());
        }
        if (params.isMember("pageData")) {
          Json::Value& page_datas = params["pageData"];
          for (Json::ArrayIndex index = 0; index < page_datas.size(); ++index) {
            Json::Value& page_data = page_datas[index];
            WeatherPageDataItem* w_page_data = w_params->add_page_datas();
            SetPageData(page_data, w_page_data);
          }
        }
        if (params.isMember("yesterday")) {
          Json::Value& yesterday = params["yesterday"];
          WeatherPageDataItem* w_yesterday = w_params->mutable_yesterday();
          SetPageData(yesterday, w_yesterday);
        }
      }
    }
  }
  VLOG(2) << "weather info: " << ProtoToString(*weather_info);
  return true;
}

bool WeatherHelper::FetchWeather(const string& city,
                                  Json::Value* weather_response) {
  string city_string = ",," + city + ",,,,0,0,";
  Json::Value request;
  request["task"] = "public.weather";
  request["appkey"] = "com.mobvoi.rom";
  request["output"] = "watch";
  request["version"] = "30004";
  request["address"] = city_string;
  string post_data = JsonToString(request);
  LOG(INFO) << "weather request json: " << post_data;
  util::HttpClient http_client;
  http_client.SetHttpMethod(util::HttpMethod::kPost);
  http_client.SetPostData(post_data);
  http_client.AddHeader("Content-Type", "application/json; charset=utf-8");
  if (!http_client.FetchUrl(FLAGS_weather_service)) {
    LOG(ERROR) << "fetch weather url failed";
    return false;
  }
  string response_body = http_client.ResponseBody();
  VLOG(2) << "weather response body:" << response_body;
  Json::Reader reader;
  try {
    reader.parse(response_body, *weather_response);
  } catch (const std::exception& e) {
    LOG(ERROR) << "parse response_body failed:" << e.what();
    return false;
  }
  if ((*weather_response)["status"] != "success") {
    LOG(ERROR) << "response data error";
    return false;
  }
  LOG(INFO) << "weather response json: " << JsonToString(*weather_response);
  return true;
}

void WeatherHelper::TomorrowWeather(const WeatherInfo& weather_info,
                                     vector<string>* weather_results) {
  const WeatherPageDataItem& tomorrow_weather = (
      weather_info.content().data(0).params().page_datas(1));
  weather_results->push_back(tomorrow_weather.weather());
  weather_results->push_back(tomorrow_weather.min_temp());
  weather_results->push_back(tomorrow_weather.max_temp());
}

void WeatherHelper::TodayWeather(const WeatherInfo& weather_info,
                                  vector<string>* weather_results) {
  const WeatherPageDataItem& tomorrow_weather = (
      weather_info.content().data(0).params().page_datas(0));
  weather_results->push_back(tomorrow_weather.weather());
  weather_results->push_back(tomorrow_weather.min_temp());
  weather_results->push_back(tomorrow_weather.max_temp());
}

void WeatherHelper::SetPageData(const Json::Value& page_data,
                                 WeatherPageDataItem* w_page_data) {
  if (page_data.isMember("aqi")) {
    w_page_data->set_aqi(page_data["aqi"].asString());
  }
  if (page_data.isMember("currentTemp")) {
    w_page_data->set_current_temp(page_data["currentTemp"].asString());
  }
  if (page_data.isMember("date")) {
    w_page_data->set_date(page_data["date"].asString());
  }
  if (page_data.isMember("imgUrl")) {
    w_page_data->set_img_url(page_data["imgUrl"].asString());
  }
  if (page_data.isMember("location")) {
    w_page_data->set_location(page_data["location"].asString());
  }
  if (page_data.isMember("maxTemp")) {
    w_page_data->set_max_temp(page_data["maxTemp"].asString());
  }
  if (page_data.isMember("minTemp")) {
    w_page_data->set_min_temp(page_data["minTemp"].asString());
  }
  if (page_data.isMember("pm25")) {
    w_page_data->set_pm25(page_data["pm25"].asString());
  }
  if (page_data.isMember("status")) {
    w_page_data->set_status(page_data["status"].asString());
  }
  if (page_data.isMember("sunrise")) {
    w_page_data->set_sunrise(page_data["sunrise"].asString());
  }
  if (page_data.isMember("sunset")) {
    w_page_data->set_sunset(page_data["sunset"].asString());
  }
  if (page_data.isMember("tips")) {
    w_page_data->set_tips(page_data["tips"].asString());
  }
  if (page_data.isMember("unit")) {
    w_page_data->set_unit(page_data["unit"].asString());
  }
  if (page_data.isMember("weather")) {
    w_page_data->set_weather(page_data["weather"].asString());
  }
  if (page_data.isMember("weatherBg")) {
    w_page_data->set_weather_bg(page_data["weatherBg"].asString());
  }
  if (page_data.isMember("weekDay")) {
    w_page_data->set_week_day(page_data["weekDay"].asString());
  }
  if (page_data.isMember("wind")) {
    w_page_data->set_wind(page_data["wind"].asString());
  }
  if (page_data.isMember("windDir")) {
    w_page_data->set_wind_dir(page_data["windDir"].asString());
  }
}

}  // namespace push_controller
