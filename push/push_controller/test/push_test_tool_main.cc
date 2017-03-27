// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include <time.h>

#include "base/string_util.h"

#include "push/push_controller/flight/flight_push_processor.h"
#include "push/push_controller/movie/movie_push_processor.h"
#include "push/push_controller/train/train_push_processor.h"
#include "push/push_controller/hotel/hotel_push_processor.h"
#include "push/push_controller/push_processor.h"

using namespace push_controller;

DEFINE_bool(use_cluster_mode, false, "");
DEFINE_bool(use_hotel, true, "");
DEFINE_bool(use_version_filter, true, "");

DEFINE_string(mysql_config,
    "config/push/push_controller/mysql_server_test.conf", "");
DEFINE_string(burypoint_kafka_log_server, "http://heartbeat-server/log/realtime", "");
DEFINE_string(burypoint_kafka_topic, "intelligent-push", "");
DEFINE_string(burypoint_upload_push_log_event_type, "test_push_controller_push_suc", "");
DEFINE_string(burypoint_upload_fail_log_event_type, "test_push_controller_push_fail", "");
DEFINE_string(flight_info_service,
    "http://flight-info-service/flight/query_flightinfo", "");
DEFINE_string(train_info_service,
    "http://train-info-service/train/query_timetable",
    "train info service addr");
DEFINE_string(link_server,
    "http://link-server/api/push_message/to_user_device", "link server addr");
DEFINE_string(weather_service,
    "https://m.mobvoi.com/search/pc", "weather server addr");
DEFINE_string(location_service, "http://location-service/geocoder?address=%s", "");
DEFINE_string(hotel_version_equal, "tic_4.9.0", "");
DEFINE_string(hotel_version_greater, "tic_4.9.0", "");

struct TestDataInfo {
  PushEventInfo push_event_info;
  UserOrderInfo user_order_info;
  vector<train::TimeTable> time_tables;
  flight::FlightResponse flight_response;
  WeatherInfo weather_info;
  string message_desc;
};

class MoviePushTest {
 public:
  MoviePushTest() {
    Init();
  }
  ~MoviePushTest() {}
  void Init() {
    string user_id = "69322dfbad6c90d9d008095ff3967227";
    string order_id;
    string movie = "佩小姐的奇幻城堡";
    string cinema = "昆明百老汇影城顺城店";
    string seat_list = "7排5座,7排4座";
    string verification_code = "410810538";
    time_t updated_time = time(NULL);
    time_t push_time = updated_time - 10;
    time_t business_time = 0;
    for (int i = 0; i < 2; ++i) {
      TestDataInfo test_data_info;
      PushEventInfo& event = test_data_info.push_event_info;
      UserOrderInfo& user = test_data_info.user_order_info;
      UserMovieInfo* user_movie = (
          user.mutable_business_info()->mutable_user_movie_info());
      string order_id = StringPrintf("%s-%d", user_id.c_str(), i);
      string event_id = StringPrintf("%s-%d", order_id.c_str(), i);
      event.set_id(event_id);
      event.set_order_id(order_id);
      event.set_user_id(user_id);
      event.set_business_type(kBusinessMovie);
      if (i == 0) {
        event.set_event_type(kEventRemindTimelineToday);
        business_time = push_time + 3600;
        test_data_info.message_desc = "movie-today";
      } else {
        event.set_event_type(kEventRemindTimeline30Min);
        business_time = push_time + 1800;
        test_data_info.message_desc = "movie-30min";
      }
      event.set_push_time(push_time);
      event.set_business_key(movie);
      event.set_is_realtime(false);
      event.set_push_status(kPushProcessing);
      event.set_updated(updated_time);
      event.set_finished_time(updated_time);
      user.set_id(order_id);
      user.set_user_id(user_id);
      user.set_business_type(kBusinessMovie);
      user.set_business_time(business_time);
      user.set_updated(updated_time);
      user.set_order_status(kOrderInitialization);
      user.set_finished_time(updated_time);
      user_movie->set_user_id(user_id);
      user_movie->set_order_id(order_id);
      user_movie->set_cinema(cinema);
      user_movie->set_movie(movie);
      user_movie->set_seat_list(seat_list);
      user_movie->set_show_time(business_time);
      user_movie->set_verification_code(verification_code);
      test_data_info_vector_.push_back(test_data_info);
    }
  }

