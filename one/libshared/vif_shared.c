/*
 *
 *   Virtual Interfaces shared function
 *
 * Copyright (c) 2001-2012 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 05/03/2012
 * Author: Henri HAN
 * Rewrite Author: Zhengyb
 *
 */
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include "ih_cmd.h"
#include "sw_ipc.h"
#ifdef NETCONF
#include "ih_sysrepo.h"
#endif
#include "vif_shared.h"
#include "build_info.h"
#include "bootenv.h"

extern IH_SVC_ID 	gl_my_svc_id;

/******************* VIF buffer Management infomation***************************************/
/* FixME: Follow these 3 steps to add a new type VIF*/
/* Step 1: add a VIF buffer management entry. 
 *	The total size of all VIF should not be larger than VIF_INFO_MAX_SIZE.
 *	Tips: Should modify the definition of VIF_INFO_MAX_SIZE while change this define.
 */
static VIF_BUFF_MGMT vif_mgmts[] = {
	/*{type, 		size, 			min, max}*/
	{IF_TYPE_CELLULAR,	VIF_CELLULAR_NUMBER, 	-1, -1},	/* Cellular */
	{IF_TYPE_SUB_CELLULAR,	VIF_SUB_CELLULAR_NUMBER,-1, -1},	/* Sub Interface of Cellular */
	//{IF_TYPE_FE,		VIF_ETH_NUMBER, 	-1, -1},	/* Fastethernet */
	{IF_TYPE_GE,		VIF_ETH_NUMBER, 	-1, -1},	/* Gigabitethernet */
	//{IF_TYPE_SUB_FE, 	VIF_SUB_NUMBER, 	-1, -1},	/* Sub Interface of Fastethernet */
  {IF_TYPE_LO, 		VIF_LOOPBACK_NUMBER, 	-1, -1}, 	/* Loopback */
	{IF_TYPE_TUNNEL, 	VIF_TUNNEL_NUMBER, 	-1, -1},	/* Tunnel*/
	//{IF_TYPE_VXLAN, 	VIF_VXLAN_NUMBER, 	-1, -1},	/* Vxlan and Tunnel use the same mgmt*/
	{IF_TYPE_SVI, 		VIF_VLAN_NUMBER, 	-1, -1},	/* Switch Virtual Interface*/
	//{IF_TYPE_DIALER, 	VIF_DIALER_NUMBER, 	-1, -1},	/* Dialer */
	{IF_TYPE_VP, 		VIF_VP_NUMBER, 		-1, -1},	/* Virtual PPP */
	{IF_TYPE_OPENVPN, 	VIF_OPENVPN_NUMBER, 	-1, -1},	/* Openvpn */
	{IF_TYPE_DOT11,		VIF_DOT11_NUMBER, 	-1, -1},	/* WLAN */
	{IF_TYPE_STA_DOT11,		VIF_DOT11_NUMBER, 	-1, -1},	/* WLAN STA */
	{IF_TYPE_SUB_DOT11,	VIF_SUB_DOT11_NUMBER, 	-1, -1},	/* Sub WLAN */
	//{IF_TYPE_BRIDGE,	VIF_BRIDGE_NUMBER, 	-1, -1},	/* BRIDGE */
	//{IF_TYPE_VE, 		VIF_VE_NUMBER, 		-1, -1},	/* Virtual Ethernet */
	{IF_TYPE_NONE, 		0, 			-1, -1}		/* Unused*/
};
/* Step 2: add Service ID of the VIF. */
IH_SVC_ID vif_svc[] = {
	IH_SVC_REDIAL,
	IH_SVC_IF,
	IH_SVC_XDSL,
#ifndef INHAND_IDTU9
	IH_SVC_GRE,
	IH_SVC_VPND,
#ifndef INHAND_IG5
	IH_SVC_DOT11,
#endif
#endif
	0
};
/* Step 3: map IF_INFO to the linux interface name. */
IH_STATUS vif_get_sys_name(IF_INFO *if_info, char *name)
{
	IH_STATUS ret = IH_ERR_OK;
	uns16 dev_id, dev_port;

        switch(if_info->type){
        case IF_TYPE_FE:
        case IF_TYPE_GE:
					if(ih_license_support(IH_FEATURE_ETH2_KSZ9893)){
						if(if_info->port == 1){
							snprintf(name, VIF_NAME_SIZE, "%s.%d", SVI_BASIC_IF, DEFAULT_KSZ9893_VLAN);
						}else{
							snprintf(name, VIF_NAME_SIZE, "%s.%d", SVI_BASIC_IF, DEFAULT_VLAN_ID);
						}
						
					} if(ih_license_support(IH_FEATURE_ER3)){
						app_port_to_dev_port(if_info->port, &dev_id, &dev_port);

						if (if_info->port == IF_LAN_ETH_ID) {	//LAN1
							snprintf(name, VIF_NAME_SIZE, VIF_SVI_LAN_NAME, dev_port);
						} else {	//WAN
							snprintf(name, VIF_NAME_SIZE, VIF_ETH_NAME, dev_port);
						}
					}else if(ih_license_support(IH_FEATURE_ER6)){
						snprintf(name, VIF_NAME_SIZE, SPI_LAN_NAME, if_info->port);
					}else{
						snprintf(name, VIF_NAME_SIZE, VIF_ETH_NAME, if_info->port - 1);
					}
					break;
        case IF_TYPE_SUB_FE:
        case IF_TYPE_SUB_GE:
                snprintf(name, VIF_NAME_SIZE, VIF_SUBETH_NAME, if_info->port - 1, if_info->sid);
                break;
        case IF_TYPE_CELLULAR:
        case IF_TYPE_SUB_CELLULAR:
		if (ih_license_support(IH_FEATURE_MODEM_PLS8)
				|| ih_license_support(IH_FEATURE_MODEM_ELS31)
				|| ih_license_support(IH_FEATURE_MODEM_ELS81)
				|| (ih_license_support(IH_FEATURE_MODEM_ME909) 
					&& (ih_key_support("TH09")
						|| ih_key_support("FH20")))){
			snprintf(name, VIF_NAME_SIZE, VIF_CELLULAR_QMI_NAME); 
		} else if (ih_license_support(IH_FEATURE_MODEM_LP41)
					//|| ih_license_support(IH_FEATURE_MODEM_ELS61)
					|| (ih_license_support(IH_FEATURE_MODEM_ME909))) {
			if ((ih_license_support(IH_FEATURE_MODEM_ME909)) && ih_check_interface_exist(VIF_CELLULAR_QMI_NAME)) {
				snprintf(name, VIF_NAME_SIZE, VIF_CELLULAR_QMI_NAME); 
			} else {	
				snprintf(name, VIF_NAME_SIZE, VIF_CELLULAR_ETH2_NAME); 
			}

		} else if (ih_license_support(IH_FEATURE_MODEM_TOBYL201)) {
				snprintf(name, VIF_NAME_SIZE, VIF_CELLULAR_QMI_NAME); 
		} else if (ih_license_support(IH_FEATURE_MODEM_RM500)
				|| ih_license_support(IH_FEATURE_MODEM_EP06)) {
				snprintf(name, VIF_NAME_SIZE, VIF_CELLULAR_PCIE_RMNET_NAME, if_info->port);
		} else if (ih_license_support(IH_FEATURE_MODEM_RM520N)) {
				snprintf(name, VIF_NAME_SIZE, VIF_CELLULAR_APN_NAME, if_info->port);
		} else if (ih_license_support(IH_FEATURE_MODEM_FM650)
				|| ih_license_support(IH_FEATURE_MODEM_RM500U)) {
				snprintf(name, VIF_NAME_SIZE, VIF_CELLULAR_PCIE0_NAME); 
		} else if (ih_license_support(IH_FEATURE_MODEM_FG360)) {
				snprintf(name, VIF_NAME_SIZE, VIF_CELLULAR_MIPC_NAME, if_info->port); 
		} else if (ih_license_support(IH_FEATURE_MODEM_EC20)
				|| ih_license_support(IH_FEATURE_MODEM_EC25)
				|| ih_license_support(IH_FEATURE_MODEM_RG500)
				|| ih_license_support(IH_FEATURE_MODEM_NL668)) {
				snprintf(name, VIF_NAME_SIZE, VIF_CELLULAR_WWAN0_NAME);
		} else {
			snprintf(name, VIF_NAME_SIZE, VIF_CELLULAR_NAME, if_info->port);
		}
                break;
        case IF_TYPE_TUNNEL:
                snprintf(name, VIF_NAME_SIZE, VIF_GRE_NAME, if_info->port);
                break;
        case IF_TYPE_VXLAN:
                snprintf(name, VIF_NAME_SIZE, VIF_VXLAN_NAME, if_info->port);
                break;
        case IF_TYPE_SVI:
                if(IF_WAN_SVI_ID_BASE <= if_info->port && if_info->port <= IF_WAN_SVI_ID_MAX){
                    //for wan system interface
                    snprintf(name, VIF_NAME_SIZE, VIF_SVI_WAN_NAME, (if_info->port - IF_WAN_SVI_ID_BASE + 1));
                }else{
                    snprintf(name, VIF_NAME_SIZE, VIF_SVI_NAME, if_info->port);
                }
                break;
        case IF_TYPE_LO:
                snprintf(name, VIF_NAME_SIZE, VIF_LO_NAME);
                break;
        case IF_TYPE_DIALER:
                //ppp11, 12...
                snprintf(name, VIF_NAME_SIZE, VIF_DIALER_NAME, if_info->port+10);
                break;
        case IF_TYPE_VP:
                //ppp21, 22...30(l2tp client)
                //ppp31, 32...40(l2tp server)
                //ppp41, 42...50(pptp client)
                //ppp51, 52...60(pptp server)
                snprintf(name, VIF_NAME_SIZE, VIF_VP_NAME, if_info->port+20);
                break;
        case IF_TYPE_OPENVPN:
				snprintf(name, VIF_NAME_SIZE, VIF_OPENVPN_NAME, if_info->port);
                break;
        case IF_TYPE_DOT11:
#if (defined INHAND_IR9 && !(defined INHAND_VG9))
				snprintf(name, VIF_NAME_SIZE, VIF_SVI_NAME, DOT11_SVI_PORT + (if_info->port - 1));
#elif defined INHAND_ER6
				if(if_info->port == 1){
					snprintf(name, VIF_NAME_SIZE, VIF_DOT11_2G_NAME, (if_info->port == 100) ? (if_info->port) : 0);
				}else{
					snprintf(name, VIF_NAME_SIZE, VIF_DOT11_5G_NAME, (if_info->port == 100) ? (if_info->port) : 0);
				}
#else
                snprintf(name, VIF_NAME_SIZE, VIF_DOT11_NAME, (if_info->port == 100) ? (if_info->port) : (if_info->port - 1));
#endif
				break;
        case IF_TYPE_SUB_DOT11:
#if (defined INHAND_IR9 && !(defined INHAND_VG9))
				snprintf(name, VIF_NAME_SIZE, VIF_SVI_NAME, DOT11_SVI_PORT + (if_info->port - 1 + if_info->sid));	//FIXME
#endif
#if defined INHAND_IR8
                snprintf(name, VIF_NAME_SIZE, VIF_SUBDOT11_NAME, (if_info->port -1), if_info->sid);
#elif defined INHAND_ER6
				if(if_info->port == 1){
					snprintf(name, VIF_NAME_SIZE, VIF_DOT11_2G_NAME, if_info->sid);
				}else{
					snprintf(name, VIF_NAME_SIZE, VIF_DOT11_5G_NAME, if_info->sid);
				}
#endif
                break;
        case IF_TYPE_STA_DOT11:
#ifdef INHAND_ER6
				snprintf(name, VIF_NAME_SIZE, "%s", (if_info->port == 100) ? VIF_DOT11_STA_NAME : "");
#else
                snprintf(name, VIF_NAME_SIZE, VIF_DOT11_NAME, (if_info->port == 100) ? (if_info->port) : (if_info->port - 1));
#endif
                break;
        case IF_TYPE_BRIDGE:
#ifdef NEW_SW_DSA
					snprintf(name, VIF_NAME_SIZE, VIF_BRIDGE_NAME);
#else
					snprintf(name, VIF_NAME_SIZE, VIF_BRIDGE_NAME, if_info->port);
#endif
                break;
        case IF_TYPE_VE:
                snprintf(name, VIF_NAME_SIZE, VIF_VE_NAME, if_info->port);
                break;
        case IF_TYPE_CAN:
                snprintf(name, VIF_NAME_SIZE, VIF_CAN_NAME, if_info->port);
                break;
        default:
                LOG_ER("Can't get vif name map for IF_INFO(%d, %d, %d)",
                        if_info->type, if_info->slot, if_info->port);
                ret = IH_ERR_FAILED;
                break;
        }
	return ret;
}

