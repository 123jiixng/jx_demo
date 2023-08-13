#ifndef _REDIAL_WEB_H_
#define _REDIAL_WEB_H_

#include "mosq_list.h"
#include "in_mosq.h"

#define REDIAL_NOTICE_TOPIC			"httpreq/v1/cellular/#"
#define REDIAL_GET_MODEM_STATUS		"v1/cellular/modem/status/get"
#define REDIAL_GET_NETWORK_STATUS	"v1/cellular/network/status/get"
#define REDIAL_GET_CONFIG			"v1/cellular/config/get"
#define REDIAL_GET_INTERFACE_LIST	"v1/cellular/interface/list/get"
#define REDIAL_SET_ENABLE_CONFIG	"v1/cellular/status/config/put"
#define REDIAL_SET_CONFIG			"v1/cellular/config/put"

#define REDIAL_SUB_TOPIC_NUM		1

void mosq_client_init();

#endif
