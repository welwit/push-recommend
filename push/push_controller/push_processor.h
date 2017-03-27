// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_PUSH_PROCESSOR_H_
#define PUSH_PUSH_CONTROLLER_PUSH_PROCESSOR_H_

#include <memory>

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/concurrent_queue.h"
#include "base/log.h"
#include "base/string_util.h"
#include "base/thread.h"
#include "proto/mysql_config.pb.h"
#include "third_party/gflags/gflags.h"
#include "third_party/jsoncpp/json.h"
#include "util/net/http_client/http_client.h"

#include "push/push_controller/push_sender.h"
#include "push/proto/flight_meta.pb.h"
#include "push/proto/train_meta.pb.h"
#include "push/proto/push_meta.pb.h"
#include "push/util/common_util.h"
#include "push/util/time_util.h"
#include "push/util/user_info_helper.h"
#include "push/util/weather_helper.h"

namespace push_controller {

const int kFontDefault = 0;
const int kTopMarginDefault = 0;
const int kBottomMarginDefault = 0;

const bool kWordWrapTrue = true;
const bool kWordWrapFalse = false;

const char kColorDefault[] = "";
const char kIconCodeDefault[] = "";
const char kBgColorDefault[] = "";
const char kLinkDataDefault[] = "";
const char kLinkGeoDefault[] = "";

const char kIdTextNavigation[] = "Navigation";
const char kIdTextTaxi[] = "Taxi";
const char kIdTextCallup[] = "Callup";

enum IconValue {
  kIconNotset = 0,
  kIconDefault = 1,
  kIconTrain = 2,
  kIconFlight = 3,
  kIconMovie = 4,
  kIconHotel = 5,
  kIconSmallDefault = 6,
};

enum ApplicationType {
  kApplicationDefault = 0,
  kApplicationNavigation = 1,
  kApplicationTaxi = 2,
  kApplicationCallup = 3,
};

struct TextType {
  string text;
  string color;
  int font;
  bool word_wrap;
  int icon;
  string icon_code;
  int top_margin;
  int bottom_margin;
};

struct LinkType {
  string text;
  string color;
  int font;
  string bg_color;
  string data;
  string geo;
  ApplicationType type;
  string id;
};

struct PushFields {
  string push_time;
  string position1;
  string position2;
  string position3;
  string position4;
  string des;
  string ui_type;
};

struct SysFields {
  string expire_time;
};

struct TimelineFields {
  string title;
  vector<string> ticker;
  string des;
};

struct DetailFields {
  int icon;
  string icon_code;
  TextType title;
  vector<TextType> text_list;
  TextType des;
  vector<LinkType> link_list;
};

struct PushContent {
  string status;
  string id;
  string event_key;
  string product_key;
  bool is_push;
  PushFields push_detail;
  SysFields sys;
  TimelineFields timeline_detail;
  DetailFields detail;
};

class PushProcessor : public mobvoi::Thread {
 public:
  PushProcessor();
  virtual ~PushProcessor();
  void PushToQueue(const PushEventInfo& push_event);
  virtual bool Process(PushEventInfo* push_event) = 0;
  void Run();
  void Init();
  
  static bool FilterVersion(const string& wear_version,
                            const string& version_equal, 
                            const string& version_greater);

 protected:
  bool GetUserOrder(const PushEventInfo& push_event,
                    UserOrderInfo* user_order);
  void SetTimeline(const string& title,
                   const vector<string>& ticker_vec,
                   Json::Value& message);
  void SetPushDetail(const string& des,
                     const string& position1,
                     const string& position2,
                     const string& position3,
                     const string& position4,
                     const string& ui_type,
                     Json::Value& message);
  void UploadPushDataToKafka(const UserOrderInfo& user_order,
                             const PushEventInfo& push_event,
                             const Json::Value& push_message);
  void UploadFailedPushDataToKafka(const PushEventInfo& push_event);
  void SetGeneralPushContent(const PushContent& content, Json::Value& message);
  void SetGeneralPushDetail(bool is_push, const PushFields& push,
                            Json::Value& message);
  void SetGeneralSys(const SysFields& sys, Json::Value& message);
  void SetGeneralTimeline(const TimelineFields& timeline, Json::Value& message);
  void SetGeneralDetail(const DetailFields& detail, Json::Value& message);

  map<BusinessType, string> business_key_map_;
  map<EventType, string> event_key_map_;
  std::unique_ptr<MysqlServer> mysql_server_;
  std::unique_ptr<PushSender> push_sender_;

 private:
  std::unique_ptr<mobvoi::ConcurrentQueue<PushEventInfo>> push_event_queue_;
  DISALLOW_COPY_AND_ASSIGN(PushProcessor);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_PUSH_PROCESSOR_H_