/* network type used by OSPF */
VIF_NET_TYPE vif_get_net_type(IF_INFO *if_info)
{
	switch(if_info->type){
		case IF_TYPE_FE:
		case IF_TYPE_GE:
		case IF_TYPE_SUB_FE:
		case IF_TYPE_SUB_GE:
		case IF_TYPE_SVI:
		case IF_TYPE_LO:
		case IF_TYPE_BRIDGE:
		case IF_TYPE_DOT11:
		case IF_TYPE_SUB_DOT11:
			return VIF_NET_BCAST;
		case IF_TYPE_CELLULAR:
		case IF_TYPE_TUNNEL:
		case IF_TYPE_VXLAN:	//FIXME, vxlan maybe broadcast
		case IF_TYPE_DIALER:
		case IF_TYPE_VP:
		case IF_TYPE_OPENVPN:
		case IF_TYPE_SUB_CELLULAR:
		case IF_TYPE_VE:
			return VIF_NET_P2P;
    default:
			return VIF_NET_NONE;
  }
}


/******* Do not change the code below when you add a new type Virtual Interface ************/
VIF_INFO	gl_vif_info[VIF_INFO_MAX_SIZE];
vif_link_change_handle gl_vif_link_change_handle = NULL;

#define FOR_EACH_VIF_MGMT(ptmp_mgmt)\
	for (ptmp_mgmt = vif_mgmts; ptmp_mgmt->type != IF_TYPE_NONE; ptmp_mgmt++)

#define FOR_EACH_VIF_INFO_BY_MGMT(mgmt, pvif)\
	for(pvif = (gl_vif_info + mgmt->min); pvif <= (gl_vif_info + mgmt->max); pvif++)

static void vif_buff_mgmts_init(void)
{
	int total_len = 0;
	VIF_BUFF_MGMT *mgmt;

	memset(gl_vif_info, 0, sizeof(gl_vif_info));
	FOR_EACH_VIF_MGMT(mgmt){
		if (total_len + mgmt->size > VIF_INFO_MAX_SIZE){
			LOG_ER("Not enough space for VIF %d", mgmt->type);
			continue;
		}
		mgmt->min = total_len;
		total_len += mgmt->size;	
		mgmt->max = total_len - 1;
	}
}

VIF_BUFF_MGMT * get_vif_buff_mgmt(IF_TYPE type)
{
	VIF_BUFF_MGMT * ret;

	FOR_EACH_VIF_MGMT(ret)
		if (type == IF_TYPE_VXLAN) {
			if (ret->type == IF_TYPE_TUNNEL)
				return ret;
		} else 	if (ret->type == type)
			return ret;
	return NULL;
}

static VIF_INFO * find_vif_info_by_if_info(VIF_BUFF_MGMT * mgmt, IF_INFO * if_info)
{
	VIF_INFO * pvif;
	
	FOR_EACH_VIF_INFO_BY_MGMT(mgmt, pvif){
		if (IF_INFO_CMP(if_info, &(pvif->if_info)) == 0)
			return pvif;
	}
	return NULL;
}

static VIF_INFO * find_unused_vif_info(VIF_BUFF_MGMT * mgmt)
{
	VIF_INFO * pvif;

	FOR_EACH_VIF_INFO_BY_MGMT(mgmt, pvif){
		if (!pvif->if_info.type)
			return pvif;
	}
	return NULL;
}

