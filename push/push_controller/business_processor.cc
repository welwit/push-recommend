// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/business_processor.h"

#include "base/compat.h"
#include "base/file/proto_util.h"
#include "base/log.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"
#include "third_party/jsoncpp/json.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"

#include "push/util/time_util.h"
#include "push/proto/push_meta.pb.h"

DECLARE_bool(use_cluster_mode);
DECLARE_string(mysql_config);

namespace {

using namespace push_controller;

static const char kTable[] = "push_event_info";

static const char kInsertFormat[] =
    "INSERT INTO %s (id, order_id, user_id, business_type, event_type, "
    "push_time, business_key, is_realtime, push_status) "
    "VALUES('%s', '%s', '%s', '%d', '%d', "
    "FROM_UNIXTIME('%d'), '%s', '%d', '%d');";

static const char kInsertFormatV2[] =
    "INSERT INTO %s (id, order_id, user_id, business_type, event_type, "
    "push_time, business_key, is_realtime, push_status, fingerprint_id) "
    "VALUES('%s', '%s', '%s', '%d', '%d', "
    "FROM_UNIXTIME('%d'), '%s', '%d', '%d', '%lu');";

static const char kQueryFormat[] =
    "SELECT id, order_id, user_id, business_type, event_type, push_detail, "
    "push_time, business_key, is_realtime, push_status, updated, "
    "finished_time "
    "FROM %s WHERE id = '%s';";

static const char kQueryFormatV2[] =
    "SELECT id, order_id, user_id, business_type, event_type, push_detail, "
    "push_time, business_key, is_realtime, push_status, updated, "
    "finished_time, fingerprint_id "
    "FROM %s WHERE id = '%s';";

static const char kUpdateFormat[] =
    "UPDATE %s SET push_time = FROM_UNIXTIME('%d'), business_key = '%s' "
    "WHERE id = '%s';";

enum TimeRelationType {
  kIsRelativeTime = 1,
  kIsFixedTime = 2,
};

struct EventTimeRelation {
  EventType event_type;
  TimeRelationType relation_type;
  int32_t relative_seconds;
  vector<int32_t> fixed_hours;
};

static const EventTimeRelation kTrainEventTimeRelation[] = {
  {kEventRemindTimelineLastDay, kIsFixedTime, -86400, {7}},
  {kEventRemindTimelineToday, kIsFixedTime, 0, {7}},
  {kEventRemindTimeline3Hour, kIsRelativeTime, -10800, {}},
  {kEventRemindTimeline1Hour, kIsRelativeTime, -3600, {}}
};

static const EventTimeRelation kFlightEventTimeRelation[] = {
  {kEventRemindTimelineLastDay, kIsFixedTime, -86400, {7}},
  {kEventRemindTimelineToday, kIsFixedTime, 0, {7}},
  {kEventRemindTimeline3Hour, kIsRelativeTime, -10800, {}},
  {kEventRemindTimeline1Hour, kIsRelativeTime, -3600, {}},
  {kEventRemindTimelineTakeoff, kIsRelativeTime, 0, {}},
  {kEventRemindTimelineArrive, kIsRelativeTime, 3600, {}}
};

static const EventTimeRelation kMovieEventTimeRelation[] = {
  {kEventRemindTimelineToday, kIsFixedTime, 0, {8}},
  {kEventRemindTimeline30Min, kIsRelativeTime, -1800, {}}
};

static const EventTimeRelation kHotelEventTimeRelation[] = {
  {kEventRemindTimelineLastDay, kIsFixedTime, -86400, {12}},
  {kEventRemindTimelineToday, kIsFixedTime, 0, {10}},
};

}

