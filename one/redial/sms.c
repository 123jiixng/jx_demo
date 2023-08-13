/*
 * $Id:
 *
 *   SMS
 *
 * Copyright (c) 2001-2013 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 03/12/2013
 * Author: Zhangly
 * Description:
 * 		sms api
 *
 */

#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/sysinfo.h>

#include "ih_ipc.h"
#include "shared.h"
#include "sms_ipc.h"

#include "modem.h"
#include "pdu.h"
#include "sms.h"

static char gl_sms_center[20] = {'\0'};
extern int gl_modem_code;

/* @brief: configure sms center & mode
 * @param dev: AT port
 * @param mode: text or pdu
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_init(char *dev, int mode)
{
	char buf[128], smsc[20], *p;
	int n;

	//ate0,curc
	//u8300
	if(gl_modem_code == IH_FEATURE_MODEM_U8300
		|| gl_modem_code == IH_FEATURE_MODEM_U8300C){
		send_at_cmd_sync(dev, "AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n", NULL, 0, 0);
	}else if (gl_modem_code == IH_FEATURE_MODEM_PLS8){
		send_at_cmd_sync(dev, "AT+CPMS=\"ME\",\"ME\",\"ME\"\r\n", NULL, 0, 0);
	}
	//mode
	if(mode==SMS_MODE_TEXT) {
		send_at_cmd_sync(dev, "AT+CMGF=1\r\n", NULL, 0, 0);
	} else {
		send_at_cmd_sync(dev, "AT+CMGF=0\r\n", NULL, 0, 1);
		//sms center(pdu only)
		send_at_cmd_sync(dev, "AT+CSCA?\r\n", buf, sizeof(buf), 1);
		/*+CSCA: "+8613010112500",145*/
		p = strstr(buf, "+CSCA:");
		if(p) {
			n = sscanf(p, "+CSCA: \"%[^\"]\"", smsc);
			if(n>0) {
				LOG_IN("sms center: %s", smsc);
				strcpy(gl_sms_center, smsc);
			}
		}
	}
	
	return 0;
}

/* @brief: send TEXT sms to one receiver
 * @param dev: AT port
 * @param receiver: receiver
 * @param data: content of sms
 * @param len: length of sms
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_send_text(char *dev, char *receiver, char *data, int len)
{
	int fd;
	char cmd[32], buf[1024];

	fd = open(dev, O_RDWR|O_NONBLOCK, 0666);
	if(fd<0) return -1;

	/*flush auto-report data*/
	flush_fd(fd);
	if(gl_modem_code == IH_FEATURE_MODEM_MC2716
			|| gl_modem_code == IH_FEATURE_MODEM_MC5635){
		//LOG_IN("VZ16 send txt sms %s", receiver);
		sprintf(cmd, "AT^HCMGS=\"%s\"\r", receiver);
	}else {
		sprintf(cmd, "AT+CMGS=\"%s\"\r", receiver);
	}
	write_fd(fd, cmd, strlen(cmd), 1000, 0);
	if(check_return2(fd, buf, sizeof(buf), ">", 3000, 0)<=0){
		close(fd);
		return -2;
	}
	//write msg
	write_fd(fd, data, len, 1000, 0);
	cmd[0] = 0x1A;
	cmd[1] = 0x00;
	write_fd(fd, cmd, 1, 1000, 0);
	if(check_return2(fd, buf, sizeof(buf), "OK", 10000, 0)<=0){
		close(fd);
		return -3;
	}
	close(fd);

	return 0;
}

