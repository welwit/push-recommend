// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/hotel/hotel_push_processor.h"

DECLARE_bool(use_hotel);
DECLARE_bool(use_version_filter);

DECLARE_string(location_service);
DECLARE_string(hotel_version_greater);
DECLARE_string(hotel_version_equal);

namespace push_controller {

HotelPushProcessor::HotelPushProcessor() {
  VLOG(2) << "HotelPushProcessor::HotelPushProcessor()";
}

HotelPushProcessor::~HotelPushProcessor() {}

bool HotelPushProcessor::Process(PushEventInfo* push_event) {
  VLOG(2) << "HotelPushProcessor::Process() ...";
  UserOrderInfo user_order;
  if (!GetUserOrder(*push_event, &user_order)) {
    LOG(ERROR) << "GetUserOrder failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "GetUserOrder success, id:" << push_event->id();
  if (!ParseDetail(&user_order)) {
    LOG(ERROR) << "ParseDetail failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "ParseDetail success, id:" << push_event->id();
  Json::Value message;
  if (!BuildPushMessage(user_order, *push_event, &message)) {
    LOG(ERROR) << "BuildPushMessage failed, id:" << push_event->id();
    return false;
  }
  VLOG(2) << "BuildPushMessage success,id:" << push_event->id();
  if (FLAGS_use_hotel) {
    if (FLAGS_use_version_filter &&
        PushProcessor::FilterVersion(push_event->watch_device().wear_version(),
          FLAGS_hotel_version_equal, FLAGS_hotel_version_greater)) {
      LOG(INFO) << "Hotel FilterVersion, device=" << push_event->id()
                << ", version=" << push_event->watch_device().wear_version()
                << ", version_support: equal=" << FLAGS_hotel_version_equal
                << ", greater=" << FLAGS_hotel_version_greater;
      return false;
    }
    string message_desc = business_key_map_[push_event->business_type()];
    if (!push_sender_->SendPush(kMessageGeneral, message_desc,
                                push_event->user_id(),message)) {
      LOG(ERROR) << "SendPush failed,id:" << push_event->id();
      return false;
    }
    VLOG(2) << "SendPush success, id:" << push_event->id();
    UploadPushDataToKafka(user_order, *push_event, message);
  } else {
    LOG(WARNING) << "hotel push disabled or version unsupported, won't send msg, id:"
                 << push_event->id() << ", version:"
                 << push_event->watch_device().wear_version();
  }
  return true;
}

bool HotelPushProcessor::ParseDetail(UserOrderInfo* user_order) {
  VLOG(2) << "HotelPushProcessor::ParseDetail(), id:"
          << user_order->id();
  Json::Value root;
  try {
    Json::Reader reader;
    if (!reader.parse(user_order->order_detail(), root)) {
      LOG(ERROR) << "JSON parse failed:"
                 << reader.getFormattedErrorMessages();
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "JSON parse failed, ewhat:" << e.what();
    return false;
  }
  if (root.isNull() || !root.isObject()) {
    LOG(ERROR) << "order detail format error, detail:"
               << user_order->order_detail();
    return false;
  }
  string tel = root["tel"].asString();
  if (!CheckPhoneNumber(tel)) {
    LOG(ERROR) << "invalid phone number:" << tel
               << ", order_id:" << user_order->id();
    return false;
  }
  BusinessInfo* business_info = user_order->mutable_business_info();
  UserHotelInfo* user_hotel_info = business_info->mutable_user_hotel_info();
  user_hotel_info->set_user_id(root["user"].asString());
  user_hotel_info->set_address(root["address"].asString());
  user_hotel_info->set_tel(tel);
  user_hotel_info->set_name(root["name"].asString());
  string arrive_time_string = root["arrive_time"].asString();
  time_t arrive_time;
  recommendation::ShortDatetimeToTimestamp(arrive_time_string, &arrive_time);
  user_hotel_info->set_arrive_time(arrive_time);
  user_hotel_info->set_room_category(root["room_category"].asString());
  user_hotel_info->set_time_interval(root["time_interval"].asString());
  user_hotel_info->set_hotel_name(root["hotel_name"].asString());
  return true;
}

bool HotelPushProcessor::BuildPushMessage(const UserOrderInfo& user_order,
                                          const PushEventInfo& push_event,
                                          Json::Value* message) {
  if (push_event.event_type() != kEventRemindTimelineToday &&
      push_event.event_type() != kEventRemindTimelineLastDay) {
    LOG(ERROR) << "ERR push event type" << push_event.event_type();
    return false;
  }
  if (!AppendCommonField(user_order, push_event, message)) {
    return false;
  }
  LOG(INFO) << "MESSAGE:" << JsonToString(*message);
  return true;
}

bool HotelPushProcessor::AppendCommonField(const UserOrderInfo& user_order,
                                           const PushEventInfo& push_event,
                                           Json::Value* msg) {
  Json::Value& message = *msg;
  const UserHotelInfo& user_hotel_info =
    user_order.business_info().user_hotel_info();
  string geo;
  if (!GetLocation(user_hotel_info.address(), &geo)) {
    return false;
  }
  PushContent content;
  content.status = "success";
  content.id = user_order.id();
  content.event_key = event_key_map_[push_event.event_type()];
  content.product_key = business_key_map_[push_event.business_type()];
  content.is_push = false;
  time_t now = time(NULL);
  string expire_time;
  recommendation::MakeExpireTime(now, &expire_time, "%Y-%m-%d %H:%M");
  SysFields sys;
  sys.expire_time = expire_time;
  content.sys = sys;
  string title = user_hotel_info.hotel_name();
  string checkin_time_string;
  recommendation::TimestampToDatetime(user_hotel_info.arrive_time(),
                                      &checkin_time_string, "%m-%d");
  string ticker_content = StringPrintf(u8"%s入住，%s",
                                       checkin_time_string.c_str(),
                                       user_hotel_info.time_interval().c_str());
  TimelineFields timeline_detail;
  timeline_detail.title = title;
  timeline_detail.ticker.push_back(ticker_content);
  string des = u8"出门问问智能推送";
  timeline_detail.des = des;
  content.timeline_detail = timeline_detail;
  DetailFields detail;
  detail.icon = kIconHotel;
  detail.icon_code = "";
  string checkin_date = StringPrintf(u8"%s入住",
                                     checkin_time_string.c_str());
  string room_demand = StringPrintf(u8"%s，%s",
                                    user_hotel_info.room_category().c_str(),
                                    user_hotel_info.time_interval().c_str());
  string address = user_hotel_info.address();
  string address_text = "地址：" + user_hotel_info.address();

  TextType detail_title = {title, "#FFFFFF", 16, kWordWrapTrue,
    kIconNotset, kIconCodeDefault, 4, 4};

  TextType detail_checkin_date = {checkin_date, "#5E83E1", 16,
    kWordWrapFalse, kIconNotset, kIconCodeDefault, 4, 4};

  TextType detail_room_demand = {room_demand, "#FFFFFF", 16,
    kWordWrapTrue, kIconNotset, kIconCodeDefault, 4, 4};

  TextType detail_address = {address_text, "#808080", 15,
    kWordWrapTrue, kIconNotset, kIconCodeDefault, 8, 4};

  detail.text_list.push_back(detail_checkin_date);
  detail.text_list.push_back(detail_room_demand);
  detail.text_list.push_back(detail_address);

  TextType detail_des = {des, kColorDefault, kFontDefault, kWordWrapFalse,
    kIconNotset, kIconCodeDefault, 8, 8};

  detail.title = detail_title;
  detail.des = detail_des;
  string taxi_text = u8"打车到酒店";
  string callup_text = u8"致电到酒店";
  string navigation_text = u8"导航到酒店";
  string tel_text = user_hotel_info.tel();

  LinkType detail_navigation = {navigation_text, "#FFFFFF", kFontDefault,
    "#5E83E1", address, geo, kApplicationNavigation, kIdTextNavigation};

  LinkType detail_taxi = {taxi_text, "#FFFFFF", kFontDefault,
    kBgColorDefault, address, geo, kApplicationTaxi, kIdTextTaxi};

  LinkType detail_callup = {callup_text, "#FFFFFF", kFontDefault,
    kBgColorDefault, tel_text, kLinkGeoDefault, kApplicationCallup,
    kIdTextCallup};

  detail.link_list.push_back(detail_navigation);
  detail.link_list.push_back(detail_taxi);
  detail.link_list.push_back(detail_callup);
  content.detail = detail;
  SetGeneralPushContent(content, message);
  return true;
}

bool HotelPushProcessor::GetLocation(const string& address, string* geo) {
  string url = StringPrintf(FLAGS_location_service.c_str(), address.c_str());
  util::HttpClient http_client;
  http_client.FetchUrl(url);
  string response_body = http_client.ResponseBody();
  Json::Value response;
  Json::Reader reader;
  try {
    reader.parse(response_body, response);
  } catch (const std::exception& e) {
    LOG(ERROR) << "response parse failed: " << e.what();
    return false;
  }
  if ("success" == response["status"].asString()) {
    const Json::Value& data_list = response["result"];
    for (Json::ArrayIndex index = 0; index < data_list.size(); ++index) {
      *geo = data_list[index]["answer"]["lat"].asString() + ","
           + data_list[index]["answer"]["lng"].asString();
      return true;
    }
  }
  LOG(ERROR) << "get location lng and lat faild";
  return false;
}

}  // namespace push_controller
