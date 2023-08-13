/*
 * $Id$ --
 *
 *   interface management common function
 *
 * Copyright (c) 2001-2010 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 07/08/2010
 * Author: Cailong Wu
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/time.h>
#include <unistd.h>

#include "is_port_map.h"
#include "ih_cmd.h"
#include "sw_ipc.h"
#include "console.h"
#include "cli_api.h"
#include "ih_adm.h"
#include "if_common.h"
#include "ae.h"
#include "g8032_ipc.h"
#include "lacp_ipc.h"



/* LACP aggregator info */
uns32 lacp_ag_info[MAX_TRUNK_ID];
RING_PORT g_ring_port;

static PBMP g_app_pbmp_cascade = 0;
static PBMP g_app_pbmp_full = 0;
static PBMP g_app_pbmp_true_full = 0;
static uns32 g_max_num_slot = 0;        /* number of logical slot */
static uns32 g_max_num_if_type = 0;     /* number of port interface type */
static uns32 g_max_num_port = 0;		/* number of port */
static uns32 g_num_dev = 0;             /* number of physical chip */

vlan_create_destroy_handle  g_vlan_create_destroy_handle  = NULL;
port_join_leave_vlan_handle g_port_join_leave_vlan_handle = NULL;
port_join_leave_trunk_handle g_port_join_leave_trunk_handle = NULL;

static uns16 g_cpu_port = 0;

PRODUCT_INFO g_product_info[] = {
	{0,NULL,0},
};

/* store VLAN settings */
VLAN_INFO g_vlan_info[MAX_NUM_VLAN];
VLAN_TABLE_MAP vlan_table_map_info[MAX_NUM_VLAN_ALLOWED];
TRUNK_INFO g_trunk_info;

SLOT_PORT_MAP_INFOR *g_if_slot_info = NULL;

DEV_PBMP *g_dev_pbmp = NULL;

PORT_CONF g_port_conf[MAX_NUM_PORT];
PORT_CONF g_port_conf_default;

const TYPE_NAME_PAIR g_vlan_member[] = {
	{VLAN_MEMBER_NOT_A_MEMBER, "none"},
	{VLAN_MEMBER_EGRESS_UNTAGGED, "untagged"},
	{VLAN_MEMBER_EGRESS_TAGGED, "tagged"},
	{VLAN_MEMBER_EGRESS_UNMODIFIED, "qinq"}
};

#if 0
const TYPE_NAME_PAIR g_if_type_name[] = {
	{IF_TYPE_FE, "fastethernet"},
	{IF_TYPE_GE, "gigabitethernet"},
	{IF_TYPE_CELLULAR, "cellular"},
	{IF_TYPE_TUNNEL, "tunnel"},
//	{IF_TYPE_VT, "virtual-template"},
	{IF_TYPE_SVI, "vlan"},
	{IF_TYPE_LO, "loopback"},
	{IF_TYPE_DIALER, "dialer"},
	{IF_TYPE_SUB_FE, "fastethernet"},
	{IF_TYPE_SUB_GE, "gigabitethernet"}
};

const TYPE_NAME_PAIR g_if_type_name_abbr[] = {
	{IF_TYPE_FE, "FE"},
	{IF_TYPE_GE, "GE"},
	{IF_TYPE_SUB_FE, "FE"},
	{IF_TYPE_SUB_GE, "GE"}	
};
#endif
const TYPE_NAME_PAIR g_duplex_pair[] = {
	{PORT_DUPLEX_AUTO, "auto"},
	{PORT_DUPLEX_FULL, "full"},
	{PORT_DUPLEX_HALF, "half"}
};

const TYPE_NAME_PAIR g_speed_pair[] = {
	{PORT_SPEED_10M, "10"},
	{PORT_SPEED_100M, "100"},
	{PORT_SPEED_1000M, "1000"},
	{PORT_SPEED_2500M, "2500"},
	{PORT_SPEED_AUTO, "auto"}
};

const TYPE_NAME_PAIR g_block_pair[] = {
	{PORT_BLOCK_NONE, "none"},
	{PORT_BLOCK_UNKNOWN_MCAST, "multicast"},
	{PORT_BLOCK_UNKNOWN_UCAST, "unicast"},
	{PORT_BLOCK_UNKNOWN, "both"}
};

const TYPE_NAME_PAIR g_discard_pair[] = {
	{PORT_DISCARD_NONE, "none"},
	{PORT_DISCARD_TAGGED, "tagged"},
	{PORT_DISCARD_UNTAGGED, "untagged"},
	{PORT_DISCARD_BOTH, "both"}
};

const TYPE_NAME_PAIR g_storm_pair[] = {
	{STORM_TRAFFIC_NONE, "none"},
	{STORM_TRAFFIC_UNKNOWN_UCAST, "unicast"},
	{STORM_TRAFFIC_UNKNOWN_MCAST, "multicast"},
	{STORM_TRAFFIC_BROADCAST, "broadcast"},
	{STORM_TRAFFIC_UNKNOWN_UCAST|STORM_TRAFFIC_UNKNOWN_MCAST, "multicast unicast"},
	{STORM_TRAFFIC_BROADCAST|STORM_TRAFFIC_UNKNOWN_MCAST, "broadcast multicast"},
	{STORM_TRAFFIC_UNKNOWN_UCAST|STORM_TRAFFIC_BROADCAST, "broadcast unicast"},
	{STORM_TRAFFIC_UNKNOWN_MCAST|STORM_TRAFFIC_UNKNOWN_UCAST|STORM_TRAFFIC_BROADCAST, "broadcast multicast unicast"},
};

const TYPE_NAME_PAIR g_override_pair[] = {
	{QOS_OVERRIDE_NONE, "none"},
	{QOS_OVERRIDE_PRIORITY_QUEUE, "priority"}
};

const TYPE_NAME_PAIR g_monitor_pair[] = {
	{MONITOR_TYPE_NONE, "none"},
	{MONITOR_TYPE_INGRESS, "ingress"},
	{MONITOR_TYPE_EGRESS, "egress"},
	{MONITOR_TYPE_BOTH, "both"}
};

const TYPE_NAME_PAIR g_stp_state_pair[] = {
	{PORT_STATE_DISABLE, "disabled"},
	{PORT_STATE_BLOCKING, "blocking"},
	{PORT_STATE_LEARNING, "learning"},
	{PORT_STATE_FORWARDING, "forwarding"}
};

const TYPE_NAME_PAIR g_net_event_pair[] = {
	{AE_PHY, "PHY"},
	{AE_VTU_PROB, "VTUPROB"},
	{AE_VTU_DONE, "VTUDONE"},
	{AE_ATU_PROB, "ATUPROB"},
	{AE_ATU_DONE, "ATUDONE"}
};

const TYPE_NAME_PAIR g_frame_mode_pair[] = {
	{PORT_FRAME_MODE_NORMAL, "normal"},
	{PORT_FRAME_MODE_DSA, "dsa"},
	{PORT_FRAME_MODE_PROVIDER, "provider"},
	{PORT_FRAME_MODE_ETHER_TYPE_DSA, "etype"},
	{PORT_FRAME_MODE_DSA_EX, "dsa-ex"}
};

const TYPE_NAME_PAIR g_egress_mode_pair[] = {
	{UNMODIFY_EGRESS, "unmodified"},
	{UNTAGGED_EGRESS, "untagged"},
	{TAGGED_EGRESS, "tagged"},
	{ADD_TAG, "add"}
};

const TYPE_NAME_PAIR g_8021q_mode_pair[] = {
	{PORT_8021Q_DISABLE, "disable"},
	{PORT_8021Q_FALLBACK, "fallback"},
	{PORT_8021Q_CHECK, "check"},
	{PORT_8021Q_SECURE, "secure"}
};

#define TYPE_NAME_ITEM(type) {type, #type}

const TYPE_NAME_PAIR g_phy_event_pair[] = {
	TYPE_NAME_ITEM(AE_PHY_DTE),
	TYPE_NAME_ITEM(AE_PHY_SPEED),
	TYPE_NAME_ITEM(AE_PHY_DUPLEX),
	TYPE_NAME_ITEM(AE_PHY_RXPAGE),
	TYPE_NAME_ITEM(AE_PHY_ANEGDONE),
	TYPE_NAME_ITEM(AE_PHY_LINK),
	TYPE_NAME_ITEM(AE_PHY_SYMERR),
	TYPE_NAME_ITEM(AE_PHY_FLSCRS),
	TYPE_NAME_ITEM(AE_PHY_FIFO),
	TYPE_NAME_ITEM(AE_PHY_MDI),
	TYPE_NAME_ITEM(AE_PHY_EDET),
	TYPE_NAME_ITEM(AE_PHY_POLARITY),
	TYPE_NAME_ITEM(AE_PHY_JABBER)
};

const TYPE_NAME_PAIR g_qinq_pair[] = {
	{QINQ_NONE, "none"},
	{QINQ_PROVIDER, "provider"},
	{QINQ_CUSTOMER, "customer"}
};

const TYPE_NAME_PAIR g_atu_violation_pair[] = {
	{ATU_PROB_MISS, "SA Miss"},
	{ATU_PROB_MEMBER, "SA Member"},
	{ATU_PROB_AGE, "Age"},
	{ATU_PROB_AGE_OUT, "Age Out"},
	{ATU_PROB_FULL, "Full"}
};

const TYPE_NAME_PAIR g_rm_request_type_name[] = {
		TYPE_NAME_ITEM(RM_GET_ID),
		TYPE_NAME_ITEM(RM_DUMP_ATU),
		TYPE_NAME_ITEM(RM_DUMP_MIBs),
		TYPE_NAME_ITEM(RM_RW_REG)
};

BOOL if_port_type_valid(IF_TYPE type)
{
	return ((type >= IF_PORT_TYPE_MIN)
			&& (type <= IF_PORT_TYPE_MAX));
}

BOOL slot_num_valid(uns16 slot)
{
	return ((slot >= MIN_SLOT) && (slot <= MAX_SLOT));
}

IH_STATUS get_product_info(PRODUCT_INFO *product_info)
{
	TRACE_ENTER();

	memcpy(product_info, &g_product_info[0], sizeof(PRODUCT_INFO));

	TRACE_LEAVE();

	return IH_ERR_OK;
}

IH_STATUS get_module_info(uns16 module_id, MODULE_INFO *module_info)
{
	PRODUCT_INFO product_info;
	uns32 i = 0;
	IH_STATUS rv = 0;

	TRACE_ENTER();

	if(!module_info){
		return IH_ERR_INVALID;
	}

	rv = get_product_info(&product_info);
	if(rv){
		LOG_ER("### get product info errno: %d\n", rv);
		return rv;
	}

	for(i = 0; i < product_info.module_count; i++){
		if(module_id == product_info.module_info[i].module_id){
			memcpy(module_info, &product_info.module_info[i], sizeof(MODULE_INFO));
			return IH_ERR_OK;
		}
	}

	TRACE_LEAVE();

	return IH_ERR_NOT_FOUND;
}

int if_build_product_info(void)
{
	PORT_MAP *portmap;
	uns16 i;
	int ret;

	ret = is_port_map_init();
	if (ret != ERR_OK) {
		LOG_ER("is_port_map_init,err(%d)\n",ret);
		return ret;
	}

	g_product_info[0].type = is_chip_info->module;
	g_product_info[0].module_count = 1;
	g_product_info[0].cpu_dev_id = is_chip_info->cpu_dev;
	g_product_info[0].cpu_dev_port = is_chip_info->cpu_port;

	g_product_info[0].module_info = (MODULE_INFO *)malloc(sizeof(MODULE_INFO));
	if (g_product_info[0].module_info == NULL) {
		LOG_ER("if_build_product_info no memory");
		return ERR_NOMEM;
	}

	g_product_info[0].module_info->module_id = 0;
	g_product_info[0].module_info->on = 1;
	g_product_info[0].module_info->with_chip = 1;
	g_product_info[0].module_info->port_map = NULL;
	g_product_info[0].module_info->port_count = is_chip_info->max_port;

	portmap = (PORT_MAP *)malloc(is_chip_info->max_port * sizeof(PORT_MAP));
	if (portmap == NULL) {
		LOG_ER("if_build_product_info no memory");
		free(g_product_info[0].module_info);
		return ERR_NOMEM;
	}

	for(i=0;i<is_chip_info->max_port;i++) {
		if (is_chip_info->port_list[i].speed == IS_SPEED_FAST) {
			portmap[i].type = IF_TYPE_FE;
		} else if (is_chip_info->port_list[i].speed == IS_SPEED_GBIT) {
			portmap[i].type = IF_TYPE_GE;
		} else {
			portmap[i].type = IF_TYPE_TE;
		}

		portmap[i].slot = is_chip_info->port_list[i].slot;
		portmap[i].port = is_chip_info->port_list[i].port;
		portmap[i].dev_id = is_chip_info->port_list[i].dev_id;
		portmap[i].dev_port = is_chip_info->port_list[i].dev_port;
		portmap[i].media = is_chip_info->port_list[i].media;
	}

	g_product_info[0].module_info->port_map = portmap;
	return ERR_OK;
}

void if_release_product_info(void)
{
	if (g_product_info[0].module_info) {
		if (g_product_info[0].module_info->port_map) {
			free(g_product_info[0].module_info->port_map);
		}

		free(g_product_info[0].module_info);
	}

	is_port_map_cleanup();
}

/*
 * this routine build port information. must be called at init phase
 *
 *
 * @return IH_ERR_OK for ok, others for error
 */