/* @brief: send PDU sms to one receiver
 * @param dev: AT port
 * @param receiver: receiver
 * @param data: content of sms
 * @param len: length of sms
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_send_pdu(char *dev, char *receiver, char *data, int len)
{
	int fd;
	char cmd[32], buf[1024], msg[512];
	PDU_PARAM pdu;
	int nPduLength;        // PDU串长度
	unsigned char nSmscLength;    // SMSC串长度

	//skip '+'
	strcpy(pdu.SCA, gl_sms_center+1);
	pdu.TP_DCS = GSM_7BIT;
	strcpy(pdu.TP_UD, data);
	pdu.TP_PID = 0;
	//skip '+'
	strcpy(pdu.TPA, receiver+1);

	nPduLength = gsmEncodePdu(&pdu, msg);    // 根据PDU参数，编码PDU串
	strcat(msg, "\x01a");        // 以Ctrl-Z结束

	gsmString2Bytes(msg, &nSmscLength, 2);    // 取PDU串中的SMSC信息长度
	nSmscLength++;        // 加上长度字节本身

	fd = open(dev, O_RDWR|O_NONBLOCK, 0666);
	if(fd<0) return -1;

	/*flush auto-report data*/
	flush_fd(fd);

	// 命令中的长度，不包括SMSC信息长度，以数据字节计
	if(gl_modem_code == IH_FEATURE_MODEM_MC2716
			|| gl_modem_code == IH_FEATURE_MODEM_MC5635){
		LOG_IN("VZ16 send pdu sms %s", receiver);
		sprintf(cmd, "AT^HCMGS=%d\r", nPduLength / 2 - nSmscLength);    // 生成命令
	}else {
		sprintf(cmd, "AT+CMGS=%d\r", nPduLength / 2 - nSmscLength);    // 生成命令
	}
	write_fd(fd, cmd, strlen(cmd), 1000, 0);
	if(check_return2(fd, buf, sizeof(buf), ">", 3000, 0)<=0){
		close(fd);
		return -2;
	}
	//write msg
	write_fd(fd, msg, strlen(msg), 1000, 0);
	if(check_return2(fd, buf, sizeof(buf), "OK", 10000, 0)<=0){
		close(fd);
		return -3;
	}
	close(fd);

	return 0;
}

/* @brief: receive text sms
 * @param dev: AT port
 * @param index: sms location
 * @param buf: buffer
 * @param len: buffer length
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_receive_one_text(char *dev, int index, char *buf, int len)
{
	//TODO
	return 0;
}

/* @brief: receive pdu sms
 * @param dev: AT port
 * @param index: sms location
 * @param buf: buffer
 * @param len: buffer length
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_receive_one_pdu(char *dev, int index, char *buf, int len)
{
	//TODO
	return 0;
}

/* @brief: receive multiple text sms according state
 * @param dev: AT port
 * @param sms_state: SMS_STATE(0-4)
 * @param buf: buffer
 * @param len: buffer length
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_receive_batch_text(char *dev, SMS_STATE sms_state, char *buf, int len)
{
	int fd;
	char *state_map[] = {"REC UNREAD","REC READ","STO UNSENT","STO SENT","ALL"};
	char cmd[32];

	fd = open(dev, O_RDWR|O_NONBLOCK, 0666);
	if(fd<0) return -1;

	/*flush auto-report data*/
	flush_fd(fd);
	if(gl_modem_code == IH_FEATURE_MODEM_MC2716
			|| gl_modem_code == IH_FEATURE_MODEM_MC5635){
		//LOG_IN("VZ16 batch txt");
		sprintf(cmd, "AT^HCMGL=%d\r\n", sms_state);    // 生成命令
	}else {	
		sprintf(cmd, "AT+CMGL=\"%s\"\r\n", state_map[sms_state]);
	}
	write_fd(fd, cmd, strlen(cmd), 1000, 0);
	if(check_return2(fd, buf, len, "OK", 3000, 0)<=0){
		close(fd);
		return -2;
	}
	close(fd);

	return 0;
}

/* @brief: receive multiple pdu sms according state
 * @param dev: AT port
 * @param sms_state: SMS_STATE(0-4)
 * @param buf: buffer
 * @param len: buffer length
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_receive_batch_pdu(char *dev, SMS_STATE sms_state, char *buf, int len)
{
	int fd;
	char cmd[32];

	fd = open(dev, O_RDWR|O_NONBLOCK, 0666);
	if(fd<0) return -1;

	/*flush auto-report data*/
	flush_fd(fd);

	//+cmgl: (0-4)
	if(gl_modem_code == IH_FEATURE_MODEM_MC2716
			|| gl_modem_code == IH_FEATURE_MODEM_MC5635){
		//LOG_IN("VZ16 batch pdu");
		sprintf(cmd, "AT^HCMGL=%d\r\n", sms_state);    // 生成命令
	}else {	
		sprintf(cmd, "AT+CMGL=%d\r\n", sms_state);
	}
	write_fd(fd, cmd, strlen(cmd), 1000, 0);
	if(check_return2(fd, buf, len, "OK", 3000, 0)<=0){
		close(fd);
		return -2;
	}
	close(fd);

	return 0;
}

