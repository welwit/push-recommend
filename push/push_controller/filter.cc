// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/push_controller/filter.h"

#include <algorithm>

#include "base/log.h"
#include "third_party/gflags/gflags.h"
#include "third_party/jsoncpp/json.h"
#include "util/net/http_client/http_client.h"

#include "push/util/common_util.h"

DECLARE_bool(is_test);
DECLARE_int32(valid_push_time_internal);
DECLARE_string(user_feedback_service);
DECLARE_int32(db_batch_query_size);

namespace {

static const char kNameTotal[] = "total";
static const char kNameSchedule[] = "schedule";
static const char kNameWeather[] = "weather";
static const char kNameMovie[] = "movie";
static const char kAbroadWearModel[] = "-i18n";

}

namespace push_controller {

UserFeedbackFilter::UserFeedbackFilter() {
  Init();
}

UserFeedbackFilter::~UserFeedbackFilter() {}

void UserFeedbackFilter::Init() {
  business_type_string_map_ = {
    {kBusinessFlight, kNameSchedule},
    {kBusinessMovie, kNameMovie},
    {kBusinessTrain, kNameSchedule},
    {kBusinessWeather, kNameWeather},
  };
}

void UserFeedbackFilter::Filtering(vector<PushEventInfo>* push_events) {
  size_t size_before = push_events->size();
  Json::Value feedback_result;
  for (auto it = push_events->begin(); it != push_events->end();) {
    feedback_result.clear();
    if (!FetchFeedback(*it, &feedback_result)) {
      LOG(WARNING) << "fetch feedback failed, id:" << it->id();
      ++it;
      continue;
    }
    if (IsUserClosedPush(*it, feedback_result)) {
      LOG(INFO) << "user feedback filtered, id:" << it->id();
      it = push_events->erase(it);
      continue;
    } else {
      ++it;
      continue;
    }
  }
  size_t size_after = push_events->size();
  VLOG(2) << "User feedback filtered, vector size from " << size_before
          << " to " << size_after;
}

bool UserFeedbackFilter::FetchFeedback(const PushEventInfo& push_event,
                                       Json::Value* feedback_result) {
  util::HttpClient http_client;
  Json::Value request;
  request["user_id"] = push_event.user_id();
  string post_data = JsonToString(request);
  http_client.SetHttpMethod(util::HttpMethod::kPost);
  http_client.SetPostData(post_data);
  http_client.AddHeader("Content-Type", "application/json; charset=utf-8");
  if (!http_client.FetchUrl(FLAGS_user_feedback_service)) {
    LOG(ERROR) << "fetch url failed";
    return false;
  }
  string response_body = http_client.ResponseBody();
  Json::Reader reader;
  try {
    if (!reader.parse(response_body, *feedback_result)) {
      LOG(ERROR) << "Parse user feedback failed";
      return false;
    }
  } catch (const std::exception& e) {
    LOG(ERROR) << "Parse user feedback exception:" << e.what();
    return false;
  }
  return true;
}

bool UserFeedbackFilter::IsUserClosedPush(const PushEventInfo& push_event,
                                          const Json::Value& feedback_result) {
  if (!feedback_result.isMember("status")) {
    LOG(ERROR) << "No member status";
    return false;
  }
  if (feedback_result["status"].asString() == "success") {
    Json::Value data_array = feedback_result["data"];
    if (data_array.empty()) {
      LOG(WARNING) << "user feedback data_array is null";
      return false;
    }
    map<string, bool> business_status_map;
    for (Json::ArrayIndex index = 0; index < data_array.size(); ++index) {
      string business = data_array[index]["business_type"].asString();
      bool status = data_array[index]["user_feedback_status"].asBool();
      business_status_map.insert(make_pair(business, status));
    }
    for (auto& business_status : business_status_map) {
      VLOG(2) << "user feedback map, business:" << business_status.first
              << ", status:" << business_status.second;
    }
    string business_string = (
        business_type_string_map_[push_event.business_type()]);
    auto it = business_status_map.find(business_string);
    if (it == business_status_map.end()) {
      VLOG(2) << "not found user feedback status, business_type:"
              << push_event.business_type() << ",business:" << business_string
              << ",id:" << push_event.id();
      return false;
    } else {
      if (it->second == false) {
        LOG(INFO) << "user closed push, id:" << push_event.id();
        return true;
      } else {
        LOG(INFO) << "user opened push, id:" << push_event.id();
        return false;
      }
    }
  } else {
    VLOG(2) << "feedback return error, id:" << push_event.id()
            << ", will use default response";
    return false;
  }
}

PushStatusFilter::PushStatusFilter() {
  push_sender_.reset(new PushSender());
}

PushStatusFilter::~PushStatusFilter() {}

void PushStatusFilter::Filtering(vector<PushEventInfo>* push_events) {
  size_t size_before = push_events->size();
  for (auto it = push_events->begin(); it != push_events->end();) {
    PushStatus push_status = it->push_status();
    VLOG(2) << "In StatusFilter, id:" << it->id()
              << ", push_status:" << push_status;
    if (push_status == kPushPending || push_status == kPushFailed) {
      // lock the data by mysql push status: kPushProcessing
      push_sender_->UpdatePushStatus(*it, kPushProcessing);
      ++it;
      continue;
    } else {
      VLOG(2) << "push status filtered, id:" << it->id();
      it = push_events->erase(it);
      continue;
    }
  }
  size_t size_after = push_events->size();
  VLOG(2) << "Push status filtered, vector size from " << size_before
          << " to " << size_after;
}

PushTimeFilter::PushTimeFilter() {}

PushTimeFilter::~PushTimeFilter() {}

void PushTimeFilter::Filtering(vector<PushEventInfo>* push_events) {
  size_t size_before = push_events->size();
  for (auto it = push_events->begin(); it != push_events->end();) {
    time_t push_time = static_cast<time_t>(it->push_time());
    VLOG(2) << "In TimeFilter, id:" << it->id()
              << ", push_time:" << push_time;
    if (IsValidPushTime(push_time)) {
      ++it;
      continue;
    } else {
      VLOG(2) << "push time filtered, id:" << it->id();
      it = push_events->erase(it);
      continue;
    }
  }
  size_t size_after = push_events->size();
  VLOG(2) << "Push time filtered, vector size from " << size_before
          << " to " << size_after;
}

bool PushTimeFilter::IsValidPushTime(time_t push_time) {
  time_t now = time(NULL);
  VLOG(2) << "push_time:" << push_time << ",now:" << now
          << ",valid_end_time:" << push_time + FLAGS_valid_push_time_internal;
  if ((now >= push_time) &&
      (now < push_time + FLAGS_valid_push_time_internal)) {
    return true;
  } else {
    return false;
  }
}

WearModelFilter::WearModelFilter() {}

WearModelFilter::~WearModelFilter() {}

void WearModelFilter::Filtering(vector<PushEventInfo>* push_events) {
  if (push_events->empty()) {
    return;
  }
  size_t size_before = push_events->size();
  FilterInternal(push_events);
  size_t size_after = push_events->size();
  VLOG(2) << "WearModel filtered, vector size from " << size_before
          << " to " << size_after;
}

void WearModelFilter::FilterInternal(vector<PushEventInfo>* push_events) {
  for (auto it = push_events->begin(); it != push_events->end();) {
    auto pos = it->watch_device().wear_model().find(kAbroadWearModel);
    if (pos == string::npos) {
      ++it;
      continue;
    } else {
      LOG(INFO) << "erase abroad user:" << it->user_id();
      it = push_events->erase(it);
      continue;
    }
  }
}

/*
 * NOTE => PushStatusFilter should be the last filter,
 * because if it works, will be locked in Processing state
 */
FilterManager::FilterManager() {
  if (!FLAGS_is_test) {
    filters_.push_back(static_cast<Filter*>(new PushTimeFilter()));
  }
  filters_.push_back(static_cast<Filter*>(new UserFeedbackFilter()));
  filters_.push_back(static_cast<Filter*>(new WearModelFilter()));
  filters_.push_back(static_cast<Filter*>(new PushStatusFilter()));
  VLOG(1) << "filter manager size:" << filters_.size();
}

FilterManager::~FilterManager() {
  for (auto it = filters_.begin(); it != filters_.end(); ++it) {
    if (*it != NULL) {
      delete(*it);
    }
  }
  filters_.clear();
}

void FilterManager::Filtering(vector<PushEventInfo>* push_events) {
  size_t size_before = push_events->size();
  for (auto it = filters_.begin(); it != filters_.end(); ++it) {
    (*it)->Filtering(push_events);
  }
  size_t size_after = push_events->size();
  VLOG(2) << "FilterManager filter, vector size from " << size_before
          << " to " << size_after;
  for (auto it = push_events->begin(); it != push_events->end(); ++it) {
    LOG(INFO) << "AFTER all filter, id:" << it->id();
  }
}

}  // namespace push_controller