void vif_dump_notify_info(VIF_INFO *pvif)
{
	//VIF_INFO *pvif = &tmp;
	char local_ip[16]={0}, local_netmask[16]={0}, remote_ip[16]={0};
	char dns1[16]={0}, dns2[16]={0};
	char *p;

	if (!pvif) {
		return;
	}

	p = inet_ntoa(pvif->local_ip);
	if(p) strlcpy(local_ip, p, sizeof(local_ip));
	p = inet_ntoa(pvif->netmask);
	if(p) strlcpy(local_netmask, p, sizeof(local_netmask));
	p = inet_ntoa(pvif->remote_ip);
	if(p) strlcpy(remote_ip, p, sizeof(remote_ip));
	p = inet_ntoa(pvif->dns1);
	if(p) strlcpy(dns1, p, sizeof(dns1));
	p = inet_ntoa(pvif->dns2);
	if(p) strlcpy(dns2, p, sizeof(dns2));
	LOG_DB("Notify VIF(type %d, slot %d, port %d, sid %d), state %d, iface %s alias %s, ip %s/%s, remote ip %s, dns1 %s, dns2 %s", 
			pvif->if_info.type, pvif->if_info.slot, pvif->if_info.port, pvif->if_info.sid,
			pvif->status, pvif->iface, pvif->alias, local_ip, local_netmask, remote_ip, 
			dns1, dns2);
}

