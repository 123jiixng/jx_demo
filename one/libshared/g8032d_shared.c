/*
 * g8032d_shared.c
 *
 *  Created on: 2011-8-5
 *      Author: henri
 */

#include "ih_ipc.h"
#include "ih_cmd.h"
#include "if_common.h"
#include "g8032_ipc.h"
#include "g8032d_shared.h"


G8032_MIB_CONFIG_SHARED g8032_mib_config_shared[MAX_G8032_INSTANCE];

IH_STATUS g8032_sync_config_mib_register(void)
{

	ih_ipc_subscribe(IPC_MSG_G8032_GET_CONFIG_MIB);
	ih_ipc_register_msg_handle(IPC_MSG_G8032_GET_CONFIG_MIB, ipc_sync_g8032_config_mib_handle);

	
	return IH_ERR_OK;
}

IH_STATUS g8032_sync_config_mib_unregister(void)
{

	ih_ipc_unsubscribe(IPC_MSG_G8032_GET_CONFIG_MIB);
	
	return IH_ERR_OK;
}


int32 ipc_sync_g8032_config_mib_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen){

	G8032_SVCINFO *g8032_config_value = NULL;
	int i;
	int id;

	if(!msg_hdr || !msg){
		LOG_ER("g8032 config mib msg_hdr or msg is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	g8032_config_value  = (G8032_SVCINFO *)msg;
	if(!g8032_config_value){
		LOG_ER("g8032_config_value is NULL\n");
		return IH_ERR_NULL_PTR;
	}

	id = g8032_config_value->instance_id -1;

	g8032_mib_config_shared[id].G8032InstanceIndex = g8032_config_value->instance_id ;
	g8032_mib_config_shared[id].G8032InstanceRingType = g8032_config_value->ring_type;
	g8032_mib_config_shared[id].G8032InstanceRelatedInstanceIndex = g8032_config_value->major_ring_instance_id;
	g8032_mib_config_shared[id].G8032InstanceNodeType = g8032_config_value->node_type;
	g8032_mib_config_shared[id].G8032InstanceControlVlan = g8032_config_value->c_vlan;
	g8032_mib_config_shared[id].G8032InstanceRingPortLeft = g8032_config_value->pid0;
	g8032_mib_config_shared[id].G8032InstanceRingPortRight = g8032_config_value->pid1;

	LOG_DB("g8032 instance %d\n",g8032_mib_config_shared[id].G8032InstanceIndex);
	LOG_DB("g8032 instance ring type %d\n",g8032_mib_config_shared[id].G8032InstanceRingType);
	LOG_DB("g8032 instance related id %d\n",g8032_mib_config_shared[id].G8032InstanceRelatedInstanceIndex);
	LOG_DB("g8032 instance node type %d\n",g8032_mib_config_shared[id].G8032InstanceNodeType);
	LOG_DB("g8032 instance cvlan id %d\n",g8032_mib_config_shared[id].G8032InstanceControlVlan);
	LOG_DB("g8032 instance left port id %d\n",g8032_mib_config_shared[id].G8032InstanceRingPortLeft);
	LOG_DB("g8032 instance right port id %d\n",g8032_mib_config_shared[id].G8032InstanceRingPortRight);


	for(i =0 ; i < MAX_SVLAN ; i++) {

		g8032_mib_config_shared[id].G8032InstanceServiceVlan[i] = g8032_config_value->s_vlan[i];

		if(g8032_mib_config_shared[id].G8032InstanceServiceVlan[i] != 0) {

			LOG_DB("g8032 instance svlan id %d\n",g8032_mib_config_shared[id].G8032InstanceServiceVlan[i]);
		}
	}

	g8032_mib_config_shared[id].G8032InstanceEnableStatus = g8032_config_value->node_state;
		
	LOG_DB("g8032 instance right status %d\n",g8032_mib_config_shared[id].G8032InstanceEnableStatus);

	return IH_ERR_OK;

}


int ipc_send_get_g8032_status_mib_value( unsigned short instance_id, unsigned char type, int *value){
	
	IH_SVC_ID peer_svc_id = IH_SVC_G8032;
	IPC_MSG_GET_G8032_STATUS get_g8032_instance;
	int32 rv = 0;
	IPC_MSG *rsp = NULL;

	memset(&get_g8032_instance, 0 , sizeof(IPC_MSG_GET_G8032_STATUS));
	get_g8032_instance.instance_id = instance_id;
	get_g8032_instance.type	= type;
	
	rv = ih_ipc_send_wait_rsp(peer_svc_id, DEFAULT_CMD_TIMEOUT, IPC_MSG_G8032_GET_STATUS_MIB, (char *)&get_g8032_instance, sizeof(IPC_MSG_GET_G8032_STATUS), &rsp);
	if(rv){
		LOG_ER("### failed to send get g8032 status mib(%d) to svc %d, error %d\n", IPC_MSG_G8032_GET_STATUS_MIB, IH_SVC_G8032, rv);
		return IH_ERR_FAIL;
	}

	if(rsp){

		//LOG_IN("G8032_IF Get response: 0x%x from service %d", rsp->hdr.type, peer_svc_id);
		//LOG_IN("G8032_IF Type   : %s", (rsp->hdr.type==IPC_MSG_NAK) ? "Not expected!" : "Good");
		//LOG_IN("G8032_IF Len    : %d", rsp->hdr.len);
		memcpy(value , rsp->body , sizeof(*value));
		ih_ipc_free_msg(rsp);
		//LOG_IN("G8032_IF Content: %d\n",*value);
		
	}

	return IH_ERR_OK;
}


int get_status_G8032InstanceRPLPort( unsigned short instance_id, int *value){

	unsigned char type = GET_G8032_INSTANCE_RPL_PORT;

	if(!ipc_send_get_g8032_status_mib_value(instance_id, type, value)){

		return GET_OK;
	}

	LOG_ER("get g8032 status rpl port err: instance id %d, value %d", instance_id, *value);
	return GET_ERR;
}

int get_status_G8032InstancePortLeftStatus( unsigned short instance_id, int *value){

	unsigned char type = GET_G8032_INSTANCE_PORT_LEFT;

	if(!ipc_send_get_g8032_status_mib_value(instance_id, type, value)){
	
		return GET_OK;
	}

	LOG_ER("get g8032 status port left err: instance id %d, value %d", instance_id, *value);
	return GET_ERR;
}

int get_status_G8032InstancePortRightStatus( unsigned short instance_id, int *value){

	unsigned char type = GET_G8032_INSTANCE_PORT_RIGHT;

	if(!ipc_send_get_g8032_status_mib_value(instance_id, type, value)){
	
		return GET_OK;
	}

	LOG_ER("get g8032 status port right err: instance id %d, value %d", instance_id, *value);
	return GET_ERR;
}

int get_status_G8032InstanceRingStatus( unsigned short instance_id, int *value){

	unsigned char type = GET_G8032_INSTANCE_RING;

	if(!ipc_send_get_g8032_status_mib_value(instance_id, type, value)){
	
		return GET_OK;
	}

	LOG_ER("get g8032 status ring status err: instance id %d, value %d", instance_id, *value);
	return GET_ERR;
}

int get_status_G8032InstanceInterconnectionStatus( unsigned short instance_id, int *value){

	unsigned char type = GET_G8032_INSTANCE_INTERCONNECTOIN;

	if(!ipc_send_get_g8032_status_mib_value(instance_id, type, value)){

		return GET_OK;
	}

	LOG_ER("get g8032 status interconnection status err: instance id %d, value %d", instance_id, *value);
	return GET_ERR;
}

int get_config_G8032InstanceRingType( unsigned short instance_id, int *value){

	int id = instance_id -1;
	
	if (g8032_mib_config_shared[id].G8032InstanceIndex == instance_id) {

		*value = g8032_mib_config_shared[id].G8032InstanceRingType;
		LOG_DB("get g8032 config instance %d ring type %d\n", instance_id, *value);
		return GET_OK;	

	} else {

		LOG_ER("get g8032 config ring type instance id err, mib instance id is %d, get instance id is %d\n",g8032_mib_config_shared[id].G8032InstanceIndex, instance_id);
		return GET_ERR;	
	}	
}

int get_config_G8032InstanceRelatedInstanceIndex( unsigned short instance_id, int *value){

	int id = instance_id -1;
	
	if (g8032_mib_config_shared[id].G8032InstanceIndex == instance_id) {

		*value = g8032_mib_config_shared[id].G8032InstanceRelatedInstanceIndex;
		LOG_DB("get g8032 config instance %d related instance id %d\n", instance_id, *value);
		return GET_OK;	

	} else {

		LOG_ER("get g8032 config instance id related instance id err, mib instance id is %d, get instance id is %d\n",g8032_mib_config_shared[id].G8032InstanceIndex, instance_id);
		return GET_ERR;	
	}	

}

int get_config_G8032InstanceControlVlan( unsigned short instance_id, int *value){

	int id = instance_id -1;
	
	if (g8032_mib_config_shared[id].G8032InstanceIndex == instance_id) {

		*value = g8032_mib_config_shared[id].G8032InstanceControlVlan;
		LOG_DB("get g8032 config instance %d control vlan %d\n", instance_id, *value);
		return GET_OK;	

	} else {

		LOG_ER("get g8032 config instance id c-vlan err, mib instance id is %d, get instance id is %d\n",g8032_mib_config_shared[id].G8032InstanceIndex, instance_id);
		return GET_ERR;	
	}

}

int get_config_G8032InstanceRingPortLeft( unsigned short instance_id, int *value){

	int id = instance_id -1;
	
	if (g8032_mib_config_shared[id].G8032InstanceIndex == instance_id) {

		*value = g8032_mib_config_shared[id].G8032InstanceRingPortLeft;
		LOG_DB("get g8032 config instance %d port left %d\n", instance_id, *value);
		return GET_OK;	

	} else {

		LOG_ER("get g8032 config instance id port_left err, mib instance id is %d, get instance id is %d\n",g8032_mib_config_shared[id].G8032InstanceIndex, instance_id);
		return GET_ERR;	
	}
}

int get_config_G8032InstanceRingPortRight( unsigned short instance_id, int *value){

	int id = instance_id -1;
	
	if (g8032_mib_config_shared[id].G8032InstanceIndex == instance_id) {

		*value = g8032_mib_config_shared[id].G8032InstanceRingPortRight;
		LOG_DB("get g8032 config instance %d port right %d\n", instance_id, *value);
		return GET_OK;	

	} else {

		LOG_ER("get g8032 config instance id port_right err, mib instance id is %d, get instance id is %d\n",g8032_mib_config_shared[id].G8032InstanceIndex, instance_id);
		return GET_ERR;	
	}
}

int get_config_G8032InstanceServiceVlanLow( unsigned short instance_id, unsigned char *low){


	int id = instance_id -1;
	char tmp[20] = "need to update";
	//int i;
	//unsigned char low[MAX_SVLAN];
	
	if (g8032_mib_config_shared[id].G8032InstanceIndex == instance_id) {


		//if (!value_change_to_bit(&g8032_mib_config_shared[id].G8032InstanceServiceVlan[0], low, SVLAN_LOW_FLAG)){

			//memcpy(value, low, MAX_SVLAN);
			memcpy(low,tmp,20);
			LOG_DB("svlan low %s\n", low);
			return GET_OK;
		//}

	} else {

		LOG_ER("get g8032 config instance id low slvan err, mib instance id is %d, get instance id is %d\n",g8032_mib_config_shared[id].G8032InstanceIndex, instance_id);
		return GET_ERR;	
	}

	return GET_OK;
}

int get_config_G8032InstanceServiceVlanHigh( unsigned short instance_id, unsigned char *high){

	int id = instance_id -1;
	char tmp[20] = "need to update";
	//int i;
	//unsigned char high[MAX_SVLAN];
	
	if (g8032_mib_config_shared[id].G8032InstanceIndex == instance_id) {


		//if (!value_change_to_bit(&g8032_mib_config_shared[id].G8032InstanceServiceVlan[0], high, SVLAN_HIGH_FLAG)){

			//memcpy(value, high, MAX_SVLAN);
			memcpy(high,tmp,20);
			LOG_DB("svlan high %s\n", high);
			return GET_OK;
		//}

	} else {

		LOG_WA("get g8032 config instance id low slvan err, mib instance id is %d, get instance id is %d\n",g8032_mib_config_shared[id].G8032InstanceIndex, instance_id);
		return GET_ERR;	
	}
	
	return GET_OK;
}

int get_config_G8032InstanceNodeType( unsigned short instance_id, int *value){


	int id = instance_id -1;
	
	if (g8032_mib_config_shared[id].G8032InstanceIndex == instance_id) {

		*value = g8032_mib_config_shared[id].G8032InstanceNodeType;
		LOG_DB("get g8032 config instance %d port node type %d\n", instance_id, *value);
		return GET_OK;	

	} else {

		LOG_WA("get g8032 config instance id node type err, mib instance id is %d, get instance id is %d\n",g8032_mib_config_shared[id].G8032InstanceIndex, instance_id);
		return GET_ERR;	
	}
}

int get_config_G8032InstanceEnableStatus( unsigned short instance_id, int *value){

	int id = instance_id -1;
	
	if (g8032_mib_config_shared[id].G8032InstanceIndex == instance_id) {

		*value = g8032_mib_config_shared[id].G8032InstanceEnableStatus;
		
		LOG_DB("get g8032 config instance %d  enable %d\n", instance_id, *value);
		return GET_OK;	

	} else {

		LOG_WA("get g8032 config instance id  enable err, mib instance id is %d, get instance id is %d\n",g8032_mib_config_shared[id].G8032InstanceIndex, instance_id);
		return GET_ERR;	
	}

}


int set_config_G8032InstanceRingType( unsigned short instance_id, int *value){

	*value = 0;
	return GET_OK;	
}
int set_config_G8032InstanceRelatedInstanceIndex( unsigned short instance_id, int *value){

	*value = 0;
	return GET_OK;	
}

int set_config_G8032InstanceControlVlan( unsigned short instance_id, int *value){

	*value = 0;
	return GET_OK;	
}

int set_config_G8032InstanceRingPortLeft( unsigned short instance_id, int *value){

	*value = 0;
	return GET_OK;	
}

int set_config_G8032InstanceRingPortRight( unsigned short instance_id, int *value){

	*value = 0;
	return GET_OK;	
}

int set_config_G8032InstanceServiceVlan( unsigned short instance_id, int *value){

	*value = 0;
	return GET_OK;	
}

int set_config_G8032InstanceEnableStatus( unsigned short instance_id, int *value){

	*value = 0;
	return GET_OK;	
}


int value_change_to_bit(unsigned short *input_value, unsigned char *output, unsigned char flag) {

	int i,id;
	int quotient;
	int remainder;
	
	memset(output, 0 , sizeof(MAX_SVLAN));

	for( i = 0; i < MAX_SVLAN; i++) {

		if(input_value[i] == 0){

			continue;
		} 
		
		LOG_DB("value to bits svlan is %d\n",input_value[i]);
		if (input_value[i] > 0 && input_value[i] < SVLAN_MIDDLE_ID && flag == SVLAN_LOW_FLAG) {

			remainder = input_value[i] % 8;
			LOG_DB("value to bits remainder is %d\n",remainder);
			quotient = input_value[i] / 8;
			
			if(!quotient){
			
				id = quotient -1;

			} else {

				id = quotient;
			}
			
			output[id] |= 1 << remainder ;

			LOG_DB("slvan is %d, low bit is %c\n",input_value[i], output[id]);

		} else if (input_value[i] >= SVLAN_MIDDLE_ID && input_value[i] < 4096 && flag == SVLAN_HIGH_FLAG) {

			
			remainder = input_value[i] % 8;
			quotient = input_value[i] / 8;

			output += quotient;
			*output |= 1 << remainder;

			LOG_DB("slvan is %d, high bit is %c\n",input_value[i], *output);
	
		} else {
				
			LOG_WA("g8032 instance service vlan is out of scope\n");
		}
	}	

	return GET_OK;
}


BOOL g8032_instance_is_valid( int instance_id) {

	int id = instance_id -1;
	
	if( g8032_mib_config_shared[id].G8032InstanceEnableStatus && g8032_mib_config_shared[id].G8032InstanceControlVlan) {

		LOG_DB("g8032 instance %d is valid\n",g8032_mib_config_shared[id].G8032InstanceIndex);
		return TRUE;

	} else {
		
		LOG_WA("g8032 instance %d is invalid\n",instance_id);
		return FALSE;

	}

}

//sync igmpsnoop vlan
BOOL g8032_svlan_port(unsigned short svlan, unsigned short port_id){

	int i,j;
	BOOL rv = FALSE;

	for( i = 0; i< MAX_G8032_INSTANCE; i++){

		for( j = 0; j<MAX_SVLAN; j++){

			if(g8032_mib_config_shared[i].G8032InstanceServiceVlan[j]==0){

				continue;

			} else {

				if( g8032_mib_config_shared[i].G8032InstanceServiceVlan[j] == svlan) {

					if( port_in_vlan(port_id, svlan)) {

						if(check_g8032_ring_port( g8032_mib_config_shared[i].G8032InstanceIndex, port_id)){

							rv = TRUE;
							return rv;
						}
					}	
				}
										
			}		
			
		}

	}

	return rv;

}

BOOL check_g8032_ring_port(int instance_id, unsigned short port_id){

	int id;
	int rv = FALSE;

	if(instance_id <1 || instance_id > MAX_G8032_INSTANCE) {

		LOG_WA("check ring port instacne id %d is out of scope\n",instance_id);
	}

	id =instance_id -1;

	if (g8032_mib_config_shared[id].G8032InstanceRingPortLeft == port_id || g8032_mib_config_shared[id].G8032InstanceRingPortRight == port_id){

		rv = TRUE;
	}

	return rv;
}
