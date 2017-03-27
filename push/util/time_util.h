// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_UTIL_TIME_UTIL_H_
#define PUSH_UTIL_TIME_UTIL_H_

#include <string.h>
#include <time.h>

#include <iostream>
#include <string>

#include "base/compat.h"

namespace recommendation {

string ToInternalDatetime(const string& datetime);

bool DatetimeToTimestamp(const string& datetime,
                         time_t* timestamp,
                         const string& format);

void ShortDatetimeToTimestamp(const string& datetime,
                              time_t* timestamp);

void ShortDatetimeToTimestampNew(const string& datetime,
                                 time_t* timestamp);

void TimestampToDatetime(const time_t timestamp,
                         string* datetime,
                         const string& format = "%Y-%m-%d %H:%M");

void TimestampToTimeStr(const time_t timestamp,
                        string* timestr);

time_t HalfHourBeforeTimestamp(time_t timestamp);

bool HalfHourBeforeDatetime(const string& datetime,
                            const string& format,
                            string* datetime_output);

bool IsDelay(const string& plan_datetime,
             const string& estimate_datetime,
             const string& actual_datetime,
             const string& format);

void MakeDate(int days, string* date);

void MakePushTime(time_t input_time, int day_hour, time_t* push_time);

void TimestampToWeekStr(const time_t timestamp,
                        string* weekstr);

bool GetTakeoff(const string& plan_takeoff,
                const string& estimate_takeoff,
                const string& actual_takeoff,
                string* takeoff);

bool GetTakeoffVec(const string& plan_takeoff,
                   const string& estimate_takeoff,
                   const string& actual_takeoff,
                   vector<string>* takeoff_vec);

bool GetArrive(const string& plan_arrive,
               const string& estimate_arrive,
               const string& actual_arrive,
               string* arrive);

bool GetArrive(const string& estimate_arrive,
               const string& actual_arrive,
               string* arrive);

void MakeExpireTime(time_t now, string* datetime, const string& format);
void MakeExpireTimeNew(time_t now, string* datetime);

}  // namespace recommendation

#endif  // PUSH_UTIL_TIME_UTIL_H_
