// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_SERVING_FLIGHT_FLIGHT_INFO_HANDLER_H_
#define PUSH_SERVING_FLIGHT_FLIGHT_INFO_HANDLER_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "onebox/http_handler.h"
#include "proto/mysql_config.pb.h"
#include "util/net/http_server/http_request.h"
#include "util/net/http_server/http_response.h"

#include "push/proto/flight_meta.pb.h"
#include "push/serving/flight/flight_db_interface.h"

namespace serving {

using namespace flight;

class FlightInfoQueryHandler : public HttpRequestHandler {
 public:
  FlightInfoQueryHandler();
  virtual ~FlightInfoQueryHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  void Init();
  string ErrorInfo(const string& error) const;
  string ResponseInfo(const FlightResponse& response);
  bool UpdateFlightNo(const string& flight_no);

  std::unique_ptr<MysqlServer> mysql_server_;
  std::unique_ptr<FlightDbInterface> flight_db_interface_;
  DISALLOW_COPY_AND_ASSIGN(FlightInfoQueryHandler);
};

class StatusHandler : public HttpRequestHandler {
 public:
  StatusHandler();
  virtual ~StatusHandler();
  virtual bool HandleRequest(util::HttpRequest* request,
                             util::HttpResponse* response);

 private:
  DISALLOW_COPY_AND_ASSIGN(StatusHandler);
};

}  // namespace serving

#endif // PUSH_SERVING_FLIGHT_FLIGHT_INFO_HANDLER_H_