/* @brief: delete a sms
 * @param dev: AT port
 * @param index: sms location
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_delete(char *dev, int index)
{
	char cmd[32];

	LOG_DB("Delete SMS:%d", index);
	sprintf(cmd, "AT+CMGD=%d\r\n", index);
	send_at_cmd_sync(dev, cmd, NULL, 0, 0);
	
	return 0;
}

/* @brief: delete all sms
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_delete_all(void)
{
	//TODO
	return 0;
}

/* @brief: check phone number if valid
 * @param acl: sms acl
 * @param phone: phone number
 * @return: <0 -- error
 *	    =0 -- ok
*/
static int sms_check_phone_valid(SMS_ACL *acl, char *phone)
{
	int i, ret=-1;

	//TODO: handle phone format
	for(i=0; i<SMS_ACL_MAX; i++) {
		if(acl[i].id && strstr(phone, acl[i].phone)) {
			if(acl[i].action==SMS_ACL_PERMIT)
				ret = 0;
			break;
		}
	}

	return ret;
}

static int sms_read_vz16(char *data, int data_len, SMS_ACL *acl, SMS *sms)
{
	char *pos = NULL;		
	char *p = (char *)data,*var;
	int ret = -1;
	uint16 sms_len=0;


	/*MC8630 format: ^HCMGR: 13581837652,2009,10,27,10,58,14,0,1,3,0,0,0,0
	 *\r\ntest^Z\r\n \r\nOK\r\n
	 *MC2716 format: ^HCMGR: 13581837652,2009,10,27,10,58,14,0,1,3,0,0,0,0
	 *\r\ntest^Z\r\nOK\r\n
	*/
	pos = strstr(data, "^HCMGR:");
	if(!pos) {
		LOG_IN("vz16 sms read invalid");
		return -1;
	}
	pos = strstr(data, "\r\nOK\r\n");
	if(!pos) {
		LOG_IN("vz16 sms read err");
		return -1;
	}
	p = p + strlen("^HCMGR:")+2;  
	var = strtok(p,","); //sms sender 
	if ( !var ) {
		LOG_IN("vz16 sms sender get err");
		return -1;
	}
	while(*var!=0 && *var==' ') var++;//eat ' '
	strlcpy(sms->phone, var, sizeof(sms->phone));
	//skip <alpha> && date
	var = strtok(NULL,"\n");
	if ( !var )  {
		LOG_IN("vz16 sms read data");
		return -1;
	}
	//point to sms body
	var += strlen(var);
	while ( *var=='\0'&&var<((char *)data+data_len) ) var++;

	var = strtok(NULL,"\r\n");
	sms_len = strlen(var)-1;//1 for "^Z"
	var[sms_len] = '\0'; 
	strlcpy(sms->data, var, sizeof(sms->data));
	sms->data_len = strlen(sms->data);//not contain '\0'
	
	LOG_IN("sms sender:%s data:%s", sms->phone, sms->data);
	//check phone acl
	if(sms_check_phone_valid(acl, sms->phone)==0) {
		LOG_IN("receive a sms from %s", sms->phone);
		//send sms to smsd
		sms->data_len = strlen(sms->data);
		ret = ih_ipc_send_no_ack(IH_SVC_SMSD, IPC_MSG_SMS_RECV, (char *)sms, sizeof(SMS));
		if (ret != ERR_OK) LOG_IN("send sms info msg not ok.");				
	}
	return 0;
}

