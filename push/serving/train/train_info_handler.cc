// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/serving/train/train_info_handler.h"

#include "base/log.h"
#include "base/string_util.h"
#include "third_party/jsoncpp/json.h"
#include "util/net/util.h"

#include "push/util/common_util.h"

DECLARE_int32(data_source);
DECLARE_string(trainno_template_format);

namespace {

static const char kTrainnoUrlFormat[] = "http://trains.ctrip.com/checi/%s/";
static const char* kTypelist[] = {
  "zhida", "gaotie", "dongche", "tekuai", "kuaiche", "other"};
static const char kSuccessText[] = "success";
static const char kFailureText[] = "failure";
static const char kNotFoundText[] = "not found";
static const char kOkText[] = "ok";
static const char kServiceName[] = "train info service";

}

namespace serving {

TimeTableQueryHandler::TimeTableQueryHandler() {
  LOG(INFO) << "Create TimeTableQueryHandler";
  if (FLAGS_data_source == kSourceCtrip) {
    time_table_dumper_.reset(new CtripTimeTableDumper());
  } else {
    time_table_dumper_.reset(new BaiduTimeTableDumper());
  }
  train_no_queue_ = std::make_shared<ConcurrentQueue<string>>();
  train_data_updater_.reset(new TrainDataUpdater(train_no_queue_));
  train_data_updater_->Start();
}

TimeTableQueryHandler::~TimeTableQueryHandler() {}

bool TimeTableQueryHandler::HandleRequest(util::HttpRequest* request,
                                          util::HttpResponse* response) {
  LOG(INFO)<< "Receive TimeTableQueryHandler request";
  response->SetJsonContentType();
  QueryStatus status = kFailure;
  try {
    Json::Value req;
    Json::Reader reader;
    reader.parse(request->GetRequestData(), req);
    Json::FastWriter writer;
    LOG(INFO) << "Time table query request:" << writer.write(req);
    string train_no = req["train_no"].asString();
    string depart_station = req["depart_station"].asString();
    vector<TimeTable> time_table_vector;
    status = time_table_dumper_->QueryTimeTable(train_no, 
                                                depart_station, 
                                                &time_table_vector);
    if (status == kSuccess) {
      LOG(INFO) << "Query time table success, record count:" 
                << time_table_vector.size();
      response->AppendBuffer(ResponseInfo(time_table_vector));
      return true;
    } else if (status == kNotFound) {
      LOG(ERROR) << "not found, train_no:" << train_no 
                 << ",depart_station:" << depart_station;
      train_no_queue_->Push(train_no);
    } else {
      LOG(ERROR) << "err occurred, train_no:" << train_no 
                 << ",depart_station:" << depart_station;
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << "Read json resquest data failed:" << e.what();
  }
  response->AppendBuffer(ErrorInfo(status));
  return false;
}
  
string TimeTableQueryHandler::ErrorInfo(QueryStatus status) const {
  Json::Value result;
  result["status"] = kFailureText;
  if (status == kNotFound) {
    result["err_msg"] = kNotFoundText;
  } else {
    result["err_msg"] = kFailureText;
  }
  result["data"] = Json::Value(Json::arrayValue);
  string res = push_controller::JsonToString(result);
  return res;
}

string TimeTableQueryHandler::ResponseInfo(
    const vector<TimeTable>& time_table_vector) {
  Json::Value result;
  Json::Value data_list;
  result["status"] = kSuccessText;
  result["err_msg"] = "";
  for (auto& time_table : time_table_vector) {
    Json::Value data;
    data["train_no"] = time_table.train_no();
    data["station_no"] = time_table.station_no();
    data["station_name"] = time_table.station_name();
    data["get_in_time"] = time_table.get_in_time();
    data["depart_time"] = time_table.depart_time();
    data["stay_time"] = time_table.stay_time();
    data_list.append(data);
  }
  result["data"] = data_list;
  string res = push_controller::JsonToString(result);
  LOG(INFO) << "Time table query response: " << res;
  return res;
}

TimeTableUpdateHandler::TimeTableUpdateHandler() {
  if (FLAGS_data_source == kSourceCtrip) {
    time_table_dumper_.reset(new CtripTimeTableDumper());
  } else {
    time_table_dumper_.reset(new BaiduTimeTableDumper());
  }
  LOG(INFO) << "Create TimeTableUpdateHandler";
}

TimeTableUpdateHandler::~TimeTableUpdateHandler() {}

bool TimeTableUpdateHandler::HandleRequest(util::HttpRequest* request,
                                           util::HttpResponse* response) {
  LOG(INFO)<< "Receive TimeTableUpdateHandler request";
  bool success = time_table_dumper_->UpdateTimeTable();
  response->SetJsonContentType();
  Json::Value value;
  LOG(INFO)<< "TimeTableUpdateHandler update finished";
  if (success) {
    value["status"] = kSuccessText;
    response->AppendBuffer(value.toStyledString());
    return true;
  } else {
    value["status"] = kFailureText;
    response->AppendBuffer(value.toStyledString());
    return false;
  }
}

TrainnoUpdateHandler::TrainnoUpdateHandler() {
  trainno_dumper_.reset(new TrainnoDumper());
  train_data_fetcher_.reset(new TrainDataFetcher());
  LOG(INFO) << "Create TrainnoUpdateHandler";
}

TrainnoUpdateHandler::~TrainnoUpdateHandler() {}

bool TrainnoUpdateHandler::HandleRequest(util::HttpRequest* request,
                                         util::HttpResponse* response) {
  LOG(INFO)<< "Receive TrainnoUpdateHandler request";
  Json::Value result;
  bool success = true;
  for (size_t i = 0; i < arraysize(kTypelist); ++i) {
    string url = StringPrintf(kTrainnoUrlFormat, kTypelist[i]);
    string template_file = StringPrintf(FLAGS_trainno_template_format.c_str(), 
                                        kTypelist[i]);
    if (!train_data_fetcher_->FetchTimeTable(url, template_file, &result)) {
      success = false;
      continue;
    }
    if (!trainno_dumper_->DumpResultIntoDb(result, TrainnoType(i+1))) {
      success = false;
    }
  }
  response->SetJsonContentType();
  Json::Value value;
  LOG(INFO)<< "TrainnoUpdateHandle update finished";
  if (success) {
    value["status"] = kSuccessText;
    response->AppendBuffer(value.toStyledString());
    return true;
  } else {
    value["status"] = kFailureText;
    response->AppendBuffer(value.toStyledString());
    return false;
  }
}

StatusHandler::StatusHandler() {
  LOG(INFO) << "Create StatusHandler";
}

StatusHandler::~StatusHandler() {}

bool StatusHandler::HandleRequest(util::HttpRequest* request,
                                  util::HttpResponse* response) {
  LOG(INFO)<< "Receive StatusHandler request";
  response->SetJsonContentType();
  Json::Value result;
  result["status"] = kOkText;
  result["host"] = util::GetLocalHostName();
  result["service"] = kServiceName;
  response->AppendBuffer(result.toStyledString());
  return true;
}

}  // namespace serving