  vector<TestDataInfo> test_data_info_vector_;
};

class TrainPushTest {
 public:
  TrainPushTest() {
    Init();
  }
  ~TrainPushTest() {}
  void Init() {
    string user_id = "69322dfbad6c90d9d008095ff3967227";
    string order_id_prefix = "E87000001";
    string train_no = "G7014";
    string seat_list = "6车14D号";
    string depart_station = "苏州";
    time_t updated_time = time(NULL);
    time_t push_time = updated_time - 10;
    time_t business_time = 0;
    for (int i = 0; i < 4; ++i) {
      TestDataInfo test_data_info;
      PushEventInfo& event = test_data_info.push_event_info;
      UserOrderInfo& user = test_data_info.user_order_info;
      UserTrainInfo* user_train = (
          user.mutable_business_info()->mutable_user_train_info());
      string order_id = StringPrintf("%s%d", order_id_prefix.c_str(), i);
      string event_id = StringPrintf("%s-%d", order_id.c_str(), i);
      event.set_id(event_id);
      event.set_order_id(order_id);
      event.set_user_id(user_id);
      event.set_business_type(kBusinessTrain);
      if (i == 0) {
        event.set_event_type(kEventRemindTimelineLastDay);
        business_time = push_time + 10 * 3600;
        test_data_info.message_desc = "train-lastday";
      } else if (i == 1) {
        event.set_event_type(kEventRemindTimelineToday);
        business_time = push_time + 5 * 3600;
        test_data_info.message_desc = "train-today";
      } else if (i == 2) {
        event.set_event_type(kEventRemindTimeline3Hour);
        business_time = push_time + 3 * 3600;
        test_data_info.message_desc = "train-3hour";
      } else {
        event.set_event_type(kEventRemindTimeline1Hour);
        business_time = push_time + 1 * 3600;
        test_data_info.message_desc = "train-1hour";
      }
      event.set_push_time(push_time);
      event.set_business_key(train_no);
      event.set_is_realtime(false);
      event.set_push_status(kPushProcessing);
      event.set_updated(updated_time);
      event.set_finished_time(updated_time);
      user.set_id(order_id);
      user.set_user_id(user_id);
      user.set_business_type(kBusinessTrain);
      user.set_business_time(business_time);
      user.set_updated(updated_time);
      user.set_order_status(kOrderInitialization);
      user.set_finished_time(updated_time);
      user_train->set_user_id(user_id);
      user_train->set_order_id(order_id);
      user_train->set_train_no(train_no);
      user_train->set_depart_station(depart_station);
      user_train->set_depart_time(business_time);
      user_train->set_seat_list(seat_list);
      test_data_info_vector_.push_back(test_data_info);
    }
  }

  vector<TestDataInfo> test_data_info_vector_;
};

