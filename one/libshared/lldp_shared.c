/*
 * lldp_shared.c
 *
 *  Created on: 2011-6-13
 *      Author: henri
 */

#include "ih_ipc.h"
#include "ih_cmd.h"
//#include "sw_ipc.h"

#include "lldp_ipc.h"
#include "lldp_shared.h"


LLDP_MIB_SHARED 	lldp_mib_shared;
LLDP_PORT_CONFIG_MIB	lldp_mib_port_config[MAX_LLDP_PORT];
LLDP_MIB_PORT_SHARED 	lldp_mib_port_shared[MAX_LLDP_PORT];
LOCAL_GLOBAL_DATA 	lldp_mib_localsys_global;
LOCAL_PORT_DATA 	lldp_mib_localsys_port[MAX_LLDP_PORT];
LOCAL_MAN_ADDR_DATA	lldp_mib_localsys_manAddr;	
LLDP_REMOTE_SYS_DATA_MIB	lldp_mib_remotesys[MAX_LLDP_NEIGHBOR];
LLDP_REMOTE_MAN_ADDR_DATA_MIB	lldp_remote_management_addr[MAX_LLDP_NEIGHBOR];
int globalLldpRemIndex = 0; 

int ipc_sync_lldp_mib_register(void)
{

	ih_ipc_subscribe(IPC_MSG_SYNC_LLDP_MIB);
	ih_ipc_subscribe(IPC_MSG_SYNC_LLDP_PORT_CONFIG_MIB);
	ih_ipc_subscribe(IPC_MSG_UPDATE_LLDP_MIB_PORT);

	ih_ipc_subscribe(IPC_MSG_SYNC_LLDP_LOCAL_SYS_GLOBAL_MIB);	
	ih_ipc_subscribe(IPC_MSG_SYNC_LLDP_LOCAL_SYS_PORT_MIB);
	ih_ipc_subscribe(IPC_MSG_SYNC_LLDP_LOCAL_SYS_MAN_ADDR);
	ih_ipc_subscribe(IPC_MSG_SYNC_LLDP_REMOTE_SYS);
	ih_ipc_subscribe(IPC_MSG_SYNC_LLDP_REMOTE_MAN_ADDR);
	ih_ipc_subscribe(IPC_MSG_DEL_LLDP_REMOTE_SYS);
	ih_ipc_subscribe(IPC_MSG_DEL_LLDP_STATS_GLOBAL);
	ih_ipc_subscribe(IPC_MSG_DEL_LOCALSYS_GLOBAL);
	ih_ipc_subscribe(IPC_MSG_DEL_LOCALSYS_PORT);

	ih_ipc_register_msg_handle(IPC_MSG_SYNC_LLDP_MIB, ipc_sync_lldp_mib_handle);
	ih_ipc_register_msg_handle(IPC_MSG_SYNC_LLDP_PORT_CONFIG_MIB, ipc_sync_lldp_port_config_mib_handle);
	ih_ipc_register_msg_handle(IPC_MSG_UPDATE_LLDP_MIB_PORT, ipc_update_lldp_mib_port_handle);
	
	ih_ipc_register_msg_handle(IPC_MSG_SYNC_LLDP_LOCAL_SYS_GLOBAL_MIB, ipc_sync_lldp_localsys_global_handle);	
	ih_ipc_register_msg_handle(IPC_MSG_SYNC_LLDP_LOCAL_SYS_PORT_MIB, ipc_sync_lldp_localsys_port_handle);
	ih_ipc_register_msg_handle(IPC_MSG_SYNC_LLDP_LOCAL_SYS_MAN_ADDR, ipc_sync_lldp_localsys_manAddr_handle);
	ih_ipc_register_msg_handle(IPC_MSG_SYNC_LLDP_REMOTE_SYS, ipc_sync_lldp_remotesys_handle);
	ih_ipc_register_msg_handle(IPC_MSG_SYNC_LLDP_REMOTE_MAN_ADDR, ipc_sync_lldp_remote_manaddr_handle);
	
	ih_ipc_register_msg_handle(IPC_MSG_DEL_LLDP_REMOTE_SYS, ipc_del_lldp_remotesys_handle);
	ih_ipc_register_msg_handle(IPC_MSG_DEL_LLDP_STATS_GLOBAL, ipc_del_lldp_stats_global_handle);

	ih_ipc_register_msg_handle(IPC_MSG_DEL_LOCALSYS_GLOBAL, ipc_del_lldp_localesys_global_handle);
	ih_ipc_register_msg_handle(IPC_MSG_DEL_LOCALSYS_PORT, ipc_del_lldp_localesys_port_handle);

	
	return IH_ERR_OK;
}

int ipc_sync_lldp_mib_unregister(void)
{

	ih_ipc_unsubscribe(IPC_MSG_SYNC_LLDP_MIB);
	ih_ipc_unsubscribe(IPC_MSG_SYNC_LLDP_PORT_CONFIG_MIB);
	ih_ipc_unsubscribe(IPC_MSG_UPDATE_LLDP_MIB_PORT);

	ih_ipc_unsubscribe(IPC_MSG_SYNC_LLDP_LOCAL_SYS_GLOBAL_MIB);	
	ih_ipc_unsubscribe(IPC_MSG_SYNC_LLDP_LOCAL_SYS_PORT_MIB);
	ih_ipc_unsubscribe(IPC_MSG_SYNC_LLDP_LOCAL_SYS_MAN_ADDR);
	ih_ipc_unsubscribe(IPC_MSG_SYNC_LLDP_REMOTE_SYS);
	ih_ipc_unsubscribe(IPC_MSG_SYNC_LLDP_REMOTE_MAN_ADDR);
	ih_ipc_unsubscribe(IPC_MSG_DEL_LLDP_REMOTE_SYS);
	ih_ipc_unsubscribe(IPC_MSG_DEL_LLDP_STATS_GLOBAL);
	
	ih_ipc_unsubscribe(IPC_MSG_DEL_LOCALSYS_GLOBAL);
	ih_ipc_unsubscribe(IPC_MSG_DEL_LOCALSYS_PORT);
	return IH_ERR_OK;
}

int ipc_send_lldp_mib_value( int value, unsigned char flag){
	
	IH_SVC_ID peer_svc_id = IH_SVC_LLDP;
	LLDP_MIB_VALUE lldp_mib_value;
	int retval;

	lldp_mib_value.value = value;
	lldp_mib_value.flag = flag;

	retval =ih_ipc_send(peer_svc_id, IPC_MSG_SET_LLDP_MIB, &lldp_mib_value, sizeof(LLDP_MIB_VALUE));

	if (retval != ERR_OK) {

		LOG_ER("Cannot send lldp mib value %d is error\n", lldp_mib_value.value);
		return ERR_FAILED;
	}
	return ERR_OK;

}