int16 if_slot_portmap_init(void)
{
	uns32 i = 0;
	uns32 j = 0;
	uns32 s = 0;
	uns16 k = 0;
	int16 idx = 0;
	int16 port = 0;
	uns16 wrong_slot = 0;
	uns16 wrong_type = 0;
	uns16 num_entry = 0;
	uns16 num_type = 0;
	int16 slot_idx = 0;
	int16 type_idx = 0;
	int16 port_idx = 0;
	int16 base = 0;
	PBMP slot_pbmp = 0;
	PBMP if_type_pbmp = 0;
	uns16 max_slot = 0;
	uns16 min_slot = 0;
	IF_TYPE max_type = 0;
	IF_TYPE min_type = 0;
	PBMP dev_pbmp_full = 0;
	DEV_PBMP *dev_pbmp = NULL;
	IH_STATUS rv = 0;
	PRODUCT_INFO product_info;
	int32 ret;
	char product_name[16];

	TRACE_ENTER();

	LOG_DB("### port map init...\n");

	ret = if_build_product_info();
	if (ret != ERR_OK) {
		LOG_ER("if_build_product_info errno: %d\n", ret);
		return ret;
	}

	rv = get_product_info(&product_info);
	if(rv){
		LOG_ER("### get product info errno: %d\n", rv);
		return rv;
	}

	if(!product_info.module_count || !product_info.module_info){
		return IH_ERR_INVALID;
	}

	g_app_pbmp_cascade = 0;
	g_app_pbmp_true_full = 0;
	g_max_num_slot = 0;
	g_max_num_if_type = 0;
	g_max_num_port = 0;
	g_num_dev = 0;

	LOG_DB("### init product %s(%#x), available module num %d.\n",
			is_model_name(product_info.type,product_name,sizeof(product_name)),
			product_info.type,
			product_info.module_count);

	/* pre statistics */
	for(i = 0; i < product_info.module_count; i++){

		LOG_DB("### module %d, %d ports module state %s", product_info.module_info[i].module_id, product_info.module_info[i].port_count, product_info.module_info[i].on? "ON": "OFF");

		for(j = 0; j < product_info.module_info[i].port_count; j++){
			if(!if_port_type_valid(product_info.module_info[i].port_map[j].type)){
				LOG_ER("### module %d, %d ports module, portmap %d, invalid if type %d\n", product_info.module_info[i].module_id, product_info.module_info[i].port_count, j, product_info.module_info[i].port_map[j].type);

				wrong_type++;
				continue;
			}

			if(!slot_num_valid(product_info.module_info[i].port_map[j].slot)){
				LOG_ER("### module %d, %d ports module, portmap %d, invalid slot %d\n", product_info.module_info[i].module_id, product_info.module_info[i].port_count, j, product_info.module_info[i].port_map[j].slot);

				wrong_slot++;
				continue;
			}

			if(!i && !j){
				max_slot = product_info.module_info[i].port_map[j].slot;
				min_slot = product_info.module_info[i].port_map[j].slot;
				max_type = product_info.module_info[i].port_map[j].type;
				min_type = product_info.module_info[i].port_map[j].type;
			}else{
				if(product_info.module_info[i].port_map[j].slot > max_slot){
					max_slot = product_info.module_info[i].port_map[j].slot;
				}

				if(product_info.module_info[i].port_map[j].slot < min_slot){
					min_slot = product_info.module_info[i].port_map[j].slot;
				}

				if(product_info.module_info[i].port_map[j].type > max_type){
					max_type = product_info.module_info[i].port_map[j].type;
				}

				if(product_info.module_info[i].port_map[j].type < min_type){
					min_type = product_info.module_info[i].port_map[j].type;
				}
			}

			slot_pbmp |= (1 << product_info.module_info[i].port_map[j].slot);
			if_type_pbmp |= (1 << product_info.module_info[i].port_map[j].type);
		}
	}

	g_max_num_if_type = bmp_count(if_type_pbmp);
	g_max_num_slot    = bmp_count(slot_pbmp);

	num_type = get_port_if_type_count();

	num_entry = get_slot_count() * num_type;

	if(g_if_slot_info){
		free(g_if_slot_info);
		g_if_slot_info = NULL;
	}

	if(!num_entry){
		LOG_ER("no port found!\n");
		return IH_ERR_EMPTY;
	}

	g_if_slot_info = (SLOT_PORT_MAP_INFOR *)malloc(sizeof(SLOT_PORT_MAP_INFOR) * num_entry);
	if(!g_if_slot_info){
		LOG_ER("no enough memory!\n");
		return IH_ERR_MEM_ALLOC;
	}
	LOG_DB("### malloc size: %zd\n", sizeof(SLOT_PORT_MAP_INFOR) * num_entry);

	bzero(g_port_conf, sizeof(g_port_conf));
	bzero(&g_port_conf_default, sizeof(PORT_CONF));

	/* build each slot's port information */
	for(s = min_slot; s <= max_slot; s++){

		type_idx = 0;
		for(k = min_type; k <= max_type; k++){
			PBMP app_pbmp = 0;
			PBMP dev_pbmp = 0;
			uns16 count = 0;
			int16 port_min = 0;
			int16 port_max = 0;

			for(i = 0; i < product_info.module_count; i++){
				for(j = 0; j < product_info.module_info[i].port_count; j++){
					if(!if_port_type_valid(product_info.module_info[i].port_map[j].type)){
						continue;
					}

					if(!slot_num_valid(product_info.module_info[i].port_map[j].slot)){
						continue;
					}

					/* slot filtering */
					if(product_info.module_info[i].port_map[j].slot != s){
						continue;
					}

					/* interface type filtering */
					if(product_info.module_info[i].port_map[j].type != k){
						continue;
					}

					port  = product_info.module_info[i].port_map[j].port;

					/* port max & min */
					if(!port_min && !port_max){
						port_min = port;
						port_max = port_min;
					}else{
						if(port < port_min){
							port_min = port;
						}

						if(port > port_max){
							port_max = port;
						}
					}

					port_idx = port + base;
					LOG_DB("####### product_info.cpu_dev_id = %d,  product_info.cpu_dev_port = %d", product_info.cpu_dev_id, product_info.cpu_dev_port);
					/* check CPU port */
					if( product_info.cpu_dev_id == product_info.module_info[i].port_map[j].dev_id &&
						product_info.cpu_dev_port == product_info.module_info[i].port_map[j].dev_port){
						LOG_DB("### got CPU port %d",port_idx);
						g_cpu_port = port_idx;
					}

					/* check Cascade port */
					if( IS_MEDIA_CASCADE == product_info.module_info[i].port_map[j].media){
						g_app_pbmp_cascade |= (1 << port_idx);
					}

					if(!g_port_conf[port_idx].valid){
						g_port_conf[port_idx].portmap = product_info.module_info[i].port_map[j];
						g_port_conf[port_idx].valid   = TRUE;

						app_pbmp |= (1 << port_idx);
						dev_pbmp |= (1 << product_info.module_info[i].port_map[j].dev_port);

						count++;
					}else{
						LOG_ER("### replicated logical port %d?\n", port_idx);
					}
				}
			}

			idx = slot_idx*num_type + type_idx;

			g_if_slot_info[idx].slot = s;
			g_if_slot_info[idx].type = k;
			g_if_slot_info[idx].app_pbmp = app_pbmp;
			g_if_slot_info[idx].port_min = port_min;
			g_if_slot_info[idx].port_max = port_max;
			g_if_slot_info[idx].port_count = count;
			g_if_slot_info[idx].base = base;

			LOG_DB("slot %d type %s, %d ports: 0x%x, min %d, max %d, base %d\n", s, get_if_name_by_type(k), count, app_pbmp, port_min, port_max, base);

			dev_pbmp_full |= dev_pbmp;
			g_app_pbmp_true_full |= app_pbmp;

			base += count;

			type_idx++;
		}

		slot_idx++;
	}

	g_app_pbmp_full = (g_app_pbmp_true_full & ~(1 << g_cpu_port));
	g_app_pbmp_full = (g_app_pbmp_full & ~g_app_pbmp_cascade);

	g_max_num_port = pbmp_port_count(g_app_pbmp_full);

	LOG_DB("\nnum slot: %d\n"
		   "min slot: %d\n"
		   "max slot: %d\n"
		   "num port: %d\n"
		   "num if type: %d\n"
		   "min if type: %d\n"
		   "max if type: %d\n",
		   g_max_num_slot,
		   min_slot,
		   max_slot,
		   g_max_num_port,
		   g_max_num_if_type,
		   min_type,
		   max_type);

	if(!g_if_slot_info){
		LOG_DB("init: g_if_slot_info is NULL!!!!\n");
	}

	dev_pbmp_free();

	/* get device port information. in g_dev_pbmp */
	for(i = 0; i < product_info.module_count; i++){
		for(j = 0; j < product_info.module_info[i].port_count; j++){
			if(!if_port_type_valid(product_info.module_info[i].port_map[j].type)){
				continue;
			}

			if(!slot_num_valid(product_info.module_info[i].port_map[j].slot)){
				continue;
			}

			port_idx = get_logical_port_number(product_info.module_info[i].port_map[j].type, product_info.module_info[i].port_map[j].slot, product_info.module_info[i].port_map[j].port);
			if(!port_is_valid(port_idx)){
				continue;
			}

			dev_pbmp = dev_pbmp_get(product_info.module_info[i].port_map[j].dev_id);
			if(!dev_pbmp){
				//alloc new node
				dev_pbmp = (DEV_PBMP *)malloc(sizeof(DEV_PBMP));
				if(!dev_pbmp){
					//free
					if(g_if_slot_info){
						free(g_if_slot_info);
						g_if_slot_info = NULL;
					}

					dev_pbmp_free();

					LOG_DB("no enough memory!\n");

					return IH_ERR_MEM_ALLOC;
				}
				LOG_DB("### malloc size: %zd\n", sizeof(DEV_PBMP));
				LOG_DB("init: add dev %d!!!!\n", product_info.module_info[i].port_map[j].dev_id);

				//add node
				bzero(dev_pbmp, sizeof(DEV_PBMP));
				dev_pbmp->dev_id = product_info.module_info[i].port_map[j].dev_id;

				dev_pbmp->next = g_dev_pbmp;
				g_dev_pbmp = dev_pbmp;

				g_num_dev++;
			}

			dev_pbmp->dev_pbmp |= (1 << product_info.module_info[i].port_map[j].dev_port);
			dev_pbmp->app_pbmp |= (1 << port_idx);

			/* mark cascade port */
			if(pbmp_has_port(g_app_pbmp_cascade, port_idx)){
				dev_pbmp->app_pbmp_cascade |= (1 << port_idx);
				dev_pbmp->dev_pbmp_cascade |= (1 << product_info.module_info[i].port_map[j].dev_port);
			}

			/* mark CPU dev */
			if(pbmp_has_port(dev_pbmp->app_pbmp, g_cpu_port)){
				dev_pbmp->cpu_dev = TRUE;
			}
		}
	}

	if(!g_dev_pbmp){
		LOG_DB("init: dev pbmp is NULL!!!!\n");
	}

	/* output information */
	if(wrong_slot){
		LOG_DB("Have %d port%s with wrong slot number!", wrong_slot, (wrong_slot > 1)? "s": "");
	}

	if(wrong_type){
		LOG_DB("Have %d port%s with wrong interface type!", wrong_type, (wrong_type > 1)? "s": "");
	}

	if(!wrong_slot && !wrong_type){
		if(pbmp_port_count(dev_pbmp_full) < g_max_num_port){
			LOG_DB("Have replicated physical ports!");
		}

		if(pbmp_port_count(g_app_pbmp_true_full) < g_max_num_port){
			LOG_DB("Have replicated logical ports!");
		}
	}

	LOG_DB("port map init done!!!\n");

	TRACE_LEAVE();

	return IH_ERR_OK;
}

/*
 * this routine free port information related memory. must be called on task exit
 *
 *
 * @return
 */
void if_slot_portmap_cleanup(void)
{
	if(g_if_slot_info){
		free(g_if_slot_info);
		g_if_slot_info = NULL;
	}

	dev_pbmp_free();

	if_release_product_info();

	return;
}

/*
 * return array index of specified slot & interface type
 *
 * @param type	    interface type
 * @param slot	    panel slot number
 *
 * @return array index
 */
int16 get_if_idx_by_type_slot(IF_TYPE type, uns16 slot)
{
	uns32 i = 0;
	uns32 count = 0;

	if(slot < 0){
		return -1;
	}

	count = get_slot_count() * get_port_if_type_count();

	for(i = 0; i < count; i++){
       		if((type == g_if_slot_info[i].type) && (slot == g_if_slot_info[i].slot)){
        		return i;
        	}
	}

	return -1;
}


/*
 * return port count of specified slot & interface type
 *
 * @param type	    interface type
 * @param slot	    panel slot number
 *
 * @return port count
 */
uns16 get_port_count_by_type_slot(IF_TYPE type, uns16 slot)
{
	int16 idx = 0;

	idx = get_if_idx_by_type_slot(type, slot);
	if(idx < 0){
		return 0;
	}

	return g_if_slot_info[idx].port_count;
}

/*
 * return minimum panel port number of specified slot & interface type
 *
 * @param type	    interface type
 * @param slot	    panel slot number
 *
 * @return minimum panel port number
 */
uns16 get_port_min_by_type_slot(IF_TYPE type, uns16 slot)
{
	int16 idx = 0;

	idx = get_if_idx_by_type_slot(type, slot);
	if(idx < 0){
		return 0;
	}

	return g_if_slot_info[idx].port_min;
}


/*
 * return maximum panel port number of specified slot & interface type
 *
 * @param type	    interface type
 * @param slot	    panel slot number
 *
 * @return maximum panel port number
 */
uns16 get_port_max_by_type_slot(IF_TYPE type, uns16 slot)
{
	int16 idx = 0;

	idx = get_if_idx_by_type_slot(type, slot);
	if(idx < 0){
		return 0;
	}

	return g_if_slot_info[idx].port_max;
}

/*
 * return port base of specified slot & interface type
 *
 * @param type	    interface type
 * @param slot	    panel slot number
 *
 * @return port base. return value < 0 indicates wrong parameter
 */
int16 get_port_base_by_type_slot(IF_TYPE type, uns16 slot)
{
	int16 idx = 0;

	idx = get_if_idx_by_type_slot(type, slot);
	if(idx < 0){
		return -1;
	}

	return g_if_slot_info[idx].base;
}

/*
 * convert panel port to logical port
 *
 * @param type	    interface type
 * @param slot	    panel slot number
 * @param port      panel port number
 *
 * @return logical port number. return value < 0 indicates wrong parameter
 */
int16 get_logical_port_number(IF_TYPE type, uns16 slot, int16 port)
{
	int16 base = 0;

	LOG_DB("get_logical_port_number of type.slot.port.[%d:%d:%d]", type, slot, port);
	base = get_port_base_by_type_slot(type, slot);
	if(base < 0){
		LOG_DB("get_logical_port_number base failed");
		return -1;
	}

	if(!port_is_valid(base + port)){
		return -1;
	}

    LOG_DB("get_logical_port_number %d", base + port);
	return base + port;
}

/*
 * convert logical port to panel port
 *
 * @param logical_port logical port number
 * @param type	       (out)interface type
 * @param slot	       (out)panel slot number
 * @param port         (out)panel port number
 *
 * @return IH_ERR_OK for ok, others for error
 */
int16 get_panel_port_number(int16 logical_port, IF_TYPE *type, uns16 *slot, uns16 *port)
{
	if(!port_is_valid(logical_port)){
		return IH_ERR_INVALID;
	}

	if(!type || !slot || !port){
		return IH_ERR_INVALID;
	}

	*type = g_port_conf[logical_port].portmap.type;
	*slot = g_port_conf[logical_port].portmap.slot;
	*port = g_port_conf[logical_port].portmap.port;

	return IH_ERR_OK;
}

/*
 * get port map information according to logical port information
 *
 * @param port	  logical port number
 *
 * @return port map information
 */
PORT_MAP *get_portmap_by_app_port(int16 port)
{
	if(!port_is_valid(port)){
		return NULL;
	}

	return &g_port_conf[port].portmap;
}

/*
 * get port map information from physical port
 *
 * @param dev_id	    chip number
 * @param dev_port	    physical port number
 *
 * @return port map information
 */
PORT_MAP *get_portmap_by_dev_port(uns16 dev_id, uns16 dev_port)
{
	uns16 i = 0;
	uns16 max_num = 0;

	max_num = ARRAY_COUNT(g_port_conf);

	for(i = 0; i < max_num; i++){

		if(!g_port_conf[i].valid){
			continue;
		}

        if((dev_port == g_port_conf[i].portmap.dev_port)
			&& (dev_id == g_port_conf[i].portmap.dev_id)){
        	return &g_port_conf[i].portmap;
        }
	}

	return NULL;
}

/*
 * convert physical port to app pbmp
 *
 * @param dev_id	    chip number
 * @param dev_port	    physical port number
 *
 * @return port map information
 */
PBMP dev_port_to_app_pbmp(uns16 dev_id, uns16 dev_port)
{
	uns16 i = 0;
	uns16 max_num = 0;

	max_num = ARRAY_COUNT(g_port_conf);

	for(i = 0; i < max_num; i++){

		if(!g_port_conf[i].valid){
			continue;
		}

        if((dev_port == g_port_conf[i].portmap.dev_port)
			&& (dev_id == g_port_conf[i].portmap.dev_id)){
        	return (1 << i);
        }
	}

	return 0;
}


/*
 * convert physical port to logical port number
 *
 * @param dev_id	    chip number
 * @param dev_port	    physical port number
 *
 * @return logical port number. return value < 0 indicates wrong parameter
 */
int16 dev_port_to_app_port(uns16 dev_id, uns16 dev_port)
{
	uns16 i = 0;
	uns16 max_num = 0;

	max_num = ARRAY_COUNT(g_port_conf);

	for(i = 0; i < max_num; i++){

		if(!g_port_conf[i].valid){
			continue;
		}

        if((dev_port == g_port_conf[i].portmap.dev_port)
			&& (dev_id == g_port_conf[i].portmap.dev_id)){
        	return i;
        }
	}

	return -1;
}

/*
 * convert logical port to physical port number
 *
 * @param port	        logical port number
 * @param dev_id	    (OUT)chip number
 * @param dev_port	    (OUT)physical port number
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 app_port_to_dev_port(int16 port, uns16 *dev_id, uns16 *dev_port)
{
	if(!port_is_valid(port)){
		return IH_ERR_INVALID;
	}

	if(!dev_id || !dev_port){
		return IH_ERR_INVALID;
	}

	*dev_id = g_port_conf[port].portmap.dev_id;
	*dev_port = g_port_conf[port].portmap.dev_port;

	return IH_ERR_OK;
}

/*
 * get switch port count
 *
 * @return switch port count
 */
uns16 get_switch_port_count(void)
{
	return g_max_num_port + 1; /* FIXME: include CPU port */
}

/*
 * return #port# interface type count
 *
 *
 * @return interface type count
 */
uns32 get_port_if_type_count(void)
{
	return g_max_num_if_type;
}

/*
 * return slot count
 *
 *
 * @return slot count
 */
uns32 get_slot_count(void)
{
	return g_max_num_slot;
}

/*
 * return dev(chip) count
 *
 *
 * @return dev(chip) count
 */
uns32 get_dev_count(void)
{
	return g_num_dev;
}

/*
 * to see if current system is a multi-chip configuration
 *
 *
 * @return TRUE indicates multi-chip, FALSE otherwise
 */
BOOL multi_chip(void)
{
	return (get_dev_count() > 1);
}

/*
 * return logical port bitmap of cascade ports
 *
 *
 * @return logical port bitmap
 */
PBMP get_app_pbmp_cascade(void)
{
	return g_app_pbmp_cascade;
}

/*
 * return logical port bitmap of all ports
 *
 *
 * @return logical port bitmap
 */
PBMP get_app_pbmp_full(void)
{
	return g_app_pbmp_full;
}

/*
 * return logical port bitmap of all ports(include CPU port)
 *
 *
 * @return logical port bitmap
 */
PBMP get_app_pbmp_true_full(void)
{
	return g_app_pbmp_true_full;
}

/*
 * return logical CPU port number
 *
 *
 * @return logical CPU port number
 */
int16 get_app_cpu_port(void)
{
	return g_cpu_port;
}

/*
 * return logical port bitmap of specified slot & interface type
 *
 * @param type	    interface type
 * @param slot	    panel slot number
 *
 * @return logical port bitmap
 */
PBMP get_app_pbmp_by_type_slot(IF_TYPE type, uns16 slot)
{
	int16 idx = 0;

	idx = get_if_idx_by_type_slot(type, slot);
	if(idx < 0){
		return 0;
	}

	return g_if_slot_info[idx].app_pbmp;
}

/* #################### application layer port bitmap operation routine #################### */


/*
 * to see if port bitmap is valid
 *
 * @param pbmp		port bitmap
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL pbmp_valid(PBMP pbmp)
{
	/* out of the full set */
	if(~get_app_pbmp_true_full() & pbmp){
		return FALSE;
	}

	return TRUE;
}

/*
 * port bitmap initialization
 *
 * @param pbmp		(in/out)port bitmap
 *
 * @return IH_ERR_OK indicates OK, otherwise indicates ERROR.
 */
IH_STATUS pbmp_init(PBMP *pbmp)
{
	if(!pbmp){
		return IH_ERR_INVALID;
	}

	*pbmp = 0;

	return IH_ERR_OK;
}

/*
 * to see if port bitmap is empty
 *
 * @param pbmp		port bitmap
 *
 * @return TRUE indicates empty, FALSE otherwise
 */
