CREATE TABLE `user_feedback` (
  `user_id` varchar(100) NOT NULL DEFAULT '' COMMENT '用户ID',
  `business_type` varchar(100) NOT NULL DEFAULT '' COMMENT '业务类型',
  `user_feedback_status` varchar(100) NOT NULL DEFAULT '' COMMENT '用户反馈状态',
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`user_id`,`business_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