class FlightPushTest {
 public:
  FlightPushTest() {
    Init();
  }
  ~FlightPushTest() {}
  void Init() {
    string user_id = "69322dfbad6c90d9d008095ff3967227";
    string order_id_prefix = "987-913860002";
    string flight_no = "MU2852";
    time_t updated_time = time(NULL);
    time_t push_time = updated_time - 10;
    time_t business_time = 0;
    time_t takeoff_time = 0;
    time_t arrival_time = 0;
    for (int i = 0; i < 6; ++i) {
      TestDataInfo test_data_info;
      PushEventInfo& event = test_data_info.push_event_info;
      UserOrderInfo& user = test_data_info.user_order_info;
      UserFlightInfo* user_flight = (
          user.mutable_business_info()->mutable_user_flight_info());
      string order_id = StringPrintf("%s%d", order_id_prefix.c_str(), i);
      string event_id = StringPrintf("%s-%d", order_id.c_str(), i);
      event.set_id(event_id);
      event.set_order_id(order_id);
      event.set_user_id(user_id);
      event.set_business_type(kBusinessFlight);
      if (i == 0) {
        event.set_event_type(kEventRemindTimelineLastDay);
        business_time = push_time + 10 * 3600;
        test_data_info.message_desc = "flight-lastday";
      } else if (i == 1) {
        event.set_event_type(kEventRemindTimelineToday);
        business_time = push_time + 5 * 3600;
        test_data_info.message_desc = "flight-today";
      } else if (i == 2) {
        event.set_event_type(kEventRemindTimeline3Hour);
        business_time = push_time + 3 * 3600;
        test_data_info.message_desc = "flight-3hour";
      } else if (i == 3) {
        event.set_event_type(kEventRemindTimeline1Hour);
        business_time = push_time + 1 * 3600;
        test_data_info.message_desc = "flight-1hour";
      } else if (i == 4) {
        event.set_event_type(kEventRemindTimelineTakeoff);
        business_time = push_time;
        test_data_info.message_desc = "flight-takeoff";
      } else {
        event.set_event_type(kEventRemindTimelineArrive);
        business_time = push_time - 2 * 3600;
        test_data_info.message_desc = "flight-arrive";
      }
      takeoff_time = business_time;
      arrival_time = business_time + 2 * 3600;
      event.set_push_time(push_time);
      event.set_business_key(flight_no);
      event.set_is_realtime(false);
      event.set_push_status(kPushProcessing);
      event.set_updated(updated_time);
      event.set_finished_time(updated_time);
      user.set_id(order_id);
      user.set_user_id(user_id);
      user.set_business_type(kBusinessFlight);
      user.set_business_time(business_time);
      user.set_updated(updated_time);
      user.set_order_status(kOrderInitialization);
      user.set_finished_time(updated_time);
      user_flight->set_user_id(user_id);
      user_flight->set_order_id(order_id);
      user_flight->set_flight_no(flight_no);
      user_flight->set_takeoff_time(takeoff_time);
      user_flight->set_arrival_time(arrival_time);
      test_data_info_vector_.push_back(test_data_info);
    }
  }

  vector<TestDataInfo> test_data_info_vector_;
};

class HotelPushTest {
 public:
  HotelPushTest() {
    Init();
  }
  ~HotelPushTest() {}
  void Init() {
    //string user_id = "69322dfbad6c90d9d008095ff3967227";
    //string user_id = "446cd65a0b216c8dbdbb33df7de4c02f";
    string user_id = "e6cfd6189edee258aec8f371e426d917";
    string order_id;
    string address = "麻涌镇东太村麻涌大道东太段1号，近东兴路";
    string tel = "076983813888";
    string name = "赵韵添";
    string room_category = "（东莞麻涌店）双床房";
    string time_interval = "1间1晚";
    string hotel_name = "八方连锁酒店";
    time_t updated_time = time(NULL);
    time_t push_time = updated_time - 10;
    time_t arrive_time = push_time;
    time_t business_time = 0;
    for (int i = 0; i < 1; ++i) {
      TestDataInfo test_data_info;
      PushEventInfo& event = test_data_info.push_event_info;
      UserOrderInfo& user = test_data_info.user_order_info;
      UserHotelInfo* user_hotel = (
          user.mutable_business_info()->mutable_user_hotel_info());
      string order_id = StringPrintf("%s-%d", user_id.c_str(), i);
      string event_id = StringPrintf("%s-%d", order_id.c_str(), i);
      event.set_id(event_id);
      event.set_order_id(order_id);
      event.set_user_id(user_id);
      event.set_business_type(kBusinessHotel);
      if (i == 0) {
        event.set_event_type(kEventRemindTimelineToday);
        business_time = push_time + 3600;
        test_data_info.message_desc = "hotel-today";
      } else {
        continue;
      }
      event.set_push_time(push_time);
      event.set_business_key(name);
      event.set_is_realtime(false);
      event.set_push_status(kPushProcessing);
      event.set_updated(updated_time);
      event.set_finished_time(updated_time);
      user.set_id(order_id);
      user.set_user_id(user_id);
      user.set_business_type(kBusinessMovie);
      user.set_business_time(business_time);
      user.set_updated(updated_time);
      user.set_order_status(kOrderInitialization);
      user.set_finished_time(updated_time);
      user_hotel->set_user_id(user_id);
      user_hotel->set_order_id(order_id);
      user_hotel->set_address(address);
      user_hotel->set_tel(tel);
      user_hotel->set_name(name);
      user_hotel->set_arrive_time(arrive_time);
      user_hotel->set_room_category(room_category);
      user_hotel->set_time_interval(time_interval);
      user_hotel->set_hotel_name(hotel_name);
      test_data_info_vector_.push_back(test_data_info);
    }
  }

