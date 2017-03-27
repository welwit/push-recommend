// Copyright 2016. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include <memory>

#include "base/at_exit.h"
#include "base/binary_version.h"
#include "base/log.h"
#include "base/singleton.h"
#include "third_party/gflags/gflags.h"
#include "util/net/http_server/http_handler.h"
#include "util/net/http_server/http_server.h"

#include "push/push_controller/business_factory.h"
#include "push/push_controller/business_processor.h"
#include "push/push_controller/flight/flight_push_processor.h"
#include "push/push_controller/hotel/hotel_push_processor.h"
#include "push/push_controller/movie/movie_push_processor.h"
#include "push/push_controller/train/train_push_processor.h"
#include "push/push_controller/push_processor.h"
#include "push/push_controller/push_scheduler.h"
#include "push/push_controller/push_pool_updater.h"
#include "push/push_controller/push_controller_handler.h"
#include "push/proto/push_meta.pb.h"
#include "push/util/zookeeper_util.h"

DEFINE_bool(is_test, true, "");
DEFINE_bool(use_cluster_mode, false, "");
DEFINE_bool(use_hotel, false, "");
DEFINE_bool(use_version_filter, false, "");
DEFINE_bool(use_channel_filter, false, "");
DEFINE_bool(enable_load_devices, false, "");
DEFINE_bool(enable_schedule_push, false, "");
DEFINE_bool(enable_update_push_pool, false, "");

DEFINE_int32(listen_port, 9048, "");
DEFINE_int32(http_server_thread_num, 8, "");

DEFINE_int32(valid_push_time_internal, 1200, "valid push_time internal");
DEFINE_int32(pool_update_internal, 120, "push pool update schedule internal");
DEFINE_int32(push_scheduler_internal, 180, "push scheduler time internal");

DEFINE_int32(zookeeper_timeout, 3000, "(In MS)");
DEFINE_int32(zookeeper_reconnect_attempt, 5, "");
DEFINE_int32(zookeeper_check_interval, 5, "(In Seconds)");
DEFINE_int32(zookeeper_default_node_port, 9048, "");

DEFINE_int32(recommendation_content_expire_seconds, 3 * 24 * 3600, "");

DEFINE_int32(db_batch_query_size, 100, "");
DEFINE_int32(mysql_page_size, 10000, "");
DEFINE_int32(redis_expire_time, 48 * 60 * 60, "redis expire time internal");
DEFINE_int32(redis_max_sleep, 128, "redis reconnect max sleep time");

DEFINE_string(user_feedback_service,
    "http://user-feedback-service/query_feedback", "user feedback url");
DEFINE_string(flight_info_service,
    "http://flight-info-service/flight/query_flightinfo", "");
DEFINE_string(train_info_service,
    "http://train-info-service/train/query_timetable", "");

DEFINE_string(location_service,
    "http://location-service/geocoder?address=%s", "");

DEFINE_string(weather_service, "https://m.mobvoi.com/search/pc", "");

DEFINE_string(link_server,
    "http://link-server/api/push_message/to_user_device", "link server addr");

DEFINE_string(recommender_server,
    "http://news-recommender-server-main/news/recommender/recommendation?user_id=%s", "");

DEFINE_string(burypoint_kafka_log_server, "http://heartbeat-server/log/realtime", "");
DEFINE_string(burypoint_kafka_topic, "intelligent-push", "");
DEFINE_string(burypoint_upload_push_log_event_type, "test_push_controller_push_suc", "");
DEFINE_string(burypoint_upload_fail_log_event_type, "test_push_controller_push_fail", "");

DEFINE_string(zookeeper_cluster_addr, "ali-hz-dev:2181", "");
DEFINE_string(zookeeper_this_node_prefix, "/ns/intelligent_push/push_controller/replica_", "");
DEFINE_string(zookeeper_watched_path, "/ns/intelligent_push/push_controller", "");

DEFINE_string(mysql_config,
    "config/push/push_controller/mysql_server_test.conf", "");

DEFINE_string(redis_conf, "config/onebox/news/redis.conf", "redis config");
DEFINE_string(redis_sub_topic, "push_controller", "redis subscribe topic");

