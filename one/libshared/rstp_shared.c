/*
 * rstp_shared.c
 *
 *  Created on: 2011-5-18
 *      Author: henri
 */

#include "if_common.h"
#include "rstpd_ipc.h"
#include "rstp_shared.h"


RSTP_SHARED gl_rstp_shard[PORT_NUMBER];



static int32 sync_rstp_edge_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen) {

	RSTP_PORT_EDGE *rstp_port_edge = NULL;
	int port_count;	
	int i;

	port_count = get_switch_port_count()-1;


	if(!msg_hdr || !msg){
		LOG_ER("rstp edge msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	LOG_DB("sync rstp edge msg from: %d, len:%d", msg_hdr->svc_id, msg_len);
	rstp_port_edge = (RSTP_PORT_EDGE *)msg;
	if(!rstp_port_edge){
		LOG_ER("rstp_port_edge is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if ((rstp_port_edge->port_index > port_count) || (rstp_port_edge->port_index < 1) ){
		
		LOG_ER("rstp_port_edge port_index is out of scope\n");
		return IH_ERR_FAILED;
	}

	i = rstp_port_edge->port_index -1;
	
	gl_rstp_shard[i].port_index = rstp_port_edge->port_index;
	gl_rstp_shard[i].port_edge_state = rstp_port_edge->port_edge_state;	

	LOG_DB("port%d edge is %d\n",gl_rstp_shard[i].port_index, gl_rstp_shard[i].port_edge_state);
	return IH_ERR_OK;	

}




static int32 sync_rstp_enable_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen) {

	RSTP_PORT_ENABLE *rstp_port_enable = NULL;
	int port_count;	
	int i;

	port_count = get_switch_port_count()-1;


	if(!msg_hdr || !msg){
		LOG_ER("rstp port state msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	LOG_DB("sync rstp port state msg from: %d, len:%d", msg_hdr->svc_id, msg_len);
	rstp_port_enable = (RSTP_PORT_ENABLE *)msg;
	if(!rstp_port_enable){
		LOG_ER("rstp_port_enable is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	if ((rstp_port_enable->port_index > port_count) || (rstp_port_enable->port_index < 1) ){
		
		LOG_ER("rstp_port_enable port_index is out of scope\n");
		return IH_ERR_FAILED;
	}

	i = rstp_port_enable->port_index -1;
	
	gl_rstp_shard[i].port_index = rstp_port_enable->port_index;
	gl_rstp_shard[i].port_rstp_state = rstp_port_enable->port_rstp_state;	

	LOG_DB("port%d rstp enable is %d\n",gl_rstp_shard[i].port_index ,gl_rstp_shard[i].port_rstp_state);
	
	return IH_ERR_OK;	

	

}


IH_STATUS rstp_sync_register(void) {


	IH_STATUS rv = 0;
	
	LOG_DB("### register rstp info sync handle.\n");

	ih_ipc_subscribe(IPC_MSG_RSTPD_SYNC_PORT_EDGE);
	ih_ipc_subscribe(IPC_MSG_RSTPD_SYNC_RSTP_STATUS);

	
	ih_ipc_register_msg_handle(IPC_MSG_RSTPD_SYNC_PORT_EDGE, sync_rstp_edge_handle);
	ih_ipc_register_msg_handle(IPC_MSG_RSTPD_SYNC_RSTP_STATUS, sync_rstp_enable_handle);


	LOG_DB("### waiting for rstp service ready...\n");
	rv = ih_ipc_wait_service_ready(IH_SVC_RSTPD, IPC_TIMEOUT_NOTIMEOUT);
	if(rv){
		LOG_ER("### wait for rstp service %d ready return error %d\n", IH_SVC_RSTPD, rv);
	}else{
		LOG_DB("### waiting for rstp service ready done!\n");
	}

	return IH_ERR_OK;

}


BOOL get_rstp_port_status( uns16 port_id){

	int i;
	int port_count;

	port_count = get_switch_port_count()-1;
	
	if( port_id > port_count || port_id<1) {

		LOG_ER("get rstp enable port_id is error\n");
		return FALSE;
	}		

	i = port_id -1;

	return gl_rstp_shard[i].port_rstp_state;

}



BOOL get_rstp_edge_status( uns16 port_id){

	int i;
	int port_count;

	port_count = get_switch_port_count()-1;
	
	if( port_id > port_count || port_id<1) {

		LOG_ER("get rstp edge status port_id is error\n");
		return FALSE;
	}		

	i = port_id -1;

	return gl_rstp_shard[i].port_edge_state;

}