int ipc_send_lldp_port_config_mib_value(int port_id, int value, unsigned char flag){


	IH_SVC_ID peer_svc_id = IH_SVC_LLDP;
	LLDP_MIB_VALUE lldp_mib_value;
	int retval;

	lldp_mib_value.port_id = port_id;
	lldp_mib_value.value = value;
	lldp_mib_value.flag = flag;

	retval =ih_ipc_send(peer_svc_id, IPC_MSG_SET_LLDP_PORT_CONFIG_MIB, &lldp_mib_value, sizeof(LLDP_MIB_VALUE));

	if (retval != ERR_OK) {

		LOG_ER("Cannot send lldp port config mib value %d is error\n", lldp_mib_value.value);
		return ERR_FAILED;
	}
	return ERR_OK;
}

int ipc_send_lldp_mib_change_time(){
	
	time_t change_time;
	int rv;

	//change_time = lldp_mib_shared.lldpStatsRemTablesLastChangeTime;

	memcpy(&change_time, &lldp_mib_shared.lldpStatsRemTablesLastChangeTime, sizeof(time_t));

	LOG_DB("lldp notification send mib agent\n");

	rv = ih_ipc_publish(IPC_MSG_LLDP_CHANGE_TIME, &change_time, sizeof(time_t));
	if(rv) {

		LOG_ER("send lldp mib changed time error\n");
		return ERR_FAILED;
	
	} 
	
	//LOG_ER("changed time %d\n", change_time);

	return ERR_OK;

}

