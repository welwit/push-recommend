// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#ifndef PUSH_PUSH_CONTROLLER_UPDATE_TIME_BACKUP_H_
#define PUSH_PUSH_CONTROLLER_UPDATE_TIME_BACKUP_H_

#include "base/basictypes.h"
#include "base/compat.h"
#include "base/singleton.h"
#include "proto/mysql_config.pb.h"

namespace push_controller {

class UpdateTimeBackup {
 public:
  ~UpdateTimeBackup();
  bool ReadTimeFromDb(time_t* update_time);
  bool BackupTimeToDb(time_t update_time);

 private:
  UpdateTimeBackup();

  friend struct DefaultSingletonTraits<UpdateTimeBackup>;
  std::unique_ptr<MysqlServer> mysql_server_;
  DISALLOW_COPY_AND_ASSIGN(UpdateTimeBackup);
};

}  // namespace push_controller

#endif  // PUSH_PUSH_CONTROLLER_UPDATE_TIME_BACKUP_H_
