// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/news/news_push_processor.h"

#include "base/base64.h"
#include "base/singleton.h"
#include "base/string_util.h"
#include "recommendation/news/proto/news_meta.pb.h"
#include "third_party/gflags/gflags.h"
#include "third_party/re2/re2/re2.h"
#include "util/net/http_client/http_client.h"
#include "util/protobuf/proto_json_format.h"

#include "push/push_controller/push_processor.h"
#include "push/util/zookeeper_util.h"

DECLARE_bool(is_test);
DECLARE_bool(use_cluster_mode);
DECLARE_bool(use_version_filter);
DECLARE_bool(use_channel_filter);

DECLARE_int32(recommendation_content_expire_seconds);

DECLARE_string(device_test);
DECLARE_string(recommender_server);
DECLARE_string(zookeeper_watched_path);
DECLARE_string(news_version_greater);
DECLARE_string(news_version_equal);

namespace {

static const char kRecContentKeyPrefix[] = "RECNEWS_";
static const char kIconUrl[] =
  "http://image.ticwear.com/appstore/5d4ab957a08b748c55ead463f56ace3f";

string JoinVecToString(const std::vector<string>& input) {
  string result;
  for (auto& str : input) {
    result += str;
  }
  return result;
}

}

namespace push_controller {

NewsPushProcessor::NewsPushProcessor() {
  device_info_helper_ = Singleton<recommendation::DeviceInfoHelper>::get();
}

NewsPushProcessor::~NewsPushProcessor() {}

bool NewsPushProcessor::Process() {
  vector<std::pair<string, string>> user_device_vec;
  if (FLAGS_is_test) {
    vector<string> user_vec;
    SplitString(FLAGS_device_test, ',', &user_vec);
    for (auto& user : user_vec) {
      string device;
      if (device_info_helper_->GetDeviceByUser(user, &device)) {
        user_device_vec.push_back(std::make_pair(user, device));
      }
    }
  } else {
    if (FLAGS_use_cluster_mode) {
      recommendation::ZkManager* zk_manager =
        Singleton<recommendation::ZkManager>::get();
      int node_pos = zk_manager->GetNodePos(FLAGS_zookeeper_watched_path);
      int node_cnt = zk_manager->GetTotalNodeCnt(FLAGS_zookeeper_watched_path);
      LOG(INFO) << "ZK node_pos:" << node_pos << ", node_cnt:" << node_cnt;
      if (node_pos < 0 || node_cnt <= 0) {
        LOG(ERROR) << "Get node data from zk failed, node_pos=" << node_pos
                   << ", node_cnt=" << node_cnt;
        return false;
      }
      device_info_helper_->GetClusterUserDeviceVec(node_pos, node_cnt, 
                                                   &user_device_vec);
    } else {
      device_info_helper_->GetAllUserDevicePair(&user_device_vec);
    }
  }
  LOG(INFO) << "News push start, user total:" << user_device_vec.size();
  for (auto& user_device : user_device_vec) {
    recommendation::DeviceInfo device_info;
    const string& user = user_device.first;
    const string& device = user_device.second;
    if (!GetDeviceInfo(device, &device_info)) {
      continue;
    }
    if (FLAGS_use_version_filter &&
        PushProcessor::FilterVersion(device_info.wear_version(), 
          FLAGS_news_version_equal, FLAGS_news_version_greater)) {
      LOG(INFO) << "News FilterVersion, device=" << device_info.id()
                << ", version=" << device_info.wear_version() 
                << ", version_support: equal=" << FLAGS_news_version_equal
                << ", greater=" << FLAGS_news_version_greater;
      continue;
    }
    if (FLAGS_use_channel_filter) {
      if (!CheckChannelInternal(device_info.wear_version_channel())) {
        LOG(INFO) << "News CheckChannel, device=" << device_info.id()
                  << ", channel=" << device_info.wear_version_channel();
        continue;
      }
    }
    Json::Value rec_results;
    if (!GetRecList(device, &rec_results)) {
      LOG(ERROR) << "Get recommendation list failed, device:" << device;
      continue;
    }
    recommendation::StoryDetail rec_content;
    if (!ChooseRecContent(rec_results, &rec_content)) {
      LOG(ERROR) << "ChooseRecContent failed, device:" << device
                 << ", rec_results:" << JsonToString(rec_results);
      continue;
    }
    Json::Value message;
    if (!BuildPushMessage(device, rec_content, &message)) {
      LOG(ERROR) << "Build push message failed, device:" << device
                 << ", rec_content:" << ProtoToString(rec_content);
      continue;
    }
    if (!SaveRecContentToDb(device, rec_content)) {
      LOG(ERROR) << "Save rec content to db failed, device:" << device;
      continue;
    }
    if (!push_sender_.SendPush(kMessageWatchface, "news", user, message)) {
      LOG(ERROR) << "Send push failed, type: news, User: " 
                 << user << ", Device:" << device;
      continue;
    }
  }
  LOG(INFO) << "News push finish";
  return true;
}

bool NewsPushProcessor::GetRecList(const string& device,
                                   Json::Value* rec_results) {
  string url = StringPrintf(FLAGS_recommender_server.c_str(), device.c_str());
  util::HttpClient http_client;
  if (!http_client.FetchUrl(url) || http_client.response_code() != 200) {
    LOG(ERROR) << "Get recommendation failed, device:" << device;
    return false;
  }
  Json::Reader reader;
  reader.parse(http_client.ResponseBody(), *rec_results);
  if ((*rec_results)["status"] == "success") {
    return true;
  } else {
    LOG(ERROR) << "Get bad recommendation, rec_results:"
               << JsonToString(*rec_results);
    return false;
  }
}

bool NewsPushProcessor::BuildPushMessage(
    const string& device, const recommendation::StoryDetail& rec_content,
    Json::Value* message) {
  (*message)["is_push"] = false;
  (*message)["id"] = StringPrintf("%s", rec_content.id().c_str());
  string intentFormat =
      "intent:#Intent;launchFlags=0x10000000;"
      "action=com.mobvoi.ticwear.news.action.READ_NEWS;"
      "S.from=push;S.news_id=%s;S.type=single;S.topic=default;end";
  string intent = StringPrintf(intentFormat.c_str(), rec_content.id().c_str());
  (*message)["intentUri"] = intent;
  (*message)["theme"] = "abc";
  (*message)["icon"] = kIconUrl;
  (*message)["product_key"] = "news";
  (*message)["desc"] = "news push";
  (*message)["status"] = "success";
  (*message)["event_key"] = rec_content.id() + "-" + rec_content.type();
  Json::Value sys;
  time_t now = time(NULL);
  string expire_time;
  recommendation::MakeExpireTimeNew(now, &expire_time);
  sys["expire_time"] = expire_time;
  string current_time;
  recommendation::TimestampToDatetime(now, &current_time);
  sys["current_time"] = current_time;
  sys["cost_time"] = "";
  (*message)["sys"] = sys;
  Json::Value timeline;
  timeline["title"] = "问问头条";
  timeline["des"] = "出门问问智能推送";
  Json::Value ticker_list;
  string content = rec_content.title();
  std::vector<string> vec;
  if (!mobvoi::SplitUTF8String(content, &vec)) {
    return false;
  }
  if (vec.size() > 20) {
    vec.resize(19);
    content = JoinVecToString(vec) + "......";
  }
  ticker_list.append(content);
  timeline["ticker"] = ticker_list;
  (*message)["timeline_detail"] = timeline;
  return true;
}

bool NewsPushProcessor::BuildPushMessageByNotification(const string& device,
      const recommendation::StoryDetail& rec_content, Json::Value* message) {
  Json::Value notification;
  notification["title"] = "问问头条";
  notification["text"] = rec_content.title();
  notification["biz_type"] = "news";
  Json::Value intent;
  intent["type"] = "activity";
  intent["action"] = "com.mobvoi.ticwear.news.action.READ_NEWS";
  Json::Value extras;
  extras["from"] = "push";
  extras["topic"] = "default";
  extras["type"] = "single";
  const string& id  = rec_content.id();
  extras["news_id"] = id;
  intent["extras"] = extras;
  notification["content_intent"] = intent;
  (*message)["notification"] = notification;
  return true;
}

bool NewsPushProcessor::ParseRecVec(const Json::Value& rec_results,
    vector<recommendation::StoryDetail>* rec_vec) {
  const Json::Value& data_list = rec_results["data"];
  for (auto& data : data_list) {
    recommendation::StoryDetail rec_content;
    if (!util::ProtoJsonFormat::ParseFromValue(data, &rec_content)) {
      return false;
    }
    if (rec_content.id().empty()) {
      string id;
      if (!ParseNewsId(rec_content, &id)) {
        return false;
      }
      rec_content.set_id(id);
    }
    rec_vec->push_back(rec_content);
  }
  return true;
}

bool NewsPushProcessor::ParseNewsId(
    const recommendation::StoryDetail& rec_content, string* id) {
  string browser_url = rec_content.browser_url();
  string format = ".*article/(\\d+?)/.*";
  if (!RE2::FullMatch(browser_url, format, id)) {
    LOG(ERROR) << "BAD url, rec_content:" << ProtoToString(rec_content);
    return false;
  }
  return true;
}

bool NewsPushProcessor::ChooseRecContent(const Json::Value& rec_results,
    recommendation::StoryDetail* rec_content) {
  vector<recommendation::StoryDetail> rec_vec;
  if (!ParseRecVec(rec_results, &rec_vec)) {
    LOG(ERROR) << "ParseRecommedationVec failed, rec_results:"
               << JsonToString(rec_results);
    return false;
  }
  for (auto& rec : rec_vec) {
    LOG(INFO) << "Rec candidate:" << ProtoToString(rec);
  }
  *rec_content = rec_vec.at(0);
  return true;
}

bool NewsPushProcessor::SaveRecContentToDb(const string& device,
    const recommendation::StoryDetail& rec_content) {
  string rec_string;
  if (!util::ProtoJsonFormat::PrintToFastString(rec_content, &rec_string)) {
    LOG(ERROR) << "PrintToFastString failed, rec_content:"
               << ProtoToString(rec_content);
    return false;
  }
  string encoded_rec_string;
  if (!base::Base64Encode(rec_string, &encoded_rec_string)) {
    LOG(ERROR) << "Base64Encode failed, rec_string:" << rec_string;
    return false;
  }
  string id = kRecContentKeyPrefix + rec_content.id();
  string value;
  if (redis_client_.Get(id, &value) && !value.empty()) {
    LOG(INFO) << "News is in redis cache, need not set it, id:" << id;
    return true;
  }
  if (!redis_client_.Set(id, encoded_rec_string,
                         FLAGS_recommendation_content_expire_seconds)) {
    LOG(ERROR) << "Redis set failed, key:" << id << ", device:" << device
               << ", rec_string:" << rec_string;
    return false;
  }
  LOG(INFO) << "Redis set success, key:" << id << ", rec_string:" << rec_string;
  return true;
}

bool NewsPushProcessor::GetDeviceInfo(
    const string& device, recommendation::DeviceInfo* device_info) {
  return device_info_helper_->GetDeviceInfo(device, device_info);
}

}  // namespace push_controller
