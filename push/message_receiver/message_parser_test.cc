// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include <iostream>

#include "base/file/proto_util.h"
#include "third_party/gtest/gtest.h"
#include "third_party/gflags/gflags.h"
#include "third_party/jsoncpp/json.h"
#include "push/message_receiver/message_parser.h"
#include "push/proto/message_receiver_meta.pb.h"

DEFINE_string(extract_rule_conf,
    "config/push/message_receiver/extract_rule.conf",
    "extract rule file conf");

using namespace message_receiver;

TEST(RegexManager, SelfCheckRules) {
  RegexManager regex_manager;
  regex_manager.DebugRules();
  EXPECT_TRUE(regex_manager.SelfCheckRules());
}

TEST(RegexManager, Extract) {
  string message_input =
      u8"{\"timestamp\":1472360912953,\"rule_id\":\"1\",\"app_key\":"
      "\"com.mobvoi.companion\",\"user_id\":"
      "\"2a016388b8f30cd9a1a3cd955d6312b3\",\"number\":\"1065795555\",\"msg\":"
      "\"【出行易确认】尊敬的客户，您预订的08月29日吉祥航空HO1007，08:35"
      "上海浦东国际机场T2起飞，11:20抵达西安咸阳国际机场T3，乘客：姜伟"
      "（票号：0181032206524）已出票，我们将在航班起飞后为您邮寄行程单，请提前"
      "2小时到达机场办理登机手续。【近期诈骗电话较多，请您提高警惕。如后续您"
      "乘坐航班发生变化，我们将通过95555或出行易服务专线021-38834600与您取得"
      "联系。】您可登录\u201c掌上生活-发现-机酒火车-出行助手\u201d查看航班"
      "动态。如有疑问，请致电出行易4000666000。[招商银行]\"}";
  MessageMatcher message_matcher;
  message_matcher.set_user_id("testor");
  message_matcher.set_message_input(message_input);
  RegexManager regex_manager;
  Json::Value output;
  EXPECT_TRUE(regex_manager.Extract(message_matcher, &output));
}

TEST(RegexManager, BadCase1) {
  string message_input =
      u8"{\"timestamp\":1476321929901,\"rule_id\":\"2\",\"number\":\"12306\","
      "\"user_id\":\"2ab2014182cbc0fb510f22a7c0eec29a\",\"app_key\":"
      "\"com.mobvoi.companion\",\"msg\":\"订单E331798106,"
      "电子保单查询号31798106.赵先生您已购10月22日K401次7车5号下铺"
      "北京西19:36开。【铁路客服】\"}";
  MessageMatcher message_matcher;
  message_matcher.set_user_id("testor");
  message_matcher.set_message_input(message_input);
  RegexManager regex_manager;
  Json::Value output;
  EXPECT_TRUE(regex_manager.Extract(message_matcher, &output));
  std::cout << output.toStyledString() << std::endl;
}

TEST(RegexManager, ExtractHotel) {
  string message_input =
      u8"{\"timestamp\":1476321929901,\"rule_id\":\"2\",\"number\":\"12306\","
      "\"user_id\":\"2ab2014182cbc0fb510f22a7c0eec29a\",\"app_key\":"
      "\"【飞猪提醒】酒店确认成功：陆军，订单2885164213979300，12月16日重庆88号鹅岭公园酒店高级标间-（限量促销）含双早1间2晚，到店支付736.00元已确认， 到酒店请报入住人姓名给前台办理入住，担保订单保留至12月17日中午12点，预订成功后，不可变更/取消，未入住酒店将扣除您的担保金368.00元，成功入住有利于提升您在未来酒店的信用评估。重庆 渝中区 鹅岭正街181号 ，近园林局。023-63677888。卖家电话:010-64329999或4009333333，飞猪客服400-1688-688。查询或操作订单点此 tb.cn/Dt8JkJx，人工在线咨询免排队，更便捷，请戳 tb.cn/uRJyzUx\"}";
  MessageMatcher message_matcher;
  message_matcher.set_user_id("testor");
  message_matcher.set_message_input(message_input);
  RegexManager regex_manager;
  Json::Value output;
  EXPECT_TRUE(regex_manager.Extract(message_matcher, &output));
  std::cout << output.toStyledString() << std::endl;
}

TEST(MessageParser, Parse) {
  ExtractRuleConf extract_rule_conf;
  string config = "config/push/message_receiver/extract_rule.conf";
  EXPECT_TRUE(file::ReadProtoFromTextFile(config, &extract_rule_conf));
  MessageParser message_parser;
  for (int i = 0; i < extract_rule_conf.extract_rule_size(); ++i) {
    auto& rule = extract_rule_conf.extract_rule(i);
    auto& test_message = rule.test_message();
    string output;
    EXPECT_TRUE(message_parser.Parse(test_message, &output));
  }
}