static int32 sync_vif_link_change_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen) 
{
	VIF_INFO old_vif_info;
	VIF_INFO *new_vif_info=NULL;
	VIF_INFO *pvif;
	int size;
	int i, j;
	IH_STATUS rv = 0;
	VIF_BUFF_MGMT * mgmt;
	BOOL allow_self_msg = FALSE;
	char local_ip[16]={0}, local_netmask[16]={0}, remote_ip[16]={0};
	char dns1[16]={0}, dns2[16]={0};
	char *p;
	char *status_str[] = {"destroy", "create", "down", "up", "standby", "dormant", "NULL"};
	IH_SVC_ID peer_svc_id = 0;

	if(!msg_hdr || !msg){
		LOG_ER("msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if(!msg_len){
		LOG_ER("msg_len is zero\n");
		return IH_ERR_FAILED;	
	}

	if(gl_my_svc_id == IH_SVC_IF
		|| gl_my_svc_id == IH_SVC_REDIAL
		|| gl_my_svc_id == IH_SVC_DOT11){
		allow_self_msg = TRUE;
	}

	peer_svc_id = msg_hdr->svc_id;

    if (gl_my_svc_id == peer_svc_id && !allow_self_msg) {
		LOG_DB("received msg from self svc[%d]\n", gl_my_svc_id);
		return IH_ERR_OK;	
    }

	size = 	msg_len/sizeof(VIF_INFO);
	if(size <0 || size > VIF_INFO_MAX_SIZE){
		LOG_ER("msg len %d is out of scope\n", size);
		return IH_ERR_FAILED;
	}
	LOG_DB("VIF updating");//zyb
	new_vif_info = (VIF_INFO *)msg;
	for(i = 0; i<size; i++){
		/* Get mgmt infomation of the type. FixMe: */
		mgmt = get_vif_buff_mgmt(new_vif_info[i].if_info.type);
		if (!mgmt){
			LOG_ER("Bad VIF buffer MGMT infomation for VIF(type %d, slot %d, port, %d, sid %d), from service %d",
				new_vif_info[i].if_info.type, new_vif_info[i].if_info.slot,
				new_vif_info[i].if_info.port, new_vif_info[i].if_info.sid, msg_hdr->svc_id);
			continue;
		}
		/* Backup old VIF infomation or get an unused VIF struct*/
		pvif = find_vif_info_by_if_info(mgmt, &(new_vif_info[i].if_info));
		if (pvif){
			memcpy(&old_vif_info, pvif, sizeof(VIF_INFO));
		}else{ 
			memset(&old_vif_info, 0, sizeof(VIF_INFO));
			pvif = find_unused_vif_info(mgmt);
			if (!pvif){
				LOG_ER("No space for VIF(type %d, slot %d, port, %d, sid %d)",
					new_vif_info[i].if_info.type, new_vif_info[i].if_info.slot,
					new_vif_info[i].if_info.port, new_vif_info[i].if_info.sid);
				continue;
			}
		}
		/* Update new VIF infomation */
		memcpy(pvif, &new_vif_info[i], sizeof(VIF_INFO));

		/* Log */
		p = inet_ntoa(pvif->local_ip);
		if(p) strlcpy(local_ip, p, sizeof(local_ip));
		p = inet_ntoa(pvif->netmask);
		if(p) strlcpy(local_netmask, p, sizeof(local_netmask));
		p = inet_ntoa(pvif->remote_ip);
		if(p) strlcpy(remote_ip, p, sizeof(remote_ip));
		p = inet_ntoa(pvif->dns1);
		if(p) strlcpy(dns1, p, sizeof(dns1));
		p = inet_ntoa(pvif->dns2);
		if(p) strlcpy(dns2, p, sizeof(dns2));
		LOG_DB("Update VIF(type %d, slot %d, port %d, sid %d), state %d(%s), iface %s, alias %s ip %s/%s, remote ip %s, dns1 %s, dns2 %s, pub ip %s, svc %d", 
			pvif->if_info.type, pvif->if_info.slot, pvif->if_info.port, pvif->if_info.sid,
			pvif->status, status_str[pvif->status], pvif->iface, pvif->alias, local_ip, local_netmask, remote_ip, 
			dns1, dns2, pvif->public_ip, peer_svc_id);
		
		for(j = 0; j < MAX_IPV6_ADDR_PER_IF; j++){
			if(IN6_IS_ADDR_UNSPECIFIED (&pvif->ip6_addr.static_ifaddr[j].ip6)){
				continue;
			}
			LOG_DB(" VIF ipv6 static address %s/%d\n", inet6_ntoa(pvif->ip6_addr.static_ifaddr[j].ip6), pvif->ip6_addr.static_ifaddr[j].prefix_len);
		}

		for(j = 0; j < MAX_IPV6_ADDR_PER_IF; j++){
			if(IN6_IS_ADDR_UNSPECIFIED (&pvif->ip6_addr.dynamic_ifaddr[j].ip6)){
				continue;
			}
			LOG_DB(" VIF ipv6 dynamic address %s/%d\n", inet6_ntoa(pvif->ip6_addr.dynamic_ifaddr[j].ip6), pvif->ip6_addr.dynamic_ifaddr[j].prefix_len);
		}

		/* Call handle function of the uper App*/
		if(gl_vif_link_change_handle){
			if(old_vif_info.if_info.type){
				rv = gl_vif_link_change_handle(&old_vif_info, &(new_vif_info[i]));
				if(rv)	LOG_ER("gl_vif_link_change_handle err");
			} else {
				rv = gl_vif_link_change_handle(NULL, &(new_vif_info[i]));
				if(rv)	LOG_ER("gl_vif_link_change_handle err");
			}						
		}
		/* Release the space if destroied*/
		if (pvif->status == VIF_DESTROY)
			memset(pvif,0,sizeof(VIF_INFO));
	}

	return IH_ERR_OK;
}


IH_STATUS vif_link_change_register(vif_link_change_handle handle) 
{
	/* Init. VIF buffers and MGMTs*/
	vif_buff_mgmts_init();
	/* Register VIF LINK message and handle functions */
	/* Init. logical port & hardware port map*/
	if_slot_portmap_init();
	ih_ipc_subscribe(IPC_MSG_VIF_LINK_CHANGE);
	gl_vif_link_change_handle = handle;
	ih_ipc_register_msg_handle(IPC_MSG_VIF_LINK_CHANGE, sync_vif_link_change_handle);
	
	LOG_DB("init vif buf and define vif handle");
	return IH_ERR_OK;
}

IH_STATUS ih_vif_req_link_change_info(IH_SVC_ID svc_id) 
{
	IH_SVC_ID i;
	int ret;

	/* Monitor*/
	for (i = 0; (i < sizeof(vif_svc)/sizeof(IH_SVC_ID)) && vif_svc[i]; i++){
		if (vif_svc[i] == svc_id) continue;

		//donot reqeust dot11, if modemname hasnot wlan
		if(vif_svc[i] == IH_SVC_DOT11){
			/*If we do not support wlan, we could not monitor dot11d service.*/
			if(!(ih_license_support(IH_FEATURE_WLAN_AP) || ih_license_support(IH_FEATURE_WLAN_STA))){
				continue;
			}
		}

		if(vif_svc[i] == IH_SVC_REDIAL){
			/*If we do not support redial, we could not monitor redial service.*/
			if (ih_license_support(IH_FEATURE_MODEM_NONE)) {
				continue;
			}
		}

		LOG_DB("Request info of svc %d", vif_svc[i]);
		ret = ih_ipc_send_no_ack(vif_svc[i], IPC_MSG_VIF_LINK_CHANGE_REQ, NULL, 0);
		if(ret){
			LOG_DB("request info of svc %d error: %d!", vif_svc[i], ret);
		}
	}

	return IH_ERR_OK;
}

IH_STATUS ih_vif_init(IH_SVC_ID svc_id) 
{
	IH_SVC_ID i;

	/* Monitor*/
	for (i = 0; (i < sizeof(vif_svc)/sizeof(IH_SVC_ID)) && vif_svc[i]; i++){
		if (vif_svc[i] == svc_id) continue;
		//donot monitor dot11, if modemname hasnot wlan
		if(vif_svc[i] == IH_SVC_DOT11){
			/*If we do not support wlan, we could not monitor dot11d service.*/
			if(!(ih_license_support(IH_FEATURE_WLAN_AP) || ih_license_support(IH_FEATURE_WLAN_STA))){
				continue;
			}
		}

		if(vif_svc[i] == IH_SVC_REDIAL){
			/*If we do not support redial, we could not monitor redial service.*/
			if (ih_license_support(IH_FEATURE_MODEM_NONE)) {
				continue;
			}
		}

		ih_ipc_monitor_service(vif_svc[i]);
		LOG_DB("Monitor svc %d", vif_svc[i]);
	}

	return IH_ERR_OK;
}

void vif_svc_event_handle(IH_SVC_ID peer_svc_id, IPC_EVENT_TYPE event, void *info)
{
	IH_STATUS rv;
	int i;

	if (!info) return ;
	for (i = 0; i < sizeof(vif_svc)/sizeof(IH_SVC_ID); i++){
		if (peer_svc_id == vif_svc[i]
			&& event == IPC_EVENT_SYNC_UP
			&& (*(BOOL*) info)){
			rv = ih_ipc_send_no_ack(peer_svc_id,  IPC_MSG_VIF_LINK_CHANGE_REQ, NULL, 0);
			if(rv)
				LOG_ER("### Request VIF infomation failed, SVC ID %d, error %d", 
					peer_svc_id, rv);
			else
				LOG_DB("### Request VIF infomation OK, SVC ID %d",
					peer_svc_id);
			break;
		}
	}
}


VIF_INFO * get_vif_by_if_info(IF_INFO *if_info) 
{
	VIF_INFO *pvif;	
	VIF_BUFF_MGMT * mgmt;

	mgmt = get_vif_buff_mgmt(if_info->type);
	if (!mgmt){
		LOG_ER("Bad VIF buffer MGMT infomation, type %d", if_info->type);
		return NULL;
	}

	FOR_EACH_VIF_INFO_BY_MGMT(mgmt, pvif){
		if (IF_INFO_CMP(if_info, &(pvif->if_info)) == 0)
			return pvif;
	}

	return NULL;
}

VIF_INFO * get_vif_by_gateway(char *gateway) 
{
	int i;

	if(gateway == NULL || !*gateway) {
		LOG_ER("get vif gateway null");
		return NULL;
	} else {
		for(i = 0; i <VIF_INFO_MAX_SIZE; i++){
			if(gl_vif_info[i].local_ip.s_addr){
				if(!strcmp(gateway, inet_ntoa(gl_vif_info[i].remote_ip))) {
					return &gl_vif_info[i];
				}			
			}		
		}
		return NULL;
	}
}

VIF_INFO * get_vif_by_gateway2(char *gateway)
{
	int i,j;
	struct in_addr in_subnet;
	struct in_addr gw_subnet;
	char gateway_subnet[VIF_IP_ADDR_SIZE]={0};
	char local_subnet[VIF_IP_ADDR_SIZE]={0};
	SND_IP_NETMASK * pip_netmask=NULL;
	if(gateway == NULL || !*gateway) {
		LOG_ER("get vif gateway null");
		return NULL;
	} else {
		for(i = 0; i <VIF_INFO_MAX_SIZE; i++){
			if(gl_vif_info[i].local_ip.s_addr && gl_vif_info[i].netmask.s_addr){
				in_subnet.s_addr = gl_vif_info[i].local_ip.s_addr&gl_vif_info[i].netmask.s_addr;
				inet_aton(gateway, &gw_subnet); 
				gw_subnet.s_addr = gw_subnet.s_addr&gl_vif_info[i].netmask.s_addr;
				memcpy(&local_subnet[0], inet_ntoa(in_subnet), VIF_IP_ADDR_SIZE);
				memcpy(&gateway_subnet[0], inet_ntoa(gw_subnet), VIF_IP_ADDR_SIZE);
				if(!strcmp(gateway_subnet,local_subnet)) {
					return &(gl_vif_info[i]);	
				}			
			}
			FOREACH_2ND_IP(&gl_vif_info[i], j, pip_netmask){
				in_subnet.s_addr = pip_netmask->ip.s_addr&pip_netmask->netmask.s_addr;
				inet_aton(gateway, &gw_subnet); 
				gw_subnet.s_addr = gw_subnet.s_addr&pip_netmask->netmask.s_addr;
				memcpy(&local_subnet[0], inet_ntoa(in_subnet), VIF_IP_ADDR_SIZE);
				memcpy(&gateway_subnet[0], inet_ntoa(gw_subnet), VIF_IP_ADDR_SIZE);

				if(!strcmp(gateway_subnet,local_subnet)) {
					return &(gl_vif_info[i]);	
				}
			}		
		}
		return NULL;
	}
}

VIF_INFO * get_vif_by_localip(char *localip) 
{
	int i;

	if(localip == NULL || !*localip) {
		LOG_ER("get vif localip null");
		return NULL;
	} else {
		for(i = 0; i <VIF_INFO_MAX_SIZE; i++){
			if(gl_vif_info[i].local_ip.s_addr){
				if(!strcmp(localip, inet_ntoa(gl_vif_info[i].local_ip))) {
					return &(gl_vif_info[i]);	
				}			
			}		
		}
		return NULL;
	}
}

VIF_INFO * get_vif_by_ip(char *ip) 
{
	int i = 0;
	int j = 0;

	if(!ip || !*ip) {
		LOG_DB("get vif ip null");
		return NULL;
	}

	for(i = 0; i <VIF_INFO_MAX_SIZE; i++){
		if(gl_vif_info[i].local_ip.s_addr){
			if(!strcmp(ip, inet_ntoa(gl_vif_info[i].local_ip))) {

				return &(gl_vif_info[i]);	
			}			
		}		

		for(j = 0; j < SND_IP_SIZE; j++){
			if (gl_vif_info[i].secondary_ip[j].ip.s_addr && gl_vif_info[i].secondary_ip[j].netmask.s_addr){
				if(!strcmp(ip, inet_ntoa((gl_vif_info[i].secondary_ip[j].ip)))) {
					return &(gl_vif_info[i]);	
				}			
			}
		}
	}

	return NULL;
}

VIF_INFO * get_vif_by_iface(char *iface) 
{
	int i;

	if(iface == NULL || !*iface) {
		LOG_ER("get vif localip null");
		return NULL;
	} else {
		for(i = 0; i <VIF_INFO_MAX_SIZE; i++){
			if(gl_vif_info[i].iface[0]){
				if(!strcmp(&gl_vif_info[i].iface[0], iface)) {
					return &(gl_vif_info[i]);	
				}			
			}		
		}
		return NULL;
	}
}

VIF_INFO * get_vif_by_subnet(char *subnet, char *netmask) 
{
	int i;
	struct in_addr in_subnet, in_mask;
	char local_subnet[VIF_IP_ADDR_SIZE];

	if(subnet == NULL || !*subnet) {
		LOG_IN("get vif subnet null");
		return NULL;
	} else {
        inet_aton(netmask, &in_mask);
		for(i = 0; i <VIF_INFO_MAX_SIZE; i++){
			if(gl_vif_info[i].local_ip.s_addr && gl_vif_info[i].netmask.s_addr
                    && gl_vif_info[i].netmask.s_addr == in_mask.s_addr){
				in_subnet.s_addr = gl_vif_info[i].local_ip.s_addr&gl_vif_info[i].netmask.s_addr;
				memcpy(&local_subnet[0], inet_ntoa(in_subnet), VIF_IP_ADDR_SIZE);
				if(!strcmp(subnet,local_subnet)) {
					return &(gl_vif_info[i]);	
				}			
			}		
		}
		return NULL;
	}
}


BOOL check_if_is_wan(IF_INFO *if_info)
{
#ifdef NETCONF
	sr_val_t *vals = NULL;
    char str[256];
    size_t val_count = 0;
#endif
	BOOL ret = FALSE;

	if (!if_info) {
		return FALSE;
	}

	switch (if_info->type) {
		case IF_TYPE_GE:
			if(ih_license_support(IH_FEATURE_ER3)){
				if (if_info->port == IF_WAN_ETH_ID) {
					return TRUE;
				} else {
					return FALSE;
				}
			}
			break;
		case IF_TYPE_CELLULAR:
			return TRUE;
			break;
		case IF_TYPE_SVI:
			if (IF_WAN_SVI_ID_BASE <= if_info->port && if_info->port <= IF_WAN_SVI_ID_MAX) {
				return TRUE;
			} else {
				return FALSE;
			}
			break;
        case IF_TYPE_STA_DOT11:
            return TRUE;
            break;
#ifdef NETCONF
        case  IF_TYPE_DOT11:
			////inhand-wlan:wlan-entries/wlan-entry/wlan-role
			snprintf(str, sizeof(str), "/inhand-wlan:wlan-entries/wlan-entry[profile-name=\"wlan%d\"]/wlan-role", if_info->port);
			ret = ih_sr_get_config_value_by_xpath(ih_sysrepo_srv.sr_sess, str, &vals, &val_count);
			if (ret == SR_ERR_OK) {
				LOG_DB("%s---%d tracing ret %d value[%s]", __func__, __LINE__, ret, sr_val_to_str(&vals[0]));
				if(!strcmp(sr_val_to_str(&vals[0]), "inhand-wlan:STA")){
					ret = TRUE;
				} else {
					ret = FALSE;
				}
			} else {
				ret = FALSE;
			}

			if (val_count) {
				sr_free_values(vals, val_count);
			}

			return ret;
			break;
#endif
		default :
			break;
	}

	return ret;
}
int get_wan_vifs_by_type(int type, VIF_INFO **vifs) 
{
	VIF_INFO *pvif;
	static VIF_INFO vif_tmp[MAX_UPLINK_NUM];
	VIF_BUFF_MGMT * mgmt;

	int num = 0;

	mgmt = get_vif_buff_mgmt(type);
	if (!mgmt){
		LOG_ER("Bad VIF buffer MGMT infomation, type %d", type);
		return 0;
	}

	FOR_EACH_VIF_INFO_BY_MGMT(mgmt, pvif){
		if (pvif->if_info.port > 0 && check_if_is_wan(&pvif->if_info)) {
			memcpy(&vif_tmp[num], pvif, sizeof(VIF_INFO));
			num++;
		}
	}

	*vifs = vif_tmp;

	LOG_DB("got wan vif num of type %d is [%d]", type, num);
	return num;
}

int get_uplink_vifs(VIF_INFO **vifs, int *num) 
{
	VIF_INFO *pvif;
	static VIF_INFO vif_tmp[MAX_UPLINK_NUM];
	int num_by_type = 0;
#ifdef INHAND_ER3
	int types[] = {IF_TYPE_GE, IF_TYPE_CELLULAR};
#else
	int types[] = {IF_TYPE_SVI, IF_TYPE_CELLULAR, IF_TYPE_STA_DOT11};
#endif
	int i, ret_n = 0;

    memset(vif_tmp, 0x0, sizeof(vif_tmp));
	for (i = 0; i < sizeof(types)/sizeof(int); i++) {
		num_by_type = get_wan_vifs_by_type(types[i], &pvif);
		if (num_by_type == 0) {
			continue;
		}

		if (ret_n + num_by_type > MAX_UPLINK_NUM) {
			LOG_ER("too many uplink wan interfaces");
			//return got wans
			break;
		}

		memcpy(&vif_tmp[ret_n], pvif, sizeof(VIF_INFO) * num_by_type);
		ret_n += num_by_type;
	}

	*vifs = vif_tmp;
	*num = ret_n;

	for (i = 0; i < ret_n; i++) {
		LOG_DB("get vif [%d] (type %d, slot %d, port %d, sid %d), state %d iface %s", i, 
			vif_tmp[i].if_info.type, vif_tmp[i].if_info.slot, vif_tmp[i].if_info.port, vif_tmp[i].if_info.sid,
			vif_tmp[i].status, vif_tmp[i].iface);
	}

	return ret_n;
}

int get_policy_route_table_id(IF_INFO *if_info)
{
	int i, table_id, port, ret;
	char panel_name[32] = {0};
	ROUTE_TABLE_MIRROR *p = NULL;
	ROUTE_TABLE_MIRROR table_mirror[] = {
		{"wan", 	1, 2},		//wan1,wan2
		{"cellular", 	1, 1}, 		//cellular1
		{"wlan-sta", 	0, 0}, 		//wlan-sta
		{"l2tp", 	1, 20}, 	//l2tp1, ... l2tp20
	};

	ret = get_panel_name_from_if_info(if_info, panel_name);
	if(ret != ERR_OK){
		return -1;
	}

	table_id = POLICY_ROUTE_TABLE_ID_BASE;
	for(i = 0; i < ARRAY_COUNT(table_mirror); i++){
		p = &table_mirror[i];
		if(!strstr(panel_name, p->panel_name)){
			table_id += p->port_start ? (p->port_end - p->port_start) + 1 : 1;
			continue;
		}
		
		//LOG_DB("name:%s table_id:%d port:%d", panel_name, table_id, if_info->port);

		if(p->port_start){
			//check port range
			if(strlen(panel_name) <= strlen(p->panel_name)){
				return -1;
			}
		
			port = atoi(panel_name + strlen(p->panel_name));
			if(port < p->port_start || port > p->port_end){
				return -1;
			}

			return table_id + (port - p->port_start);
		}else{
			if(strcmp(panel_name, p->panel_name)){
				return -1;
			}

			return table_id;
		}
	}
	
	return -1;
}

/*
 * get uplink display name
 *
 * @param if_info       [in]            IF_INFO structure
 * @param str           [out]           interface string(such as 'WAN1', 'Wi-Fi(STA)', 'Cellular(SIM1)'...)
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 get_uplink_dispay_name_from_vif_info(VIF_INFO *vif_info, char * str)
{
	IF_INFO *if_info = NULL;

    if(!str || !vif_info){
        return IH_ERR_NULL_PTR;
    }

	if_info = &vif_info->if_info;
	if (if_info->port <= 0 || !check_if_is_wan(if_info)) {
        return IH_ERR_INVALID;
	}

	LOG_DB("uplink interface type %d", if_info->type);
    switch(if_info->type) {
        case IF_TYPE_GE:
			if(ih_license_support(IH_FEATURE_ER3)){
                snprintf(str, IF_CMD_NAME_SIZE, "WAN");
			}
            break;
        case IF_TYPE_CELLULAR:
            snprintf(str, IF_CMD_NAME_SIZE, "Cellular(SIM%d)", vif_info->sim_id);
            break;
        case IF_TYPE_SVI:
            if (IF_WAN_SVI_ID_BASE <= if_info->port && if_info->port <= IF_WAN_SVI_ID_MAX) {
                snprintf(str, IF_CMD_NAME_SIZE, "WAN%d", (if_info->port - IF_WAN_SVI_ID_BASE + 1));
				break;
            }

			LOG_ER("unknown uplink interface %s", vif_info->iface);
			return IH_ERR_INVALID;
        case IF_TYPE_STA_DOT11:
            snprintf(str, IF_CMD_NAME_SIZE, "%s", "Wi-Fi(STA)");
            break;
        default:
			LOG_ER("unknown uplink interface %s", vif_info->iface);
			return IH_ERR_INVALID;
            break;
    }

    return IH_ERR_OK;
}


#if 0
IH_STATUS get_all_vif_by_type(int type, VIF_INFO *vif, int *size){
	
	if(!type) {
		LOG_ER("get vif type is invalid");
		return IH_ERR_FAILED;
	} else {

		switch(type){
			case IF_TYPE_TUNNEL: {
				vif_info_get_allelemet_by_type(VIF_TUNNEL_MIN, VIF_TUNNEL_MAX, IF_TYPE_TUNNEL, vif, size);
				break;
			}
			case IF_TYPE_CELLULAR: {
				vif_info_get_allelemet_by_type(VIF_CELLULAR_MIN, VIF_CELLULAR_MAX, IF_TYPE_CELLULAR, vif, size);
				break;
			}
			case IF_TYPE_SVI: {		
				vif_info_get_allelemet_by_type(VIF_VLAN_MIN, VIF_VLAN_MAX, IF_TYPE_SVI, vif, size);
				break;
			}
			case IF_TYPE_LO: {		
				vif_info_get_allelemet_by_type(VIF_LOOPBACK_MIN, VIF_LOOPBACK_MAX, IF_TYPE_LO, vif, size);
				break;
			}
			case IF_TYPE_DIALER: {		
				vif_info_get_allelemet_by_type(VIF_DIALER_MIN, VIF_DIALER_MAX, IF_TYPE_DIALER, vif, size);
				break;
			}
			case IF_TYPE_VP: {		
				vif_info_get_allelemet_by_type(VIF_VP_MIN, VIF_VP_MAX, IF_TYPE_VP, vif, size);
				break;
			}
			case IF_TYPE_FE:
			case IF_TYPE_GE:{		
				vif_info_get_allelemet_by_type(VIF_ETH_MIN, VIF_ETH_MAX, type, vif, size);
 				break;
			}
			case IF_TYPE_SUB_FE:
			case IF_TYPE_SUB_GE:{		
				vif_info_get_allelemet_by_type(VIF_SUB_MIN, VIF_SUB_MAX, type, vif, size);
 				break;
			}
			default :{
				LOG_ER("get all vif by type is %d invalid", type);
				return IH_ERR_FAILED;
			}	
		}
		return IH_ERR_OK;
	}
}
#endif 

IH_STATUS vif_get_iface(IF_INFO * if_info, char *iface)
{
	VIF_BUFF_MGMT * mgmt;
	VIF_INFO * pvif;

	mgmt = get_vif_buff_mgmt(if_info->type);
	if (!mgmt){
		LOG_ER("Bad VIF buffer MGMT infomation, type %d", if_info->type);
		return IH_ERR_FAILED;
	}

	pvif = find_vif_info_by_if_info(mgmt, if_info);
	if (!pvif){
		LOG_ER("VIF is not in list, type %d ,port %d", if_info->type, if_info->port);
		return IH_ERR_FAILED;
	}

	strlcpy(iface, &pvif->iface[0], VIF_IFACE_SIZE);
	return IH_ERR_OK;	
}

IH_STATUS vif_get_local_ip(IF_INFO * if_info, char *local_ip)
{
	VIF_BUFF_MGMT * mgmt;
	VIF_INFO * pvif;

	mgmt = get_vif_buff_mgmt(if_info->type);
	if (!mgmt){
		LOG_DB("Bad VIF buffer MGMT infomation, type %d", if_info->type);
		return IH_ERR_FAILED;
	}

	pvif = find_vif_info_by_if_info(mgmt, if_info);
	if (!pvif){
		//LOG_ER("VIF is not in list");
		return IH_ERR_FAILED;
	}
	strlcpy(local_ip, inet_ntoa(pvif->local_ip), VIF_IP_SIZE);
	return IH_ERR_OK;
}

IH_STATUS vif_get_netmask(IF_INFO * if_info, struct in_addr *netmask)
{
	VIF_BUFF_MGMT * mgmt;
	VIF_INFO * pvif;

	mgmt = get_vif_buff_mgmt(if_info->type);
	if (!mgmt){
		LOG_ER("Bad VIF buffer MGMT infomation, type %d", if_info->type);
		return IH_ERR_FAILED;
	}

	pvif = find_vif_info_by_if_info(mgmt, if_info);
	if (!pvif){
		//LOG_ER("VIF is not in list");
		return IH_ERR_FAILED;
	}
	netmask->s_addr = pvif->netmask.s_addr;
	//strlcpy(netmask,inet_ntoa(gl_vif_info[i].netmask),VIF_IP_SIZE);
	return IH_ERR_OK;
}


IH_STATUS vif_get_ip6_addr(IF_INFO * if_info, IPv6_IFADDR* ip6_addr)
{
	VIF_BUFF_MGMT * mgmt;
	VIF_INFO * pvif;
	
	if(NULL == ip6_addr || NULL == if_info){
		return IH_ERR_FAILED;
	}
	mgmt = get_vif_buff_mgmt(if_info->type);
	if (!mgmt){
		LOG_ER("Bad VIF buffer MGMT infomation, type %d", if_info->type);
		return IH_ERR_FAILED;
	}

	pvif = find_vif_info_by_if_info(mgmt, if_info);
	if (!pvif){
		//LOG_ER("VIF is not in list");
		return IH_ERR_FAILED;
	}
	*ip6_addr = pvif->ip6_addr;
	return IH_ERR_OK;
}

IH_STATUS vif_get_status(IF_INFO * if_info, VIF_STATUS *status)
{
	VIF_BUFF_MGMT * mgmt;
	VIF_INFO * pvif;

	mgmt = get_vif_buff_mgmt(if_info->type);
	if (!mgmt){
		LOG_ER("Bad VIF buffer MGMT infomation, type %d", if_info->type);
		return IH_ERR_FAILED;
	}

	pvif = find_vif_info_by_if_info(mgmt, if_info);
	if (!pvif){
		* status = VIF_DESTROY;
		return IH_ERR_FAILED;
	}
	*status = pvif->status;
	return IH_ERR_OK;
}

IH_STATUS vif_check_valid(IF_INFO * if_info, BOOL *status)
{
	VIF_BUFF_MGMT * mgmt;
	VIF_INFO * pvif;

	*status = FALSE;
	mgmt = get_vif_buff_mgmt(if_info->type);
	if (!mgmt){
		LOG_ER("Bad VIF buffer MGMT infomation, type %d", if_info->type);
		return IH_ERR_FAILED;
	}

	pvif = find_vif_info_by_if_info(mgmt, if_info);
	if (!pvif){
		*status = FALSE;
		LOG_DB("get vif type %d port %d failed", if_info->type, if_info->port);
		return IH_ERR_OK;
	}
	if (pvif->status == VIF_DESTROY){
		*status = FALSE;
		LOG_DB("vif type %d port %d destroyed", if_info->type, if_info->port);
		return IH_ERR_OK;
	}
	*status = TRUE;
	return IH_ERR_OK;
}

IH_STATUS vif_get_dns_pair(IF_INFO * if_info, char *dns1, char *dns2)
{
	VIF_BUFF_MGMT * mgmt;
	VIF_INFO * pvif;

	dns1[0] = 0;
	dns2[0] = 0;
	mgmt = get_vif_buff_mgmt(if_info->type);
	if (!mgmt){
		LOG_ER("Bad VIF buffer MGMT infomation, type %d", if_info->type);
		return IH_ERR_FAILED;
	}

	pvif = find_vif_info_by_if_info(mgmt, if_info);
	if (!pvif){
		//LOG_ER("VIF is not in list");
		return IH_ERR_FAILED;
	}
	if (pvif->dns1.s_addr)
		strlcpy(dns1, inet_ntoa(pvif->dns1), VIF_DNS_SIZE);
	if (pvif->dns2.s_addr)
		strlcpy(dns2, inet_ntoa(pvif->dns2), VIF_DNS_SIZE);
	if (dns1[0] || dns2[0])
		return IH_ERR_OK;
	return IH_ERR_FAIL;
}

IH_STATUS vif_get_ipv6dns_pair(IF_INFO * if_info, struct in6_addr **dns1, struct in6_addr **dns2)
{
	VIF_BUFF_MGMT * mgmt;
	VIF_INFO * pvif;

	mgmt = get_vif_buff_mgmt(if_info->type);
	if (!mgmt){
		LOG_ER("Bad VIF buffer MGMT infomation, type %d", if_info->type);
		return IH_ERR_FAILED;
	}

	pvif = find_vif_info_by_if_info(mgmt, if_info);
	if (!pvif){
		//LOG_ER("VIF is not in list");
		return IH_ERR_FAILED;
	}

	*dns1 = &pvif->ip6_addr.ra_dns[0];
	*dns2 = &pvif->ip6_addr.ra_dns[1];

	if (*dns1 || *dns2)
		return IH_ERR_OK;
	return IH_ERR_FAIL;
}

//FIX ME
VIF_INFO * get_vif_by_ipv6_gateway(struct in6_addr gateway)
{
	VIF_INFO *vif;
	VIF_BUFF_MGMT *ptmp_mgmt;
	IPv6_SUBNET *ip6_addr;
	IF_TYPE i, j;

	for(i=IF_TYPE_NONE; i<IF_TYPE_INVALID; i++){
		FOR_EACH_VIF_INFO_BY_TYPE(i, vif, ptmp_mgmt){
			//check ipv6 gw
			FOREACH_STATIC_IPV6(vif, j, ip6_addr){
				if(IN6_ARE_ADDR_EQUAL(&gateway, &ip6_addr->gateway)) {
					LOG_DB("gw %s found, at type %d port %d", 
							inet6_ntoa(gateway), vif->if_info.type, vif->if_info.port);
					return vif;	
				}
			}

			FOREACH_DYNAMIC_IPV6(vif, j, ip6_addr){
				if(IN6_ARE_ADDR_EQUAL(&gateway, &ip6_addr->gateway)) {
					LOG_DB("gw %s found, at type %d port %d", 
							inet6_ntoa(gateway), vif->if_info.type, vif->if_info.port);
					return vif;	
				}
			}
		}
	}
	return NULL;
}

VIF_INFO * get_vif_by_ipv6_prefix(struct in6_addr prefix, int prefix_len)
{
	VIF_INFO *vif;
	VIF_BUFF_MGMT *ptmp_mgmt;
	IPv6_SUBNET *ip6_addr;
	IF_TYPE i, j;
	struct in6_addr vif_prefix;

	for(i=IF_TYPE_NONE; i<IF_TYPE_INVALID; i++){
		FOR_EACH_VIF_INFO_BY_TYPE(i, vif, ptmp_mgmt){
			//check ipv6 gw
			FOREACH_STATIC_IPV6(vif, j, ip6_addr){
				in6_addr_2prefix(ip6_addr->ip6, ip6_addr->prefix_len, &vif_prefix);
				if(ip6_addr->prefix_len == prefix_len 
						&& IN6_ARE_ADDR_EQUAL(&vif_prefix, &prefix)) {
					LOG_DB("prefix %s found, at type %d port %d", 
							inet6_ntoa(prefix), vif->if_info.type, vif->if_info.port);
					return vif;	
				}
			}

			FOREACH_DYNAMIC_IPV6(vif, j, ip6_addr){
				in6_addr_2prefix(ip6_addr->ip6, ip6_addr->prefix_len, &vif_prefix);
				if(ip6_addr->prefix_len == prefix_len 
						&& IN6_ARE_ADDR_EQUAL(&vif_prefix, &prefix)) {
					LOG_DB("prefix %s found, at type %d port %d", 
							inet6_ntoa(prefix), vif->if_info.type, vif->if_info.port);
					return vif;	
				}
			}
		}
	}
	return NULL;
}

VIF_INFO * get_vif_by_ipv6_address(char *local_ip6, char *dst_ip6)
{
	VIF_INFO *vif;
	VIF_BUFF_MGMT *ptmp_mgmt;
	IF_TYPE i;
	struct in6_addr vif_addr, ip6, dst;
	BOOL have_dst = FALSE;
	int ret;

	ret = inet_pton (AF_INET6, local_ip6, &ip6);
	if (ret == 0){
		return NULL;
	}

	ret = inet_pton (AF_INET6, dst_ip6, &dst);
	if (ret != 0){
		have_dst = TRUE;
	}

	for(i=IF_TYPE_NONE; i<IF_TYPE_INVALID; i++){
		FOR_EACH_VIF_INFO_BY_TYPE(i, vif, ptmp_mgmt){
			if (have_dst) {
				ret = iface_get_ip6_src_addr(vif->iface, &vif->ip6_addr, &dst, &vif_addr);
			} else {
				ret = iface_get_ip6_src_addr(vif->iface, &vif->ip6_addr, NULL, &vif_addr);
			}

			if(ret != IH_ERR_OK) {
				continue;
			}


			if(IN6_ARE_ADDR_EQUAL(&vif_addr, &ip6)){
				LOG_DB("ipv6 address %s found, at type %d port %d", 
						local_ip6, vif->if_info.type, vif->if_info.port);
				return vif;	
			}
		}
	}
	return NULL;
}

/* clear all IPv4 address from interface's VIF_INFO structure. */
int iface_clear_all_ip4_addr(char *iface, VIF_INFO *if_info)
{
	if(!iface || !if_info){
		return IH_ERR_NULL_PTR; 
	}

	LOG_IN("interface %s clear all IPv4 address", iface);

	memset(&if_info->local_ip, 0, sizeof(if_info->local_ip));
	memset(&if_info->netmask, 0, sizeof(if_info->local_ip));
	memset(&if_info->remote_ip, 0, sizeof(if_info->remote_ip));
	memset(&if_info->dns1, 0, sizeof(if_info->dns1));
	memset(&if_info->dns2, 0, sizeof(if_info->dns2));
	memset(if_info->secondary_ip, 0, sizeof(if_info->secondary_ip));

	return IH_ERR_OK;
}

IH_STATUS vif_if_ip_conflict(IF_INFO *if_info, char *ip, char *mask)
{
	struct in_addr in_ip, in_mask, min_mask;
	int i = 0, j = 0;
	char if_str[MAX_NAME_LEN]={0};

	if (!if_info || !ip || !*ip || !mask || !*mask) {
		return ERR_INVAL;
	}

	in_ip.s_addr = inet_addr(ip);
	in_mask.s_addr = inet_addr(mask);

	for(i = 0; i <VIF_INFO_MAX_SIZE; i++){
		if (!gl_vif_info[i].if_info.type) {
			continue;
		}

		if (IF_INFO_CMP(if_info, &(gl_vif_info[i].if_info)) == 0) {
			continue;
		}

		if (!gl_vif_info[i].local_ip.s_addr){
			continue;
		}

		/* use the shorter mask length to check, eg: 192.168.1.0/24 and 192.168.0.0/16 should be mutex*/
		min_mask.s_addr = in_mask.s_addr & gl_vif_info[i].netmask.s_addr;
		if ((in_ip.s_addr & min_mask.s_addr) == (gl_vif_info[i].local_ip.s_addr & min_mask.s_addr)) {
			get_str_from_if_info(&gl_vif_info[i].if_info, if_str);
			LOG_IN("%s/%s overlaps with %s!", ip, mask, if_str);
			ih_cmd_rsp_print("%%error: %s/%s overlaps with %s!", ip, mask, if_str);
			return ERR_OK;
		}

		for(j = 0; j< SND_IP_SIZE; j++) {
			if (!gl_vif_info[i].secondary_ip[j].ip.s_addr) {
				continue;
			}

			min_mask.s_addr = in_mask.s_addr & gl_vif_info[i].secondary_ip[j].netmask.s_addr;
			if ((in_ip.s_addr & min_mask.s_addr) == (gl_vif_info[i].secondary_ip[j].ip.s_addr & min_mask.s_addr)) {
				get_str_from_if_info(&gl_vif_info[i].if_info, if_str);
				LOG_IN("%s/%s overlaps with %s!", ip, mask, if_str);
				ih_cmd_rsp_print("%%error: %s/%s overlaps with %s!", ip, mask, if_str);
				return ERR_OK;
			}
		}
	}

	return ERR_FAILED;
}

IH_STATUS vif_if_route_conflict(char *ip, char *mask)
{
	struct in_addr in_ip, in_mask, min_mask;
	int i = 0, j = 0;
	char if_str[MAX_NAME_LEN]={0};

	if (!ip || !*ip || !mask || !*mask) {
		return ERR_INVAL;
	}

	in_ip.s_addr = inet_addr(ip);
	in_mask.s_addr = inet_addr(mask);

	for(i = 0; i <VIF_INFO_MAX_SIZE; i++){
		if (!gl_vif_info[i].if_info.type) {
			continue;
		}

		if (!gl_vif_info[i].local_ip.s_addr){
			continue;
		}

		if ((in_ip.s_addr & in_mask.s_addr) == (gl_vif_info[i].local_ip.s_addr & gl_vif_info[i].netmask.s_addr)) {
			get_str_from_if_info(&gl_vif_info[i].if_info, if_str);
			LOG_IN("%s/%s overlaps with %s!", ip, mask, if_str);
			ih_cmd_rsp_print("%%error: %s/%s overlaps with %s!", ip, mask, if_str);
			return ERR_OK;
		}

		for(j = 0; j< SND_IP_SIZE; j++) {
			if (!gl_vif_info[i].secondary_ip[j].ip.s_addr) {
				continue;
			}

			if ((in_ip.s_addr & in_mask.s_addr) == (gl_vif_info[i].secondary_ip[j].ip.s_addr & gl_vif_info[i].secondary_ip[j].netmask.s_addr)) {
				get_str_from_if_info(&gl_vif_info[i].if_info, if_str);
				LOG_IN("%s/%s overlaps with %s!", ip, mask, if_str);
				ih_cmd_rsp_print("%%error: %s/%s overlaps with %s!", ip, mask, if_str);
				return ERR_OK;
			}
		}
	}

	return ERR_FAILED;
}

int get_cli_str_from_sys_ifname(char *iface, char *str)
{
    VIF_INFO * vif;

    if (!iface || !str) {
	return ERR_FAILED;
    }

    vif = get_vif_by_iface(iface);

    if (!vif) {
	LOG_DB("no vif match to interface %s!", iface);
	return ERR_FAILED;
    }

    if(get_str_from_if_info(&(vif->if_info), str)){
	LOG_DB("cann't get cli str from interface %s!", iface);
	return ERR_FAILED;
    }

    return ERR_OK;
}

int get_sys_ifname_from_cli_str(char *str, char *iface)
{
    IF_INFO if_info;
    int ret;

    if (!iface || !str) {
	return ERR_FAILED;
    }

    ret = get_if_info_from_str2(str, &if_info);

    if (ret) {
	LOG_DB("no if info match to interface!");
	return ERR_FAILED;
    }

    if(vif_get_sys_name(&if_info, iface)){
	LOG_DB("cann't get system ifname!");
	return ERR_FAILED;
    }

    return ERR_OK;
}

int get_redial_info(MY_REDIAL_INFO *redial_info)
{
	IPC_MSG *rsp = NULL;
	int ret = 0;
	int flag = 1;

	if (ih_license_support(IH_FEATURE_MODEM_NONE)) {
		return ERR_FAILED;
	}

	if(!redial_info) {
		return ERR_FAILED;
	}

	ret = ih_ipc_send_wait_rsp(IH_SVC_REDIAL, 2000, IPC_MSG_REDIAL_SVCINFO_REQ, (char *)&flag, sizeof(flag), &rsp);
	if (ret) {
		syslog(LOG_INFO, "Request redial svcinfo failed[%d]!", ret);
		return ERR_FAILED;
	}
	
	memcpy(redial_info, (MY_REDIAL_INFO*)rsp->body, sizeof(MY_REDIAL_INFO));

	ih_ipc_free_msg(rsp);

	return ERR_OK;
}

int suspend_redial(BOOL enable)
{
	int ret = 0;

	if (ih_license_support(IH_FEATURE_MODEM_NONE)) {
		return ERR_OK;
	}

	ret = ih_ipc_send_no_ack(IH_SVC_REDIAL, IPC_MSG_REDIAL_SUSPEND_REQ, &enable, sizeof(enable));
	if(ret){
		LOG_IN("failed to send suspend redial request!");
	}

	return ret;
}

int get_port_status(PORT_STATUS_MSG *port_status, size_t num)
{
	IPC_MSG *rsp = NULL;
	int ret = 0;
	int flag = 1;

	if(!port_status) {
		return ERR_FAILED;
	}

	ret = ih_ipc_send_wait_rsp(IH_SVC_IF, 2000, IPC_MSG_IF_PORT_STATUS_REQ, (char *)&flag, sizeof(flag), &rsp);
	if (ret) {
		syslog(LOG_INFO, "Request port status failed[%d]!", ret);
		return ERR_FAILED;
	}
	
	memcpy(port_status, (PORT_STATUS_MSG*)rsp->body, rsp->hdr.len < (num * sizeof(PORT_STATUS_MSG)) ? rsp->hdr.len : num * sizeof(PORT_STATUS_MSG));

	ih_ipc_free_msg(rsp);

	return ERR_OK;
}

/*
 * get linux interface name from panel name
 * Note: input string will not be changed.
 *
 * @param str(IN)       interface string(such as 'wlan1, wan1, cellular1, vlan1, bridge1')
 * @param iface(OUT)     linux interface string(such as 'br1, eth1, eth0, ath1, ath2')
 *
 * @return ERR_OK for ok, others for error
 */
int get_sys_ifname_from_panel_name(char *str, char *iface)
{
    IF_INFO if_info;
    int ret;

    if (!iface || !str) {
	    return ERR_FAILED;
    }

    ret = get_if_info_from_panel_name(str, &if_info);

    if (ret) {
    	LOG_DB("no if info match to interface!");
	    return ERR_FAILED;
    }

    if(vif_get_sys_name(&if_info, iface)){
    	LOG_DB("cann't get system ifname!");
    	return ERR_FAILED;
    }

    return ERR_OK;
}

int uplink_interface_check(char *interface)
{
    int ret = 0;
    int i = 0;
    char *iface[] = {"any", "wan1", "wan2", "cellular1", "wlan-sta", NULL};
    
    if(!interface)
        return ret;
    
    while(iface[i]){
        if(!strcmp(interface, iface[i])){
            ret = 1;
            break;
        } 

        i++;
    }

    return ret;
}

int get_sta_status(DOT11_STA_STATUS *sta_status)
{
	IPC_MSG *rsp = NULL;
	int ret = 0;
	int flag = 1;

	if(!sta_status) {
		return ERR_FAILED;
	}

	if (!ih_license_support(IH_FEATURE_WLAN_STA)) {
		return ERR_FAILED;
	}

	ret = ih_ipc_send_wait_rsp(IH_SVC_DOT11, 2000, IPC_MSG_DOT11_STA_STATUS_REQ, (char *)&flag, sizeof(flag), &rsp);
	if (ret) {
		syslog(LOG_INFO, "Request dot11 sta status failed[%d]!", ret);
		return ERR_FAILED;
	}
	
	memcpy(sta_status, (DOT11_STA_STATUS*)rsp->body, sizeof(DOT11_STA_STATUS));

	ih_ipc_free_msg(rsp);

	return ERR_OK;
}

int vif_dormant_status_check(VIF_INFO *vif)
{
    VIF_STATUS vif_status;
    
    if(!vif){
        return ERR_FAILED;
    }

    memset(&vif_status, 0x0, sizeof(VIF_STATUS));
    vif_get_status(&(vif->if_info), &vif_status);
    if(vif_status == VIF_DORMANT){
        LOG_IN("get the vif status dormant");
        vif->status = VIF_DORMANT;
	} else if (vif_status == VIF_STANDBY) {
        LOG_IN("get the vif status standby");
        vif->status = VIF_STANDBY;
	}

    return ERR_OK;
}

/*
 *get the lan interface's ip
 */
VIF_INFO * get_lan_vif_by_ip(char *ip)
{
	int i = 0;
	struct in_addr in_subnet;
	struct in_addr ip_subnet;
	char subnet[VIF_IP_ADDR_SIZE]={0};
	char local_subnet[VIF_IP_ADDR_SIZE]={0};
	
    if(!ip || !*ip) {
		LOG_ER("get lan vif ip null");
		return NULL;
	} 
    
	for(i = 0; i <VIF_INFO_MAX_SIZE; i++){
		if(ih_license_support(IH_FEATURE_ER3)){
			if(gl_vif_info[i].if_info.type != IF_TYPE_GE) {
				continue;
			}

			if(gl_vif_info[i].if_info.port != IF_LAN_ETH_ID) {
				continue;
			}
		} else {
			if(gl_vif_info[i].if_info.type != IF_TYPE_SVI)
				continue;
		}

        if(gl_vif_info[i].local_ip.s_addr && gl_vif_info[i].netmask.s_addr){
		    in_subnet.s_addr = gl_vif_info[i].local_ip.s_addr & gl_vif_info[i].netmask.s_addr;
            memset(local_subnet, 0x0, sizeof(local_subnet));
            memcpy(&local_subnet[0], inet_ntoa(in_subnet), VIF_IP_ADDR_SIZE);
			
            inet_aton(ip, &ip_subnet); 
			ip_subnet.s_addr = ip_subnet.s_addr & gl_vif_info[i].netmask.s_addr;
            memset(subnet, 0x0, sizeof(subnet));
			memcpy(&subnet[0], inet_ntoa(ip_subnet), VIF_IP_ADDR_SIZE);
			
            if(!strcmp(subnet, local_subnet)) {
			    return &(gl_vif_info[i]);	
			}			
		}
    }
    
    return NULL;
}
