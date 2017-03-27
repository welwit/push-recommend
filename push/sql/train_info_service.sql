CREATE TABLE `train_no` (
  `train_no` varchar(200) NOT NULL DEFAULT '' COMMENT '车次编号',
  `type` smallint(6) DEFAULT '0' COMMENT '车次类型',
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`train_no`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `train_time_table` (
  `train_no` varchar(200) NOT NULL DEFAULT '' COMMENT '车次编号',
  `station_no` smallint(6) NOT NULL DEFAULT '0' COMMENT '车站序号',
  `station_name` varchar(200) NOT NULL DEFAULT '' COMMENT '车站名称',
  `get_in_time` varchar(200) NOT NULL DEFAULT '' COMMENT '进站时间',
  `depart_time` varchar(200) NOT NULL DEFAULT '' COMMENT '发车时间',
  `stay_time` varchar(200) NOT NULL DEFAULT '' COMMENT '停留时间',
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`train_no`,`station_no`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