DEFINE_string(device_test, "a960d251aa89828eb49c8ec701efb6bd,b5ae7f699b22135628d14fecbe19d38d", "");

DEFINE_string(news_version_equal, "tic_4.9.0", "");
DEFINE_string(news_version_greater, "tic_4.9.0", "");
DEFINE_string(hotel_version_equal, "tic_4.9.0", "");
DEFINE_string(hotel_version_greater, "tic_4.9.0", "");

using namespace push_controller;

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  mobvoi::SetupBinaryVersion();
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::InitGoogleLogging(argv[0]);

  recommendation::ZkManager* zk_manager =
    Singleton<recommendation::ZkManager>::get();
  zk_manager->set_is_register_this_node(true);
  zk_manager->AddWatchPath(FLAGS_zookeeper_watched_path);
  zk_manager->Start();

  // singleton of BusinessFactory
  BusinessFactory* business_factory = Singleton<BusinessFactory>::get();
  // new and register business processor
  BaseBusinessProcessor* train_business_processor = (
      new TrainBusinessProcessor());
  business_factory->RegisterBusinessProcessor(
      BusinessType::kBusinessTrain, train_business_processor);
  BaseBusinessProcessor* flight_business_processor = (
      new FlightBusinessProcessor());
  business_factory->RegisterBusinessProcessor(
      BusinessType::kBusinessFlight, flight_business_processor);
  BaseBusinessProcessor* movie_business_processor = (
      new MovieBusinessProcessor());
  business_factory->RegisterBusinessProcessor(
      BusinessType::kBusinessMovie, movie_business_processor);
  BaseBusinessProcessor* hotel_business_processor = (
      new HotelBusinessProcessor());
  business_factory->RegisterBusinessProcessor(
      BusinessType::kBusinessHotel, hotel_business_processor);

  // new and register push processor
  PushProcessor* train_push_processor = new TrainPushProcessor();
  business_factory->RegisterPushProcessor(
      BusinessType::kBusinessTrain, train_push_processor);
  PushProcessor* flight_push_processor = new FlightPushProcessor();
  business_factory->RegisterPushProcessor(
      BusinessType::kBusinessFlight, flight_push_processor);
  PushProcessor* movie_push_processor = new MoviePushProcessor();
  business_factory->RegisterPushProcessor(
      BusinessType::kBusinessMovie, movie_push_processor);
  PushProcessor* hotel_push_processor = new HotelPushProcessor();
  business_factory->RegisterPushProcessor(
      BusinessType::kBusinessHotel, hotel_push_processor);

  // make shared worker thread class
  std::shared_ptr<PushPoolUpdater> push_pool_updater = (
      std::make_shared<PushPoolUpdater>());
  std::shared_ptr<PushScheduler> push_scheduler = (
      std::make_shared<PushScheduler>());

  push_scheduler->Start();
  push_pool_updater->Start();
  train_push_processor->Start();
  flight_push_processor->Start();
  movie_push_processor->Start();
  hotel_push_processor->Start();

  LOG(INFO) << "start push controller server ...";
  util::HttpServer http_server(FLAGS_listen_port,
      static_cast<size_t>(FLAGS_http_server_thread_num));

  serving::StatusHandler status_handler;
  serving::NewsPushHandler news_push_handler;

  auto status_callback = std::bind(
      &serving::StatusHandler::HandleRequest,
      &status_handler,
      std::placeholders::_1,
      std::placeholders::_2);
  auto news_push_callback = std::bind(
      &serving::NewsPushHandler::HandleRequest,
      &news_push_handler,
      std::placeholders::_1,
      std::placeholders::_2);

  util::DefaultHttpHandler status_http_handler(status_callback);
  util::DefaultHttpHandler news_push_http_handler(news_push_callback);

  http_server.RegisterHttpHandler("/push_controller/status",
                                  &status_http_handler);
  http_server.RegisterHttpHandler("/push_controller/news_push",
                                  &news_push_http_handler);

  LOG(INFO) << "push controller is started";
  http_server.Serv();

  push_pool_updater->Join();
  push_scheduler->Join();
  train_push_processor->Join();
  flight_push_processor->Join();
  movie_push_processor->Join();
  hotel_push_processor->Join();

  return 0;
}