BOOL pbmp_empty(PBMP pbmp)
{
	return !pbmp;
}

/*
 * get port count in port bitmap
 *
 * @param pbmp	    port bitmap
 *
 * @return port count in port bitmap
 */
uns32 pbmp_port_count(PBMP pbmp)
{
	return bmp_count(pbmp);
}

/*
 * to see if port bitmap has port
 *
 * @param pbmp		port bitmap
 * @param port		logical port number
 *
 * @return TRUE indicates port belongs to pbmp, FALSE otherwise
 */
BOOL pbmp_has_port(PBMP pbmp, int16 port)
{
	if(!pbmp_valid(pbmp)){
		return FALSE;
	}

	if(!port_is_valid(port)){
		return FALSE;
	}

	return pbmp & (1 << port);
}

/*
 * add port to port bitmap
 *
 * @param pbmp		(in/out)port bitmap
 * @param port		logical port number
 *
 * @return IH_ERR_OK indicates OK, otherwise indicates ERROR.
 */
IH_STATUS pbmp_add_port(PBMP *pbmp, int16 port)
{
	if(!pbmp){
		return IH_ERR_NULL_PTR;
	}

	if(!pbmp_valid(*pbmp)){
		return IH_ERR_PARAM;
	}

	if(!port_is_valid(port)){
		return IH_ERR_PARAM;
	}

	*pbmp |= (1 << port);

	return IH_ERR_OK;
}

/*
 * delete port from port bitmap
 *
 * @param pbmp		(in/out)port bitmap
 * @param port		logical port number
 *
 * @return IH_ERR_OK indicates OK, otherwise indicates ERROR
 */
IH_STATUS pbmp_del_port(PBMP *pbmp, int16 port)
{
	if(!pbmp){
		return IH_ERR_NULL_PTR;
	}

	if(!pbmp_valid(*pbmp)){
		return IH_ERR_PARAM;
	}

	if(!port_is_valid(port)){
		return IH_ERR_PARAM;
	}

	*pbmp &= ~(1 << port);

	return IH_ERR_OK;
}

/*
 * bitmap and operation
 *
 * @param pbmp1		port bitmap
 * @param pbmp2		port bitmap
 *
 * @return anded port bitmap
 */
PBMP pbmp_and(PBMP pbmp1, PBMP pbmp2)
{
	return pbmp1 & pbmp2;
}

/*
 * bitmap or operation
 *
 * @param pbmp1		port bitmap
 * @param pbmp2		port bitmap
 *
 * @return ored port bitmap
 */
PBMP pbmp_or(PBMP pbmp1, PBMP pbmp2)
{
	return pbmp1 | pbmp2;
}

/*
 * bitmap xor operation
 *
 * @param pbmp1		port bitmap
 * @param pbmp2		port bitmap
 *
 * @return xored port bitmap
 */
PBMP pbmp_xor(PBMP pbmp1, PBMP pbmp2)
{
	return pbmp1 ^ pbmp2;
}

/*
 * bitmap complement operation
 *
 * @param pbmp		port bitmap
 *
 * @return Complement of port bitmap(the rest port bitmap in full port bitmap except pbmp)
 */
PBMP pbmp_complement(PBMP pbmp)
{
	return (get_app_pbmp_full() & ~pbmp);
}

/*
 * bitmap sub operation
 *
 * @param pbmp1		port bitmap
 * @param pbmp2		port bitmap
 *
 * @return port bitmap in pbmp1 but not in pbmp2
 */
PBMP pbmp_sub(PBMP pbmp1, PBMP pbmp2)
{
	return (pbmp1 ^ pbmp2) & pbmp1;
}

/*
 * bit-1 count in bitmap
 *
 * @param bmp	    bitmap
 *
 * @return port count in port bitmap
 */
uns32 bmp_count(BMP bmp)
{
	uns32 count = 0;

	while(bmp){
		if(bmp & 0x1){
			count++;
		}

		bmp >>= 1;
	}

	return count;
}

BOOL bmp_has_bit(BMP bmp, uns16 bit)
{
	return bmp & (1 << bit);
}

/*
 * convert application layer port bitmap to string.
 *  (example: if pbmp == 6, then formatted string may be "fastethernet 1/1-2" if abbr == FALSE, or it will be "FE 1/1-2")
 *
 * @param only_port	FALSE: may display trunk, TRUE: only display port
 * @param abbr		FALSE for fastethernet|gigabitethernet, TRUE for FE|GE
 * @param pbmp		application layer port bitmap
 * @param port_str	(out)formated type/slot/port string
 *
 * @return IH_ERR_OK for ok, others for error
 */
static int16 app_pbmp_to_str_original(BOOL only_port, BOOL abbr, PBMP app_pbmp, PORT_STR *port_str)
{
	PORT_MAP *portmap = NULL;
	PBMP tmpbmp = 0;
	PBMP logpbmp = 0;
	uns16 i = 0;
	uns16 j = 0;
	uns32 len = 0;
	uns32 buff_len = 0;
	int16 rv = 0;
	BOOL first = TRUE;
	char tmp_str[MAX_PORT_STR_LEN] = {0};

	if(!pbmp_valid(app_pbmp) || !port_str){
		return IH_ERR_PARAM;
	}

	bzero(port_str->str, sizeof(port_str->str));

	buff_len = sizeof(port_str->str);

	/* hide CPU port */
	app_pbmp &= ~(1 << g_cpu_port);

	/* hide cascade port */
	if(multi_chip()){
		app_pbmp &= ~g_app_pbmp_cascade;
	}

	if(!only_port){
		first = TRUE;
		for(i = 0; i < MAX_TRUNK_GROUP; i++){

			if(!g_trunk_info.trunk[i].member){
				continue;
			}

			if(g_trunk_info.trunk[i].member & app_pbmp){
				len += snprintf(port_str->str+len, buff_len-len, "%sT%d ", first? "": ",", i+1);
				if(first){
					first = FALSE;
				}

				app_pbmp &= ~(g_trunk_info.trunk[i].member);
			}
		}
	}

	for(i = IF_PORT_TYPE_MIN; i <= IF_PORT_TYPE_MAX; i++){

		/* if any type filter exist, add here */

		first = TRUE;
		for(j = MIN_SLOT; j <= MAX_SLOT; j++){ /* there should be no replicated logical ports for specified type & slot */

			tmpbmp = app_pbmp & get_app_pbmp_by_type_slot(i, j);
			if(tmpbmp){
				bzero(tmp_str, sizeof(tmp_str));

				logpbmp = 0;

				PBMP_FOREACH_PORTMAP(tmpbmp, portmap){
					logpbmp |= 1 << portmap->port;
				}
				PBMP_FOREACH_PORTMAP_END

				rv = slot_pbmp_to_str(j, logpbmp, tmp_str);
				if(!rv && strlen(tmp_str)){
					if(abbr){
						len += snprintf(port_str->str+len, buff_len-len, "%s%s%s", first? get_if_name_abbr_by_type(i): ",", first? " ": "", tmp_str);
					}else{
						len += snprintf(port_str->str+len, buff_len-len, "%s%s%s", first? get_if_name_by_type(i): ",", first? " ": "", tmp_str);
					}
					if(first){
						first = FALSE;
					}
				}
			}
		}

		if(!first){
			len += snprintf(port_str->str+len, buff_len-len, " ");
		}
	}

	if(len > 1){ /* remove "," at the end */
		port_str->str[len-1] = 0;
	}

	return IH_ERR_OK;
}

/*
 * convert application layer port bitmap to string.
 *  (example: if pbmp == 6, then formatted string may be "fastethernet 1/1-2")
 *  this routine may display trunk such as: "T1, fastethernet 1/3-5"
 *
 * @param pbmp		application layer port bitmap
 * @param port_str	(out)formated type/slot/port string
 *
 * @return IH_ERR_OK for ok, others for error
 */
int16 app_pbmp_to_str(PBMP app_pbmp, PORT_STR *port_str)
{
	return app_pbmp_to_str_original(FALSE, FALSE, app_pbmp, port_str);
}

/*
 * convert application layer port bitmap to string.
 *  (example: if pbmp == 6, then formatted string may be "FE 1/1-2")
 *  this routine may display trunk such as: "T1, FE 1/3-5"
 *
 * @param pbmp		application layer port bitmap
 * @param port_str	(out)formated type/slot/port string
 *
 * @return IH_ERR_OK for ok, others for error
 */
int16 app_pbmp_to_str_abbr(PBMP app_pbmp, PORT_STR *port_str)
{
	return app_pbmp_to_str_original(FALSE, TRUE, app_pbmp, port_str);
}

/*
 * convert application layer port bitmap to string.
 *  (example: if pbmp == 6, then formatted string may be "fastethernet 1/1-2")
 *  this routine only display port information
 *
 * @param pbmp		application layer port bitmap
 * @param port_str	(out)formated type/slot/port string
 *
 * @return IH_ERR_OK for ok, others for error
 */
int16 app_pbmp_to_port_str(PBMP app_pbmp, PORT_STR *port_str)
{
	return app_pbmp_to_str_original(TRUE, FALSE, app_pbmp, port_str);
}

/*
 * convert application layer port bitmap to string.
 *  (example: if pbmp == 6, then formatted string may be "FE 1/1-2")
 *  this routine only display port information
 *
 * @param pbmp		application layer port bitmap
 * @param port_str	(out)formated type/slot/port string
 *
 * @return IH_ERR_OK for ok, others for error
 */
int16 app_pbmp_to_port_str_abbr(PBMP app_pbmp, PORT_STR *port_str)
{
	return app_pbmp_to_str_original(TRUE, TRUE, app_pbmp, port_str);
}

/* #################### parameter check functions #################### */

/*
 * to see if MAC address is a multicast address. bit[40] == 1
 *
 * @param addr		MAC address
 *
 * @return TRUE indicates multicast, FALSE otherwise
 */
BOOL mac_is_mcast(MAC_ADDR addr)
{
	return ((addr.mac[0] & 0x1) && (!mac_is_bcast(addr)));
}

/*
 * to see if MAC address is a broadcast address.
 *
 * @param addr		MAC address
 *
 * @return TRUE indicates broadcast, FALSE otherwise
 */
BOOL mac_is_bcast(MAC_ADDR addr)
{
	return ((0xff == addr.mac[0]) && (0xff == addr.mac[1]) && (0xff == addr.mac[2]) && (0xff == addr.mac[3]) && (0xff == addr.mac[4]) && (0xff == addr.mac[5]));
}

/*
 * to see if MAC address is a unicast address. bit[40] != 1
 *
 * @param addr		MAC address
 *
 * @return TRUE indicates unicast, FALSE otherwise
 */
BOOL mac_is_ucast(MAC_ADDR addr)
{
	return (!(addr.mac[0] & 0x1));
}

/*
 * to see if MAC address is a all zero address.
 *
 * @param addr		MAC address
 *
 * @return TRUE indicates broadcast, FALSE otherwise
 */
BOOL mac_is_all_zero(MAC_ADDR addr)
{
	return ((0x0 == addr.mac[0]) && (0x0 == addr.mac[1]) && (0x0 == addr.mac[2]) && (0x0 == addr.mac[3]) && (0x0 == addr.mac[4]) && (0x0 == addr.mac[5]));
}

int get_vlan_info_idx_by_vid(int vid)
{
    int i;

    for (i = 0; i < MAX_NUM_VLAN_ALLOWED; i++) {
        if (vid == vlan_table_map_info[i].vid) {
            return vlan_table_map_info[i].vlan_info_idx;
        }
    }

    for (i = 0; i < MAX_NUM_VLAN_ALLOWED; i++) {
        if (vlan_table_map_info[i].vid == 0) {
            vlan_table_map_info[i].vid = vid;
            vlan_table_map_info[i].vlan_info_idx = i + 1;
            return vlan_table_map_info[i].vlan_info_idx;
        }
    }


    return -1;
}

void del_vlan_info_idx_by_vid(int vid)
{
    int i;

    for (i = 0; i < MAX_NUM_VLAN_ALLOWED; i++) {
        if (vid == vlan_table_map_info[i].vid) {
            memset(&vlan_table_map_info[i], 0, sizeof(vlan_table_map_info[i]));
        }
    }

    return;
}

/*
 * to see if VLAN info idx is valid
 *
 * @param vid		VLAN id
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL vlan_info_idx_is_valid(uns16 vlan_info_idx)
{
	return ((vlan_info_idx > 0) && (vlan_info_idx <= MAX_NUM_VLAN_ALLOWED));
}


/*
 * to see if VLAN id is valid
 *
 * @param vid		VLAN id
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL vid_is_valid(uns16 vid)
{
	return ((vid >= MIN_VLAN_ID) && (vid <= MAX_VLAN_ID));
}


/*
 * to see if VLAN id is in use
 *
 * @param vid		VLAN id
 *
 * @return TRUE indicates in use, FALSE otherwise
 */
BOOL vlan_in_use(uns16 vid)
{
	if(!vid_is_valid(vid)){
		return FALSE;
	}

	return g_vlan_info[vid - 1].valid;
}

/*
 * to see if VLAN is used by other service
 *
 * @param vid		VLAN id
 *
 * @return TRUE indicates in use, FALSE otherwise
 */
BOOL vlan_is_used_by_g8032(uns16 vid)
{
	if(!vid_is_valid(vid)){
		return FALSE;
	}

	return g_vlan_info[vid - 1].flag & SERVICE_G8032_USE_VLAN;
}

/*
 * to see if port is valid
 *
 * @param port        logical port number
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL port_is_valid(int16 port)
{
	/*TODO: detect IR900 model*/
#if 1
	if(!PORT_RANGE_VALID(port)){
		return FALSE;
	}

	if(!g_port_conf[port].valid){
		return FALSE;
	}
#else
	if (port < 1 || port > 2)
		return FALSE;
#endif
	return TRUE;
}

/*
 * to see if port is off
 *
 * @param port        logical port number
 *
 * @return TRUE indicates off, FALSE otherwise
 */
BOOL port_is_off(int16 port)
{
	/* to do */

	return FALSE;
}


BOOL port_in_vlan(int16 port, uns16 vid)
{
	if(!vid_is_valid(vid)){
		return FALSE;
	}

	if(!port_is_valid(port)){
		return FALSE;
	}

	return (g_vlan_info[vid - 1].member & (1 << port));
}

BOOL port_is_vlan_member(int16 port, uns16 vid, MEMBER_TYPE type)
{
	if(!port_in_vlan(port, vid)){
		return FALSE;
	}

	if(VLAN_MEMBER_EGRESS_UNTAGGED == type){
		return g_vlan_info[vid - 1].untagged & (1 << port);
	}else if(VLAN_MEMBER_EGRESS_TAGGED == type){
		return g_vlan_info[vid - 1].tagged & (1 << port);
	}else if(VLAN_MEMBER_EGRESS_UNMODIFIED == type){
		return g_vlan_info[vid - 1].unmodified & (1 << port);
	}else{
		return FALSE;
	}

	return FALSE;
}

/*
 * to see if port is a trunk member
 *
 * @param port		logical port number
 * @param member	(out)TRUE indicates port is a trunk member, FALSE otherwise
 * @param tid		(out)port is a member of trunk 'tid' if returned member is TRUE
 *
 * @return IH_ERR_OK for ok, others for error
 */
IH_STATUS port_get_trunk_member(int16 port, BOOL *member, uns16 *tid)
{
	uns32 i = 0;

	if(!member || !tid){
		return IH_ERR_NULL_PTR;
	}

	if(!port_is_valid(port)){
		return IH_ERR_INVALID;
	}

	*member = FALSE;

	for(i = 0; i < MAX_TRUNK_GROUP; i++){

		if(g_trunk_info.trunk[i].member & (1 << port)){
			*tid = i+1;
			*member = TRUE;
			return IH_ERR_OK;
		}
	}

	return IH_ERR_OK;
}

/*
 * to see if port is a trunk member
 *
 * @param port		logical port number
 *
 * @return TRUE indicates port is a trunk member, FALSE otherwise
 */
BOOL port_is_trunk_member(int16 port)
{
	BOOL member = FALSE;
	IH_STATUS rv = 0;
	uns16 tid = 0;

	rv = port_get_trunk_member(port, &member, &tid);
	if(rv){
		return FALSE;
	}

	return member;
}

/*
 * to see if port is CPU port
 *
 * @param port		logical port number
 *
 * @return TRUE indicates port is CPU port, FALSE otherwise
 */
BOOL port_is_cpu_port(int16 port)
{
	return (port == get_app_cpu_port());
}

/*
 * to see if port is cascade port
 *
 * @param port		logical port number
 *
 * @return TRUE indicates port is cascade port, FALSE otherwise
 */
BOOL port_is_cascade_port(int16 port)
{
	return pbmp_has_port(get_app_pbmp_cascade(), port);
}

RING_PORT_TYPE port_is_ring_port(int16 port)
{
	if(!port_is_valid(port)){
		return RING_PORT_NONE;
	}

	if(port == g_ring_port.left){
		return RING_PORT_LEFT;
	}

	if(port == g_ring_port.right){
		return RING_PORT_RIGHT;
	}

	return RING_PORT_NONE;
}