  vector<TestDataInfo> test_data_info_vector_;
};

int main(int argc, char** argv) {
  PushSender push_sender;
  MoviePushProcessor movie_push_processor;
  MoviePushTest movie_push_test;
  for (int i = 1; i < 2; ++i) {
    Json::Value message;
    TestDataInfo& data = movie_push_test.test_data_info_vector_[i];
    CHECK(movie_push_processor.BuildPushMessage(data.user_order_info,
                                                data.push_event_info,
                                                &message));
    CHECK(push_sender.SendPush(kMessageMovie,
                               data.message_desc,
                               data.user_order_info.user_id(),
                               message));
  }
  TrainPushProcessor train_push_processor;
  TrainPushTest train_push_test;
  for (int i = 2; i < 3; ++i) {
    Json::Value message;
    TestDataInfo& data = train_push_test.test_data_info_vector_[i];
    CHECK(train_push_processor.GetTimeTable(data.user_order_info,
                                            &data.time_tables));
    CHECK(train_push_processor.BuildPushMessage(data.user_order_info,
                                                data.push_event_info,
                                                data.time_tables,
                                                &message));
    CHECK(push_sender.SendPush(kMessageTrain,
                               data.message_desc,
                               data.user_order_info.user_id(),
                               message));
  }
  FlightPushProcessor flight_push_processor;
  FlightPushTest flight_push_test;
  for (int i = 2; i < 3; ++i) {
    Json::Value message;
    TestDataInfo& data = flight_push_test.test_data_info_vector_[i];
    CHECK(flight_push_processor.GetFlightResponse(data.user_order_info,
                                                  &data.flight_response));
    CHECK(flight_push_processor.FetchWeather(data.flight_response,
                                             &data.weather_info));
    bool ret = flight_push_processor.BuildPushMessage(data.user_order_info,
                                                      data.push_event_info,
                                                      data.flight_response,
                                                      data.weather_info,
                                                      &message);
    if (!ret) {
      continue;
    }
    CHECK(push_sender.SendPush(kMessageFlight,
                               data.message_desc,
                               data.user_order_info.user_id(),
                               message));
  }
  HotelPushProcessor hotel_push_processor;
  HotelPushTest hotel_push_test;
  for (int i = 0; i < 1; ++i) {
    Json::Value message;
    TestDataInfo& data = hotel_push_test.test_data_info_vector_[i];
    bool ret = hotel_push_processor.BuildPushMessage(data.user_order_info,
                                                     data.push_event_info,
                                                     &message);
    if (!ret) {
      continue;
    }
    CHECK(push_sender.SendPush(kMessageGeneral,
                               data.message_desc,
                               data.user_order_info.user_id(),
                               message));
  }
  return 0;
}
