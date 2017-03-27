// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_SERVING_FLIGHT_FLIGHT_DATA_UPDATER_H_
#define PUSH_SERVING_FLIGHT_FLIGHT_DATA_UPDATER_H_

#include <memory>

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/thread.h"
#include "proto/mysql_config.pb.h"

#include "push/serving/flight/flight_db_interface.h"
#include "push/proto/flight_meta.pb.h"

namespace flight {

class FlightDataUpdater : public mobvoi::Thread {
 public:
  FlightDataUpdater();
  virtual ~FlightDataUpdater();
  virtual void Run();

 private:
  void Init();
  bool QueryAllFlightNo(vector<string>* flight_no_vector);
  bool UpdateAllFlightInfo(const vector<string>& flight_no_vector);
  
  time_t schedule_internal_;
  std::unique_ptr<MysqlServer> mysql_server_;
  std::unique_ptr<FlightDbInterface> flight_db_interface_;
  DISALLOW_COPY_AND_ASSIGN(FlightDataUpdater);
};

}  // namespace flight 

#endif  // PUSH_SERVING_FLIGHT_FLIGHT_DATA_UPDATER_H_