MEMBER_TYPE port_get_vlan_member_type(int16 port, uns16 vid)
{
	if(!vid_is_valid(vid)){
		return VLAN_MEMBER_NOT_A_MEMBER;
	}

	if(!port_is_valid(port)){
		return VLAN_MEMBER_NOT_A_MEMBER;
	}

	if(!(g_vlan_info[vid - 1].member & (1 << port))){
		return VLAN_MEMBER_NOT_A_MEMBER;
	}

	if(g_vlan_info[vid - 1].untagged & (1 << port)){
		return VLAN_MEMBER_EGRESS_UNTAGGED;
	}else if(g_vlan_info[vid - 1].tagged & (1 << port)){
		return VLAN_MEMBER_EGRESS_TAGGED;
	}else if(g_vlan_info[vid - 1].unmodified & (1 << port)){
		return VLAN_MEMBER_EGRESS_UNMODIFIED;
	}

	return VLAN_MEMBER_NOT_A_MEMBER;
}

/*
 * get port's description
 *
 * @param port	        logical port number
 *
 * @return port's description, NULL if port is invalid
 */
char *port_get_desc(int16 port)
{
	if(!port_is_valid(port)){
		return NULL;
	}

	return g_port_conf[port].desc;
}

BOOL trunk_in_vlan(uns16 tid, uns16 vid)
{
	if(!vid_is_valid(vid)){
		return FALSE;
	}

	if(!trunk_is_valid(tid)){
		return FALSE;
	}

	if(!g_vlan_info[vid - 1].member){
		return FALSE;
	}

	if(!g_trunk_info.trunk[tid-1].member){
		return FALSE;
	}

	if((g_vlan_info[vid - 1].member ^ g_trunk_info.trunk[tid-1].member)
		& g_trunk_info.trunk[tid-1].member){
		return FALSE;
	}

	return TRUE;
}

MEMBER_TYPE trunk_get_vlan_member_type(uns16 tid, uns16 vid)
{
	if(!trunk_in_vlan(tid, vid)){
		return VLAN_MEMBER_NOT_A_MEMBER;
	}

	if(!((g_vlan_info[vid - 1].untagged ^ g_trunk_info.trunk[tid-1].member)
			& g_trunk_info.trunk[tid-1].member)){
		return VLAN_MEMBER_EGRESS_UNTAGGED;
	}else if(!((g_vlan_info[vid - 1].tagged ^ g_trunk_info.trunk[tid-1].member)
			& g_trunk_info.trunk[tid-1].member)){
		return VLAN_MEMBER_EGRESS_TAGGED;
	}else if(!((g_vlan_info[vid - 1].unmodified ^ g_trunk_info.trunk[tid-1].member)
			& g_trunk_info.trunk[tid-1].member)){
		return VLAN_MEMBER_EGRESS_UNMODIFIED;
	}else{
		return VLAN_MEMBER_NOT_A_MEMBER;
	}

	return VLAN_MEMBER_NOT_A_MEMBER;
}


BOOL trunk_is_vlan_member(uns16 tid, uns16 vid, MEMBER_TYPE type)
{

	if(!trunk_in_vlan(tid, vid)){
		return FALSE;
	}

	if(VLAN_MEMBER_EGRESS_UNTAGGED == type){
		return (!((g_vlan_info[vid - 1].untagged ^ g_trunk_info.trunk[tid-1].member)
				& g_trunk_info.trunk[tid-1].member));
	}else if(VLAN_MEMBER_EGRESS_TAGGED == type){
		return (!((g_vlan_info[vid - 1].tagged ^ g_trunk_info.trunk[tid-1].member)
				& g_trunk_info.trunk[tid-1].member));
	}else if(VLAN_MEMBER_EGRESS_UNMODIFIED == type){
		return (!((g_vlan_info[vid - 1].unmodified ^ g_trunk_info.trunk[tid-1].member)
				& g_trunk_info.trunk[tid-1].member));
	}else{
		return FALSE;
	}

	return FALSE;
}

/*
 * to see if trunk is valid
 *
 * @param tid         trunk id
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL trunk_is_valid(uns16 tid)
{
	return ((tid >= MIN_TRUNK_ID) && (tid <= MAX_TRUNK_ID));
}

/*
 * to see if trunk is in use
 *
 * @param tid         trunk id
 *
 * @return TRUE indicates in use, FALSE otherwise
 */
BOOL trunk_in_use(uns16 tid)
{
	if(!trunk_is_valid(tid)){
		return FALSE;
	}

	if(g_trunk_info.trunk[tid-1].lacp){
		return FALSE;
	}

	return (!pbmp_empty(g_trunk_info.trunk[tid-1].member));
}

/*
 * return member port(s) of trunk
 *   the returned value is logical port bitmap
 *
 * @param tid         trunk id
 *
 * @return member port(s)
 */
PBMP trunk_get_member(uns16 tid)
{
	if(!trunk_is_valid(tid)){
		return 0;
	}

	return g_trunk_info.trunk[tid-1].member;
}

/*
 * get master(primary) port of trunk: application layer configured minimum trunk port
 *
 * @param tid          trunk id
 * @param master_port  (OUT)master(primary) port of trunk; INVALID_PORT indicate no member
 *
 * @return IH_ERR_OK for ok, others for error
 */
IH_STATUS trunk_get_master_port(uns16 tid, int16 *master_port)
{
	PBMP pbmp = 0;
	int16 port = 0;

	if(!trunk_is_valid(tid)){
		return IH_ERR_INVALID;
	}

	if(!master_port || !g_trunk_info.trunk[tid-1].member){
		return IH_ERR_INVALID;
	}

	*master_port = INVALID_PORT;

	pbmp = g_trunk_info.trunk[tid-1].member;
	PBMP_FOREACH_PORT(pbmp, port){
		*master_port = port;

		return IH_ERR_OK;
	}
	PBMP_FOREACH_PORT_END

	return IH_ERR_OK;
}

/*
 * to see if MSTP instance id is valid
 *
 * @param sid         MSTP instance id
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL sid_is_valid(uns16 sid)
{
	return ((sid >= MIN_MSTP_ID) && (sid <= MAX_MSTP_ID));
}

/*
 * to see if DSCP is valid(valid range 0-63)
 *
 * @param dscp         DSCP
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL dscp_is_valid(uns8 dscp)
{
	return ((dscp >= MIN_DSCP) && (dscp <= MAX_DSCP));
}


/*
 * to see if priority is valid(valid range 0-7)
 *
 * @param priority         priority
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL priority_is_valid(uns8 priority)
{
	return ((priority >= MIN_FRAME_PRIORITY) && (priority <= MAX_FRAME_PRIORITY));
}

/*
 * to see if traffic class is valid(chip specific)
 *
 * @param queue         traffic class
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL queue_is_valid(uns8 queue)
{
	return ((queue >= MIN_PORT_QUEUE) && (queue <= MAX_PORT_QUEUE));
}


/* #################### PBMP(port bitmap) functions #################### */

/*
 * get device port bitmap by device id
 *
 * @param dev_id	        device id
 *
 * @return device port bitmap structure
 */
DEV_PBMP *dev_pbmp_get(uns16 dev_id)
{
	DEV_PBMP *dev_pbmp = NULL;

	dev_pbmp = g_dev_pbmp;

	while(dev_pbmp){
		if(dev_id == dev_pbmp->dev_id){
			return dev_pbmp;
		}

		dev_pbmp = dev_pbmp->next;
	}

	return NULL;
}

/*
 * device port bitmap dump
 *
 *
 * @return IH_ERR_OK for ok, others for error
 */
IH_STATUS dev_pbmp_dump(void)
{
	DEV_PBMP *dev_pbmp = NULL;
	PORT_STR port_str;

	dev_pbmp = g_dev_pbmp;

	ih_cmd_rsp_print("\nDevice physical info:\n");
	LOG_DB("\nDevice physical info:\n");

	if(multi_chip()){
		ih_cmd_rsp_print("multi-chip system\n");
		LOG_DB("multi-chip system\n");
	}

	ih_cmd_rsp_print("number of device: %d\n", get_dev_count());
	LOG_DB("number of device: %d\n", get_dev_count());
	while(dev_pbmp){

		ih_cmd_rsp_print("Dev #%d, pbmp 0x%x\n", dev_pbmp->dev_id, dev_pbmp->dev_pbmp);
		LOG_DB("Dev #%d, pbmp 0x%x\n", dev_pbmp->dev_id, dev_pbmp->dev_pbmp);

		if(dev_pbmp->cpu_dev){
			ih_cmd_rsp_print(" CPU Device.\n");
			LOG_DB(" CPU Device.\n");
		}

		bzero(port_str.str, sizeof(port_str.str));
		slot_pbmp_to_str(dev_pbmp->dev_id, dev_pbmp->dev_pbmp, port_str.str);
		ih_cmd_rsp_print(" dev ports: %s\n", port_str.str);
		LOG_DB(" dev ports: %s\n", port_str.str);

		bzero(port_str.str, sizeof(port_str.str));
		slot_pbmp_to_str(dev_pbmp->dev_id, dev_pbmp->dev_pbmp_cascade, port_str.str);
		ih_cmd_rsp_print(" dev cascade ports: %s\n", port_str.str);
		LOG_DB(" dev cascade ports: %s\n", port_str.str);

		app_pbmp_to_str(dev_pbmp->app_pbmp, &port_str);
		ih_cmd_rsp_print(" app ports: %s\n", port_str.str);
		LOG_DB(" app ports: %s\n", port_str.str);

		app_pbmp_to_str(dev_pbmp->app_pbmp_cascade, &port_str);
		ih_cmd_rsp_print(" app cascade ports: %s\n", port_str.str);
		LOG_DB(" app cascade ports: %s\n", port_str.str);

		dev_pbmp = dev_pbmp->next;
	}

	return IH_ERR_OK;
}

/*
 * slot port info dump
 *
 *
 * @return IH_ERR_OK for ok, others for error
 */
IH_STATUS app_pbmp_dump(void)
{
	uns32 num_entry = 0;
	uns32 i = 0;
	PORT_STR port_str;
	PBMP pbmp = 0;

	//dump g_if_slot_info
	LOG_DB("\napp port information: \n"
			"true pbmp full: %#x\n"
			"pbmp full: %#x\n"
			"pbmp cascade: %#x\n"
			"num_type:%d\n"
			"num_slot:%d\n"
			"num_port:%d\n"
			"cpu_port:%d\n",
			get_app_pbmp_true_full(),
			get_app_pbmp_full(),
			get_app_pbmp_cascade(),
			g_max_num_if_type,
			g_max_num_slot,
			g_max_num_port,
			g_cpu_port);
	ih_cmd_rsp_print("\napp port information: \n"
			"true pbmp full: %#x\n"
			"pbmp full: %#x\n"
			"pbmp cascade: %#x\n"
			"num_type:%d\n"
			"num_slot:%d\n"
			"num_port:%d\n"
			"cpu_port:%d\n",
			get_app_pbmp_true_full(),
			get_app_pbmp_full(),
			get_app_pbmp_cascade(),
			g_max_num_if_type,
			g_max_num_slot,
			g_max_num_port,
			g_cpu_port);

	pbmp = get_app_pbmp_true_full();
	app_pbmp_to_str(pbmp, &port_str);
	LOG_DB("\nall ports: %s\n", port_str.str);
	ih_cmd_rsp_print("\nall ports: %s\n", port_str.str);

	num_entry = get_slot_count() * get_port_if_type_count();
	for(i = 0; i < num_entry; i++){
		app_pbmp_to_str(g_if_slot_info[i].app_pbmp, &port_str);
		LOG_DB("\n%s "
				"slot %d:\n"
				"port base %d\n"
				"port_min %d\n"
				"port_max %d\n"
				"port_count %d\n"
				"panel ports: %s\n",
				get_if_name_by_type(g_if_slot_info[i].type),
				g_if_slot_info[i].slot,
				g_if_slot_info[i].base,
				g_if_slot_info[i].port_min,
				g_if_slot_info[i].port_max,
				g_if_slot_info[i].port_count,
				port_str.str);

		ih_cmd_rsp_print("\n%s "
				"slot %d:\n"
				"port base %d\n"
				"port_min %d\n"
				"port_max %d\n"
				"port_count %d\n"
				"panel ports: %s\n",
				get_if_name_by_type(g_if_slot_info[i].type),
				g_if_slot_info[i].slot,
				g_if_slot_info[i].base,
				g_if_slot_info[i].port_min,
				g_if_slot_info[i].port_max,
				g_if_slot_info[i].port_count,
				port_str.str);
	}

	return IH_ERR_OK;
}

/*
 * free device port bitmap list
 *
 *
 * @return IH_ERR_OK for ok, others for error
 */
IH_STATUS dev_pbmp_free(void)
{
	DEV_PBMP *dev_pbmp = NULL;

	while(g_dev_pbmp){
		dev_pbmp = g_dev_pbmp;
		g_dev_pbmp = g_dev_pbmp->next;

		free(dev_pbmp);
	}

	return IH_ERR_OK;
}

int32 show_vlan_info(uns16 vid)
{
	PBMP member = 0;
	uns32 i = 0;
	PORT_STR port_str;

	if(!vid_is_valid(vid)){
		return IH_ERR_PARAM;
	}

	if(!g_vlan_info[vid - 1].valid){
		return IH_ERR_OK;
	}

	app_pbmp_to_str(g_vlan_info[vid - 1].member, &port_str);
	ih_cmd_rsp_print("\n"FM_30S_COLON_D_S""FM_30S_COLON_S""FM_30S_COLON_D""FM_30S_COLON_S
								,"VLAN id", vid, (DEFAULT_VLAN_ID == vid)? "(default)": ""
								,"name", g_vlan_info[vid - 1].name
								,"priority", g_vlan_info[vid - 1].priority
								,"member", port_str.str);
	LOG_DB("\n"FM_30S_COLON_D_S""FM_30S_COLON_S""FM_30S_COLON_D""FM_30S_COLON_S
			,"VLAN id", vid, (DEFAULT_VLAN_ID == vid)? "(default)": ""
			,"name", g_vlan_info[vid - 1].name
			,"priority", g_vlan_info[vid - 1].priority
			,"member", port_str.str);

	for(i = 0; i < get_vlan_member_type_count(); i++){

		/* filter */
		if((VLAN_MEMBER_NOT_A_MEMBER == get_vlan_member_type_by_idx(i)
			|| (VLAN_MEMBER_EGRESS_UNMODIFIED == get_vlan_member_type_by_idx(i)))){
			continue;
		}

		member = get_vlan_member_by_type(vid, get_vlan_member_type_by_idx(i));

		app_pbmp_to_str(member, &port_str);
		ih_cmd_rsp_print(FM_30S_COLON_S, get_vlan_member_type_name_by_idx(i), port_str.str);
		LOG_DB("\n"FM_30S_COLON_S, get_vlan_member_type_name_by_idx(i), port_str.str);
	}

	return IH_ERR_OK;
}

int32 show_port_info(int16 port)
{
	PORT_STR port_str;

	if(!port_is_valid(port)){
		return IH_ERR_PARAM;
	}

	app_pbmp_to_port_str(1<<port, &port_str);
	ih_cmd_rsp_print("\n"FM_30S_COLON_D""FM_30S_COLON_S""FM_30S_COLON_S
								,"port", port
								,"name", port_str.str
								,"description", g_port_conf[port].desc);
	LOG_DB("\n"FM_30S_COLON_D""FM_30S_COLON_S""FM_30S_COLON_S
			,"port", port
			,"name", port_str.str
			,"description", g_port_conf[port].desc);

	return IH_ERR_OK;
}

int32 show_trunk_info(uns16 tid)
{
	PORT_STR port_str;

	if(trunk_is_valid(tid)){
		app_pbmp_to_port_str(g_trunk_info.trunk[tid - 1].member, &port_str);

		LOG_DB("\n"FM_30S_COLON_D""FM_30S_COLON_S""FM_30S_COLON_S
				,"trunk ID", tid
				,"type", g_trunk_info.trunk[tid - 1].lacp? "lacp": "static"
				,"member", port_str.str);
	}

	return IH_ERR_OK;
}

/* #################### other service request interface to do certain task #################### */

/*
 * send port shutdown request and wait for response
 *
 * @param port       --- port
 * @param shutdown   --- shutdown or not
 *
 * @return IH_ERR_OK for ok, others for error
 *   IH_ERR_FAIL        --- failed
 *   IH_ERR_INVALID     --- invalid port
 */
int32 port_shutdown_request(int16 port, BOOL shutdown)
{
	int32 rv = 0;
	int32 rsp_code = IH_ERR_OK;
	IPC_MSG *rsp = NULL;
	PORT_SHUTDOWN_REQ req;

	if(!port_is_valid(port)){
		LOG_ER("### invalid port %d\n", port);
		return IH_ERR_INVALID;
	}

	req.port = port;
	req.shutdown = shutdown? TRUE: FALSE;

	LOG_DB("### request to %s port %d\n", req.shutdown? "shutdown": "no shutdown", req.port);

	rv = ih_ipc_send_wait_rsp(IH_SVC_IF, DEFAULT_CMD_TIMEOUT, IPC_MSG_IF_SHUTDOWN_PORT_REQ, (char *)&req, sizeof(PORT_SHUTDOWN_REQ), &rsp);
	if(rv){
		LOG_ER("### failed to send shutdown request(%d) to svc %d, error %d.\n", IPC_MSG_IF_SHUTDOWN_PORT_REQ, IH_SVC_IF, rv);
		return IH_ERR_FAIL;
	}

	if(rsp){
		int32 *ip = NULL;
		ip = (int32 *)rsp->body;
		rsp_code  = *ip;

		ih_ipc_free_msg(rsp);
	}

	if(rsp_code){
		LOG_ER("### rsp_code is not OK: %d.\n", rsp_code);
		return rsp_code;
	}

	return IH_ERR_OK;
}

