// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_SERVING_FLIGHT_FLIGHT_DATA_FETCHER_H_
#define PUSH_SERVING_FLIGHT_FLIGHT_DATA_FETCHER_H_

#include "base/basictypes.h"
#include "base/compat.h"

#include "push/proto/flight_meta.pb.h"

namespace flight {

// 航班数据提取程序，数据来源：去哪网
class FlightDataFetcher {
 public:
  FlightDataFetcher();
  ~FlightDataFetcher();
  bool FetchFlightData(const FlightRequest& request, 
                       FlightResponse* response);
 
 private:
  bool GetRepsonseBody(const string& flight_no, 
                       const string& depart_date, 
                       string* response_body);
  bool BuildResponse(const string& response_body,
                     const string& depart_date,
                     FlightResponse* response);

  DISALLOW_COPY_AND_ASSIGN(FlightDataFetcher);
};

}  // namespace flight 

#endif  // PUSH_SERVING_FLIGHT_FLIGHT_DATA_FETCHER_H_
