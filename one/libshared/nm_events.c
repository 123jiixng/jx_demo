/**************************************************************************** **
 * Copyright (C) 2001-2020 Inhand Networks, Inc.
 **************************************************************************** **/

/* ************************************************************************** **
 *     MODULE NAME            : system
 *     LANGUAGE               : C
 *     TARGET ENVIRONMENT     : Any
 *     FILE NAME              : nm_events.c
 *     FIRST CREATION DATE    : 2021/02/23
 * --------------------------------------------------------------------------
 *     Version                : 1.0
 *     Author                 : EExuke
 *     Last Change            : 2021/02/23
 *     FILE DESCRIPTION       :
** ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ih_ipc.h"
#include "nm_events.h"
#include "nm_ipc.h"
#include "ih_svcs.h"
#include "ih_events.h"

static const EVENTS_INFO events_info_tb[] = {
	{EVENT_SIM_SWITCH,            EV_CODE_UPLINK_SWITCH+WAN_SWITCH_SIM,  "Current cellular switch"},
	{EVENT_LOCAL_CONFIG,          EV_CODE_CONFIG,                        "Config changed"},
	{EVENT_REBOOT,                EV_CODE_REBOOT,                        "Reboot device"},
	{EVENT_FW_UPGRADE,            EV_CODE_UPGRADE_OK,                    "Upgrade device"},
	{EVENT_UPLINK_SWITCH,         EV_CODE_UPLINK_SWITCH,                 "Uplink switch"},
	{EVENT_ETH_WAN_CONNECTED,     EV_CODE_UPLINK_STATE_C+WAN_STATE_ETH,  "Ethernet wan connected"},
	{EVENT_ETH_WAN_DISCONNECTED,  EV_CODE_UPLINK_STATE_D+WAN_STATE_ETH,  "Ethernet wan disconnect"},
	{EVENT_SIM_WAN_CONNECTED,     EV_CODE_UPLINK_STATE_C+WAN_STATE_SIM,  "Modem_wan connected"},
	{EVENT_SIM_WAN_DISCONNECTED,  EV_CODE_UPLINK_STATE_D+WAN_STATE_SIM,  "Modem_wan disconnect"},
	{EVENT_WLAN_WAN_CONNECTED,    EV_CODE_UPLINK_STATE_C+WAN_STATE_WLAN, "Wlan wan connected"},
	{EVENT_WLAN_WAN_DISCONNECTED, EV_CODE_UPLINK_STATE_D+WAN_STATE_WLAN, "Wlan disconnect"},
	{{NULL}, 0, {NULL}}
};

/*
 * event msg
 */
typedef struct{
	int code;
	int dataNum;
	char data[MAX_EVENT_DATA_NUM][MAX_EVENT_DATA_LEN];
}EVENT_MSG;

/*
 * get event info
 */
int get_event_info_by_code(EVENTS_INFO *event_info, int code)
{
	int i = 0;

	while(events_info_tb[i].code) {
		if (events_info_tb[i].code == code) {
			snprintf(event_info->type, sizeof(event_info->type), "%s", events_info_tb[i].type);
			event_info->code = code;
			snprintf(event_info->message, sizeof(event_info->message), "%s", events_info_tb[i].message);
			return 0;
		}
		i++;
	}

	return -1;
}

/*
 * publish events by NM use IPC_MSG_NM_PUB_EVENTS_REQ
 */
int pub_events_by_nm(json_t *events_json)
{
	char *events_json_str = NULL;

	events_json_str = json_dumps(events_json, JSON_INDENT(0));
	if (!events_json_str) {
		LOG_ER("json_dumps events msg to NM failed.");
		return -1;
	}

	ih_ipc_send_no_ack(IH_SVC_NETWORKMANAGER, IPC_MSG_NM_PUB_EVENTS_REQ, events_json_str, strlen(events_json_str));

	free(events_json_str);
	events_json_str = NULL;

	return 0;
}

/*
 * publish events by NM
 */
int send_events_for_report(int eventCode, char *data[], int dataNum)
{
	int i;
	EVENT_MSG msg;

	msg.code = eventCode;
	msg.dataNum = dataNum < MAX_EVENT_DATA_NUM ? dataNum : MAX_EVENT_DATA_NUM;
	for (i=0; i<dataNum && i<MAX_EVENT_DATA_NUM; i++) {
		snprintf(msg.data[i], MAX_EVENT_DATA_LEN, "%s", (char *)data+(i*MAX_EVENT_DATA_LEN));
	}
	ih_ipc_send_no_ack(IH_SVC_NETWORKMANAGER, IPC_MSG_NM_PUB_EVENTS_REQ, &msg, sizeof(msg));

	return 0;
}

