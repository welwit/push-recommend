// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "third_party/gtest/gtest.h"
#include "push/util/common_util.h"

using namespace push_controller;

TEST(JsonWriteTest, WriteTest) {
  std::string test_text = "{\"key1\":\"value1\"}";
  Json::Reader reader;
  Json::Value root;
  EXPECT_TRUE(reader.parse(test_text, root));
  std::string output;
  EXPECT_TRUE(JsonWrite(root, &output));
  EXPECT_STREQ(output.c_str(), test_text.c_str());
}

TEST(CheckPhoneNumberTest, ValidTest) {
  string phone_number = "7895858";
  EXPECT_TRUE(CheckPhoneNumber(phone_number));
  phone_number = "0313-7895858";
  EXPECT_TRUE(CheckPhoneNumber(phone_number));
  phone_number = "(0313)-7895858";
  EXPECT_TRUE(CheckPhoneNumber(phone_number));
  phone_number = "15803135678";
  EXPECT_TRUE(CheckPhoneNumber(phone_number));
  phone_number = "a-7895858";
  EXPECT_FALSE(CheckPhoneNumber(phone_number));
  phone_number = "0313a-7895858";
  EXPECT_FALSE(CheckPhoneNumber(phone_number));
  phone_number = "158a03135678";
  EXPECT_FALSE(CheckPhoneNumber(phone_number));
}

TEST(GetVersionTest, case1) {
  string version = "tic_4.8.0";
  VersionType version_type;
  EXPECT_TRUE(GetVersion(version, &version_type));
  EXPECT_EQ(version_type.major, 4);
  EXPECT_EQ(version_type.minor, 8);
  EXPECT_EQ(version_type.revision, 0);
  version = "tic_4.8.0_a2";
  EXPECT_TRUE(GetVersion(version, &version_type));
  EXPECT_EQ(version_type.major, 4);
  EXPECT_EQ(version_type.minor, 8);
  EXPECT_EQ(version_type.revision, 0);
  version = "Tic_4.8.0_a2";
  EXPECT_FALSE(GetVersion(version, &version_type));
}

TEST(CheckVersionMatchTest, case1) {
  string version_standard = "tic_4.9.0_a2";
  string version_cmp = "tic_4.9.0_b3";
  EXPECT_TRUE(CheckVersionMatch(version_cmp, version_standard));
}

TEST(CheckVersionMatch2Test, case1) {
  string version_standard = "tic_4.9.0_a2";
  string version_cmp = "tic_4.9.0_b3";
  EXPECT_FALSE(CheckVersionMatch2(version_cmp, version_standard));
  
  version_standard = "tic_4.9.0_a2";
  version_cmp = "tic_4.9.0_a2";
  EXPECT_TRUE(CheckVersionMatch2(version_cmp, version_standard));
}

TEST(CheckVersionGreaterThanTest, case1) {
  string version_greater = "tic_4.9.0";
  string wear_version = "tic_4.10.0_a1";
  EXPECT_TRUE(CheckVersionGreaterThan(wear_version, version_greater));
  
  version_greater = "tic_4.9.0";
  wear_version = "tic_4.9.0_b1";
  EXPECT_FALSE(CheckVersionGreaterThan(wear_version, version_greater));
  
  version_greater = "tic_4.9.0";
  wear_version = "tic_4.9.0_a5";
  EXPECT_FALSE(CheckVersionGreaterThan(wear_version, version_greater));
  
  version_greater = "tic_4.9.0";
  wear_version = "tic_4.9.0";
  EXPECT_FALSE(CheckVersionGreaterThan(wear_version, version_greater));
}

TEST(CheckChannelInternalTest, case1) {
  string channel = "internal";
  EXPECT_TRUE(CheckChannelInternal(channel));
  channel = "other";
  EXPECT_FALSE(CheckChannelInternal(channel));
}

TEST(ExtractMatchedGroupResultsTest, onecase) {
  string pattern = u8".*【出行易确认】尊敬的客户，您预订的(?P<MONTH>\\d{1,2})月(?P<DAY>\\d{1,2})日[^A-Za-z0-9]+?(?P<FLIGHTNUM>[A-Za-z0-9]{3,10})，(?P<DHour>\\d{1,2}):(?P<DMin>\\d{1,2})(?P<FromAirport>.+?)起飞，(?P<AHour>\\d{1,2}):(?P<AMin>\\d{1,2})抵达(?P<ToAirport>.+?)，乘客：.+?（票号：(?P<TICKETNO>[-0-9]+).*";
  string text = u8"{\"timestamp\":1472360912953,\"rule_id\":\"1\",\"app_key\":\"com.mobvoi.companion\",\"user_id\":\"2a016388b8f30cd9a1a3cd955d6312b3\",\"number\":\"1065795555\",\"msg\":\"【出行易确认】尊敬的客户，您预订的08月29日吉祥航空HO1007，08:35上海浦东国际机场T2起飞，11:20抵达西安咸阳国际机场T3，乘客：姜伟（票号：018-1032206524）已出票，我们将在航班起飞后为您邮寄行程单，请提前2小时到达机场办理登机手续。【近期诈骗电话较多，请您提高警惕。如后续您乘坐航班发生变化，我们将通过95555或出行易服务专线021-38834600与您取得联系。】您可登录\u201c掌上生活-发现-机酒火车-出行助手\u201d查看航班动态。如有疑问，请致电出行易4000666000。[招商银行]\"}";
  map<string, string> group_value_map;
  EXPECT_TRUE(ExtractMatchedGroupResults(pattern, text, &group_value_map));
  for (auto& group_value_pair : group_value_map) {
    std::cout << "(" << group_value_pair.first
              << "," << group_value_pair.second
              << ")\n";
  }
}
