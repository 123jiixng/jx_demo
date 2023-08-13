#ifndef _REDIAL_CLOUOD_WEB_H_
#define _REDIAL_CLOUOD_WEB_H_

#include "mosq_list.h"
#include "in_mosq.h"

#define REDIAL_SUB_TOPIC_NUM		1

void mosq_client_init();

inline char *ntoa(struct in_addr __in)
{
	char *str = inet_ntoa(__in);
	return (str && !strcmp(str, "0.0.0.0")) ? "" : str;
}

#endif