/*
 * send create VLAN request and wait for response
 *
 * @param req   --- VLAN information
 *
 * @return IH_ERR_OK for ok, others for error
 *   IH_ERR_FAIL        --- failed
 *   IH_ERR_UNAVAILABLE --- VLAN is used by other service
 */
int32 vlan_create_request(VLAN_CREATE_REQ req)
{
	int32 rv = 0;
	int32 rsp_code = IH_ERR_FAIL;
	IPC_MSG *rsp = NULL;

	LOG_DB("### request to create VLAN %d, untagged %#x, tagged %#x\n", req.vid, req.untagged, req.tagged);

	rv = ih_ipc_send_wait_rsp(IH_SVC_IF, DEFAULT_CMD_TIMEOUT, IPC_MSG_IF_VLAN_CREATE_REQ, (char *)&req, sizeof(VLAN_CREATE_REQ), &rsp);
	if(rv){
		LOG_ER("### failed to send vlan create request(%d) to svc %d\n", IPC_MSG_IF_VLAN_CREATE_REQ, IH_SVC_IF);
		return IH_ERR_FAIL;
	}

	if(rsp){
		int32 *ip = NULL;
		ip = (int32 *)rsp->body;
		rsp_code  = *ip;

		ih_ipc_free_msg(rsp);
	}

	if(rsp_code){
		LOG_ER("### rsp_code is not OK: %d.\n", rsp_code);
		return rsp_code;
	}

	return IH_ERR_OK;
}

/*
 * send destroy VLAN request and wait for response
 *
 * @param vid  --- VLAN ID
 *
 * @return IH_ERR_OK for ok, others for error
 *   IH_ERR_FAIL        --- failed
 *   IH_ERR_UNAVAILABLE --- VLAN is used by other service
 *   IH_ERR_NOT_FOUND   --- VLAN does not exist
 */
int32 vlan_destroy_request(uns16 vid)
{
	int32 rv = 0;
	int32 rsp_code = IH_ERR_FAIL;
	IPC_MSG *rsp = NULL;

	LOG_DB("### request to destroy VLAN %d\n", vid);

	rv = ih_ipc_send_wait_rsp(IH_SVC_IF, DEFAULT_CMD_TIMEOUT, IPC_MSG_IF_VLAN_DESTROY_REQ, (char *)&vid, sizeof(vid), &rsp);
	if(rv){
		LOG_ER("### failed to send vlan destroy request(%d) to svc %d, error %d\n", IPC_MSG_IF_VLAN_DESTROY_REQ, IH_SVC_IF, rv);
		return IH_ERR_FAIL;
	}

	if(rsp){
		int32 *ip = NULL;
		ip = (int32 *)rsp->body;
		rsp_code  = *ip;

		ih_ipc_free_msg(rsp);
	}

	if(rsp_code){
		LOG_ER("### rsp_code is not OK: %d.\n", rsp_code);
		return rsp_code;
	}

	return IH_ERR_OK;
}

/*
 * check if my service is owner of specified VLAN
 *
 * @param vid      --- VLAN ID
 * @param svc_id   --- (out) owner service id, -1 indicates VLAN does not exist
 *
 * @return TRUE for owner, FALSE for not
 */
BOOL i_am_vlan_owner(uns16 vid, IH_SVC_ID *svc_id)
{
	if(!svc_id){
		return FALSE;
	}

	*svc_id = -1;

	if(!vlan_in_use(vid)){
		return FALSE;
	}

	*svc_id = g_vlan_info[vid - 1].owner_svc_id;

	if(ih_ipc_get_svc_id() == *svc_id){
		return TRUE;
	}

	return FALSE;
}

/* #################### interface event process routine #################### */

int32 vlan_create_destroy_hook(uns16 vid, BOOL create)
{
	LOG_DB("### VLAN %d %s!\n", vid, create? "created": "destroyed");

	return IH_ERR_OK;
}

int32 port_join_leave_vlan_hook(VLAN_MEMBER_EVENT vlan_event)
{
	PORT_STR port_str;

	app_pbmp_to_str(vlan_event.pbmp, &port_str);

	if(vlan_event.join){
		LOG_DB("### port %s join VLAN %d %s!\n", port_str.str, vlan_event.vid, get_member_name_by_type(vlan_event.member_type));
	}else{
		LOG_DB("### port %s leave VLAN %d!\n", port_str.str, vlan_event.vid);
	}

	return IH_ERR_OK;
}

int32 port_join_leave_trunk_hook(TRUNK_MEMBER_EVENT trunk_event)
{
	PORT_STR port_str;

	app_pbmp_to_port_str(trunk_event.pbmp, &port_str);

	LOG_DB("### port %s %s TRUNK %d!\n", port_str.str, trunk_event.join? "join": "leave", trunk_event.tid);

	return IH_ERR_OK;
}

/*
 * register VLAN create/destroy handle
 *
 * @param handle --- callback handle
 *
 * @return IH_ERR_OK for ok, others for error
 */
IH_STATUS register_handle_vlan_create_destroy(vlan_create_destroy_handle handle)
{
	g_vlan_create_destroy_handle = handle;

	return IH_ERR_OK;
}

/*
 * register port join/leave VLAN handle
 *
 * @param handle --- callback handle
 *
 * @return IH_ERR_OK for ok, others for error
 */
IH_STATUS register_handle_port_join_leave_vlan(port_join_leave_vlan_handle handle)
{
	g_port_join_leave_vlan_handle = handle;

	return IH_ERR_OK;
}

/*
 * register port join/leave TRUNK handle
 *
 * @param handle --- callback handle
 *
 * @return IH_ERR_OK for ok, others for error
 */
IH_STATUS register_handle_port_join_leave_trunk(port_join_leave_trunk_handle handle)
{
	g_port_join_leave_trunk_handle = handle;

	return IH_ERR_OK;
}

/*
 * build VLAN changes and call related handle
 *
 * @param vid
 * @param vlan_info_old --- old VLAN information
 * @param vlan_info     --- new VLAN information
 *
 * @return IH_ERR_OK for ok, others for error
 */
IH_STATUS vlan_change_process(uns16 vid, VLAN_INFO vlan_info_old, VLAN_INFO vlan_info)
{
	PBMP member_join_t = 0;
	PBMP member_leave_t = 0;
	PBMP member_join_u = 0;
	PBMP member_leave_u = 0;
	BOOL vlan_create = FALSE;
	BOOL vlan_destroy = FALSE;

	PORT_STR port_str;
	VLAN_MEMBER_EVENT vlan_event;
	IH_STATUS rv = 0;

	if(vlan_info.valid != vlan_info_old.valid){
		vlan_create  = vlan_info.valid? TRUE: FALSE;
		vlan_destroy = !vlan_create;
	}

	/* VLAN create event */
	if(g_vlan_create_destroy_handle && vlan_create){

		LOG_DB("### VLAN %d created!\n", vid);

		/* run hook routine */
		rv = g_vlan_create_destroy_handle(vid, TRUE);
		if(rv){
			LOG_DB(" vlan_create_destroy callback routine errno: %d\n", rv);
		}
	}

	/* port join/leave VLAN event */
	if(g_port_join_leave_vlan_handle){
		member_join_t = pbmp_sub(vlan_info.tagged, vlan_info_old.tagged);
		member_leave_t = pbmp_sub(vlan_info_old.tagged, vlan_info.tagged);
		member_join_u = pbmp_sub(vlan_info.untagged, vlan_info_old.untagged);
		member_leave_u = pbmp_sub(vlan_info_old.untagged, vlan_info.untagged);

		bzero(&vlan_event, sizeof(vlan_event));
		vlan_event.vid = vid;
		if(member_join_t){

			vlan_event.member_type = VLAN_MEMBER_EGRESS_TAGGED;
			vlan_event.pbmp = member_join_t;
			vlan_event.join = TRUE;

			app_pbmp_to_str(vlan_event.pbmp, &port_str);

			LOG_DB("### port %s join VLAN %d %s!\n", port_str.str, vlan_event.vid, get_member_name_by_type(vlan_event.member_type));

			/* run hook routine */
			rv = g_port_join_leave_vlan_handle(vlan_event);
			if(rv){
				LOG_DB(" port_join_leave_vlan callback routine errno: %d\n", rv);
			}
		}

		if(member_leave_t){

			vlan_event.member_type = VLAN_MEMBER_EGRESS_TAGGED;
			vlan_event.pbmp = member_leave_t;
			vlan_event.join = FALSE;

			app_pbmp_to_str(vlan_event.pbmp, &port_str);

			LOG_DB("### port %s leave VLAN %d!\n", port_str.str, vlan_event.vid);

			/* run hook routine */
			rv = g_port_join_leave_vlan_handle(vlan_event);
			if(rv){
				LOG_DB(" port_join_leave_vlan callback routine errno: %d\n", rv);
			}
		}

		if(member_join_u){

			vlan_event.member_type = VLAN_MEMBER_EGRESS_UNTAGGED;
			vlan_event.pbmp = member_join_u;
			vlan_event.join = TRUE;

			app_pbmp_to_str(vlan_event.pbmp, &port_str);

			LOG_DB("### port %s join VLAN %d %s!\n", port_str.str, vlan_event.vid, get_member_name_by_type(vlan_event.member_type));

			/* run hook routine */
			rv = g_port_join_leave_vlan_handle(vlan_event);
			if(rv){
				LOG_DB(" port_join_leave_vlan callback routine errno: %d\n", rv);
			}
		}

		if(member_leave_u){

			vlan_event.member_type = VLAN_MEMBER_EGRESS_UNTAGGED;
			vlan_event.pbmp = member_leave_u;
			vlan_event.join = FALSE;

			app_pbmp_to_str(vlan_event.pbmp, &port_str);

			LOG_DB("### port %s leave VLAN %d!\n", port_str.str, vlan_event.vid);

			/* run hook routine */
			rv = g_port_join_leave_vlan_handle(vlan_event);
			if(rv){
				LOG_DB(" port_join_leave_vlan callback routine errno: %d\n", rv);
			}
		}
	}

	/* VLAN destroy event */
	if(g_vlan_create_destroy_handle && vlan_destroy){
		LOG_DB("### VLAN %d destroyed!\n", vid);

		/* run hook routine */
		rv = g_vlan_create_destroy_handle(vid, FALSE);
		if(rv){
			LOG_DB(" vlan_create_destroy callback routine errno: %d\n", rv);
		}
	}

	return IH_ERR_OK;
}

/*
 * build Trunk changes and call related handle
 *
 * @param tid ---  Trunk id
 * @param member_old --- old Trunk member
 * @param member     --- new Trunk member
 *
 * @return IH_ERR_OK for ok, others for error
 */
IH_STATUS trunk_change_process(uns16 tid, PBMP member_old, PBMP member)
{
	TRUNK_MEMBER_EVENT trunk_event;
	PBMP member_join = 0;
	PBMP member_leave = 0;
	IH_STATUS rv = 0;

	if(g_port_join_leave_trunk_handle){

		member_join  = pbmp_sub(member, member_old);
		member_leave = pbmp_sub(member_old, member);

		trunk_event.tid = tid;

		if(member_join){
			trunk_event.pbmp = member_join;
			trunk_event.join = TRUE;

			rv = g_port_join_leave_trunk_handle(trunk_event);
			if(rv){
				LOG_ER("### port_join_leave_trunk errno: %d\n", rv);
			}
		}

		if(member_leave){
			trunk_event.pbmp = member_leave;
			trunk_event.join = FALSE;

			rv = g_port_join_leave_trunk_handle(trunk_event);
			if(rv){
				LOG_ER("### port_join_leave_trunk errno: %d\n", rv);
			}
		}
	}

	return IH_ERR_OK;
}

/*
 * handle VLAN sync event
 *
 * @param msg_hdr
 * @param msg
 * @param msg_len
 * @param obuf
 * @param olen
 *
 * @return IH_ERR_OK for ok, others for error
 */