/* @brief: parse text sms
 * @param dev: AT port
 * @param data: content of sms
 * @param acl: sms acl
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_parse_text(char *dev, char *data, SMS_ACL *acl)
{
	char *p, *q;
	int index, n, ret, fd;
	SMS sms;
	char at_cmd[128]={0},at_return[8192]={0};
	int len;

	q = strstr(data, "\r\nOK\r\n");
	if(!q) return -1;
	*q = '\0';

	if(gl_modem_code == IH_FEATURE_MODEM_MC2716
			|| gl_modem_code == IH_FEATURE_MODEM_MC5635){
		
		p = strstr(data, "^HCMGL:");
		while(p) {
			data = p+7;
			q = strstr(data, "^HCMGL:");
			p = q;
			
			while(*data!=0 && *data==' ') data++;
			index = atoi(data);
						
			fd = open(dev, O_RDWR|O_NONBLOCK, 0666);
			if(fd<0) return -1;

			/*flush auto-report data*/
			flush_fd(fd);
			
			sprintf(at_cmd, "AT^HCMGR=%d\r\n", index);
			write_fd(fd, at_cmd, strlen(at_cmd), 1000,  0);
			len = sizeof(at_return)-1;
			if(check_return2(fd, at_return, len, "OK", 3000, 0)<=0){
				//LOG_IN("vz16 read sms %d err", index);
				close(fd);
				sms_delete(dev, index);
				continue;
			}
			close(fd);
			sms_read_vz16(at_return, strlen(at_return), acl, &sms);
			//delete sms
			sms_delete(dev, index);
		}
	}else {
	/*
	*+CMGL: 2,"REC READ","10010",,"13/03/07,11:31:43+32"
	*test
	*+CMGL: 3,"REC READ","+8613581837650",,"13/03/13,17:26:25+32"
	*reboot
	*
	*OK\r\n
	*/
	//SMS end，is "\r\nOK\r\n"
		p = strstr(data, "+CMGL:");
		while(p) {
			data = p+6;
			q = strstr(data, "+CMGL:");
			if(q) {
				*(q-2) = '\0';//eat \r\n
			}
			memset(&sms, 0, sizeof(sms));
			/*here: use '~' as end*/
			n = sscanf(p, "+CMGL: %d,%*[^,],\"%[^\"]\",,\"%*[^\"]\"\r\n%[^~]", &index, sms.phone, sms.data);
			if(n>0) {
				//LOG_DB("%d %s %s", index, sms.phone, sms.data);
				//check phone acl
				if(sms_check_phone_valid(acl, sms.phone)==0) {
					LOG_IN("receive a sms from %s", sms.phone);
					//send sms to smsd
					sms.data_len = strlen(sms.data);
					ret = ih_ipc_send_no_ack(IH_SVC_SMSD, IPC_MSG_SMS_RECV, (char *)&sms, sizeof(sms));
					if (ret != ERR_OK) LOG_IN("send sms info msg not ok.");				
				}
				//delete sms
				sms_delete(dev, index);
			}
			p = q;
		}
	}
	return 0;
}

/* @brief: parse pdu sms
 * @param dev: AT port
 * @param data: content of sms
 * @param acl: sms acl
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_parse_pdu(char *dev, char *data, SMS_ACL *acl)
{
	char *p, *q;
	int index, n, ret;
	SMS sms;
	PDU_PARAM pdu;

	/*
	*+CMGL: 0,1,,26
	*0891683110104105F0240D91683185817356F000003130811231252306C8329BFD0E01
	*
	*OK\r\n
	*/
	//SMS end，is "\r\n\r\nOK\r\n"
	q = strstr(data, "\r\n\r\nOK\r\n");
	if(!q) return -1;
	*q = '\0';

	p = strstr(data, "+CMGL:");
	while(p) {
		data = p+6;
		q = strstr(data, "+CMGL:");
		if(q) {
			*(q-2) = '\0';//eat \r\n
		}
		memset(&sms, 0, sizeof(sms));
		/*here: use '~' as end*/
		n = sscanf(p, "+CMGL: %d,%*[^\r]\r\n%[^~]", &index, sms.data);
		if(n>0) {
			LOG_DB("%d %s", index, sms.data);
			gsmDecodePdu(sms.data, &pdu);
			LOG_DB("SCA: %s", pdu.SCA);
			LOG_DB("TPA: %s", pdu.TPA);
			LOG_DB("TP-PID: %x", pdu.TP_PID);
			LOG_DB("TP-DCS: %x", pdu.TP_DCS);
			LOG_DB("TP-SCTS: %s", pdu.TP_SCTS);
			LOG_DB("TP-UD: %s", pdu.TP_UD);
			sprintf(sms.phone, "+%s", pdu.TPA);
			strlcpy(sms.data, pdu.TP_UD, sizeof(sms.data));
			//check phone acl
			if(sms_check_phone_valid(acl, sms.phone)==0) {
				LOG_IN("receive a sms from %s", sms.phone);
				//send sms to smsd
				sms.data_len = strlen(sms.data);
				ret = ih_ipc_send_no_ack(IH_SVC_SMSD, IPC_MSG_SMS_RECV, (char *)&sms, sizeof(sms));
				if (ret != ERR_OK) LOG_IN("send sms info msg not ok.");				
			}
			//delete sms
			sms_delete(dev, index);
		}
		p = q;
	}

	return 0;
}

