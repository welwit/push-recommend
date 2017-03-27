// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_FLIGHT_PUSH_PROCESSOR_H_
#define PUSH_PUSH_CONTROLLER_FLIGHT_PUSH_PROCESSOR_H_

#include "push/push_controller/push_processor.h"

namespace push_controller {

class FlightPushProcessor : public PushProcessor {
 public:
  FlightPushProcessor();
  virtual ~FlightPushProcessor();
  virtual bool Process(PushEventInfo* push_event);
  bool ParseDetail(UserOrderInfo* user_order);
  bool GetFlightResponse(const UserOrderInfo& user_order,
                         flight::FlightResponse* flight_response);
  bool FetchWeather(const flight::FlightResponse& flight_response,
                    WeatherInfo* weather_info);
  bool BuildPushMessage(const UserOrderInfo& user_order,
                        const PushEventInfo& push_event,
                        const flight::FlightResponse& flight_response,
                        const WeatherInfo& weather_info,
                        Json::Value* message);

 private:
  bool AppendCommonField(const UserOrderInfo& user_order,
                         const PushEventInfo& push_event,
                         const flight::FlightResponse& flight_response,
                         const WeatherInfo& weather_info,
                         Json::Value* message);
  bool AppendEventField(const UserOrderInfo& user_order,
                        const PushEventInfo& push_event,
                        const flight::FlightResponse& flight_response,
                        const WeatherInfo& weather_info,
                        Json::Value* message);
  bool HandleLastDay(const UserOrderInfo& user_order,
                     const PushEventInfo& push_event,
                     const flight::FlightResponse& flight_response,
                     const WeatherInfo& weather_info,
                     Json::Value* message);
  bool HandleToday(const UserOrderInfo& user_order,
                   const PushEventInfo& push_event,
                   const flight::FlightResponse& flight_response,
                   const WeatherInfo& weather_info,
                   Json::Value* message);
  bool Handle3Hour(const UserOrderInfo& user_order,
                   const PushEventInfo& push_event,
                   const flight::FlightResponse& flight_response,
                   const WeatherInfo& weather_info,
                   Json::Value* message);
  bool Handle1Hour(const UserOrderInfo& user_order,
                   const PushEventInfo& push_event,
                   const flight::FlightResponse& flight_response,
                   const WeatherInfo& weather_info,
                   Json::Value* message);
  bool HandleTakeoff(const UserOrderInfo& user_order,
                     const PushEventInfo& push_event,
                     const flight::FlightResponse& flight_response,
                     const WeatherInfo& weather_info,
                     Json::Value* message);
  bool HandleArrival(const UserOrderInfo& user_order,
                     const PushEventInfo& push_event,
                     const flight::FlightResponse& flight_response,
                     const WeatherInfo& weather_info,
                     Json::Value* message);
  bool GetTakeoff(const flight::FlightResponse& flight_response,
                  string* takeoff);

  std::unique_ptr<WeatherHelper> weather_helper_;
  DISALLOW_COPY_AND_ASSIGN(FlightPushProcessor);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_FLIGHT_PUSH_PROCESSOR_H_