namespace push_controller {

BaseBusinessProcessor::BaseBusinessProcessor() {
  VLOG(1) << "BaseBusinessProcessor::BaseBusinessProcessor()";
  mysql_server_.reset(new MysqlServer());
  CHECK(file::ReadProtoFromTextFile(FLAGS_mysql_config, mysql_server_.get()));
  VLOG(1) << "Read MySQLConf from file:" << FLAGS_mysql_config;
}

BaseBusinessProcessor::~BaseBusinessProcessor() {}

bool BaseBusinessProcessor::UpdateEventsToDb(
    const vector<PushEventInfo>& push_events) {
  VLOG(2) << "BaseBusinessProcessor::UpdateEventsToDb()";
  try {
    sql::Driver* driver = sql::mysql::get_driver_instance();
    std::unique_ptr<sql::Connection>
        connection(driver->connect(
            mysql_server_->host(),
            mysql_server_->user(),
            mysql_server_->password()));
    connection->setSchema(mysql_server_->database());
    std::unique_ptr<sql::Statement> statement(connection->createStatement());
    string insert_sql, select_sql, update_sql;
    int number_of_insert = 0, number_of_update = 0;
    for (auto& push_event : push_events) {
      const string& id = push_event.id();
      if (FLAGS_use_cluster_mode) {
        select_sql = StringPrintf(kQueryFormatV2, kTable, id.c_str());
      } else {
        select_sql = StringPrintf(kQueryFormat, kTable, id.c_str());
      }
      std::unique_ptr<sql::ResultSet> result_set(
          statement->executeQuery(select_sql));
      if (result_set->rowsCount() > 0) {
        CHECK(result_set->rowsCount() == 1);
        PushEventInfo existed_push_event;
        while (result_set->next()) {
          existed_push_event.set_id(result_set->getString("id"));
          existed_push_event.set_order_id(result_set->getString("order_id"));
          existed_push_event.set_user_id(result_set->getString("user_id"));
          existed_push_event.set_business_type(
              static_cast<BusinessType>(result_set->getInt("business_type")));
          existed_push_event.set_event_type(
              static_cast<EventType>(result_set->getInt("event_type")));
          if (FLAGS_use_cluster_mode) {
            existed_push_event.set_fingerprint_id(
                result_set->getInt("fingerprint_id"));
          }
          break;
        }
        if (!CheckAttribute(existed_push_event, push_event)) {
          continue;
        }
        update_sql = StringPrintf(kUpdateFormat,
                                  kTable,
                                  push_event.push_time(),
                                  push_event.business_key().c_str(),
                                  push_event.id().c_str());
        statement->executeUpdate(update_sql);
        ++number_of_update;
      } else {
        if (FLAGS_use_cluster_mode) {
          insert_sql = StringPrintf(kInsertFormatV2,
              kTable,
              push_event.id().c_str(),
              push_event.order_id().c_str(),
              push_event.user_id().c_str(),
              push_event.business_type(),
              push_event.event_type(),
              push_event.push_time(),
              push_event.business_key().c_str(),
              push_event.is_realtime(),
              push_event.push_status(),
              push_event.fingerprint_id());
        } else {
          insert_sql = StringPrintf(kInsertFormat,
              kTable,
              push_event.id().c_str(),
              push_event.order_id().c_str(),
              push_event.user_id().c_str(),
              push_event.business_type(),
              push_event.event_type(),
              push_event.push_time(),
              push_event.business_key().c_str(),
              push_event.is_realtime(),
              push_event.push_status());
        }
        statement->executeUpdate(insert_sql);
        ++number_of_insert;
      }
    }
    LOG(INFO) << "update event to db success, insert num:"
              << number_of_insert << ", update num:" << number_of_update;
    return true;
  } catch (const sql::SQLException &e) {
    LOG(ERROR) << "SQLException: " << e.what();
    return false;
  }
}

void BaseBusinessProcessor::CreateId(PushEventInfo* push_event_info) {
  string event_id = StringPrintf("%s-%d",
      push_event_info->order_id().c_str(),
      static_cast<int32_t>(push_event_info->event_type()));
  push_event_info->set_id(event_id);
}

bool BaseBusinessProcessor::CheckAttribute(
    const PushEventInfo& push_event_left,
    const PushEventInfo& push_event_right) {
  if (push_event_left.id() == push_event_right.id() &&
      push_event_left.order_id() == push_event_right.order_id() &&
      push_event_left.user_id() == push_event_right.user_id() &&
      push_event_left.business_type() == push_event_right.business_type() &&
      push_event_left.event_type() == push_event_right.event_type() &&
      push_event_left.fingerprint_id() == push_event_right.fingerprint_id()) {
    return true;
  } else {
    LOG(WARNING) << "CheckAttribute failed, id" << push_event_left.id();
    return false;
  }
}

TrainBusinessProcessor::TrainBusinessProcessor() {
  VLOG(2) << "TrainBusinessProcessor::TrainBusinessProcessor()";
}

TrainBusinessProcessor::~TrainBusinessProcessor() {}

bool TrainBusinessProcessor::CreatePushEvent(
    UserOrderInfo* user_order_info,
    vector<PushEventInfo>* push_event_vector) {
  VLOG(2) << "TrainBusinessProcessor::CreatePushEvent, order:"
          << user_order_info->id() << ", user:"<< user_order_info->user_id();
  PushEventInfo push_event_info;
  push_event_info.set_order_id(user_order_info->id());
  push_event_info.set_user_id(user_order_info->user_id());
  push_event_info.set_business_type(user_order_info->business_type());
  push_event_info.set_is_realtime(false);
  push_event_info.set_push_status(kPushPending);
  if (FLAGS_use_cluster_mode) {
    push_event_info.set_fingerprint_id(user_order_info->fingerprint_id());
  }
  string detail = user_order_info->order_detail();
  if (!ParseDetail(detail, user_order_info)) {
    LOG(ERROR) << "Parse detail for train failed, order:"
               << user_order_info->id() << ", user:"
               << user_order_info->user_id();
    return false;
  }
  VLOG(2) << "Parse detail for train success, order:"
          << user_order_info->id() << ", user:" << user_order_info->user_id();

  string key = user_order_info->business_info().user_train_info().train_no();
  time_t business_time = user_order_info->business_time();
  for (size_t i = 0; i < arraysize(kTrainEventTimeRelation); ++i) {
    push_event_info.set_event_type(kTrainEventTimeRelation[i].event_type);
    time_t push_time = 0;
    if (kTrainEventTimeRelation[i].relation_type == kIsRelativeTime) {
      push_time = business_time + kTrainEventTimeRelation[i].relative_seconds;
    } else {
      time_t time_for_push_day = business_time
                               + kTrainEventTimeRelation[i].relative_seconds;
      int hour_for_push_day = kTrainEventTimeRelation[i].fixed_hours[0];
      recommendation::MakePushTime(
          time_for_push_day, hour_for_push_day, &push_time);
    }
    push_event_info.set_push_time(push_time);
    push_event_info.set_business_key(key);
    CreateId(&push_event_info);
    push_event_vector->push_back(push_event_info);
  }
  VLOG(2) << "create push events for train success, order:"
          << user_order_info->id() << ", user:" << user_order_info->user_id();
  return true;
}

bool TrainBusinessProcessor::ParseDetail(
    const string& detail, UserOrderInfo* user_order_info) {
  Json::Value root;
  try {
    Json::Reader reader;
    if (!reader.parse(detail, root)) {
      LOG(ERROR) << "JSON parse failed:"
                 << reader.getFormattedErrorMessages();
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "JSON parse failed, Exception:" << e.what();
    return false;
  }
  CHECK(!root.isNull() && root.isObject());
  BusinessInfo* business_info = user_order_info->mutable_business_info();
  UserTrainInfo* user_train_info = business_info->mutable_user_train_info();
  user_train_info->set_train_no(root["train_num"].asString());
  return true;
}

FlightBusinessProcessor::FlightBusinessProcessor() {
  VLOG(2) << "FlightBusinessProcessor::FlightBusinessProcessor()";
}

FlightBusinessProcessor::~FlightBusinessProcessor() {}

bool FlightBusinessProcessor::CreatePushEvent(
    UserOrderInfo* user_order_info,
    vector<PushEventInfo>* push_event_vector) {
  VLOG(2) << "Create event for flight, order:" << user_order_info->id()
            << ", user:"<< user_order_info->user_id();
  PushEventInfo push_event_info;
  push_event_info.set_order_id(user_order_info->id());
  push_event_info.set_user_id(user_order_info->user_id());
  push_event_info.set_business_type(user_order_info->business_type());
  push_event_info.set_is_realtime(false);
  push_event_info.set_push_status(kPushPending);
  if (FLAGS_use_cluster_mode) {
    push_event_info.set_fingerprint_id(user_order_info->fingerprint_id());
  }
  string detail = user_order_info->order_detail();
  if (!ParseDetail(detail, user_order_info)) {
    LOG(ERROR) << "Parse detail for flight failed,order:"
               << user_order_info->id() << ",user:"
               << user_order_info->user_id();
    return false;
  }
  VLOG(2) << "Parse detail for flight success, order:"
          << user_order_info->id() << ", user:" << user_order_info->user_id();

  string key = user_order_info->business_info().user_flight_info().flight_no();
  time_t business_time = user_order_info->business_time();
  for (size_t i = 0; i < arraysize(kFlightEventTimeRelation); ++i) {
    push_event_info.set_event_type(kFlightEventTimeRelation[i].event_type);
    time_t push_time = 0;
    if (kFlightEventTimeRelation[i].relation_type == kIsRelativeTime) {
      push_time = business_time + kFlightEventTimeRelation[i].relative_seconds;
    } else {
      time_t time_for_push_day = business_time
                               + kFlightEventTimeRelation[i].relative_seconds;
      int hour_for_push_day = kFlightEventTimeRelation[i].fixed_hours[0];
      recommendation::MakePushTime(
          time_for_push_day, hour_for_push_day, &push_time);
    }
    push_event_info.set_push_time(push_time);
    push_event_info.set_business_key(key);
    CreateId(&push_event_info);
    push_event_vector->push_back(push_event_info);
  }
  VLOG(2) << "create push events for flight success, order:"
          << user_order_info->id() << ", user:" << user_order_info->user_id();
  return true;
}

bool FlightBusinessProcessor::ParseDetail(
    const string& detail, UserOrderInfo* user_order_info) {
  Json::Value root;
  try {
    Json::Reader reader;
    if (!reader.parse(detail, root)) {
      LOG(ERROR) << "JSON parse failed:"
                 << reader.getFormattedErrorMessages();
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "JSON parse failed, Exception:" << e.what();
    return false;
  }
  CHECK(!root.isNull() && root.isObject());
  BusinessInfo* business_info = user_order_info->mutable_business_info();
  UserFlightInfo* user_flight_info = business_info->mutable_user_flight_info();
  user_flight_info->set_flight_no(root["flightno"].asString());
  return true;
}

MovieBusinessProcessor::MovieBusinessProcessor() {
  VLOG(2) << "MovieBusinessProcessor::MovieBusinessProcessor()";
}

MovieBusinessProcessor::~MovieBusinessProcessor() {}

bool MovieBusinessProcessor::CreatePushEvent(
    UserOrderInfo* user_order_info,
    vector<PushEventInfo>* push_event_vector) {
  VLOG(2) << "Create event for movie, order:" << user_order_info->id()
          << ", user:"<< user_order_info->user_id();
  PushEventInfo push_event_info;
  push_event_info.set_order_id(user_order_info->id());
  push_event_info.set_user_id(user_order_info->user_id());
  push_event_info.set_business_type(user_order_info->business_type());
  push_event_info.set_is_realtime(false);
  push_event_info.set_push_status(kPushPending);
  if (FLAGS_use_cluster_mode) {
    push_event_info.set_fingerprint_id(user_order_info->fingerprint_id());
  }
  string detail = user_order_info->order_detail();
  if (!ParseDetail(detail, user_order_info)) {
    LOG(ERROR) << "Parse detail for movie failed, order:"
               << user_order_info->id() << ", user:"
               << user_order_info->user_id();
    return false;
  }
  VLOG(2) << "Parse detail for movie success, order:"
          << user_order_info->id() << ", user:" << user_order_info->user_id();

  string key = user_order_info->business_info().user_movie_info().movie();
  time_t business_time = user_order_info->business_time();
  for (size_t i = 0; i < arraysize(kMovieEventTimeRelation); ++i) {
    push_event_info.set_event_type(kMovieEventTimeRelation[i].event_type);
    time_t push_time = 0;
    if (kMovieEventTimeRelation[i].relation_type == kIsRelativeTime) {
      push_time = business_time + kMovieEventTimeRelation[i].relative_seconds;
    } else {
      time_t time_for_push_day = business_time
                               + kMovieEventTimeRelation[i].relative_seconds;
      int hour_for_push_day = kMovieEventTimeRelation[i].fixed_hours[0];
      recommendation::MakePushTime(
          time_for_push_day, hour_for_push_day, &push_time);
    }
    push_event_info.set_push_time(push_time);
    push_event_info.set_business_key(key);
    CreateId(&push_event_info);
    push_event_vector->push_back(push_event_info);
  }
  VLOG(2) << "create push events for movie success, order:"
          << user_order_info->id() << ", user:" << user_order_info->user_id();
  return true;
}

bool MovieBusinessProcessor::ParseDetail(const string& detail,
                                         UserOrderInfo* user_order_info) {
  Json::Value root;
  try {
    Json::Reader reader;
    if (!reader.parse(detail, root)) {
      LOG(ERROR) << "JSON parse failed:"
                 << reader.getFormattedErrorMessages();
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "JSON parse failed, ewhat:" << e.what();
    return false;
  }
  CHECK(!root.isNull() && root.isObject());
  BusinessInfo* business_info = user_order_info->mutable_business_info();
  UserMovieInfo* user_movie_info = business_info->mutable_user_movie_info();
  user_movie_info->set_movie(root["movie"].asString());
  return true;
}

HotelBusinessProcessor::HotelBusinessProcessor() {
  VLOG(2) << "HotelBusinessProcessor::HotelBusinessProcessor";
}

HotelBusinessProcessor::~HotelBusinessProcessor() {}

bool HotelBusinessProcessor::CreatePushEvent(
    UserOrderInfo* user_order_info,
    vector<PushEventInfo>* push_event_vector) {

  VLOG(2) << "Create event for hotel, order:" << user_order_info->id()
            << ", user:"<< user_order_info->user_id();
  PushEventInfo push_event_info;
  push_event_info.set_order_id(user_order_info->id());
  push_event_info.set_user_id(user_order_info->user_id());
  push_event_info.set_business_type(user_order_info->business_type());
  push_event_info.set_is_realtime(false);
  push_event_info.set_push_status(kPushPending);
  string detail = user_order_info->order_detail();
  if (!ParseDetail(detail, user_order_info)) {
    LOG(ERROR) << "Parse detail for hotel failed, order:"
               << user_order_info->id() << ", user:"
               << user_order_info->user_id();
    return false;
  }
  VLOG(2) << "Parse detail for hotel success, order:"
          << user_order_info->id() << ", user:" << user_order_info->user_id();
  string key = user_order_info->business_info().user_hotel_info().name();
  time_t business_time = user_order_info->business_time();
  for (size_t i = 0; i < arraysize(kHotelEventTimeRelation); ++i) {
    push_event_info.set_event_type(kHotelEventTimeRelation[i].event_type);
    time_t push_time = 0;
    if (kHotelEventTimeRelation[i].relation_type == kIsRelativeTime) {
      push_time = business_time + kHotelEventTimeRelation[i].relative_seconds;
    } else {
      time_t time_for_push_day = business_time
                               + kHotelEventTimeRelation[i].relative_seconds;
      int hour_for_push_day = kHotelEventTimeRelation[i].fixed_hours[0];
      recommendation::MakePushTime(time_for_push_day,
                                   hour_for_push_day,
                                   &push_time);
    }
    push_event_info.set_push_time(push_time);
    push_event_info.set_business_key(key);
    CreateId(&push_event_info);
    push_event_vector->push_back(push_event_info);
  }
  VLOG(2) << "create push events for hotel success, order:"
          << user_order_info->id() << ", user" << user_order_info->user_id();
  return true;
}

bool HotelBusinessProcessor::ParseDetail(const string& detail,
                                         UserOrderInfo* user_order_info) {
  Json::Value root;
  try {
    Json::Reader reader;
    if (!reader.parse(detail, root)) {
      LOG(ERROR) << "JSON parse faild:"
                 << reader.getFormattedErrorMessages();
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "JSON parse faild, Exception:" << e.what();
    return false;
  }
  CHECK(!root.isNull() && root.isObject());
  BusinessInfo* business_info = user_order_info->mutable_business_info();
  UserHotelInfo* user_hotel_info = business_info->mutable_user_hotel_info();
  user_hotel_info->set_name(root["name"].asString());
  return true;
}

}  // namespace push_controller