//get/set handle(include trigger according cli format)
/* @brief: handle incoming sms
 *	get status/config via cli+json;
 *	set config/action(trigger,reboot...) via cli;
 *	Consider simplicity, get status from redial service firstly.
 * @param dev: AT port
 * @param mode: SMS_MODE_TEXT|SMS_MODE_PDU
 * @param acl: sms acl
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_handle_insms(char *dev, int mode, SMS_ACL *acl)
{
#define SMS_BUF_MAX	65536
	char *buf;

	buf = malloc(SMS_BUF_MAX);
	if(!buf) {
		LOG_ER("sms_handle_insms malloc fail");
		return -1;
	}

	if(mode==SMS_MODE_TEXT) {
		//receive sms
		sms_receive_batch_text(dev, SMS_STATE_ALL, buf, SMS_BUF_MAX);
		//parse sms
		sms_parse_text(dev, buf, acl);
	} else {
		//receive sms
		sms_receive_batch_pdu(dev, SMS_STATE_ALL, buf, SMS_BUF_MAX);
		sms_parse_pdu(dev, buf, acl);
	}
	free(buf);

	return 0;
}

/* @brief: handle outcoming sms
 * @param dev: AT port
 * @param mode: SMS_MODE_TEXT|SMS_MODE_PDU
 * @param psms: SMS struct pointer
 * @return: <0 -- error
 *	    =0 -- ok
*/
int sms_handle_outsms(char *dev, int mode, SMS *psms)
{
	int ret = -1;

	if(mode==SMS_MODE_TEXT) {
		ret = sms_send_text(dev, psms->phone, psms->data, psms->data_len);
	} else {
		ret = sms_send_pdu(dev, psms->phone, psms->data, psms->data_len);
	}

	return ret;
}

int pysms_handle_outsms(char *dev, SMS *psms)
{
	
	int fd;
	char cmd[32], buf[1024], msg[1280];
	int len;

	sms_init(dev, SMS_MODE_PDU);
	fd = open(dev, O_RDWR|O_NONBLOCK, 0666);
	if(fd<0){ 
		LOG_ER("open %s err(%d:%s)", dev, errno, strerror(errno));
		return -1;
	}

	/*flush auto-report data*/
	flush_fd(fd);
	// 命令中的长度，不包括SMSC信息长度，以数据字节计
	if(gl_modem_code == IH_FEATURE_MODEM_MC2716
			|| gl_modem_code == IH_FEATURE_MODEM_MC5635){
		sprintf(cmd, "AT^HCMGS=%d\r", psms->msg_len);    // 生成命令
	}else {
		sprintf(cmd, "AT+CMGS=%d\r", psms->msg_len);    // 生成命令
	}
	write_fd(fd, cmd, strlen(cmd), 1000, 0);
	if(check_return2(fd, buf, sizeof(buf), ">", 3000, 1)<=0){
		close(fd);
		return -2;
	}
	len = snprintf(msg, sizeof(msg), "%s\x01a", psms->data);
	LOG_IN("pysms send txt %s len %d", msg, len);
	//write msg
	write_fd(fd, msg, len, 1000, 0);
	if(check_return2(fd, buf, sizeof(buf), "OK", 5000, 1)<=0){
		close(fd);
		return -3;
	}
	close(fd);
	//sms_send_pdu(dev, psms->phone, "hello", strlen("hello"));
	return 0;
}
