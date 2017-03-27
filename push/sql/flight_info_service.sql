CREATE TABLE `flight_no` (
  `flight_no` varchar(200) NOT NULL DEFAULT '' COMMENT 'º½°à±àºÅ',
  `updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`flight_no`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