static int32 sync_vlan_msg_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen)
{
	VLAN_SYNC *vlan_sync = NULL;
	VLAN_INFO vlan_info_old;


	if(!msg_hdr || !msg){
		LOG_ER("msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}
	LOG_DB("handle sync VLAN msg from: %d, len:%d", msg_hdr->svc_id, msg_len);

	vlan_sync = (VLAN_SYNC *)msg;
	if(!vlan_sync){
		LOG_ER("vlan_sync is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if(!vid_is_valid(vlan_sync->vid)){
		LOG_ER("### invalid VLAN ID %d\n", vlan_sync->vid);
		return IH_ERR_INVALID;
	}

	LOG_DB("### current VLAN %d info:\n", vlan_sync->vid);
	show_vlan_info(vlan_sync->vid);

	memcpy(&vlan_info_old, &g_vlan_info[vlan_sync->vid - 1], sizeof(VLAN_INFO));
	memcpy(&g_vlan_info[vlan_sync->vid - 1], &vlan_sync->vlan_info, sizeof(VLAN_INFO));

	vlan_change_process(vlan_sync->vid, vlan_info_old, vlan_sync->vlan_info);

	LOG_DB("### sync VLAN %d info done!\n", vlan_sync->vid);
	show_vlan_info(vlan_sync->vid);

	return IH_ERR_OK;
}

/*
 * handle PORT sync event
 *
 * @param msg_hdr
 * @param msg
 * @param msg_len
 * @param obuf
 * @param olen
 *
 * @return IH_ERR_OK for ok, others for error
 */
static int32 sync_port_msg_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen)
{
	PORT_SYNC *port_sync = NULL;


	if(!msg_hdr || !msg){
		LOG_ER("msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}
	LOG_DB("handle sync PORT msg from: %d, len:%d", msg_hdr->svc_id, msg_len);

	port_sync = (PORT_SYNC *)msg;
	if(!port_sync){
		LOG_ER("port_sync is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if(!port_is_valid(port_sync->port)){
		LOG_ER("### invalid port %d\n", port_sync->port);
		return IH_ERR_INVALID;
	}

	memcpy(&g_port_conf[port_sync->port], &port_sync->port_conf, sizeof(PORT_CONF));

	LOG_DB("### sync PORT %d info done!\n", port_sync->port);
	show_port_info(port_sync->port);

	return IH_ERR_OK;
}

/*
 * handle TRUNK sync event
 *
 * @param msg_hdr
 * @param msg
 * @param msg_len
 * @param obuf
 * @param olen
 *
 * @return IH_ERR_OK for ok, others for error
 */
static int32 sync_trunk_msg_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen)
{
	TRUNK_SYNC *trunk_sync = NULL;
	PBMP member_old = 0;


	if(!msg_hdr || !msg){
		LOG_ER("msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}
	LOG_DB("handle sync TRUNK msg from: %d, len:%d", msg_hdr->svc_id, msg_len);

	trunk_sync = (TRUNK_SYNC *)msg;
	if(!trunk_sync){
		LOG_ER("trunk_sync is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if(!trunk_is_valid(trunk_sync->tid)){
		LOG_ER("invalid trunk id %d\n", trunk_sync->tid);
		return IH_ERR_INVALID;
	}

	LOG_DB("### current TRUNK %d info:\n", trunk_sync->tid);
	show_trunk_info(trunk_sync->tid);

	member_old = g_trunk_info.trunk[trunk_sync->tid - 1].member;
	g_trunk_info.trunk[trunk_sync->tid - 1].member = trunk_sync->pbmp;

	if(IH_SVC_LACP == msg_hdr->svc_id){
		g_trunk_info.trunk[trunk_sync->tid - 1].lacp = trunk_sync->pbmp? TRUE: FALSE;
	}

	if(member_old != g_trunk_info.trunk[trunk_sync->tid - 1].member){
		trunk_change_process(trunk_sync->tid, member_old, trunk_sync->pbmp);
	}

	LOG_DB("### sync TRUNK %d info done!\n", trunk_sync->tid);
	show_trunk_info(trunk_sync->tid);

	return IH_ERR_OK;
}

/*
 * handle VLAN use sync event
 *
 * @param msg_hdr
 * @param msg
 * @param msg_len
 * @param obuf
 * @param olen
 *
 * @return IH_ERR_OK for ok, others for error
 */
static int32 sync_vlan_use_msg_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen)
{
	IPC_MSG_G8032_VLAN *vlan_use_sync = NULL;


	if(!msg_hdr || !msg){
		LOG_ER("msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}
	LOG_DB("handle sync VLAN use msg from: %d, len:%d", msg_hdr->svc_id, msg_len);

	vlan_use_sync = (IPC_MSG_G8032_VLAN *)msg;
	if(!vlan_use_sync){
		LOG_ER("vlan_use_sync is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if(!vid_is_valid(vlan_use_sync->vid)){
		LOG_ER("### invalid VLAN ID %d\n", vlan_use_sync->vid);
		return IH_ERR_INVALID;
	}

	if(vlan_use_sync->vlan_in_g8032){
		g_vlan_info[vlan_use_sync->vid - 1].flag |= SERVICE_G8032_USE_VLAN;
	}else{
		g_vlan_info[vlan_use_sync->vid - 1].flag &= ~SERVICE_G8032_USE_VLAN;
	}

	LOG_DB("### Service %d %s use for VLAN %d!\n", msg_hdr->svc_id, vlan_use_sync->vlan_in_g8032? "register": "unregister", vlan_use_sync->vid);

	return IH_ERR_OK;
}

/*
 * handle Ring port role sync event: port left or right
 *
 * @param msg_hdr
 * @param msg
 * @param msg_len
 * @param obuf
 * @param olen
 *
 * @return IH_ERR_OK for ok, others for error
 */
static int32 sync_port_role_msg_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen)
{
	IPC_MSG_G8032_PORT_VLAN *port_role_sync = NULL;


	if(!msg_hdr || !msg){
		LOG_ER("msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}
	LOG_DB("handle sync port role msg from: %d, len:%d", msg_hdr->svc_id, msg_len);

	port_role_sync = (IPC_MSG_G8032_PORT_VLAN *)msg;
	if(!port_role_sync){
		LOG_ER("port_role_sync is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if(!port_is_valid(port_role_sync->port_index)){
		LOG_ER("### invalid port %d\n", port_role_sync->port_index);
		return IH_ERR_INVALID;
	}

	if(port_role_sync->port_role){
		g_ring_port.right = port_role_sync->port_vlan_in_g8032? port_role_sync->port_index: 0;
	}else{
		g_ring_port.left  = port_role_sync->port_vlan_in_g8032? port_role_sync->port_index: 0;
	}

	LOG_DB("### Service %d %s %s port %d!\n", msg_hdr->svc_id, port_role_sync->port_vlan_in_g8032? "register": "unregister", port_role_sync->port_role? "right": "left", port_role_sync->port_index);

	return IH_ERR_OK;
}

IH_STATUS if_event_register(void)
{
	LOG_DB("### This function is just an example at first, please remove it!\n");

	return IH_ERR_OK;
}


/*
 * register handle to sync information for port, trunk, VLAN and others
 * this routine first registers handle to sync interface info, then it initiates a service info request to interface service.
 *
 * Note: this function should be called in main function of service which need the information
 */
IH_STATUS if_sync_register(void)
{
	IH_STATUS rv = 0;

	LOG_DB("### register interface info sync handle.\n");

	bzero(&g_trunk_info, sizeof(TRUNK_INFO));

	/* register handle to sync interface info */
	ih_ipc_subscribe(IPC_MSG_IF_VLAN_SYNC);
	ih_ipc_subscribe(IPC_MSG_IF_PORT_SYNC);
	ih_ipc_subscribe(IPC_MSG_IF_TRUNK_SYNC);

	ih_ipc_register_msg_handle(IPC_MSG_IF_VLAN_SYNC, sync_vlan_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_IF_PORT_SYNC, sync_port_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_IF_TRUNK_SYNC, sync_trunk_msg_handle);

	LOG_DB("### waiting for interface service ready...\n");
	rv = ih_ipc_wait_service_ready(IH_SVC_IF, IPC_TIMEOUT_NOTIMEOUT);
	if(rv){
		LOG_ER("### wait for interface service %d ready errno: %d\n", IH_SVC_IF, rv);
	}else{
		LOG_DB("### waiting for interface service ready done!\n");
	}

	/* initiates a service info request to interface service */
	rv = ih_ipc_send_no_ack(IH_SVC_IF, IPC_MSG_IF_SVCINFO_REQ, NULL, 0);
	if(rv){
		LOG_ER("### Request interface service %d info failed, error %d\n", IH_SVC_IF, rv);
	}else{
		LOG_DB("### Request interface service %d info OK.\n", IH_SVC_IF);
	}

	return IH_ERR_OK;
}

/*
 * register handle to sync VLAN use
 * Note: this function should be called in main function of service which need the information
 */
IH_STATUS if_sync_vlan_use(void)
{
	LOG_DB("### register VLAN use sync.\n");

	ih_ipc_register_msg_handle(IPC_MSG_G8032_REGISTER_VLAN, sync_vlan_use_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_G8032_REGISTER_PORT, sync_port_role_msg_handle);

	return IH_ERR_OK;
}

/*
 * publish a single trunk information
 *
 * @param tid         trunk id
 * @param member      trunk member ports
 *
 * @return IH_ERR_OK for ok, others for error
 */
IH_STATUS publish_trunk_info(uns16 tid, PBMP member)
{
	IH_STATUS rv = 0;
	TRUNK_SYNC trunk_sync;

	if(!trunk_is_valid(tid)){
		LOG_ER("### invalid tid %d\n", tid);
		return IH_ERR_INVALID;
	}

	bzero(&trunk_sync, sizeof(TRUNK_SYNC));
	trunk_sync.tid = tid;
	trunk_sync.pbmp = member;
	rv = ih_ipc_publish(IPC_MSG_IF_TRUNK_SYNC, &trunk_sync, sizeof(TRUNK_SYNC));
	if(rv){
		LOG_ER("### publish TRUNK %d info failed, error %d\n", tid, rv);
	}

	return rv;
}

/* LACP */

/******************************************************************************
 * Process subscribed LACP messages
 *
 * @param void
 *
 * @return
 *
 */
static int32 lacp_ag_id_msg (IPC_MSG_HDR *msg_hdr,
		char *msg,uns32 msg_len, char *obuf,uns32 olen)
{
	if(!msg_hdr || !msg){
		LOG_ER("msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	memcpy((void *)&lacp_ag_info, (void *)msg, sizeof(lacp_ag_info));

	return IH_ERR_OK;
}

/******************************************************************************
 * Get the LACP aggregator ID's idle info
 *
 * @param ag_id         aggregator id
 *
 * @return 0 if the aggregator id is idle, !0 is busy
 */
BOOL lacp_ag_id_busy (uns8 ag_id)
{
	if (!ag_id || (ag_id > MAX_TRUNK_ID)) {
		LOG_ER("The aggregator id %d is error\n", ag_id);
		return IH_ERR_INVALID;
	}

	return (lacp_ag_info[ag_id - 1]);
}

/******************************************************************************
 * Get the LACP port's idle info
 *
 * @param port         LACP port
 *
 * @return 0 if the port is idle, !0 is busy
 */
BOOL lacp_port_busy (uns16 port)
{
	uns8 ag_id;

	if (!port || (port > MAX_NUM_PORT) || (port > 32)) {
		LOG_ER("The port %d is error\n", port);
		return IH_ERR_INVALID;
	}

	for (ag_id = 0; ag_id < MAX_TRUNK_ID; ag_id++)
		if ((lacp_ag_info[ag_id] >> port) & 1)
			return 1;

	return 0;
}

/******************************************************************************
 * Publish LACP messages
 *
 * @param void
 *
 * @return
 *
 */
IH_STATUS lacp_publish_info (void)
{
	IH_STATUS rv = 0;

	rv = ih_ipc_publish(IPC_MSG_LACP_AG_ID,
		(void *)&lacp_ag_info, sizeof(lacp_ag_info));
	if(rv){
		LOG_ER("LACP publish  info failed, error %d\n", rv);
	}

	return rv;
}
/******************************************************************************
 * Publish LACP messages
 *
 * @param void
 *
 * @return
 *
 */
IH_STATUS lacp_add_lacp_info (int cg_num, int port)
{
	/* sanity */
	/* ToDo... */

	lacp_ag_info[cg_num - 1] |= (1 << port);

	lacp_publish_info();

	return ERR_OK;
}

/******************************************************************************
 * Publish LACP messages
 *
 * @param void
 *
 * @return
 *
 */
IH_STATUS lacp_del_lacp_info (int cg_num, int port)
{
	/* sanity */
	/* ToDo... */

	lacp_ag_info[cg_num - 1] &= ~(1 << port);

	lacp_publish_info();

	return ERR_OK;
}

/******************************************************************************
 * Subscribe LACP messages
 *
 * @param void
 *
 * @return
 *
 */
IH_STATUS lacp_msg_register(void)
{
	ih_ipc_subscribe(IPC_MSG_LACP_AG_ID);

	ih_ipc_register_msg_handle(IPC_MSG_LACP_AG_ID, lacp_ag_id_msg);

	return IH_ERR_OK;
}


/*#################### below are helper functions for parameter check & show ####################*/

/*
 * ether type string valid check
 *
 * @param etype_str	ether type string(such as '9100' (hex))
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL etype_str_valid(char *etype_str)
{
	char *pchar = NULL;
	uns32 count = 0;
	uns32 etype = 0;

	if(!etype_str){
		return FALSE;
	}

	pchar = etype_str;
	while(*pchar){
		count++;

		if((*pchar >= '0' && *pchar <= '9')
				|| (*pchar >= 'a' && *pchar <= 'f')
				|| (*pchar >= 'A' && *pchar <= 'F')){

		}else{
			return FALSE;
		}

		pchar++;
	}

	if(!count || count > 4){
		return FALSE;
	}

	sscanf(etype_str, "%4x", &etype);

	if(etype < 1536){
		return FALSE;
	}

	return TRUE;
}

#if 0
/*
 * MAC address string valid check
 *
 * @param mac_str	MAC address string(such as '0000.0000.0001')
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL mac_str_valid(char *mac_str)
{
	char *pchar = NULL;
	uns32 count = 0;

	if(!mac_str){
		return FALSE;
	}

	pchar = mac_str;
	while(*pchar){
		count++;

		if((5 == count) || (10 == count)){
			if('.' != *pchar){
				return FALSE;
			}
		}else{
			if((*pchar >= '0' && *pchar <= '9')
					|| (*pchar >= 'a' && *pchar <= 'f')
					|| (*pchar >= 'A' && *pchar <= 'F')){

			}else{
				return FALSE;
			}
		}

		if(count > 14){
			return FALSE;
		}

		pchar++;
	}

	if(count != 14){
		return FALSE;
	}

	return TRUE;
}
#endif

#if 1
/*
 * MAC address string valid check
 *
 * @param mac_str	MAC address string(such as '0000.0000.0001')
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL mac_str_valid(char *mac_str)
{
	char *pchar = NULL;
	uns32 count = 0;

	if(!mac_str){
		return FALSE;
	}

	pchar = mac_str;
	while(*pchar){
		count++;

		if((5 == count) || (10 == count)){
			if('.' != *pchar){
				return FALSE;
			}
		}else{
			if((*pchar >= '0' && *pchar <= '9')
					|| (*pchar >= 'a' && *pchar <= 'f')
					|| (*pchar >= 'A' && *pchar <= 'F')){

			}else{
				return FALSE;
			}
		}

		if(count > 14){
			return FALSE;
		}

		pchar++;
	}

	if(count != 14){
		return FALSE;
	}

	return TRUE;
}
#else
/*
 * MAC address string valid check
 *
 * @param mac_str	MAC address string(such as '00:00:00:00:00:01')
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL mac_str_valid(char *mac_str)
{
	char *pchar = NULL;
	uns32 count = 0;

	if(!mac_str){
		return FALSE;
	}

	pchar = mac_str;
	while(*pchar){
		count++;

		//validate the mac rule
		if((3 == count) || (6 == count) || (9 == count) || (12 == count) || (15 == count)){
			if(':' != *pchar){
				return FALSE;
			}
		}else{
			if((*pchar >= '0' && *pchar <= '9')
					|| (*pchar >= 'a' && *pchar <= 'f')
					|| (*pchar >= 'A' && *pchar <= 'F')){

			}else{
				return FALSE;
			}
		}

		if(count > 17){
			return FALSE;
		}

		pchar++;
	}

	if(count != 17){
		return FALSE;
	}

	return TRUE;
}
#endif
/*
 * MAC address string valid check
 *
 * @param mac_str	MAC address string(such as '00:00:00:00:00:01')
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL fw_mac_str_valid(char *mac_str)
{
	char *pchar = NULL;
	uns32 count = 0;

	if(!mac_str){
		return FALSE;
	}

	pchar = mac_str;
	while(*pchar){
		count++;

		//validate the mac rule
		if((3 == count) || (6 == count) || (9 == count) || (12 == count) || (15 == count)){
			if(':' != *pchar){
				return FALSE;
			}
		}else{
			if((*pchar >= '0' && *pchar <= '9')
					|| (*pchar >= 'a' && *pchar <= 'f')
					|| (*pchar >= 'A' && *pchar <= 'F')){

			}else{
				return FALSE;
			}
		}

		if(count > 17){
			return FALSE;
		}

		pchar++;
	}

	if(count != 17){
		return FALSE;
	}

	return TRUE;
}

/*
 * ipv6 interface id string valid check
 *
 * @param id_str	interface ID string(such as '0:0:0:1')
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL ip6_if_id_str_valid(char *id_str)
{
	char test_addr[48] = {'0'};
	struct in6_addr ip6;
	int ret = 0;

	if (!id_str) {
		return FALSE;
	}

	snprintf(test_addr, sizeof(test_addr), "2001:0:0:0:");

	strncat(test_addr, id_str, sizeof(test_addr)-strlen("2001:0:0:0:")-1);
	
	ret = inet_pton(AF_INET6, test_addr, &ip6);
	if (ret == 0) {
		LOG_ER("validate %s, failed",  test_addr);
		return FALSE;
	}
	
	if ((ip6.s6_addr32[2] == 0) && (ip6.s6_addr32[3] == 0)) {
		return FALSE;
	}

	return TRUE;
}

/*
 * IEC61850 MAC string(last 3 bytes) valid check
 *
 * @param last3_byte_str	MAC string(last 3 bytes)(such as '001' (hex), valid range: <000-11f>)
 *
 * @return TRUE indicates valid, FALSE otherwise
 */
BOOL mac_last3_byte_str_valid(char *last3_byte_str)
{
	char *pchar = NULL;
	uns32 count = 0;
	uns32 value = 0;

	if(!last3_byte_str){
		return FALSE;
	}

	pchar = last3_byte_str;
	while(*pchar){
		count++;

		if((*pchar >= '0' && *pchar <= '9')
				|| (*pchar >= 'a' && *pchar <= 'f')
				|| (*pchar >= 'A' && *pchar <= 'F')){

		}else{
			return FALSE;
		}

		pchar++;
	}

	if(!count || count > 3){
		return FALSE;
	}

	sscanf(last3_byte_str, "%3x", &value);

	if((value > 0x1ff)){
		return FALSE;
	}

	return TRUE;
}

BOOL hex_str_valid(char *hex_str, uns32 count_min, uns32 count_max)
{
	char *pchar = NULL;
	uns32 count = 0;

	if(!hex_str){
		return FALSE;
	}

	pchar = hex_str;
	while(*pchar){
		count++;

		if((*pchar >= '0' && *pchar <= '9')
				|| (*pchar >= 'a' && *pchar <= 'f')
				|| (*pchar >= 'A' && *pchar <= 'F')){

		}else{
			return FALSE;
		}

		pchar++;
	}

	if(count < count_min || count > count_max){
		return FALSE;
	}

	return TRUE;
}

/*
 * get MAC address from MAC address string
 *
 * @param mac_str	MAC address string(such as '0000.0000.0001')
 * @param addr	   (out)MAC address
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 get_mac_from_str(char *mac_str, MAC_ADDR *addr)
{
	uns32 i = 0;

	unsigned int mac[MAX_MAC_LEN] = {0};


	if(!mac_str || !addr){
		return IH_ERR_NULL_PTR;
	}

	if(!mac_str_valid(mac_str)){
		return IH_ERR_INVALID;
	}

	sscanf(mac_str, "%2x%2x.%2x%2x.%2x%2x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

	for(i = 0; i < MAX_MAC_LEN; i++){
		addr->mac[i] = mac[i];
	}

	return IH_ERR_OK;
}

/*
 * get MAC address string from MAC address
 *
 * @param addr		MAC address
 * @param mac_str	(out)formated MAC address string
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 mac_to_str(MAC_ADDR addr, MAC_STR *mac_str)
{
	if(!mac_str){
		return IH_ERR_NULL_PTR;
	}

	bzero(mac_str->str, sizeof(mac_str->str));

	sprintf(mac_str->str, "%02x%02x.%02x%02x.%02x%02x", addr.mac[0], addr.mac[1], addr.mac[2], addr.mac[3], addr.mac[4], addr.mac[5]);

	return IH_ERR_OK;
}

/*
 * convert slot & port bitmap to string.
 *  (example: slot = 1, pbmp = 6, then str will be "1/1-2")
 *
 * @param slot		slot number
 * @param pbmp		just port bitmap(bitx represents portx)
 * @param str		(out)formated slot/port string
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 slot_pbmp_to_str(uns16 slot, PBMP pbmp, char *str)
{
    uns8 start = 0;
    uns8 end = 0;
    uns32 port = 0;
    uns32 len = 0;
    BOOL printed = TRUE;
    BOOL begin = FALSE;

    if(!str){
    	return IH_ERR_NULL_PTR;
    }

    PBMP_FOREACH_PORT(pbmp, port){
		if(!begin){ /* mark the start */
			start = port;
			end = start;

			begin =TRUE;
		}else if(port == (end + 1)){ /* in sequential order, increase the end */
			end = port;
		}else{ /* print the last range */
			if(start == end){
			    len += sprintf(str+len, "%d/%d,", slot, start);
			}else{
				len += sprintf(str+len, "%d/%d-%d,", slot, start, end);
			}

			/* new range begins */
			start = port;
			end = start;
		}

		printed = FALSE;
    }
    PBMP_FOREACH_PORT_END

    if(!printed){ /* print the latest range */
		if(start == end){
			len += sprintf(str+len, "%d/%d,", slot, start);
		}else{
			len += sprintf(str+len, "%d/%d-%d,", slot, start, end);
		}
    }

    if(len >= 1){
        str[len - 1] = 0; /* ',' ---> 0 */
    }

    return IH_ERR_OK;
}


int32 get_cmd_type(IH_CMD *cmd, BOOL *no_cmd, BOOL *default_cmd)
{
	if(!cmd || !no_cmd || !default_cmd){
		return IH_ERR_PARAM;
	}

	*no_cmd = (CMD_TYPE_NO == cmd->type);
	*default_cmd = (CMD_TYPE_DEFAULT == cmd->type);

	return IH_ERR_OK;
}

#if 0
/*
 * get interface information from interface string
 *
 * @param str	    interface string(such as 'fastethernet 1/8')
 * @param if_info	(out)IF_INFO structure
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 get_interface_info_from_str(char * str, IF_INFO *if_info)
{
	char *p = NULL;
	char *q = NULL;
	char *ptmp = NULL;
	uns16 len = 0;

	if(!str || !if_info){
		return IH_ERR_NULL_PTR;
	}

	ptmp = str;
	while(*ptmp){
		len++;
		ptmp++;
	}

	q = strchr(str, ' ');
	p = strchr(str, '/');
	if (!p || (q > p) || (p == str)){
		LOG_ER("### q is %p, p is %p, str is %p\n", q, p, str);
		return IH_ERR_INVALID;
	}

	SKIP_WHITE(q);
	ptmp = q;

	if(!*ptmp){
		return IH_ERR_INVALID;
	}

	while(*ptmp && ptmp <p){ /* slot check */
		if(!((*ptmp >= '0') && (*ptmp <= '9'))){
			LOG_ER("### slot check failed! c = %c.\n", *ptmp);
			return IH_ERR_INVALID;
		}
		ptmp++;
	}

	ptmp = p+1;

	trim_str(ptmp);

	if(!*ptmp){
		return IH_ERR_INVALID;
	}

	while(*ptmp){ /* port check */
		if(!((*ptmp >= '0') && (*ptmp <= '9'))){
			LOG_ER("### port check failed! c = %c.\n", *ptmp);
			return IH_ERR_INVALID;
		}
		ptmp++;
	}

	if_info->type = get_if_type_by_name(str);
	if(!if_info->type){
		return IH_ERR_INVALID;
	}

	if_info->slot = atoi(q);
	if_info->port = atoi(p+1);

	return IH_ERR_OK;
}
#endif


#if 0
/*
 * get interface string  from IF_INFO *
 *
 * @param if_info	[in] 		IF_INFO structure
 * @param str	    	[out]		interface string(such as 'fastethernet 1/8', 'vlan 1')
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 get_str_from_interface_info(IF_INFO *if_info, char * str)
{
	char *type;
	uns32 port, vid;
	if(!str || !if_info){
		return IH_ERR_NULL_PTR;
	}

	type = get_if_name_by_type(if_info->type);

	if (!type) return IH_ERR_FAILED;
	
	switch(if_info->type) {
	case IF_TYPE_CELLULAR:
	case IF_TYPE_TUNNEL:
	case IF_TYPE_SVI:		
	case IF_TYPE_VT:
	case IF_TYPE_DIALER:
	case IF_TYPE_VP:
		snprintf(str, 256, "%s %d", type, if_info->port);
		break;
	case IF_TYPE_FE:
	case IF_TYPE_GE:
		snprintf(str, 256, "%s %d/%d", type, if_info->slot, if_info->port);
		break;
	case IF_TYPE_SUB_FE:
	case IF_TYPE_SUB_GE:
		port = (if_info->port/4096) + 1;
		vid = if_info->port % 4096;
		snprintf(str, 256, "%s %d/%d.%d", type, if_info->slot, port, vid);
		break;
	default:
		snprintf(str, 256, "%s %d/%d", type, if_info->slot, if_info->port);
		break;
	}
	
	return IH_ERR_OK;
}

/*
 * get interface information from interface string
 *
 * @param str	    interface string(such as 'fastethernet 1/8')
 * @param if_info	(out)IF_INFO structure
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 get_interface_info_from_str(char * str, IF_INFO *if_info)
{
	char *p, *q, *s;

	if(!str || !if_info){
		return IH_ERR_NULL_PTR;
	}

	if_info->type = get_if_type_by_name(str);
	if(!if_info->type){
		LOG_ER("invalid interface type");
		return IH_ERR_INVALID;
	}
	q = strchr(str, ' ');
	p = strchr(str, '/');
	s = strchr(str, '.');
	
	SKIP_WHITE(q);

	switch(if_info->type) {
	case IF_TYPE_CELLULAR:
	case IF_TYPE_TUNNEL:
	case IF_TYPE_SVI:		
	case IF_TYPE_LO:
	case IF_TYPE_VT:
	case IF_TYPE_DIALER:
	case IF_TYPE_VP:
		if_info->slot = 0;
		if_info->port = atoi(q);
		break;
	case IF_TYPE_FE:
		if_info->slot = atoi(q);
		if_info->port = atoi(p+1);
		if (s){
			if_info->vid = atoi(s+1);
			if_info->type = IF_TYPE_SUB_FE;			
		}
		break;
	case IF_TYPE_GE:
		if_info->slot = atoi(q);
		if_info->port = atoi(p+1);
		if (s){
			if_info->vid = atoi(s+1);
			if_info->type = IF_TYPE_SUB_GE;
		}
		break;
	default:
		return IH_ERR_INVALID;
		break;
	}

	return IH_ERR_OK;
}

/*
 * get interface information from CMD string.
 *  note: string modification will be done(by calling cmdsep).
 *
 * @param pcmd	    part of CMD string(such as 'fastethernet 1/8 xxx zzz')
 * @param if_info	(out)IF_INFO structure
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 get_if_info_from_str(char **pcmd, IF_INFO *if_info)
{
	char *p = NULL;
	char *s = NULL;
	char *ptmp = NULL;
	char *value_str = NULL;
	int16 idx = 0;
	uns16 port = 0;
	PBMP app_full = 0;

	if(!pcmd || !(*pcmd)|| !if_info){
		return IH_ERR_NULL_PTR;
	}

	value_str = cmdsep(pcmd); /* interface type */

	if_info->type = get_if_type_by_name(value_str);
	if(!if_info->type){
		return IH_ERR_NOT_FOUND;
	}

	value_str = cmdsep(pcmd); /* slot/port */

	s = strchr(value_str, '.');
	p = strchr(value_str, '/');
	if (!p || (p == value_str)){
		return IH_ERR_NOT_FOUND;
	}

	ptmp = value_str;
	if(!*ptmp){
		return IH_ERR_INVALID;
	}

	while(*ptmp && ptmp <p){ /* slot check */
		if(!((*ptmp >= '0') && (*ptmp <= '9'))){
			LOG_ER("### slot check failed! c = %c.\n", *ptmp);
			return IH_ERR_INVALID;
		}
		ptmp++;
	}

	ptmp = p+1;

	if(!*ptmp){
		return IH_ERR_INVALID;
	}

	if (s){
		while(*ptmp && ptmp < s){ /* port check */
			if(!((*ptmp >= '0') && (*ptmp <= '9'))){
				LOG_ER("### port check failed! c = %c.\n", *ptmp);
				return IH_ERR_INVALID;
			}
			ptmp++;
		}
	}else{
		while(*ptmp){ /* port check */
			if(!((*ptmp >= '0') && (*ptmp <= '9'))){
				LOG_ER("### port check failed! c = %c.\n", *ptmp);
				return IH_ERR_INVALID;
			}
			ptmp++;
		}
	}

	if_info->slot = atoi(value_str);
	if_info->port = atoi(p+1);
	
	if (s){
		ptmp = s+1;

		if(!*ptmp){
			return IH_ERR_INVALID;
		}		
		while(*ptmp && ptmp < s){ /* vlan check */
			if(!((*ptmp >= '0') && (*ptmp <= '9'))){
				LOG_ER("### vlan id check failed! c = %c.\n", *ptmp);
				return IH_ERR_INVALID;
			}
			ptmp++;
		}
		if (if_info->type == IF_TYPE_FE)
			if_info->type = IF_TYPE_SUB_FE;
		else if (if_info->type == IF_TYPE_GE)
			if_info->type = IF_TYPE_SUB_GE;
		
		if_info->vid= atoi(s+1);
	}

	// add for IR9x2 by zyb
	if (1){
		if (!((if_info->slot == 0)
			&& (if_info->port >= 1) 
			&& (if_info->port <= 2))){
			return IH_ERR_INVALID;
		}
	}
	return IH_ERR_OK;

	/* check type & slot */
	idx = get_if_idx_by_type_slot(if_info->type, if_info->slot);
	if(idx < 0){
		return IH_ERR_INVALID;
	}

	if((if_info->port < g_if_slot_info[idx].port_min)
		|| (if_info->port > g_if_slot_info[idx].port_max)){
		return IH_ERR_INVALID;
	}

	port = g_if_slot_info[idx].base + if_info->port;

	app_full = get_app_pbmp_full();
	if(!pbmp_has_port(app_full, port)){
		LOG_ER("### no such port.\n");
		return IH_ERR_OUT_OF_RANGE;
	}

	return IH_ERR_OK;
}

#endif

/*
 * get logical port number from CMD string(panel port number).
 *  note: string modification will be done(by calling cmdsep).
 *
 * @param pcmd	    part of CMD string(such as 'fastethernet 1/8 xxx zzz')
 * @param port		(out)logical port number
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 get_port_from_str(char **pcmd, int16 *port)
{
	char *p = NULL;
	char *ptmp = NULL;
	char *value_str = NULL;
	int16 idx = 0;
	IF_INFO if_info;

	if(!pcmd || !(*pcmd)|| !port){
		return IH_ERR_NULL_PTR;
	}

	*port = -1;

	value_str = cmdsep(pcmd); /* interface type */

	if_info.type = get_if_type_by_name(value_str);
	if(!if_info.type){
		return IH_ERR_NOT_FOUND;
	}

	value_str = cmdsep(pcmd); /* slot/port */

	p = strchr(value_str, '/');
	if (!p || (p == value_str)){
		return IH_ERR_NOT_FOUND;
	}

	ptmp = value_str;
	if(!*ptmp){
		return IH_ERR_INVALID;
	}

	while(*ptmp && ptmp <p){ /* slot check */
		if(!((*ptmp >= '0') && (*ptmp <= '9'))){
			LOG_ER("### slot check failed! c = %c.\n", *ptmp);
			return IH_ERR_INVALID;
		}
		ptmp++;
	}

	ptmp = p+1;

	if(!*ptmp){
		return IH_ERR_INVALID;
	}

	while(*ptmp){ /* port check */
		if(!((*ptmp >= '0') && (*ptmp <= '9'))){
			LOG_ER("### port check failed! c = %c.\n", *ptmp);
			return IH_ERR_INVALID;
		}
		ptmp++;
	}

	if_info.slot = atoi(value_str);
	if_info.port = atoi(p+1);

	/* check type & slot */
	idx = get_if_idx_by_type_slot(if_info.type, if_info.slot);
	if(idx < 0){
		return IH_ERR_INVALID;
	}

	if((if_info.port < g_if_slot_info[idx].port_min)
		|| (if_info.port > g_if_slot_info[idx].port_max)){
		return IH_ERR_INVALID;
	}

	*port = g_if_slot_info[idx].base + if_info.port;

	if(!port_is_valid(*port)){
		return IH_ERR_INVALID;
	}

	if(!pbmp_has_port(get_app_pbmp_full(), *port)){
		LOG_ER("### no such port.\n");
		return IH_ERR_OUT_OF_RANGE;
	}

	return IH_ERR_OK;
}

/*
 * get port bitmap from interface range string, with string modification
 *
 * @param pcmd	    interface range string(such as 'fastethernet 1/1-3,1/5,1/8')
 * @param pbmp   	(out)logical port bitmap
 *
 * @return IH_ERR_OK for ok, others for error
 */
uns32 get_ifrange_portmap_from_str(char **pcmd, PBMP *pbmp)
{
	char *p = NULL;
	char *q = NULL;
	char *pstr = NULL;
	char *ptmp = NULL;
	uns16 slot = 0;
	uns16 min_port = 0;
	uns16 max_port = 0;
	uns32 last_max = 0;
	uns16 base = 0;
	uns16 i = 0;
	int16 port_idx = 0;
	IF_TYPE type = IF_TYPE_NONE;

	if(!pcmd || !pbmp)
	{
		return IH_ERR_NULL_PTR;
	}

	ptmp = cmdsep(pcmd); /* interface type */

	type = get_if_type_by_name(ptmp);
	if(!type){
		return IH_ERR_NOT_FOUND;
	}

	*pbmp = 0;

	pstr = stringsep(pcmd, ' ');

	ptmp = pstr;
	while(*ptmp){
        if(isdigit(*ptmp) || *ptmp == '/' || *ptmp == ',' || *ptmp == '-'){
        	ptmp++;
        }else{
        	return IH_ERR_INVALID;
        }
	}

	ptmp = pstr;
	while(*ptmp){
		min_port = 0;

		/* slot check */
        p = strchr(ptmp, '/');
        if(!p){
        	return IH_ERR_INVALID;
        }
		*p = 0;

		if(!is_digital_str(ptmp)){
			return IH_ERR_INVALID;
		}

        slot = atoi(ptmp);
        if(slot < MIN_SLOT || slot > MAX_SLOT){
        	return IH_ERR_INVALID;
        }

		p++;
		ptmp = p;

		p = strchr(ptmp, '-');
		q = strchr(ptmp, ',');

		/* check port min */
		if(p && ((q && (p < q)) || !q)){
			*p = 0;
			if(!is_digital_str(ptmp)){
				return IH_ERR_INVALID;
			}
			min_port = atoi(ptmp);

			port_idx = get_logical_port_number(type, slot, min_port);
			if(!port_is_valid(port_idx)){
				return IH_ERR_INVALID;
			}

			if(last_max > min_port){
				return IH_ERR_INVALID;
			}

			p++;
			ptmp = p;
		}

		/* check port max */
		if(q){
		    *q = 0;
		}

		if(!is_digital_str(ptmp)){
			return IH_ERR_INVALID;
		}
		max_port = atoi(ptmp);

		port_idx = get_logical_port_number(type, slot, max_port);
		if(!port_is_valid(port_idx)){
			return IH_ERR_INVALID;
		}

		if(min_port && (min_port > max_port)){
			return IH_ERR_INVALID;
		}

		if(!min_port){
			min_port = max_port;
		}

		if(last_max > max_port){
			return IH_ERR_INVALID;
		}

		base = get_port_base_by_type_slot(type, slot);

		for(i = min_port; i <= max_port; i++){
			*pbmp |= (1 << (i + base));
		}

		last_max = max_port;

		if(q){
		    q++;
		    ptmp = q;
		}else{
		    break;
		}
	}

	return IH_ERR_OK;
}

IH_STATUS fdb_entry_dump(FDB_ENTRY *fdb_entry, FDB_STR *fdb_str)
{
	MAC_STR mac_str;
	PORT_STR port_str;

	if(!fdb_entry || !fdb_str){
		return IH_ERR_NULL_PTR;
	}

	mac_to_str(fdb_entry->addr, &mac_str);

	if(fdb_entry->flag & FDB_FLAG_TRUNK_MEMBER){
		snprintf(port_str.str, sizeof(port_str.str), "T%d", fdb_entry->pbmp);
	}else{
		app_pbmp_to_str_abbr(fdb_entry->pbmp, &port_str);
	}

	snprintf(fdb_str->str, sizeof(fdb_str->str), "%-16s%-6d%-10d%#-10x%s\n", mac_str.str, fdb_entry->vid, fdb_entry->prio, fdb_entry->flag, port_str.str);

	return IH_ERR_OK;
}

/*
 * check if string is digital string or not
 *
 * @param str	    string
 *
 * @return IH_ERR_OK for ok, others for error
 */
BOOL is_digital_str(char *str)
{
	if(!str){
		return FALSE;
	}

	if(!*str){
		return FALSE;
	}

    while(*str){
    	if(!isdigit(*str)){
    		return FALSE;
    	}
    	str++;
    }

    return TRUE;
}

/*
 * check if string is digital string or not, ALLOW digital strings begin with '+' or '-'
 *
 * @param str	    string
 *
 * @return IH_ERR_OK for ok, others for error
 */
BOOL is_digital_str2(char *str)
{
	if(!str){
		return FALSE;
	}

	if(!*str){
		return FALSE;
	}

	if ((!isdigit(*str)) && ((*str) != '+') && ((*str) != '-'))
		return FALSE;
	else 
		str++;

    while(*str){
    	if(!isdigit(*str)){
    		return FALSE;
    	}
    	str++;
    }

    return TRUE;
}


/*
 * this routine get the portion before character 'c' if 'c' exist, and modify str location to be just after 'c'.
 * note that this implementation rewrite 'c' to '\0'
 *
 * @param str	    string
 *
 * @return IH_ERR_OK for ok, others for error
 */
char *stringsep(char **str, char c)
{
	char *p;
	char *pcmd = *str;

	if (!pcmd) return NULL;

	SKIP_WHITE(pcmd);

	p = strchr(pcmd, c);


	if (!p) {
		*str = NULL;
		return pcmd;
	}

	*p = '\0';
	p++;
	SKIP_WHITE(p);
	*str = *p ? p : NULL;

	return pcmd;
}

uns16 get_member_type_by_name(char *name)
{
	uns32 i = 0;

	if(!name){
		return VLAN_MEMBER_NOT_A_MEMBER;
	}

	for(i = 0; i < ARRAY_COUNT(g_vlan_member); i++){
        if(!strncmp(name, g_vlan_member[i].name, 1)){
        	return g_vlan_member[i].type;
        }
	}

	return VLAN_MEMBER_NOT_A_MEMBER;
}

char *get_member_name_by_type(MEMBER_TYPE type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_vlan_member); i++){
        if(type == g_vlan_member[i].type){
        	return g_vlan_member[i].name;
        }
	}

	return NULL;
}

uns32 get_vlan_member_type_count(void)
{
	return ARRAY_COUNT(g_vlan_member);
}

BOOL vlan_member_type_valid(MEMBER_TYPE type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_vlan_member); i++){

		if(VLAN_MEMBER_NOT_A_MEMBER == g_vlan_member[i].type){
			continue;
		}

		if(type == g_vlan_member[i].type){
			return TRUE;
		}
	}

	return FALSE;
}

uns32 get_vlan_member_type_by_idx(uns16 idx)
{
	if(idx < 0 || idx >= ARRAY_COUNT(g_vlan_member)){
		return VLAN_MEMBER_NOT_A_MEMBER;
	}

	return g_vlan_member[idx].type;
}

char *get_vlan_member_type_name_by_idx(uns16 idx)
{
	if(idx < 0 || idx >= ARRAY_COUNT(g_vlan_member)){
		return NULL;
	}

	return g_vlan_member[idx].name;
}

#define monitor_str_equal(storm1, storm2)!strncmp(storm1, storm2, 1)

int16 get_monitor_type_by_name(char *name, uint8 *type)
{
	uns32 i = 0;

	if(!name || !type){
        return IH_ERR_NULL_PTR;
	}

	for(i = 1; i < ARRAY_COUNT(g_monitor_pair); i++){
        if(monitor_str_equal(name, g_monitor_pair[i].name)){
        	*type = g_monitor_pair[i].type;
        	return IH_ERR_OK;
        }
	}

	return IH_ERR_NOT_FOUND;
}

char *get_monitor_name_by_type(MONITOR_TYPE type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_monitor_pair); i++){
        if(type == g_monitor_pair[i].type){
        	return g_monitor_pair[i].name;
        }
	}

	return NULL;
}

