// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/serving/user_feedback/user_feedback_handler.h"

#include <exception>

#include "base/hash_tables.h"
#include "base/log.h"
#include "base/string_util.h"
#include "third_party/gflags/gflags.h"
#include "third_party/jsoncpp/json.h"
#include "third_party/mysql_client_cpp/include/cppconn/driver.h"
#include "third_party/mysql_client_cpp/include/cppconn/exception.h"
#include "third_party/mysql_client_cpp/include/cppconn/resultset.h"
#include "third_party/mysql_client_cpp/include/cppconn/statement.h"
#include "third_party/mysql_client_cpp/include/mysql_connection.h"
#include "third_party/mysql_client_cpp/include/mysql_driver.h"
#include "util/net/http_client/http_client.h"
#include "util/net/util.h"
#include "util/url/encode/url_encode.h"

#include "push/serving/user_feedback/db_handler.h"
#include "push/util/common_util.h"

namespace {

static const char kNameTotal[] = "total";
static const char kNameSchedule[] = "schedule";
static const char kNameWeather[] = "weather";
static const char kNameMovie[] = "movie";
static const char kNameNews[] = "news";
static const char kTextSuccess[] = "success";
static const char kTextError[] = "errro";

}

namespace serving {

UserFeedbackHandler::UserFeedbackHandler() {
  LOG(INFO) << "Construct UserFeedbackHandler";
  Init();
}

UserFeedbackHandler::~UserFeedbackHandler() {}

void UserFeedbackHandler::Init() {
  business_type_name_map_ = {
    {kTotal, kNameTotal},
    {kSchedule, kNameSchedule},
    {kWeather, kNameWeather},
    {kMovie, kNameMovie},
    {kNews, kNameNews},
  };
  business_name_type_map_ = {
    {kNameTotal, kTotal},
    {kNameSchedule, kSchedule},
    {kNameWeather, kWeather},
    {kNameMovie, kMovie},
    {kNameNews, kNews},
  };
  total_status_text_map_ = {
    {kSuccess, kTextSuccess},
    {kFailed, kTextError},
  };
  total_text_status_map_ = {
    {kTextSuccess, kSuccess},
    {kTextError, kFailed},
  };
}

string UserFeedbackHandler::ErrorInfo(const UserFeedbackRequest& request, 
                                      const std::string& error_info) {
  Json::Value result;
  result["user_id"] = request.user_id();
  result["status"] = "error";
  result["err_msg"] = error_info;
  result["data"] = Json::Value(Json::arrayValue);
  string res = push_controller::JsonToString(result);
  return res;
}

string 
UserFeedbackHandler::ResponseInfo(const UserFeedbackResponse& response) {
  Json::Value result;
  Json::Value data;
  result["user_id"] = response.user_id();
  result["err_msg"] = response.err_msg();
  for (int i = 0; i < response.data_list_size(); ++i) {
    Json::Value element;
    element["response_status"] = response.data_list(i).response_status();
    BusinessType type = response.data_list(i).business_type();
    auto it = business_type_name_map_.find(type);
    if (it == business_type_name_map_.end()) {
      continue;
    }
    element["business_type"] = business_type_name_map_[type];
    data.append(element);
  }
  result["data"] = data;
  result["status"] = total_status_text_map_[response.status()];
  string res = push_controller::JsonToString(result);
  LOG(INFO) << "Feedback response data:" << res;
  return res;
}

bool UserFeedbackHandler::HandleRequest(util::HttpRequest* request,
                                        util::HttpResponse* response) {
  UserFeedbackRequest user_feedback_request;
  UserFeedbackResponse user_feedback_response;
  try {
    LOG(INFO) << "Receive feedback request"; 
    response->AppendHeader("Content-Type", "application/json;charset=UTF-8");
    Json::Value req;
    Json::Reader reader;
    reader.parse(request->GetRequestData(), req);
    Json::FastWriter writer;
    LOG(INFO) << "Feedback request data:" << writer.write(req);
    user_feedback_request.set_user_id(req["user_id"].asString());
    Json::Value req_data;
    req_data = req["data"];
    for (size_t index = 0; index < req_data.size(); ++index) {
      string business_name = req_data[index]["business_type"].asString();
      bool feedback_status = req_data[index]["user_feedback_status"].asBool();
      auto it = business_name_type_map_.find(business_name);
      if (it == business_name_type_map_.end()) {
        continue;
      }
      BusinessType business_type = business_name_type_map_[business_name];
      UserFeedbackRequest_DataList* data_list = (
          user_feedback_request.add_data_list());
      data_list->set_business_type(business_type);
      data_list->set_user_feedback_status(feedback_status);
    }
    LOG(INFO) << "user_feedback_request:\n" 
              << user_feedback_request.Utf8DebugString();
    bool ret = db_handler_.InsertUserFeedback(&user_feedback_request, 
                                              &user_feedback_response);
    if (!ret) {
      response->AppendBuffer(
          ErrorInfo(user_feedback_request, 
                    user_feedback_response.response_reason()));
      return false;
    }
    string result = ResponseInfo(user_feedback_response);
    response->AppendBuffer(result);
    return true;
  } catch (const std::exception &e) {
    LOG(ERROR) << e.what();
    response->AppendBuffer(ErrorInfo(user_feedback_request, e.what()));
    return false;
  }
}

UserFeedbackQueryHandler::UserFeedbackQueryHandler() {
  LOG(INFO) << "Construct UserFeedbackQueryHandler";
  Init();
}

UserFeedbackQueryHandler::~UserFeedbackQueryHandler() {}

void UserFeedbackQueryHandler::Init() {
  business_type_name_map_ = {
    {kTotal, kNameTotal},
    {kSchedule, kNameSchedule},
    {kWeather, kNameWeather},
    {kMovie, kNameMovie},
  };
  business_name_type_map_ = {
    {kNameTotal, kTotal},
    {kNameSchedule, kSchedule},
    {kNameWeather, kWeather},
    {kNameMovie, kMovie},
  };
  total_status_text_map_ = {
    {kSuccess, kTextSuccess},
    {kFailed, kTextError},
  };
  total_text_status_map_ = {
    {kTextSuccess, kSuccess},
    {kTextError, kFailed},
  };
}

string 
UserFeedbackQueryHandler::ErrorInfo(const UserFeedbackQueryResquest& request, 
                                    const std::string& error_info) {
  Json::Value result;
  result["user_id"] = request.user_id();
  result["status"] = "error";
  result["err_msg"] = error_info;
  result["data"] = Json::Value(Json::arrayValue);
  string res = push_controller::JsonToString(result);
  return res;
}

string UserFeedbackQueryHandler::ResponseInfo(
    const UserFeedbackQueryResponse& response) {
  Json::Value result;
  Json::Value data;
  result["user_id"] = response.user_id();
  result["err_msg"] = response.err_msg();
  for (int i = 0; i < response.data_list_size(); ++i) {
    Json::Value element;
    element["response_status"] = response.data_list(i).response_status();
    element["user_feedback_status"] = (
        response.data_list(i).user_feedback_status());
    BusinessType type = response.data_list(i).business_type();
    auto it = business_type_name_map_.find(type);
    if (it == business_type_name_map_.end()) {
      continue;
    }
    element["business_type"] = business_type_name_map_[type];
    data.append(element);
  }
  result["data"] = data;
  result["status"] = total_status_text_map_[response.status()];
  string res = push_controller::JsonToString(result);
  LOG(INFO) << "Feedback response data:" << res;
  return res;
}

bool UserFeedbackQueryHandler::HandleRequest(util::HttpRequest* request,
                                             util::HttpResponse* response) {
  UserFeedbackQueryResquest user_feedback_query_request;
  UserFeedbackQueryResponse user_feedback_query_response;
  try {
    LOG(INFO) << "Receive feedback query request"; 
    response->AppendHeader("Content-Type", "application/json;charset=UTF-8");
    Json::Value req;
    Json::Reader reader;
    reader.parse(request->GetRequestData(), req);
    Json::FastWriter writer;
    LOG(INFO) << "Feedback query request data:" << writer.write(req);
    user_feedback_query_request.set_user_id(req["user_id"].asString());
    bool ret = db_handler_.QueryUserFeedback(&user_feedback_query_request, 
                                             &user_feedback_query_response);
    if (!ret) {
      response->AppendBuffer(
          ErrorInfo(user_feedback_query_request, 
                    user_feedback_query_response.response_reason()));
      return false;
    }
    string result = ResponseInfo(user_feedback_query_response);
    response->AppendBuffer(result);
    return true;
  } catch (const std::exception &e) {
    LOG(ERROR) << e.what();
    response->AppendBuffer(ErrorInfo(user_feedback_query_request, e.what()));
    return false;
  }
}

StatusHandler::StatusHandler() {}

StatusHandler::~StatusHandler() {}

bool StatusHandler::HandleRequest(util::HttpRequest* request,
                                  util::HttpResponse* response) {
  LOG(INFO)<< "Receive StatusHandler request";
  response->SetJsonContentType();
  Json::Value result;
  result["status"] = "ok";
  result["host"] = util::GetLocalHostName();
  result["service"] = "user_feedback_service";
  response->AppendBuffer(result.toStyledString());
  return true;
}

}  // namespace serving 
