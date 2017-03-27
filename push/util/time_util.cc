// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/util/time_util.h"

#include <chrono>   // chrono::system_clock
#include <ctime>    // localtime and strftime
#include <iomanip>  // put_time
#include <sstream>
#include <string>
#include <stdlib.h>
#include <stdio.h>

#include "base/log.h"
#include "base/string_util.h"

namespace {

static const int kMonthJanValue = 0;
static const int kMonthFebValue = 1;
static const int kMonthMarValue = 2;
static const int kMonthOctValue = 9;
static const int kMonthNovValue = 10;
static const int kMonthDecValue = 11;

static std::set<int> kMonthBeginSet =
  {kMonthJanValue, kMonthFebValue, kMonthMarValue};
static std::set<int> kMonthEndSet =
  {kMonthOctValue, kMonthNovValue, kMonthDecValue};

}

namespace recommendation {

string ToInternalDatetime(const string& datetime) {
  // from "2016-06-24 10:33:18+08" to "2016-06-24 10:33"
  if (datetime.size() == 22) {
    return datetime.substr(0, 16);
  } else {
    return datetime;
  }
}

bool DatetimeToTimestamp(const string& datetime,
                         time_t* timestamp,
                         const string& format) {
  int retry = 3;
  while (retry-- > 0) {
    struct tm dt;
    memset(&dt, 0, sizeof(dt));
    if (strptime(datetime.c_str(), format.c_str(), &dt) != NULL) {
      *timestamp = mktime(&dt);
      return true;
    } else {
      continue;
    }
  }
  return false;
}

// from "08-01 21:20" to timestamp
void ShortDatetimeToTimestamp(const string& datetime,
                              time_t* timestamp) {
  time_t now = time(NULL);
  struct tm* dt;
  dt = localtime(&now);
  char timebuff[30] = {0};
  strftime(timebuff, sizeof(timebuff), "%Y", dt);
  string full_datetime = string(timebuff) + "-" + datetime;
  struct tm dt_now;
  memset(&dt_now, 0, sizeof(dt_now));
  strptime(full_datetime.c_str(), "%Y-%m-%d %H:%M", &dt_now);
  *timestamp = mktime(&dt_now);
}


void ShortDatetimeToTimestampNew(const string& datetime,
                                 time_t* timestamp) {
	time_t now = time(NULL);
	struct tm* tm_now = localtime(&now);
  int year_now = tm_now->tm_year + 1900;

  struct tm tm_event;
  strptime(datetime.c_str(), "%m-%d %H:%M", &tm_event);

  struct tm tm_new;
  tm_new.tm_mon = tm_event.tm_mon;
  tm_new.tm_mday = tm_event.tm_mday;
  tm_new.tm_hour = tm_event.tm_hour;
  tm_new.tm_min = tm_event.tm_min;
  tm_new.tm_sec = 0;

  int event_year = 0;
  if (kMonthBeginSet.end() != kMonthBeginSet.find(tm_event.tm_mon) &&
    kMonthEndSet.end() != kMonthEndSet.find(tm_now->tm_mon)) {
    event_year = year_now + 1;
  } else if (kMonthBeginSet.end() != kMonthBeginSet.find(tm_now->tm_mon) &&
    kMonthEndSet.end() != kMonthEndSet.find(tm_event.tm_mon)) {
    event_year = year_now - 1;
  } else {
    event_year = year_now;
  }
  tm_new.tm_year = event_year - 1900;
  *timestamp = mktime(&tm_new);
}

void TimestampToDatetime(const time_t timestamp,
                         string* datetime,
                         const string& format) {
  struct tm* dt;
  dt = localtime(&timestamp);
  char timebuff[30] = {0};
  strftime(timebuff, sizeof(timebuff), format.c_str(), dt);
  *datetime = string(timebuff);
}

// from timestamp to "22:10"
void TimestampToTimeStr(const time_t timestamp,
                        string* timestr) {
  string datetime;
  TimestampToDatetime(timestamp, &datetime, "%Y-%m-%d %H:%M");
  std::string::size_type found = datetime.find(" ");
  CHECK(found != std::string::npos);
  *timestr = datetime.substr(found + 1);
}

void TimestampToShortDate(const time_t timestamp,
                          string* datestr) {
  string datetime;
  TimestampToDatetime(timestamp, &datetime, "%Y-%m-%d %H:%M");
  size_t beg = datetime.find_first_of("-");
  size_t end = datetime.find(" ");
  *datestr = datetime.substr(beg + 1, end - beg - 1);
}

time_t HalfHourBeforeTimestamp(time_t timestamp) {
  return timestamp - 1800;
}

bool HalfHourBeforeDatetime(const string& datetime,
                            const string &format,
                            string* datetime_output) {
  time_t timestamp;
  bool ret = DatetimeToTimestamp(datetime, &timestamp, format);
  if (ret) {
    timestamp = HalfHourBeforeTimestamp(timestamp);
    TimestampToDatetime(timestamp, datetime_output);
  } else {
    *datetime_output = datetime;
  }
  return ret;
}

bool IsDelay(const string& plan_datetime,
             const string& estimate_datetime,
             const string& actual_datetime,
             const string& format) {
  if (plan_datetime.empty() ||
      (estimate_datetime.empty() &&  actual_datetime.empty())) {
    return false;
  }
  time_t plan_timestamp, estimate_timestamp, actual_timestamp;
  bool ret_plan = DatetimeToTimestamp(plan_datetime, &plan_timestamp, format);
  if (!actual_datetime.empty()) {
    bool ret_actual = DatetimeToTimestamp(actual_datetime,
                                          &actual_timestamp,
                                          format);
    if (!ret_plan || !ret_actual) {
      return false;
    } else {
      return (plan_timestamp < actual_timestamp);
    }
  } else {
    bool ret_estimate = DatetimeToTimestamp(estimate_datetime,
                                            &estimate_timestamp,
                                            format);
    if (!ret_plan || !ret_estimate) {
      return false;
    } else {
      return (plan_timestamp < estimate_timestamp);
    }
  }
}

// Make date for some days after or before now
void MakeDate(int days, string* date) {
  string format = "%Y-%m-%d";
  typedef std::chrono::duration<int, std::ratio<60*60*24>> days_type;
  days_type add_days(days);
  auto today = std::chrono::system_clock::now();
  auto tomorrow = today + add_days;
  time_t timestamp = std::chrono::system_clock::to_time_t(tomorrow);
  std::tm* local_time = std::localtime(&timestamp);
  char buff[30] = {0};
  std::strftime(buff, sizeof(buff), format.c_str(), local_time);
  *date = string(buff);
}

void MakePushTime(time_t input_time, int day_hour, time_t* push_time) {
  std::tm* local_time = std::localtime(&input_time);
  std::tm new_time;
  new_time.tm_year = local_time->tm_year;
  new_time.tm_mon = local_time->tm_mon;
  new_time.tm_mday = local_time->tm_mday;
  new_time.tm_hour = day_hour;
  new_time.tm_min = 0;
  new_time.tm_sec = 0;
  *push_time = mktime(&new_time);
}

void TimestampToWeekStr(const time_t timestamp,
                        string* weekstr) {
  string format = "%w";
  struct tm* dt;
  dt = localtime(&timestamp);
  char timebuff[30] = {0};
  strftime(timebuff, sizeof(timebuff), format.c_str(), dt);
  string week = string(timebuff);
  if (week == "0") {
    *weekstr = u8"周日";
  } else if (week == "1") {
    *weekstr = u8"周一";
  } else if (week == "2") {
    *weekstr = u8"周二";
  } else if (week == "3") {
    *weekstr = u8"周三";
  } else if (week == "4") {
    *weekstr = u8"周四";
  } else if (week == "5") {
    *weekstr = u8"周五";
  } else if (week == "6") {
    *weekstr = u8"周六";
  }
}

bool GetTakeoff(const string& plan_takeoff,
                const string& estimate_takeoff,
                const string& actual_takeoff,
                string* takeoff) {
  if (!actual_takeoff.empty()) {
    *takeoff = actual_takeoff;
  } else if (!estimate_takeoff.empty()) {
    *takeoff = estimate_takeoff;
  } else if (!plan_takeoff.empty()) {
    *takeoff = plan_takeoff;
  } else {
    return false;
  }
  return true;
}

bool GetTakeoffVec(const string& plan_takeoff,
                   const string& estimate_takeoff,
                   const string& actual_takeoff,
                   vector<string>* takeoff_vec) {
  string takeoff;
  if (!GetTakeoff(plan_takeoff,
                  estimate_takeoff,
                  actual_takeoff,
                  &takeoff)) {
    return false;
  }
  SplitString(takeoff, ' ', takeoff_vec);
  return true;
}

bool GetArrive(const string& plan_arrive,
               const string& estimate_arrive,
               const string& actual_arrive,
               string* arrive) {
  if (!actual_arrive.empty()) {
    *arrive = actual_arrive;
  } else if (!estimate_arrive.empty()) {
    *arrive = estimate_arrive;
  } else if (!plan_arrive.empty()) {
    *arrive = plan_arrive;
  } else {
    return false;
  }
  return true;
}

bool GetArrive(const string& estimate_arrive,
               const string& actual_arrive,
               string* arrive) {
  string arrive_datetime;
  if (!actual_arrive.empty()) {
    arrive_datetime = actual_arrive;
  } else if (!estimate_arrive.empty()) {
    arrive_datetime = estimate_arrive;
  } else {
    return false;
  }
  vector<string> arrive_vec;
  SplitString(arrive_datetime, ' ', &arrive_vec);
  if (arrive_vec.size() != 2) {
    return false;
  }
  *arrive = arrive_vec[1];
  return true;
}

void MakeExpireTime(time_t now, string* datetime, const string& format) {
  time_t expire_time = now + 86400;
  return TimestampToDatetime(expire_time, datetime, format);
}

void MakeExpireTimeNew(time_t now, string* datetime) {
  string format = "%Y-%m-%d %H:%M";
  time_t expire_time = now + 86400;
  TimestampToDatetime(expire_time, datetime, format);
  size_t pos = datetime->find(" ");
  datetime->assign(datetime->substr(0,  pos) + " 00:00");
}

}  // namespace recommendation