int32 ipc_sync_lldp_port_config_mib_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	LLDP_MIB_VALUE *lldp_para = NULL;
	int id;

	if(!msg_hdr || !msg){
		LOG_ER("lldp sync port msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	lldp_para  = (LLDP_MIB_VALUE *)msg;
	if(!lldp_para){
		LOG_ER("lldp_sync port para is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	LOG_DB("sync lldp port mib info is value:%d, flag:%d\n", lldp_para->value, lldp_para->flag );

	if (lldp_para->port_id <1 || lldp_para->port_id > MAX_LLDP_PORT ) {

		LOG_ER("sync lldp port mib info, port id %d is out of scope\n",lldp_para->port_id);
	}
	
	id = lldp_para->port_id -1;

	switch(lldp_para->flag) {

		case LLDP_SET_PORT:{

			if( lldp_para->value == LLDP_PORT_DISABLE) {

				lldp_mib_port_config[id].lldpPortConfigAdminStatus = LLDP_MIB_PORT_CONFIG_ADMIN_STATUS_DISABLE;	
	
			} else {

				lldp_mib_port_config[id].lldpPortConfigAdminStatus = (LLDP_PORT_ADMIN_TYPE)lldp_para->value;
			}
			
			break;
		}

		case LLDP_SET_NOTIFICATION:{

			if(lldp_para->value == LLDP_NOTIFICATION_DISABLE) {

				lldp_mib_port_config[id].lldpPortConfigNotificationEnable = LLDP_MIB_PORT_CONFIG_NOTIF_STATUS_DISABLE;

			} else {

				lldp_mib_port_config[id].lldpPortConfigNotificationEnable = lldp_para->value;
			}
			
			break;
		}

		default: {

			LOG_WA("lldp sync info error\n");
			break;
		}
	}
	return IH_ERR_OK;
}


int32 ipc_sync_lldp_mib_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	LLDP_MIB_VALUE *lldp_para = NULL;

	if(!msg_hdr || !msg){
		LOG_ER("lldp sync msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	lldp_para  = (LLDP_MIB_VALUE *)msg;
	if(!lldp_para){
		LOG_ER("lldp_sync para is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	LOG_DB("sync lldp mib info is value:%d, flag:%d\n", lldp_para->value, lldp_para->flag );
	
	switch(lldp_para->flag) {

		case LLDP_SET_REINIT:{

			lldp_mib_shared.reinitDelay = lldp_para->value;
			break;
		}
		case LLDP_SET_HOLDTIMER	:{

			lldp_mib_shared.msgTxHold = lldp_para->value;
			break;
		}
		case LLDP_SET_MSGINTERVAL:{

			lldp_mib_shared.msgTxInterval = lldp_para->value;
			break;
		}
		case LLDP_SET_TXDELAY:{

			lldp_mib_shared.txDelay = lldp_para->value;
			break;
		}
		case LLDP_SET_NOTIF_INTERVAL:{
			
			lldp_mib_shared.notificationInterval = lldp_para->value;
			break;
		}
		case LLDP_UPDATE_CHANGE_TIME:{

			if(lldp_mib_shared.lldpStatsRemTablesLastChangeTime != lldp_para->value) {

				lldp_mib_shared.lldpStatsRemTablesLastChangeTime = lldp_para->value;
				if(ipc_send_lldp_mib_change_time()!= ERR_OK) {
			
					LOG_ER("sync lldp changed time error\n");
				}
			}
			break;
		} 
		case LLDP_UPDATE_AGEOUT:{

			lldp_mib_shared.lldpStatsRemTablesAgeouts++;
			break;
		}
		case LLDP_UPDATE_INSERT:{

			lldp_mib_shared.lldpStatsRemTablesInserts++;
			break;
		}
		case LLDP_UPDATE_DELETE:{

			lldp_mib_shared.lldpStatsRemTablesDeletes++;
			break;
		}
		case LLDP_UPDATE_DROPS:{

			lldp_mib_shared.lldpStatsRemTablesDrops++;
			break;
		} 				

		default: {

			LOG_WA("lldp sync info error\n");
			break;
		}
	}
	return IH_ERR_OK;

}

int32 ipc_update_lldp_mib_port_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	LLDP_MIB_PORT *lldp_mib_port_para = NULL;
	int port_id;

	if(!msg_hdr || !msg){
		LOG_ER("lldp update mib msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	lldp_mib_port_para  = (LLDP_MIB_PORT *)msg;
	if(!lldp_mib_port_para){
		LOG_ER("lldp_update para is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if(lldp_mib_port_para->lldp_port_id < 1 || lldp_mib_port_para->lldp_port_id >LLDP_MAX_PORT) {

		LOG_ER("ipc update lldp mib error: lldp port%d\n",lldp_mib_port_para->lldp_port_id);
		return IH_ERR_FAILED;
	}
	
	port_id = lldp_mib_port_para->lldp_port_id -1;

	memset(&lldp_mib_port_shared[port_id], 0 , sizeof(LLDP_MIB_PORT_SHARED));

	lldp_mib_port_shared[port_id].lldp_mib_port_id 				= lldp_mib_port_para->lldp_port_id;
	
	lldp_mib_port_shared[port_id].lldpMibStatsRxPortAgeoutsTotal		= lldp_mib_port_para->lldpStatsRxPortAgeoutsTotal;
	//LOG_ER("port %d lldp shared ageout %d\n", lldp_mib_port_shared[port_id].lldp_mib_port_id ,lldp_mib_port_shared[port_id].lldpMibStatsRxPortAgeoutsTotal);

	lldp_mib_port_shared[port_id].lldpMibStatsRxPortFramesDiscardedTotal	= lldp_mib_port_para->lldpStatsRxPortFramesDiscardedTotal;
	//LOG_ER("port %d lldp shared frm drops %d\n", lldp_mib_port_shared[port_id].lldp_mib_port_id ,lldp_mib_port_shared[port_id].lldpMibStatsRxPortFramesDiscardedTotal);

	lldp_mib_port_shared[port_id].lldpMibStatsRxPortFramesErrors		= lldp_mib_port_para->lldpStatsRxPortFramesErrors;
	//LOG_ER("port %d lldp shared frm err %d\n", lldp_mib_port_shared[port_id].lldp_mib_port_id ,lldp_mib_port_shared[port_id].lldpMibStatsRxPortFramesErrors);

	lldp_mib_port_shared[port_id].lldpMibStatsRxPortFramesTotal		= lldp_mib_port_para->lldpStatsRxPortFramesTotal;
	//LOG_ER("port %d lldp shared frm IN %d\n", lldp_mib_port_shared[port_id].lldp_mib_port_id ,lldp_mib_port_shared[port_id].lldpMibStatsRxPortFramesTotal);

	lldp_mib_port_shared[port_id].lldpMibStatsRxPortTLVsDiscardedTotal	= lldp_mib_port_para->lldpStatsRxPortTLVsDiscardedTotal;
	//LOG_ER("port %d lldp shared TLVs drops %d\n", lldp_mib_port_shared[port_id].lldp_mib_port_id ,lldp_mib_port_shared[port_id].lldpMibStatsRxPortTLVsDiscardedTotal);

	lldp_mib_port_shared[port_id].lldpMibStatsRxPortTLVsUnrecognizedTotal 	= lldp_mib_port_para->lldpStatsRxPortTLVsUnrecognizedTotal;
	//LOG_ER("port %d lldp shared TLVs unknown %d\n", lldp_mib_port_shared[port_id].lldp_mib_port_id ,lldp_mib_port_shared[port_id].lldpMibStatsRxPortTLVsUnrecognizedTotal);
	//tx stats
	lldp_mib_port_shared[port_id].lldpMibStatsTxPortFramesTotal	 	= lldp_mib_port_para->lldpStatsTxPortFramesTotal;
	//LOG_ER("port %d lldp shared frm OUT %d\n", lldp_mib_port_shared[port_id].lldp_mib_port_id ,lldp_mib_port_shared[port_id].lldpMibStatsTxPortFramesTotal);

	//lldp_mib_port_shared[port_id].lldpMibLocPortIdSubtype 			= lldp_mib_port_para->lldpLocPortIdSubtype;
	//lldp_mib_port_shared[port_id].lldpMibLocPortId 				= lldp_mib_port_para->lldpLocPortId;

	//memset(lldp_mib_port_shared[port_id].lldpMibLocPortDesc, 0, LLDP_TLV_LENGTH);

	//memcpy(lldp_mib_port_shared[port_id].lldpMibLocPortDesc, lldp_mib_port_para->lldpLocPortDesc, LLDP_TLV_LENGTH);

	return IH_ERR_OK;	
}


int32 ipc_sync_lldp_localsys_global_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	LOCAL_GLOBAL_DATA *local_sys_global = NULL;

	if(!msg_hdr || !msg){
		LOG_ER("lldp local system global msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	local_sys_global  = (LOCAL_GLOBAL_DATA *)msg;

	if(!local_sys_global){

		LOG_ER("lldp_local system global data is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	lldp_mib_localsys_global.locChassisIdSubType = local_sys_global->locChassisIdSubType;
	
	memcpy(&lldp_mib_localsys_global.locChassisId[0], &local_sys_global->locChassisId[0], LLDP_MIB_MAX_LEN);
	memcpy(&lldp_mib_localsys_global.sysName[0], &local_sys_global->sysName[0], LLDP_MIB_MAX_LEN);
	memcpy(&lldp_mib_localsys_global.sysDesc[0], &local_sys_global->sysDesc[0], LLDP_MIB_MAX_LEN);

	lldp_mib_localsys_global.sysCapSupported = local_sys_global->sysCapSupported;
	lldp_mib_localsys_global.sysCapeEnabled = local_sys_global->sysCapeEnabled;
		
	return IH_ERR_OK;
}

int32 ipc_sync_lldp_localsys_port_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	LOCAL_PORT_DATA *local_sys_port = NULL;

	int port_id;

	if(!msg_hdr || !msg){
		LOG_ER("lldp local system port msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	local_sys_port  = (LOCAL_PORT_DATA *)msg;

	if(!local_sys_port){
		LOG_ER("lldp_local system port data is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if(local_sys_port->port_id < 1 || local_sys_port->port_id > LLDP_MAX_PORT) {

		LOG_ER("ipc lldp local system data mib error: lldp port%d\n", local_sys_port->port_id);
		return IH_ERR_FAILED;
	}
	
	port_id = local_sys_port->port_id -1;

	memset(&lldp_mib_localsys_port[port_id], 0 , sizeof(LOCAL_PORT_DATA));
	
	lldp_mib_localsys_port[port_id].port_id = local_sys_port->port_id;
	lldp_mib_localsys_port[port_id].locPortIdSubtype = local_sys_port->locPortIdSubtype;
	memcpy(&lldp_mib_localsys_port[port_id].locPortId[0], &local_sys_port->locPortId[0], LLDP_MIB_MAX_LEN);
	memcpy(&lldp_mib_localsys_port[port_id].locPortDesc[0], &local_sys_port->locPortDesc[0], LLDP_MIB_MAX_LEN);

	return IH_ERR_OK;
}

int32 ipc_sync_lldp_localsys_manAddr_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	LOCAL_MAN_ADDR_DATA *local_sys_manAddr = NULL;

	if(!msg_hdr || !msg){
		LOG_ER("lldp local system manAddr msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	local_sys_manAddr  = (LOCAL_MAN_ADDR_DATA *)msg;

	if(!local_sys_manAddr){

		LOG_ER("lldp_local system manAddr data is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	lldp_mib_localsys_manAddr.localManAddrType.mamAddrType = local_sys_manAddr->localManAddrType.mamAddrType;
	lldp_mib_localsys_manAddr.localManAddrType.len = local_sys_manAddr->localManAddrType.len;
	memcpy(&lldp_mib_localsys_manAddr.localManAddrType.ipaddr[0], &local_sys_manAddr->localManAddrType.ipaddr[0], IP_ADDR_LEN);

	lldp_mib_localsys_manAddr.localManAddrLen = local_sys_manAddr->localManAddrLen;
	lldp_mib_localsys_manAddr.ifType = local_sys_manAddr->ifType;
	lldp_mib_localsys_manAddr.ifID = local_sys_manAddr->ifID;
	lldp_mib_localsys_manAddr.oid = local_sys_manAddr->oid;

	return IH_ERR_OK;
}


int32 ipc_del_lldp_localesys_global_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	if(!msg_hdr){

		LOG_ER("lldp del local system msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	lldp_mib_localsys_global.locChassisIdSubType = 0;
	memset(&lldp_mib_localsys_global.locChassisId[0],0, LLDP_MIB_MAX_LEN);
	memset(&lldp_mib_localsys_global.sysName[0], 0, LLDP_MIB_MAX_LEN);
	memset(&lldp_mib_localsys_global.sysDesc[0],0, LLDP_MIB_MAX_LEN);
	lldp_mib_localsys_global.sysCapSupported = 0;
	lldp_mib_localsys_global.sysCapeEnabled = 0;

	lldp_mib_localsys_manAddr.localManAddrType.mamAddrType = 0;
	lldp_mib_localsys_manAddr.localManAddrType.len = 0;
	memset(&lldp_mib_localsys_manAddr.localManAddrType.ipaddr[0], 0, IP_ADDR_LEN);

	lldp_mib_localsys_manAddr.localManAddrLen = 0;
	lldp_mib_localsys_manAddr.ifType = 0;
	lldp_mib_localsys_manAddr.ifID = 0;
	lldp_mib_localsys_manAddr.oid = 0;
		
	return IH_ERR_OK;
}

int32 ipc_del_lldp_localesys_port_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	LOCAL_PORT_DATA *local_sys_port = NULL;

	int port_id;

	if(!msg_hdr || !msg){
		LOG_ER("lldp local system port msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	local_sys_port  = (LOCAL_PORT_DATA *)msg;

	if(!local_sys_port){
		LOG_ER("lldp_local system port data is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if(local_sys_port->port_id < 1 || local_sys_port->port_id > LLDP_MAX_PORT) {

		LOG_ER("ipc lldp local system data mib error: lldp port%d\n", local_sys_port->port_id);
		return IH_ERR_FAILED;
	}
	
	port_id = local_sys_port->port_id -1;

	memset(&lldp_mib_localsys_port[port_id], 0 , sizeof(LOCAL_PORT_DATA));

	return IH_ERR_OK;

}


int32 ipc_del_lldp_remotesys_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	LLDP_REMOTE_SYS_DATA_MIB *lldp_remote_data = NULL;
	int id;

	if(!msg_hdr || !msg){
		LOG_ER("lldp del remote system msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	lldp_remote_data  = (LLDP_REMOTE_SYS_DATA_MIB *)msg;

	if(!lldp_remote_data){

		LOG_ER("lldp_del_remote system data is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if( lldp_remote_data->lldpRemLocalPortNum <1 || lldp_remote_data->lldpRemLocalPortNum > MAX_LLDP_NEIGHBOR) {

		LOG_ER("lldp_del_remote system port %d is out of scope\n",lldp_remote_data->lldpRemLocalPortNum);
		return IH_ERR_FAILED;
	}
	
	id = lldp_remote_data->lldpRemLocalPortNum -1;
	
	memset(&lldp_mib_remotesys[id], 0, sizeof(LLDP_REMOTE_SYS_DATA_MIB));
	
	memset(&lldp_remote_management_addr[id], 0, sizeof(LLDP_REMOTE_MAN_ADDR_DATA_MIB));

	return IH_ERR_OK;

}

int32 ipc_del_lldp_stats_global_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){


	if(!msg_hdr || !msg){
		LOG_ER("lldp del remote system msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	lldp_mib_shared.lldpStatsRemTablesAgeouts = 0;

	lldp_mib_shared.lldpStatsRemTablesInserts= 0;

	lldp_mib_shared.lldpStatsRemTablesDeletes= 0;

	lldp_mib_shared.lldpStatsRemTablesDrops= 0;

	return IH_ERR_OK;

}

int32 ipc_sync_lldp_remotesys_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	LLDP_REMOTE_SYS_DATA_MIB *lldp_remote_data = NULL;
	int id;

	if(!msg_hdr || !msg){
		LOG_ER("lldp remote system msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	lldp_remote_data  = (LLDP_REMOTE_SYS_DATA_MIB *)msg;

	if(!lldp_remote_data){

		LOG_ER("lldp_remote system data is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if( lldp_remote_data->lldpRemLocalPortNum <1 || lldp_remote_data->lldpRemLocalPortNum > MAX_LLDP_NEIGHBOR) {

		LOG_ER("lldp_remote system port %d is out of scope\n",lldp_remote_data->lldpRemLocalPortNum);
		return IH_ERR_FAILED;
	}

	id = lldp_remote_data->lldpRemLocalPortNum -1;
	globalLldpRemIndex ++;

	memset(&lldp_mib_remotesys[id], 0, sizeof(LLDP_REMOTE_SYS_DATA_MIB));
	
	lldp_mib_remotesys[id].lldpRemTimeMark = lldp_remote_data->lldpRemTimeMark;
	lldp_mib_remotesys[id].lldpRemLocalPortNum = lldp_remote_data->lldpRemLocalPortNum;
	lldp_mib_remotesys[id].lldpRemIndex = globalLldpRemIndex;
	lldp_mib_remotesys[id].lldpRemChassisIdSubtype= lldp_remote_data->lldpRemChassisIdSubtype;
	memcpy(&lldp_mib_remotesys[id].lldpRemChassisId[0], &lldp_remote_data->lldpRemChassisId[0], LLDP_MIB_MAX_LEN);
	lldp_mib_remotesys[id].lldpRemPortIdSubtype = lldp_remote_data->lldpRemPortIdSubtype;
	memcpy(&lldp_mib_remotesys[id].lldpRemPortId[0],&lldp_remote_data->lldpRemPortId[0], LLDP_MIB_MAX_LEN);
	memcpy(&lldp_mib_remotesys[id].lldpRemPortDesc[0],&lldp_remote_data->lldpRemPortDesc[0],LLDP_MIB_MAX_LEN);
	memcpy(&lldp_mib_remotesys[id].lldpRemSysName[0],&lldp_remote_data->lldpRemSysName[0],LLDP_MIB_MAX_LEN);
	memcpy(&lldp_mib_remotesys[id].lldpRemSysDesc[0],&lldp_remote_data->lldpRemSysDesc[0],LLDP_MIB_MAX_LEN);
	lldp_mib_remotesys[id].lldpRemSysCapSupported = lldp_remote_data->lldpRemSysCapSupported;
	lldp_mib_remotesys[id].lldpRemSysCapEnabled = lldp_remote_data->lldpRemSysCapEnabled;

	return IH_ERR_OK;
	
}

int32 ipc_sync_lldp_remote_manaddr_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	LLDP_REMOTE_MAN_ADDR_DATA_MIB *lldp_remote_manaddr = NULL;
	int id;

	if(!msg_hdr || !msg){
		LOG_ER("lldp remote manaddr msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	lldp_remote_manaddr  = (LLDP_REMOTE_MAN_ADDR_DATA_MIB *)msg;

	if(!lldp_remote_manaddr){

		LOG_ER("lldp_remote system manaddr is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if( lldp_remote_manaddr->remoteManAddrType.port_id <1 || lldp_remote_manaddr->remoteManAddrType.port_id > MAX_LLDP_NEIGHBOR) {

		LOG_ER("lldp_remote system manaddr port %d is out of scope\n",lldp_remote_manaddr->remoteManAddrType.port_id);
		return IH_ERR_FAILED;
	}

	id = lldp_remote_manaddr->remoteManAddrType.port_id -1;

	memset(&lldp_remote_management_addr[id], 0, sizeof(LLDP_REMOTE_MAN_ADDR_DATA_MIB));

	lldp_remote_management_addr[id].remoteManAddrType.mamAddrType =  lldp_remote_manaddr->remoteManAddrType.mamAddrType;

	//lldp_remote_management_addr[id].remoteManAddrType.len =  lldp_remote_manaddr->remoteManAddrType.len;
	lldp_remote_management_addr[id].remoteManAddrType.len = IP_ADDR_LEN;
	
	lldp_remote_management_addr[id].remoteManAddrType.port_id =  lldp_remote_manaddr->remoteManAddrType.port_id;

	memcpy(&lldp_remote_management_addr[id].remoteManAddrType.ipaddr[0], &lldp_remote_manaddr->remoteManAddrType.ipaddr[0], IP_ADDR_LEN);

	lldp_remote_management_addr[id].ifType = lldp_remote_manaddr->ifType;

	lldp_remote_management_addr[id].ifID = lldp_remote_manaddr->ifID;
	
	memcpy(&lldp_remote_management_addr[id].oid[0], &lldp_remote_manaddr->oid[0], MAN_ADDR_OID_LENGTH);

	return IH_ERR_OK;
	
}

int set_lldpPortConfigAdminStatus(int port_id, LLDP_PORT_ADMIN_TYPE lldp_port_admin_status) {

	return ipc_send_lldp_port_config_mib_value(port_id, lldp_port_admin_status, LLDP_SET_PORT);
}

int set_lldpPortConfigNotificationEnable(int port_id, BOOL lldp_port_notification_status) {

	return ipc_send_lldp_port_config_mib_value(port_id, lldp_port_notification_status, LLDP_SET_NOTIFICATION);
}


void get_lldpPortConfigAdminStatus(int port_id, LLDP_PORT_ADMIN_TYPE *lldp_port_admin_status){

	int id ;

	if(port_id <1 || port_id > MAX_LLDP_PORT){
		
		LOG_ER("get lldp port config admin status port id %d is out of scope\n",port_id);
	}
	
	id = port_id -1;

	*lldp_port_admin_status = lldp_mib_port_config[id].lldpPortConfigAdminStatus;
}

void get_lldpPortConfigNotificationEnable(int port_id, BOOL *lldp_port_notification_status){

	int id ;

	if(port_id <1 || port_id > MAX_LLDP_PORT){
		
		LOG_ER("get lldp port config admin notification port id %d is out of scope\n",port_id);
	}
	
	id = port_id -1;

	*lldp_port_notification_status = lldp_mib_port_config[id].lldpPortConfigNotificationEnable;
}

void get_lldpPortConfigTLVsTxEnable(int port_id, unsigned char *lldp_port_notification_status){

	int id ;

	if(port_id <1 || port_id > MAX_LLDP_PORT){
		
		LOG_ER("get lldp port config admin tlv enable port id %d is out of scope\n",port_id);
	}
	
	id = port_id -1;

	//*lldp_port_notification_status = PORT_DESC_TLV_ENABLE | SYS_NAME_TLV_ENABLE | SYS_CAP_TLV_ENABLE | SYS_DESC_TLV_ENABLE;
	//for SNMPB 
	*lldp_port_notification_status = LLDP_ALL_TLV;
}

void get_lldpConfigManAddrPortsTxEnable(LOCAL_MAN_ADDR_TYPE localManAddrType, unsigned char *lldpConfigManAddrPortsTxEnable) {

	unsigned char mibLldpConfigManAddrPortsTxEnable[LLDP_MIB_MAX_PORT_BITMAP];

	memset(&mibLldpConfigManAddrPortsTxEnable[0], 0xff, LLDP_MIB_MAX_PORT_BITMAP);

	memcpy(lldpConfigManAddrPortsTxEnable, &mibLldpConfigManAddrPortsTxEnable[0], LLDP_MIB_MAX_PORT_BITMAP);
	
}


int set_lldp_txdelay(unsigned short txDelay){

	return ipc_send_lldp_mib_value(txDelay, LLDP_SET_TXDELAY);

}


void get_lldp_txdelay(unsigned short *txDelay) {

	*txDelay = lldp_mib_shared.txDelay;
}


int set_lldp_reinitDelay(unsigned short reinitDelay){

	return ipc_send_lldp_mib_value(reinitDelay, LLDP_SET_REINIT);

}


void get_lldp_reinitDelay(unsigned short *reinitDelay){

	*reinitDelay = lldp_mib_shared.reinitDelay;

}


int set_lldp_msgTxHold(unsigned short msgTxHold){

	return ipc_send_lldp_mib_value(msgTxHold, LLDP_SET_HOLDTIMER);

}


void get_lldp_msgTxHold(unsigned short *msgTxHold) {

	*msgTxHold = lldp_mib_shared.msgTxHold;
}


int set_lldp_msgTxInterval(unsigned short msgTxInterval){

	return ipc_send_lldp_mib_value(msgTxInterval, LLDP_SET_MSGINTERVAL);

}

void get_lldp_msgTxInterval(unsigned short *msgTxInterval){

	*msgTxInterval = lldp_mib_shared.msgTxInterval;
}

int set_lldpNotificationInterval(unsigned int notificationInterval){

	return ipc_send_lldp_mib_value(notificationInterval, LLDP_SET_NOTIF_INTERVAL);
}

void get_lldpNotificationInterval(unsigned int *notificationInterval){

	*notificationInterval = lldp_mib_shared.notificationInterval;
}


void get_lldpStatsRemTablesInserts(unsigned int *lldpStatsRemTablesInserts){

	*lldpStatsRemTablesInserts = lldp_mib_shared.lldpStatsRemTablesInserts;
}

void get_lldpStatsRemTablesDeletes(unsigned int *lldpStatsRemTablesDeletes){

	*lldpStatsRemTablesDeletes = lldp_mib_shared.lldpStatsRemTablesDeletes;
}

void get_lldpStatsRemTablesDrops(unsigned int *lldpStatsRemTablesDrops){

	*lldpStatsRemTablesDrops = lldp_mib_shared.lldpStatsRemTablesDrops;
}

void get_lldpStatsRemTablesAgeouts(unsigned int *lldpStatsRemTablesAgeouts){

	*lldpStatsRemTablesAgeouts = lldp_mib_shared.lldpStatsRemTablesAgeouts;
}

void get_lldpStatsRemTablesLastChangeTime(time_t *lldpStatsRemTablesLastChangeTime){


	*lldpStatsRemTablesLastChangeTime = lldp_mib_shared.lldpStatsRemTablesLastChangeTime;
	//LOG_ER("change timer is %d\n", *lldpStatsRemTablesLastChangeTime);

}


void get_lldpMibStatsRxPortAgeoutsTotal(int port_id, unsigned int *lldpMibStatsRxPortAgeoutsTotal){

	int i;

	int id = port_id-1;

	for( i = 0; i<MAX_LLDP_PORT; i++){

		if(i == id){

			*lldpMibStatsRxPortAgeoutsTotal = (unsigned int)lldp_mib_port_shared[i].lldpMibStatsRxPortAgeoutsTotal;
			return;
		} 
	
	}
	
	LOG_ER("GET LLDP MIB:lldpMibStatsRxPortAgeoutsTotal error\n");	
}


void get_lldpMibStatsRxPortFramesDiscardedTotal(int port_id, unsigned int *lldpMibStatsRxPortFramesDiscardedTotal){

	int i;

	int id = port_id-1;

	for( i = 0; i<MAX_LLDP_PORT; i++){

		if(i == id){

			*lldpMibStatsRxPortFramesDiscardedTotal = (unsigned int)lldp_mib_port_shared[i].lldpMibStatsRxPortFramesDiscardedTotal;
			return;
		} 
	
	}
	
	LOG_ER("GET LLDP MIB:lldpMibStatsRxPortFramesDiscardedTotal error\n");	

}

void get_lldpMibStatsRxPortFramesErrors(int port_id, unsigned int *lldpMibStatsRxPortFramesErrors){


	int i;

	int id = port_id-1;

	for( i = 0; i<MAX_LLDP_PORT; i++){

		if(i == id){

			*lldpMibStatsRxPortFramesErrors = (unsigned int)lldp_mib_port_shared[i].lldpMibStatsRxPortFramesErrors;
			return;
		} 
	
	}
	
	LOG_ER("GET LLDP MIB:lldpMibStatsRxPortFramesErrors error\n");	

}


void get_lldpMibStatsRxPortFramesTotal(int port_id, unsigned int *lldpMibStatsRxPortFramesTotal){

	int i;
	
	int id = port_id-1;

	for( i = 0; i<MAX_LLDP_PORT; i++){

		if(i == id){

			*lldpMibStatsRxPortFramesTotal = (unsigned int)lldp_mib_port_shared[i].lldpMibStatsRxPortFramesTotal;
			//LOG_ER("lldp frame in, port %d, %d, %d\n", port_id, *lldpMibStatsRxPortFramesTotal,(unsigned int)lldp_mib_port_shared[i].lldpMibStatsRxPortFramesTotal );
			return;
		} 
	
	}
	
	LOG_ER("GET LLDP MIB:lldpMibStatsRxPortFramesTotal error\n");	
}


void get_lldpMibStatsRxPortTLVsDiscardedTotal(int port_id, unsigned int *lldpMibStatsRxPortTLVsDiscardedTotal){


	int i;

	int id = port_id-1;

	for( i = 0; i<MAX_LLDP_PORT; i++){

		if(i == id){

			*lldpMibStatsRxPortTLVsDiscardedTotal = (unsigned int)lldp_mib_port_shared[i].lldpMibStatsRxPortTLVsDiscardedTotal;
			return;
		} 
	
	}
	
	LOG_ER("GET LLDP MIB:lldpMibStatsRxPortTLVsDiscardedTotal error\n");	

}


void get_lldpMibStatsRxPortTLVsUnrecognizedTotal(int port_id, unsigned int *lldpMibStatsRxPortTLVsUnrecognizedTotal){


	int i;

	int id = port_id-1;

	for( i = 0; i<MAX_LLDP_PORT; i++){

		if(i == id){

			*lldpMibStatsRxPortTLVsUnrecognizedTotal = (unsigned int)lldp_mib_port_shared[i].lldpMibStatsRxPortTLVsUnrecognizedTotal;
			return;
		} 
	
	}
	
	LOG_ER("GET LLDP MIB:lldpMibStatsRxPortTLVsUnrecognizedTotal error\n");	

}


void get_lldpMibStatsTxPortFramesTotal(int port_id, unsigned int *lldpMibStatsTxPortFramesTotal){

	int i;

	int id = port_id-1;

	for( i = 0; i<MAX_LLDP_PORT; i++){

		if(i == id){

			*lldpMibStatsTxPortFramesTotal = (unsigned int)lldp_mib_port_shared[i].lldpMibStatsTxPortFramesTotal;
			//LOG_ER("lldp frame out, port %d, %d, %d\n", port_id, *lldpMibStatsTxPortFramesTotal,  (unsigned int)lldp_mib_port_shared[i].lldpMibStatsTxPortFramesTotal);
			return;
		} 
	
	}
	
	LOG_ER("GET LLDP MIB:lldpMibStatsTxPortFramesTotal error\n");	

}

//Local system data mib
void get_lldpLocChassisIdSubtype(unsigned char *locChassisIdSubtype){

	*locChassisIdSubtype = lldp_mib_localsys_global.locChassisIdSubType; 
}

void get_lldpLocChassisId(unsigned char *locChassisId){

	memcpy(locChassisId, &lldp_mib_localsys_global.locChassisId[0], LLDP_MIB_MAX_LEN);
}

void get_lldpLocSysName(unsigned char *locSysName){

	memcpy(locSysName, &lldp_mib_localsys_global.sysName[0], LLDP_MIB_MAX_LEN);
}

void get_lldpLocSysDesc(unsigned char *locSysDesc){

	memcpy(locSysDesc, &lldp_mib_localsys_global.sysDesc[0], LLDP_MIB_MAX_LEN);
}

void get_lldpLocSysCapSupported(unsigned char *locSysCapSupported){

	unsigned char capability = 0;
	
	switch(lldp_mib_localsys_global.sysCapSupported)
	{
		case LLDP_SYSTEM_CAPABILITY_OTHER:{

			capability = OTHER;
			break;
		}

		case LLDP_SYSTEM_CAPABILITY_REPEATER:{

			capability = REPEATER;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_BRIDGE:{

			capability = BRIDGE;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_WLAN:{

			capability = WLAN;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_ROUTER:{

			capability = ROUTER;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_TELEPHONE:{

			capability = TELEPHONE;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_DOCSIS:{

			capability = DOCSIS;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_STATION:{

			capability = STATION;
			break;
		}
		default:{

			capability = 0;
			LOG_ER("unknown capacity\n");
			break;
		}
	};
	*locSysCapSupported = capability;
}

void get_lldpLocSysCapEnabled(unsigned char *locSysCapEnabled){

	unsigned char capability = 0;
	
	switch(lldp_mib_localsys_global.sysCapeEnabled)
	{
		case LLDP_SYSTEM_CAPABILITY_OTHER:{

			capability = OTHER;
			break;
		}

		case LLDP_SYSTEM_CAPABILITY_REPEATER:{

			capability = REPEATER;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_BRIDGE:{

			capability = BRIDGE;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_WLAN:{

			capability = WLAN;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_ROUTER:{

			capability = ROUTER;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_TELEPHONE:{

			capability = TELEPHONE;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_DOCSIS:{

			capability = DOCSIS;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_STATION:{

			capability = STATION;
			break;
		}
		default:{

			capability = 0;
			LOG_ER("unknown capacity\n");
			break;
		}
	};
	*locSysCapEnabled = capability;
}

void get_lldpLocPortIdSubtype(int port_index, unsigned char *locPortIdSubtype) {

	int id;

	if(port_index < 1 || port_index >LLDP_MAX_PORT){

		LOG_ER("lldp local system data port subtype %d is out of scope\n", port_index);
		return;
	}

	id = port_index -1;
	
	if( port_index == lldp_mib_localsys_port[id].port_id ){
	
		*locPortIdSubtype = lldp_mib_localsys_port[id].locPortIdSubtype;

	} else {

		LOG_ER("lldp local system data subtype port %d is not correspendant, %d \n", port_index, lldp_mib_localsys_port[id].port_id);
	}
}

void get_lldpLocPortId(int port_index, unsigned char *locPortId) {

	int id;

	if(port_index < 1 || port_index >LLDP_MAX_PORT){

		LOG_ER("lldp local system data portID %d is out of scope\n", port_index);
		return;
	}

	id = port_index -1;
	
	if( port_index == lldp_mib_localsys_port[id].port_id ){
	
		memcpy(locPortId, &lldp_mib_localsys_port[id].locPortId[0],LLDP_MIB_MAX_LEN);

	} else {

		LOG_ER("lldp local system data portID %d is not correspendant, %d \n", port_index, lldp_mib_localsys_port[id].port_id);
	}

	//LOG_ER("lldp shared port id %s\n",locPortId);
}

void get_lldpLocPortDesc(int port_index, unsigned char *locPortDesc) {

	int id;

	if(port_index < 1 || port_index >LLDP_MAX_PORT){

		LOG_ER("lldp local system data portDesc %d is out of scope\n", port_index);
		return;
	}

	id = port_index -1;
	
	if( port_index == lldp_mib_localsys_port[id].port_id ){
	
		memcpy(locPortDesc, &lldp_mib_localsys_port[id].locPortDesc[0],LLDP_MIB_MAX_LEN);

	} else {

		LOG_ER("lldp local system data portDesc %d is not correspendant, %d \n", port_index, lldp_mib_localsys_port[id].port_id);
	}

	//LOG_ER("lldp shared port desc %s\n",locPortDesc);
}

void get_lldpLocManAddrIfSubtype(LOCAL_MAN_ADDR_TYPE localManAddrType, unsigned char *locManAddrIfSubtype ){

	if(!cmp_lldpLocManAddr(localManAddrType, lldp_mib_localsys_manAddr.localManAddrType )){

		*locManAddrIfSubtype = lldp_mib_localsys_manAddr.ifType;
		LOG_ER("get lldp manAddr ifsubtype %d \n", *locManAddrIfSubtype);

	} else {

		LOG_ER("get lldp local manaddr iftype error\n");
	}
}

void get_lldpLocManAddrLen(LOCAL_MAN_ADDR_TYPE localManAddrType, unsigned int *locManAddrLen ){

	if(!cmp_lldpLocManAddr(localManAddrType, lldp_mib_localsys_manAddr.localManAddrType)){

		*locManAddrLen = lldp_mib_localsys_manAddr.localManAddrLen ;

	} else {

		LOG_ER("get lldp local manaddr len error\n");
	}
}

void get_lldpLocManAddrIfId(LOCAL_MAN_ADDR_TYPE localManAddrType, unsigned char *locManAddrIfId ){

	if(!cmp_lldpLocManAddr(localManAddrType, lldp_mib_localsys_manAddr.localManAddrType )){

		*locManAddrIfId = lldp_mib_localsys_manAddr.ifID;

	} else {

		LOG_ER("get lldp local manaddr ifID error\n");
	}
}

void get_lldpLocManAddrOID(LOCAL_MAN_ADDR_TYPE localManAddrType, unsigned char *locManAddrOID ){

	if(!cmp_lldpLocManAddr(localManAddrType, lldp_mib_localsys_manAddr.localManAddrType )){

		*locManAddrOID = lldp_mib_localsys_manAddr.oid;

	} else {

		LOG_ER("get lldp local manaddr oid error\n");
	}
}

int cmp_lldpLocManAddr(LOCAL_MAN_ADDR_TYPE localManAddrType, LOCAL_MAN_ADDR_TYPE lldp_shared_manAddr){

	if( (localManAddrType.mamAddrType == lldp_shared_manAddr.mamAddrType) &&
	    (localManAddrType.len == lldp_shared_manAddr.len) &&
	    (strncmp (&localManAddrType.ipaddr[0],&lldp_shared_manAddr.ipaddr[0],IP_ADDR_LEN) ==0)
	){
		LOG_ER("get manaddr head is same\n");	
		return 0;

	} else {
		LOG_ER("get manaddr head is different\n");
		return -1;
	}

}

void get_lldpRemTimeMark(int port_index, unsigned long *lldpRemTimeMark){

	int id;

	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote time mark port id %d is out of scope\n", port_index);
	}

	id = port_index -1;
	 *lldpRemTimeMark = lldp_mib_remotesys[id].lldpRemTimeMark;
	
}


void get_lldpRemIndex(int port_index, int *lldpRemIndex){

	int id;

	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote inedx port id %d is out of scope\n", port_index);
	}

	id = port_index -1;
	 *lldpRemIndex = lldp_mib_remotesys[id].lldpRemIndex;
	
}

void get_lldpRemChassisIdSubtype(int port_index, unsigned char *remChassisIdSubtype){

	int id;

	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remChassisIdSubtype port id %d is out of scope\n", port_index);
	}

	id = port_index -1;
	 *remChassisIdSubtype = lldp_mib_remotesys[id].lldpRemChassisIdSubtype;
	
}

void get_lldpRemChassisId(int port_index, unsigned char *remChassisId){

	int id;

	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remChassisId port id %d is out of scope\n", port_index);
	}

	id = port_index -1;
	memcpy(remChassisId, &lldp_mib_remotesys[id].lldpRemChassisId[0], LLDP_MIB_MAX_LEN);
}

void get_lldpRemPortIdSubtype(int port_index, unsigned char *remPortIdSubtype){

	int id;

	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remPortIdSubtype port id %d is out of scope\n", port_index);
	}

	id = port_index -1;
	 *remPortIdSubtype = lldp_mib_remotesys[id].lldpRemPortIdSubtype;

}

void get_lldpRemPortId(int port_index, unsigned char *remPortId){

	int id;

	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remPortId port id %d is out of scope\n", port_index);
	}

	id = port_index -1;
	memcpy(remPortId, &lldp_mib_remotesys[id].lldpRemPortId[0], LLDP_MIB_MAX_LEN);

}

void get_lldpRemPortDesc(int port_index, unsigned char *remPortDesc) {

	int id;

	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remPortDesc port id %d is out of scope\n", port_index);
	}

	id = port_index -1;
	memcpy(remPortDesc, &lldp_mib_remotesys[id].lldpRemPortDesc[0], LLDP_MIB_MAX_LEN);
}


void get_lldpRemSysName(int port_index, unsigned char *remSysName){

	int id;

	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remSysName port id %d is out of scope\n", port_index);
	}

	id = port_index -1;
	memcpy(remSysName, &lldp_mib_remotesys[id].lldpRemSysName[0], LLDP_MIB_MAX_LEN);
}


void get_lldpRemSysDesc(int port_index, unsigned char *remSysDesc){

	int id;

	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remSysDesc port id %d is out of scope\n", port_index);
	}

	id = port_index -1;
	 
	memcpy(remSysDesc, &lldp_mib_remotesys[id].lldpRemSysDesc[0], LLDP_MIB_MAX_LEN);

}


void get_lldpRemSysCapEnabled(int port_index, unsigned char *remSysCapEnabled){

	int id;
	unsigned char capability = 0;
	
	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remSysCapEnabled port id %d is out of scope\n", port_index);
	}

	id = port_index -1;

	
	switch(lldp_mib_remotesys[id].lldpRemSysCapEnabled)
	{
		case LLDP_SYSTEM_CAPABILITY_OTHER:{

			capability = OTHER;
			break;
		}

		case LLDP_SYSTEM_CAPABILITY_REPEATER:{

			capability = REPEATER;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_BRIDGE:{

			capability = BRIDGE;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_WLAN:{

			capability = WLAN;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_ROUTER:{

			capability = ROUTER;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_TELEPHONE:{

			capability = TELEPHONE;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_DOCSIS:{

			capability = DOCSIS;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_STATION:{

			capability = STATION;
			break;
		}
		default:{

			capability = 0;
			LOG_ER("unknown capacity\n");
			break;
		}
	};

	 *remSysCapEnabled = capability;

}

void get_lldpRemSysCapSupported(int port_index, unsigned char *remSysCapSupported){

	int id;
	unsigned char capability = 0;
	
	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remSysCapSupported port id %d is out of scope\n", port_index);
	}

	id = port_index -1;

	switch(lldp_mib_remotesys[id].lldpRemSysCapSupported)
	{
		case LLDP_SYSTEM_CAPABILITY_OTHER:{

			capability = OTHER;
			break;
		}

		case LLDP_SYSTEM_CAPABILITY_REPEATER:{

			capability = REPEATER;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_BRIDGE:{

			capability = BRIDGE;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_WLAN:{

			capability = WLAN;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_ROUTER:{

			capability = ROUTER;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_TELEPHONE:{

			capability = TELEPHONE;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_DOCSIS:{

			capability = DOCSIS;
			break;
		}
		case LLDP_SYSTEM_CAPABILITY_STATION:{

			capability = STATION;
			break;
		}
		default:{

			capability = 0;
			LOG_ER("unknown capacity\n");
			break;
		}
	};

	 *remSysCapSupported = capability;

}


void get_lldpRemManAddrIfType(int port_index, unsigned char *remManAddrIfType){

	int id;
	
	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remManAddrIfType port id %d is out of scope\n", port_index);
	}

	id = port_index -1;
	
	*remManAddrIfType = lldp_remote_management_addr[id].ifType;	
}

void get_lldpRemManAddrIfID(int port_index, unsigned int *remManAddrIfID){

	int id;
	
	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remManAddrIfID port id %d is out of scope\n", port_index);
	}

	id = port_index -1;

	*remManAddrIfID = lldp_remote_management_addr[id].ifID;
	
}

void get_lldpRemManAddrOid(int port_index, unsigned char *remManAddrOid){

	int id;
	
	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remManAddrOid port id %d is out of scope\n", port_index);
	}

	id = port_index -1;

	memcpy(remManAddrOid, &lldp_remote_management_addr[id].oid[0], MAN_ADDR_OID_LENGTH);

}


void get_lldpRemManAddrType(int port_index, REMOTE_MAN_ADDR_TYPE *remManAddrType){

	int id;
	
	if(port_index < 1 || port_index > MAX_LLDP_PORT){

		LOG_ER("lldp remote remManAddrType port id %d is out of scope\n", port_index);
	}

	id = port_index -1;

	remManAddrType->mamAddrType = lldp_remote_management_addr[id].remoteManAddrType.mamAddrType;
	remManAddrType->len = lldp_remote_management_addr[id].remoteManAddrType.len;
	memcpy(&remManAddrType->ipaddr[0], &lldp_remote_management_addr[id].remoteManAddrType.ipaddr[0],IP_ADDR_LEN);

}
