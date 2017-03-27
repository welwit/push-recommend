// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include <string>

#include "base/basictypes.h"
#include "base/compat.h"
#include "third_party/gtest/gtest.h"
#include "push/util/time_util.h"

namespace recommendation {

struct TestContext1 {
  string datetime_test_input;
  string datetime_expect_output;
};

static const TestContext1 kTestCases1[] = {
  {"2016-06-24 10:33:18+08", "2016-06-24 10:33"},
  {"2016-06-24 10:33:18", "2016-06-24 10:33:18"},
};

TEST(ToInternalDatetimeTest, NormalInput) {
  for (size_t i = 0; i < arraysize(kTestCases1); ++i) {
    EXPECT_EQ(ToInternalDatetime(kTestCases1[i].datetime_test_input), 
                                 kTestCases1[i].datetime_expect_output);
  }
}

struct TestContext2 {
  string datetime_test_input;
  time_t timestamp_expect_output;
  string format;
};

static const TestContext2 kTestCases2[] {
  {"2016-06-24 22:10", 1466777400, "%Y-%m-%d %H:%M"},
  {"2016-06-24 22:10:00", 1466777400, "%Y-%m-%d %H:%M:%S"},
};

TEST(DatetimeToTimestampTest, NormalInput) {
  for (size_t i = 0; i < arraysize(kTestCases2); ++i) {
    time_t timestamp;
    EXPECT_TRUE(DatetimeToTimestamp(kTestCases2[i].datetime_test_input, 
                                    &timestamp, 
                                    kTestCases2[i].format));
    EXPECT_EQ(timestamp, kTestCases2[i].timestamp_expect_output);
  }
}

struct TestContext3 {
  time_t timestamp_test_input;
  string datetime_expect_output;
  string format;
};

static const TestContext3 kTestCases3[] = {
  {1466777400, "2016-06-24 22:10", "%Y-%m-%d %H:%M"},
  {1466777400, "2016-06-24", "%Y-%m-%d"},
  {1466777400, "22:10", "%H:%M"},
  {1466777400, "06-24", "%m-%d"},
};

TEST(TimestampToDatetimeTest, NormalInput) {
  for (size_t i = 0; i < arraysize(kTestCases3); ++i) {
    string datetime;
    TimestampToDatetime(kTestCases3[i].timestamp_test_input, 
                        &datetime, 
                        kTestCases3[i].format);
    EXPECT_EQ(datetime, kTestCases3[i].datetime_expect_output);
  }
}

struct TestContext4 {
  string plan_datetime;
  string estimate_datetime;
  string actual_datetime;
};

static const TestContext4 kTestCases4[] = {
  {"2016-06-24 10:00", "", ""},
  {"2016-06-24 10:00", "2016-06-24 10:00", ""},
  {"2016-06-24 10:00", "2016-06-24 09:00", ""},
  {"2016-06-24 10:00", "", "2016-06-24 10:00"},
  {"2016-06-24 10:00", "", "2016-06-24 09:00"},
  {"2016-06-24 10:00", "2016-06-24 10:00", "2016-06-24 09:00"},
  {"2016-06-24 10:00", "2016-06-24 10:00", "2016-06-24 10:00"},
  {"2016-06-24 10:00", "2016-06-24 11:00", "2016-06-24 10:00"},
  {"2016-06-24 10:00", "2016-06-24 09:00", "2016-06-24 10:00"},
};

TEST(IsDelayTest, NormalInput) {
  string format = "%Y-%m-%d %H:%M";
  for (size_t i = 0; i < arraysize(kTestCases4); ++i) {
    EXPECT_FALSE(IsDelay(kTestCases4[i].plan_datetime, 
                         kTestCases4[i].estimate_datetime, 
                         kTestCases4[i].actual_datetime, format));
  }
}

//struct TestContext5 {
//  string datetime_test_input;
//  time_t timestamp_expect_output;
//};
//
//static const TestContext5 kTestCases5[] {
//  {"06-24 22:10", 1466777400},
//};
//
//TEST(ShortDatetimeToTimestampTest, NormalInput) {
//  for (size_t i = 0; i < arraysize(kTestCases5); ++i) {
//    time_t timestamp;
//    ShortDatetimeToTimestamp(kTestCases5[i].datetime_test_input, &timestamp);
//    EXPECT_EQ(timestamp, kTestCases5[i].timestamp_expect_output);
//  }
//}


struct TestContext6 {
  time_t timestamp_test_input;
  string time_expect_output;
};

static const TestContext6 kTestCases6[] = {
  {1466777400, "22:10"},
};

TEST(TimestampToTimeStrTest, NormalInput) {
  for (size_t i = 0; i < arraysize(kTestCases6); ++i) {
    string time;
    TimestampToTimeStr(kTestCases6[i].timestamp_test_input, &time);
    EXPECT_EQ(time, kTestCases6[i].time_expect_output);
  }
}

struct TestContext7 {
  time_t timestamp_test_input;
  string time_expect_output;
};

static const TestContext7 kTestCases7[] = {
  {1466777400, u8"周五"},
  {1470917956, u8"周四"},
};

TEST(TimestampToWeekStrTest, NormalInput) {
  for (size_t i = 0; i < arraysize(kTestCases7); ++i) {
    string time;
    TimestampToWeekStr(kTestCases7[i].timestamp_test_input, &time);
    EXPECT_EQ(time, kTestCases7[i].time_expect_output);
  }
}

TEST(MakeExpireTimeTest, NormalInput) {
  time_t now = 0;
  string input = "2016-10-10 10:00";
  string expect_output = "2016-10-11 10:00";
  string output;
  string format = "%Y-%m-%d %H:%M";
  EXPECT_TRUE(DatetimeToTimestamp(input, &now, format));
  MakeExpireTime(now, &output, format);
  EXPECT_EQ(output, expect_output);
}

struct TestContext8 {
  string datetime_test_input;
  time_t timestamp_expect_output;
};

static const TestContext8 kTestCases8[] {
  {"11-20 10:20", 1479608400},
};

TEST(ShortDatetimeToTimestampNewTest, NormalInput) {
  for (size_t i = 0; i < arraysize(kTestCases8); ++i) {
    time_t timestamp;
    ShortDatetimeToTimestampNew(kTestCases8[i].datetime_test_input, &timestamp);
    EXPECT_EQ(timestamp, kTestCases8[i].timestamp_expect_output);
  }
}

struct TestContext9 {
  string datetime_test_input;
  time_t timestamp_expect_output;
};

static const TestContext9 kTestCases9[] {
  {"01-20 10:20", 1484878800},
};

TEST(ShortDatetimeToTimestampNewTest2, NormalInput) {
  for (size_t i = 0; i < arraysize(kTestCases9); ++i) {
    time_t timestamp;
    ShortDatetimeToTimestampNew(kTestCases9[i].datetime_test_input, &timestamp);
    EXPECT_EQ(timestamp, kTestCases9[i].timestamp_expect_output);
  }
}

//struct TestContext10 {
//  string datetime_test_input;
//  time_t timestamp_expect_output;
//};
//
//static const TestContext10 kTestCases10[] {
//  {"05-20 10:20", 1463710800},
//};
//
//TEST(ShortDatetimeToTimestampNewTest3, NormalInput) {
//  for (size_t i = 0; i < arraysize(kTestCases10); ++i) {
//    time_t timestamp;
//    ShortDatetimeToTimestampNew(kTestCases10[i].datetime_test_input, &timestamp);
//    EXPECT_EQ(timestamp, kTestCases10[i].timestamp_expect_output);
//  }
//}

}  // namespace recommendation
