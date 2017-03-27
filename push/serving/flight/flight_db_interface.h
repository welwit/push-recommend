// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_SERVING_FLIGHT_FLIGHT_DB_INTERFACE_H
#define PUSH_SERVING_FLIGHT_FLIGHT_DB_INTERFACE_H

#include "base/basictypes.h"
#include "base/compat.h"

#include "push/serving/flight/flight_data_fetcher.h"
#include "push/proto/flight_meta.pb.h"
#include "push/util/redis_util.h"

namespace flight {

class FlightDbInterface {
 public:
  FlightDbInterface();
  ~FlightDbInterface();
  bool GetFlightInfo(FlightRequest* request,
                     FlightResponse* response);
  bool UpdateFlightData(const string& flight_no);

 private:
  bool UpdateFlightDataToRedis(FlightResponse* response);
  void BuildFlightRequestForToday(const string& flight_no,
                                  FlightRequest* flight_request);
  void BuildFlightRequestForTomorrow(const string& flight_no,
                                     FlightRequest* flight_request);
  bool FetchAndUpdateFlight(const FlightRequest& request,
                            FlightResponse* response);
  void SetFlightDataMap(FlightResponse* response,
                        map<string, string>* flight_data_map_pointer);
  void SetPlanGateToMap(const string& key,
                        FlightResponse* response,
                        map<string, string>* flight_data_map_pointer);

  std::unique_ptr<FlightDataFetcher> flight_data_fetcher_;
  recommendation::RedisClient redis_client_;
  DISALLOW_COPY_AND_ASSIGN(FlightDbInterface);
};

}  // namespace flight 

#endif  // PUSH_SERVING_FLIGHT_FLIGHT_DB_INTERFACE_H