#define duplex_str_equal(duplex1, duplex2) !strncmp(duplex1, duplex2, 1)

uns16 get_duplex_type_by_name(char *name, uint8 *type)
{
	uns32 i = 0;

	if(!name || !type){
        return IH_ERR_NULL_PTR;
	}

	for(i = 0; i < ARRAY_COUNT(g_duplex_pair); i++){
        if(duplex_str_equal(name, g_duplex_pair[i].name)){
        	*type = g_duplex_pair[i].type;
        	return IH_ERR_OK;
        }
	}

	return IH_ERR_NOT_FOUND;
}

char *get_duplex_name_by_type(PORT_DUPLEX type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_duplex_pair); i++){
        if(type == g_duplex_pair[i].type){
        	return g_duplex_pair[i].name;
        }
	}

	return NULL;
}

#define speed_str_equal(speed1, speed2) !strcmp(speed1, speed2)

uns16 get_speed_type_by_name(char *name, uint8 *type)
{
	uns32 i = 0;

	if(!name || !type){
        return IH_ERR_NULL_PTR;
	}

	for(i = 0; i < ARRAY_COUNT(g_speed_pair); i++){
        if(speed_str_equal(name, g_speed_pair[i].name)){
        	*type = (PORT_SPEED)g_speed_pair[i].type;
        	return IH_ERR_OK;
        }
	}

	return IH_ERR_NOT_FOUND;
}

