/**************************************************************************** **
 * Copyright (C) 2001-2020 Inhand Networks, Inc.
 **************************************************************************** **/

/* ************************************************************************** **
 *     MODULE NAME            : system
 *     LANGUAGE               : C
 *     TARGET ENVIRONMENT     : Any
 *     FILE NAME              : events.c
 *     FIRST CREATION DATE    : 2021/06/07
 * --------------------------------------------------------------------------
 *     Version                : 1.0
 *     Author                 : EExuke
 *     Last Change            : 2021/06/07
 *     FILE DESCRIPTION       :
** ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <stdarg.h>
#include <time.h>

#include "ih_events.h"
#include "events_ipc.h"
#include "ih_ipc.h"
#include "ih_svcs.h"

int ev_record_event_to_db(int code, ...)
{
	IH_EVENT_MSG msg = {0};
	va_list valist;
	char *p_arg = NULL;

	msg.timestamp = time(NULL);
	msg.code = code;
	msg.argc = 0;

	va_start(valist, code);
	do {
		p_arg = va_arg(valist, char*);
		if (!p_arg || !p_arg[0]) {
			break;
		}
		snprintf(msg.argv[msg.argc++], MAX_EVENT_ARG_LEN, "%s", p_arg);
	} while (msg.argc < MAX_EVENT_ARG_NUM);
    va_end(valist);

	ih_ipc_send_no_ack(IH_SVC_EVENTS, IPC_MSG_EV_PUB_EVENTS_REQ, &msg, sizeof(msg));

	return 0;
}

json_t *ev_get_events_by_page(char *msg)
{
	int ret = 0;
	IPC_MSG *rsp = NULL;
	json_t *result_obj = NULL;
	//need malloc
	char *result_str = NULL;
	json_error_t failed;

	ret = ih_ipc_send_wait_rsp(IH_SVC_EVENTS, 2000, IPC_MSG_EV_GET_EVENTS_REQ, msg, strlen(msg), &rsp);
	if(ret) {
		LOG_ER("%s failed ret=%d", __func__, ret);
		return NULL;
	}

	result_str = (char *)malloc(rsp->hdr.len+1);
	snprintf(result_str, rsp->hdr.len+1, "%s", (char*)rsp->body);
	//LOG_DB("%s:get rep:[%s]", __func__, result_str);

	result_obj = json_loads(result_str, JSON_COMPACT, &failed);

	ih_ipc_free_msg(rsp);
	if (result_str) {
		free(result_str);
		result_str = NULL;
	}

	return result_obj;
}

// 0:ok
int ev_events_all_clear()
{
	int ret = 0;
	char msg[] = "clear";
	IPC_MSG *rsp = NULL;

	ret = ih_ipc_send_wait_rsp(IH_SVC_EVENTS, 2000, IPC_MSG_EV_CLEAR_EVENTS_REQ, msg, strlen(msg), &rsp);
	if(ret) {
		LOG_ER("%s failed ret=%d", __func__, ret);
		return ret;
	}
	LOG_DB("%s:get rep:[%s]", __func__, rsp->body);

	if (!strcmp((char *)(rsp->body), "ok")) {
		ret = 0;
	}
	ih_ipc_free_msg(rsp);

	LOG_DB("%s:[ret=%d]", __func__, ret);
	return ret;
}

int ev_backup_events()
{
	int ret = 0;
	char msg[] = "backup";
	IPC_MSG *rsp = NULL;

	ret = ih_ipc_send_wait_rsp(IH_SVC_EVENTS, 4000, IPC_MSG_EV_BACKUP_EVENTS_REQ, msg, strlen(msg), &rsp);
	if(ret) {
		LOG_ER("%s failed ret=%d", __func__, ret);
		return ret;
	}
	LOG_DB("%s:get rep:[%s]", __func__, rsp->body);

	ret = strcmp((char *)(rsp->body), "ok");
	ih_ipc_free_msg(rsp);

	LOG_DB("%s:[ret=%d]", __func__, ret);
	return ret;
}

