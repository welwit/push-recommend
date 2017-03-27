CREATE TABLE `user_order_info` (
  `id` varchar(200) NOT NULL DEFAULT '' COMMENT '订单编号',
  `user_id` varchar(200) NOT NULL DEFAULT '' COMMENT '用户ID',
  `business_type` smallint(6) DEFAULT '0' COMMENT '业务类型',
  `business_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT '业务启动时间',
  `order_detail` text COMMENT '订单详情',
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  `order_status` smallint(6) DEFAULT '0' COMMENT '订单状态',
  `finished_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT '订单完成时间',
  PRIMARY KEY (`id`),
  KEY `user_id` (`user_id`),
  KEY `business_time` (`business_time`),
  KEY `updated` (`updated`),
  KEY `finished_time` (`finished_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `push_event_info` (
  `id` varchar(200) NOT NULL DEFAULT '' COMMENT '事件编号',
  `order_id` varchar(200) NOT NULL DEFAULT '' COMMENT '订单编号',
  `user_id` varchar(200) NOT NULL DEFAULT '' COMMENT '用户ID',
  `business_type` smallint(6) DEFAULT '0' COMMENT '业务类型',
  `event_type` smallint(6) DEFAULT '0' COMMENT '事件类型',
  `push_detail` text COMMENT '推送详情',
  `push_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT '推送时间',
  `business_key` varchar(200) NOT NULL DEFAULT '' COMMENT '业务关键字',
  `is_realtime` tinyint(1) DEFAULT '0' COMMENT '是否实时推送',
  `push_status` smallint(6) DEFAULT '0' COMMENT '推送状态',
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  `finished_time` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT '事件完成时间',
  PRIMARY KEY (`id`),
  KEY `order_id` (`order_id`),
  KEY `user_id` (`user_id`),
  KEY `push_time` (`push_time`),
  KEY `business_key` (`business_key`),
  KEY `updated` (`updated`),
  KEY `finished_time` (`finished_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `push_controller_backup` (
  `id` varchar(200) NOT NULL DEFAULT '' COMMENT '唯一key',
  `time_backup` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT '时间值备份',
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

ALTER TABLE `user_order_info`
    ADD COLUMN `fingerprint_id` bigint(20) unsigned NOT NULL DEFAULT 0;

ALTER TABLE `push_event_info`
    ADD COLUMN `fingerprint_id` bigint(20) unsigned NOT NULL DEFAULT 0;