char *get_speed_name_by_type(PORT_SPEED type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_speed_pair); i++){
        if(type == g_speed_pair[i].type){
        	return g_speed_pair[i].name;
        }
	}

	return NULL;
}

#define block_str_equal(block1, block2)!strncmp(block1, block2, 1)

uns16 get_block_type_by_name(char *name, uint8 *type)
{
	uns32 i = 0;

	if(!name || !type){
        return IH_ERR_NULL_PTR;
	}

	for(i = 1; i < ARRAY_COUNT(g_block_pair); i++){
        if(block_str_equal(name, g_block_pair[i].name)){
        	*type = g_block_pair[i].type;
        	return IH_ERR_OK;
        }
	}

	return IH_ERR_NOT_FOUND;
}

char *get_block_name_by_type(PORT_BLOCK type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_block_pair); i++){
        if(type == g_block_pair[i].type){
        	return g_block_pair[i].name;
        }
	}

	return NULL;
}

#define discard_str_equal(discard1, discard2)!strncmp(discard1, discard2, 1)

uns16 get_discard_type_by_name(char *name, uint8 *type)
{
	uns32 i = 0;

	if(!name || !type){
        return IH_ERR_NULL_PTR;
	}

	for(i = 1; i < ARRAY_COUNT(g_discard_pair); i++){
        if(discard_str_equal(name, g_discard_pair[i].name)){
        	*type = g_discard_pair[i].type;
        	return IH_ERR_OK;
        }
	}

	return IH_ERR_NOT_FOUND;
}

char *get_discard_name_by_type(PORT_DISCARD type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_discard_pair); i++){
        if(type == g_discard_pair[i].type){
        	return g_discard_pair[i].name;
        }
	}

	return NULL;
}


#define storm_str_equal(storm1, storm2)!strncmp(storm1, storm2, 1)

uns16 get_storm_type_by_name(char *name, int16 *type)
{
	uns32 i = 0;

	if(!name || !type){
        return IH_ERR_NULL_PTR;
	}

	for(i = 1; i < ARRAY_COUNT(g_storm_pair); i++){
        if(storm_str_equal(name, g_storm_pair[i].name)){
        	*type = g_storm_pair[i].type;
        	return IH_ERR_OK;
        }
	}

	return IH_ERR_NOT_FOUND;
}

char *get_storm_name_by_type(int16 type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_storm_pair); i++){
	        if(type == g_storm_pair[i].type){
	        	return g_storm_pair[i].name;
	        }
	}

	return NULL;
}


uns32 get_vlan_member_by_type(uns16 vid, MEMBER_TYPE type)
{
	if(!vid_is_valid(vid)){
		return 0;
	}

	if(VLAN_MEMBER_EGRESS_TAGGED == type){
		return g_vlan_info[vid - 1].tagged;
	}else if(VLAN_MEMBER_EGRESS_UNTAGGED == type){
		return g_vlan_info[vid - 1].untagged;
	}else if(VLAN_MEMBER_EGRESS_UNMODIFIED == type){
		return g_vlan_info[vid - 1].unmodified;
	}else{
		return 0;
	}

	return 0;
}

#define override_str_equal(override1, override2)!strncmp(override1, override2, 1)

uns16 get_override_type_by_name(char *name, int16 *type)
{
	uns32 i = 0;

	if(!name || !type){
        return IH_ERR_NULL_PTR;
	}

	for(i = 1; i < ARRAY_COUNT(g_override_pair); i++){
        if(override_str_equal(name, g_override_pair[i].name)){
        	*type = g_override_pair[i].type;
        	return IH_ERR_OK;
        }
	}

	return IH_ERR_NOT_FOUND;
}

char *get_override_name_by_type(int16 type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_override_pair); i++){
        if(type == g_override_pair[i].type){
        	return g_override_pair[i].name;
        }
	}

	return NULL;
}

/*
 * get net event name
 *
 * @param str	    net event type
 *
 * @return net event name
 */
char *get_net_event_name(uns32 type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_net_event_pair); i++){
        if(type == g_net_event_pair[i].type){
        	return g_net_event_pair[i].name;
        }
	}

	return NULL;
}

/*
 * get PHY event name
 *
 * @param type	    PHY event type
 *
 * @return PHY event name
 */
char *get_phy_event_name(uns32 type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_phy_event_pair); i++){
        if(type == g_phy_event_pair[i].type){
        	return g_phy_event_pair[i].name;
        }
	}

	return NULL;
}

/*
 * get QinQ mode name
 *
 * @param type	    QinQ mode
 *
 * @return QinQ mode name
 */
char *get_qinq_mode_name(uns32 type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_qinq_pair); i++){
        if(type == g_qinq_pair[i].type){
        	return g_qinq_pair[i].name;
        }
	}

	return NULL;
}

/*
 * get ATU violation name
 *
 * @param type	    ATU violation
 *
 * @return ATU violation name
 */
char *get_atu_violation_name(uns32 type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_atu_violation_pair); i++){
        if(type == g_atu_violation_pair[i].type){
        	return g_atu_violation_pair[i].name;
        }
	}

	return NULL;
}

/*
 * get RM request name
 *
 * @param type	    request type
 *
 * @return product type name
 */
char *get_rm_request_name(uns32 type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_rm_request_type_name); i++){
        if(type == g_rm_request_type_name[i].type){
        	return g_rm_request_type_name[i].name;
        }
	}

	return NULL;
}

/*
 * get STP state by string
 *
 * @param str	    STP state string
 *
 * @return STP state
 */
int16 get_stp_state_by_name(char *str)
{
	uns32 i = 0;

	if(!str){
		return -1;
	}

	for(i = 0; i < ARRAY_COUNT(g_stp_state_pair); i++){
        if(!strncmp(str, g_stp_state_pair[i].name, 1)){
        	return g_stp_state_pair[i].type;
        }
	}

	return -1;
}

/*
 * get STP state name
 *
 * @param type	    STP state
 *
 * @return STP state name
 */
char *get_stp_name_by_state(PORT_STATE type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_stp_state_pair); i++){
        if(type == g_stp_state_pair[i].type){
        	return g_stp_state_pair[i].name;
        }
	}

	return NULL;
}


/*
 * get frame mode by string
 *
 * @param str	    frame mode string
 *
 * @return frame mode
 */
int16 get_frame_mode_by_name(char *str)
{
	uns32 i = 0;

	if(!str){
		return -1;
	}

	for(i = 0; i < ARRAY_COUNT(g_frame_mode_pair); i++){
        if(!strncmp(str, g_frame_mode_pair[i].name, 1)){
        	return g_frame_mode_pair[i].type;
        }
	}

	return -1;
}

/*
 * get frame mode name
 *
 * @param type	    frame mode
 *
 * @return frame mode name
 */
char *get_frame_mode_name_by_type(PORT_FRAME_MODE type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_frame_mode_pair); i++){
        if(type == g_frame_mode_pair[i].type){
        	return g_frame_mode_pair[i].name;
        }
	}

	return NULL;
}

/*
 * get egress mode by string
 *
 * @param str	    egress mode string
 *
 * @return egress mode
 */
int16 get_egress_mode_by_name(char *str)
{
	uns32 i = 0;

	if(!str){
		return -1;
	}

	for(i = 0; i < ARRAY_COUNT(g_egress_mode_pair); i++){
        if(!strncmp(str, g_egress_mode_pair[i].name, 3)){
        	return g_egress_mode_pair[i].type;
        }
	}

	return -1;
}

/*
 * get egress mode name
 *
 * @param type	    egress mode
 *
 * @return egress mode name
 */
char *get_egress_mode_name_by_type(PORT_EGRESS_MODE type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_egress_mode_pair); i++){
        if(type == g_egress_mode_pair[i].type){
        	return g_egress_mode_pair[i].name;
        }
	}

	return NULL;
}

/*
 * get 802.1Q mode by string
 *
 * @param str	    802.1Q mode string
 *
 * @return 802.1Q mode
 */
int16 get_8021q_mode_by_name(char *str)
{
	uns32 i = 0;

	if(!str){
		return -1;
	}

	for(i = 0; i < ARRAY_COUNT(g_8021q_mode_pair); i++){
        if(!strncmp(str, g_8021q_mode_pair[i].name, 1)){
        	return g_8021q_mode_pair[i].type;
        }
	}

	return -1;
}

/*
 * get 802.1Q mode name
 *
 * @param type	    802.1Q mode
 *
 * @return 802.1Q mode name
 */
char *get_8021q_mode_name_by_type(PORT_8021Q_MODE type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_8021q_mode_pair); i++){
        if(type == g_8021q_mode_pair[i].type){
        	return g_8021q_mode_pair[i].name;
        }
	}

	return NULL;
}
char *qos_global_policy_name(int policy)
{
	switch(policy){
		case QOS_SCHEDULE_MODE_SP:
		return "sp";
		break;

		case QOS_SCHEDULE_MODE_RATIO_10_1:
		return "10vs1";
		break;

		case QOS_SCHEDULE_MODE_RATIO_5_1:
		return "5vs1";
		break;

		case QOS_SCHEDULE_MODE_RATIO_2_1:
		return "2vs1";
		break;

		default:
		break;
	}

	return "invalid";
}

#if 0
/*
 * return #port# interface type by interface type name
 *
 * @param str	    interface type name
 *
 * @return interface type
 */
IF_TYPE get_if_type_by_name(char *str)
{
	uns32 i = 0;
	char * p, *q;
	IF_TYPE type = IF_TYPE_NONE;
	
	if(!str){
		return IF_TYPE_NONE;
	}
	
	p = str;
	SKIP_WHITE(p);
	q = strchr(p, ' ');
	if (q) 	*q = '\0';

	for(i = 0; i < ARRAY_COUNT(g_if_type_name); i++){
	        if(!strncmp(p, g_if_type_name[i].name, 
				(strlen(p) < strlen(g_if_type_name[i].name))
					?(strlen(p)):(strlen(g_if_type_name[i].name)))){
			type = g_if_type_name[i].type;
			break;
	        }
	}

	if (q) 	*q = ' ';
	return type;
}

/*
 * return #port# interface type name
 *
 * @param type	    interface type
 *
 * @return interface type name
 */
char *get_if_name_by_type(IF_TYPE type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_if_type_name); i++){
        if(type == g_if_type_name[i].type){
        	return g_if_type_name[i].name;
        }
	}

	return NULL;
}

/*
 * return #port# interface type abbr name
 *
 * @param type	    interface type
 *
 * @return interface type name
 */
char *get_if_name_abbr_by_type(IF_TYPE type)
{
	uns32 i = 0;

	for(i = 0; i < ARRAY_COUNT(g_if_type_name_abbr); i++){
        if(type == g_if_type_name_abbr[i].type){
        	return g_if_type_name_abbr[i].name;
        }
	}

	return NULL;
}

#endif



#if 0
/** @Get VIF name by IF_INFO. The vif maybe do not exist.
 * 
 * @param if_info	 	[in]	interface infomation [type, slot, port], always got from CLI
 * @param name		[in]   pointer to the memery where name of vif is going to be stroed.
 *						The size MUST NOT be less than VIF_NAME_SIZE.
 *
 * @return IH_ERR_OK-- Successful;  IH_ERR_FAILED -- Failure
 */
IH_STATUS get_iface_sys_name(IF_INFO *if_info, char *name){

	IH_STATUS ret = IH_ERR_OK;

	switch(if_info->type){
	case IF_TYPE_FE:
	case IF_TYPE_GE:
		snprintf(name, VIF_NAME_SIZE, VIF_ETH_NAME, if_info->port - 1);
		break;		
	case IF_TYPE_CELLULAR:
		snprintf(name, VIF_NAME_SIZE, VIF_CELLULAR_NAME, if_info->port);
		break;
	case IF_TYPE_TUNNEL:
		snprintf(name, VIF_NAME_SIZE, VIF_GRE_NAME, if_info->port);
		break;
	case IF_TYPE_SVI:
		snprintf(name, VIF_NAME_SIZE, VIF_SVI_NAME, if_info->port);
		break;
	case IF_TYPE_LO:
		snprintf(name, VIF_NAME_SIZE, VIF_LO_NAME);
		break;
	case IF_TYPE_DIALER:
		//ppp11, 12...
		snprintf(name, VIF_NAME_SIZE, VIF_DIALER_NAME, if_info->port+10);
		break;
	case IF_TYPE_VP:
		//ppp21, 22...
		snprintf(name, VIF_NAME_SIZE, VIF_VP_NAME, if_info->port+20);
		break;
	default:
		LOG_ER("Can't get vif name map for IF_INFO(%d, %d, %d)",
			if_info->type, if_info->slot, if_info->port);
		ret = IH_ERR_FAILED;
		break;
	}

	return ret;
}


/*
 * Get IF_INFO information from CMD string. This function is called by services except CLI.
 * This func will not check the value of ARGs, so validating of ARGs must be done in CLI.
 *  note: string modification will be done(by calling cmdsep).
 *  interface string:
 *	<type> [<slot/>]<port>[.<sub-id>] 
 *
 * @param pcmd	    part of CMD string(such as 'vlan 1 xxx zzz')
 * @param if_info	(out)IF_INFO structure
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 get_iface_info_from_str(char **pcmd, IF_INFO *if_info)
{
	char *p, *s;
	char *ptmp = NULL;
	char *value_str = NULL;
	
	if(!pcmd || !(*pcmd)|| !if_info){
		return IH_ERR_NULL_PTR;
	}

	value_str = cmdsep(pcmd); /* interface type */

	if_info->type = get_if_type_by_name(value_str);
	if(!if_info->type)
		return IH_ERR_NOT_FOUND;

	value_str = cmdsep(pcmd); /* [<slot>/]<port>[.<sub-id>] */
	if (!value_str || !*value_str)
		return IH_ERR_NOT_FOUND;

	ptmp = value_str;
	s = strchr(value_str, '.');
	p = strchr(value_str, '/');
	if (!p){
		if_info->slot = 0;
	}else{
		*p = '\0';
		if_info->slot = atoi(ptmp);
		*p = '/';
		ptmp = p + 1;;
	}
	if (s)
		*s = '\0';
	if_info->port = atoi(ptmp);
	if (s){
		*s = '.';
		ptmp = s + 1;
		if_info->vid = atoi(ptmp);
		switch(if_info->type){
		case IF_TYPE_FE:
			if_info->type = IF_TYPE_SUB_FE;
			break;
		case IF_TYPE_GE:
			if_info->type = IF_TYPE_SUB_GE;
			break;
		/*TODO: add new sub interface*/
		default:
			break;
		}
	}else{
		if_info->vid = 0;
	}
	
	return IH_ERR_OK;
}
#endif 
