#include "shared.h"
#include "shutils.h"
#include "ih_ipc.h"
#include "io_ipc.h"
#include "ih_cmd.h"
#include "dtu_ipc.h"
#include "misc.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "nm_ipc.h"

void clear_conntrack6(void)
{
	eval("conntrack", "-D", "-f", "ipv6");
}

int get_connector_openvpn_client_port()
{
    int ret = 0;
    char cmdbuf[128] = {0};
    FILE *pf = NULL;

    memset(cmdbuf, 0x0, sizeof(cmdbuf));
    snprintf(cmdbuf, sizeof(cmdbuf), "netstat -napu | grep openvpncl | awk '{print $4}' | cut -d ':' -f2");

    if((pf = popen(cmdbuf, "r"))){
        fscanf(pf, "%d", &ret);
        pclose(pf);
    }

    return ret;
}

static int get_connector_openvpn_conntrack_info(CONNTRACK_INFO_TYPE type, int port, char *info, int num)
{
    char cmdbuf[128] = {0};
    FILE *pf = NULL;

    if(!info){
        return -1;
    }

    memset(cmdbuf, 0x0, sizeof(cmdbuf));
    switch(type){
        case SRCIP:
            snprintf(cmdbuf, sizeof(cmdbuf), "cat %s | grep sport=%d | awk 'NR==%d {print $6}' | cut -d '=' -f2", TMP_CONNTRACK_TABLE, port, num);
            break;
        case DSTIP:
            snprintf(cmdbuf, sizeof(cmdbuf), "cat %s | grep sport=%d | awk 'NR==%d {print $7}' | cut -d '=' -f2", TMP_CONNTRACK_TABLE, port, num);
            break;
        case SPORT:
            snprintf(cmdbuf, sizeof(cmdbuf), "cat %s | grep sport=%d | awk 'NR==%d {print $8}' | cut -d '=' -f2", TMP_CONNTRACK_TABLE, port, num);
            break;
        case DPORT:
            snprintf(cmdbuf, sizeof(cmdbuf), "cat %s | grep sport=%d | awk 'NR==%d {print $9}' | cut -d '=' -f2", TMP_CONNTRACK_TABLE, port, num);
            break;
        default:
            break;
    }

    if((pf = popen(cmdbuf, "r"))){
        fscanf(pf, "%s", info);
        pclose(pf);
    }
   
    return 0;
}

static int get_connector_openvpn_conntrack_number(int port)
{
    int num = 0;
    char cmdbuf[128] = {0};
    FILE *pf = NULL;

    memset(cmdbuf, 0x0, sizeof(cmdbuf));
    snprintf(cmdbuf, sizeof(cmdbuf), "cat %s | grep sport=%d | wc -l", TMP_CONNTRACK_TABLE, port);

    if((pf = popen(cmdbuf, "r"))){
        fscanf(pf, "%d", &num);
        pclose(pf);
    }
   
    if(0 == num) num = 1;

    return num;
}

int restore_connector_openvpn_conntrack_info()
{
    int port = 0;
    char srcip[32] = {0};
    char dstip[32] = {0};
    char sport[16] = {0};
    char dport[16] = {0};
    int i = 1;
    int number = 0;

    port = get_connector_openvpn_client_port();

    if(port == 0){
        LOG_DB("can't find connector's openvpn client listen port");
        return -1;
    }

    number = get_connector_openvpn_conntrack_number(port);

    for(i = 1; i <= number; i++ ){
        memset(srcip, 0x0, sizeof(srcip));
        memset(dstip, 0x0, sizeof(dstip));
        memset(sport, 0x0, sizeof(sport));
        memset(dport, 0x0, sizeof(dport));

        get_connector_openvpn_conntrack_info(SRCIP, port, srcip, i);
        get_connector_openvpn_conntrack_info(DSTIP, port, dstip, i);
        get_connector_openvpn_conntrack_info(SPORT, port, sport, i);
        get_connector_openvpn_conntrack_info(DPORT, port, dport, i);
    
        //maybe have status info, not udp data
        if(!strchr(srcip, '.')) continue;

        if(strlen(srcip) && strlen(dstip) && strlen(sport) && strlen(dport)){
            LOG_IN("restore openvpn conntrack number %d: srcip[%s]-dstip[%s]-sport[%s]-dport[%s]",
                i, srcip, dstip, sport, dport);
            
            eval("conntrack", "-I", "-s", srcip, "-d", dstip, "-p", "udp",
                "--sport", sport, "--dport", dport, "--timeout", "120");
        }else{
            LOG_IN("restore openvpn conntrack number %d failed: srcip[%s]-dstip[%s]-sport[%s]-dport[%s]",
                i, srcip, dstip, sport, dport);
        }
    }
    
    return 0;
}

//notice:clear the conntrack table must use this function
void clear_conntrack(void)
{
    if(f_exists(TMP_CONNTRACK_TABLE)){
        LOG_IN("clear conntrack already in commission");
        return;
    }

    eval("cp", "/proc/net/nf_conntrack", TMP_CONNTRACK_TABLE);
    
    eval("conntrack", "-D", "-f", "ipv4");
   
    //restore some conntrack info after clear the conntrack table
    restore_connector_openvpn_conntrack_info();
    unlink(TMP_CONNTRACK_TABLE);
}

void clear_route_cache(void)
{
	eval("ip", "route", "flush", "cache");
}


void set_rp_filter(char *name, int enable)
{
	char s[256];
	
	/*
		block obviously spoofed IP addresses

		rp_filter - BOOLEAN
			1 - do source validation by reversed path, as specified in RFC1812
			    Recommended option for single homed hosts and stub network
			    routers. Could cause troubles for complicated (not loop free)
			    networks running a slow unreliable protocol (sort of RIP),
			    or using static routes.
			0 - No source validation.
	*/
	snprintf(s, sizeof(s), "/proc/sys/net/ipv4/conf/%s/rp_filter", name);
	if (enable)
		f_write_string(s, "1", 0, 0);
	else
		f_write_string(s, "0", 0, 0);	
	
}



void garp(char *name, char * ip)
{
	eval("arping", "-Abq", "-c", "1", "-I", name, ip);
}

int request_io_output_pluse(unsigned int output_idx, unsigned int on, unsigned long int time_ms)
{
	IO_OUTPUT_MSG io_output_msg;
	int ret = 0;

	if(time_ms == 0){
		LOG_IN("IO timer value:%ld is invalid", time_ms);
		return ERR_INVALID;
	}

	if(output_idx != 1){
		LOG_IN("IO index value:%d is invalid", output_idx);
		return ERR_INVALID;
	}
  /*body*/
	io_output_msg.output = output_idx;
	io_output_msg.val = on;
	io_output_msg.time_ms = time_ms;
	io_output_msg.pluse = 1;

	//syslog(LOG_ERR, "^^^^IO event: output%d changes to %d time:%ld, pluse:%d", output_idx, io_output_msg.val, io_output_msg.time_ms, io_output_msg.pluse);
	ret = ih_ipc_send_no_ack(IH_SVC_IO, IPC_MSG_IO_OUTPUT_CONTROL, (void *)&io_output_msg, sizeof(IO_OUTPUT_MSG));
	if(ret != ERR_OK){
		LOG_IN("Send msg failed, ret %d", ret);
		return ret;
	}
	
	return ERR_OK;
}

int request_io_output_set(unsigned int output_idx, unsigned int on)
{
	IO_OUTPUT_MSG io_output_msg;
	int ret = 0;

	if(output_idx != 1){
		LOG_IN("IO index value:%d is invalid", output_idx);
		return ERR_INVALID;
	}

  /*body*/
	io_output_msg.output = output_idx;
	io_output_msg.val = on;
	io_output_msg.time_ms = 0;
	io_output_msg.pluse = 0;

	ret = ih_ipc_send_no_ack(IH_SVC_IO, IPC_MSG_IO_OUTPUT_CONTROL, (void *)&io_output_msg, sizeof(IO_OUTPUT_MSG));
	if(ret != ERR_OK){
		LOG_IN("Send msg failed, ret %d", ret);
		return ret;
	}
	
	return 0;
}

int request_io_output_get(unsigned int output_idx, int *val)
{
	IPC_MSG *rsp = NULL;
	int ret = 0;
	int *p = NULL;

	if(NULL == val){
		return ERR_INVALID;
	}

	ret = ih_ipc_send_wait_rsp(IH_SVC_IO, DEFAULT_CMD_TIMEOUT, IPC_MSG_IO_OUTPUT_GET_VALUE, (char *)&output_idx, sizeof(output_idx), &rsp);
	if (ret) {
		syslog(LOG_INFO, "Request io output val failed[%d]!", ret);
		return ERR_FAILED;
	}

	p = (int *)(rsp->body);
	*val = *p;
	ih_ipc_free_msg(rsp);
	
	return 0;
}

int request_io_input_get(unsigned int input_idx, int *val)
{
	IPC_MSG *rsp = NULL;
	int ret = 0;
	int *p = NULL;

	if(NULL == val){
		return ERR_INVALID;
	}

	ret = ih_ipc_send_wait_rsp(IH_SVC_IO, DEFAULT_CMD_TIMEOUT, IPC_MSG_IO_INPUT_GET_VALUE, (char *)&input_idx, sizeof(input_idx), &rsp);
	if (ret) {
		syslog(LOG_INFO, "Request io input val failed[%d]!", ret);
		return ERR_FAILED;
	}

	p = (int *)(rsp->body);
	*val = *p;
	ih_ipc_free_msg(rsp);
	
	return 0;
}

/*
 * return value: TRUE(start)/FALSE(don't start)
 */
BOOL check_if_start_by_io_ctl(IO_CTL *io_ctl, char *service)
{
	BOOL start = TRUE;
	int in_val, i, j;
	IO_CTL_INPUT *input = NULL;

	if (!io_ctl || !service || !*service) {
		return TRUE;
	}

	LOG_DB("check io input");

	for (i = 0; i < sizeof(io_ctl->input)/sizeof(io_ctl->input[0]); i++) {
		input = &io_ctl->input[i];
		if (!input->idx) {
			continue;
		}

		if (request_io_input_get(input->idx, &in_val) != ERR_OK) {
			continue;
		}

		for (j = 0; j < IO_CTL_SERVICE_MAX; j++) {
			if (!input->service[j].name[0]) {
				continue;
			}

			if (strcmp(input->service[j].name, service)) {
				continue;
			}

			if ((in_val == IO_IN_HIGH && input->service[j].high_action == IO_INPUT_CTL_STOP)
						|| (in_val == IO_IN_LOW && input->service[j].low_action == IO_INPUT_CTL_STOP)) {
				LOG_IN("stop by io control");
				start = FALSE;
			}
			break;
		}
	}

	return start;
}

int io_ctl_update_output_by_if_status(IO_CTL *io_ctl, char *service, BOOL status)
{
	int on = -1;
	int i, j;
	IO_CTL_OUTPUT *output = NULL;

	if (!io_ctl) {
		return ERR_OK;
	}

	LOG_DB("update io output");

	for (i = 0; i < sizeof(io_ctl->output)/sizeof(io_ctl->output[0]); i++) {
		output = &io_ctl->output[i];
		if (!output->idx) {
			continue;
		}

		on = -1;
		for (j = 0; j < IO_CTL_SERVICE_MAX; j++) {
			if (!output->service[j].name[0]) {
				continue;
			}

			if (strcmp(output->service[j].name, service)) {
				continue;
			}

			if (status) {
				if (output->service[j].on_policy == IO_OUTPUT_BY_IF_UP) {
					on = 1;
				} else if (output->service[j].off_policy == IO_OUTPUT_BY_IF_UP){
					on = 0;
				}
			} else {
				if (output->service[j].on_policy == IO_OUTPUT_BY_IF_DOWN) {
					on = 1;
				} else if (output->service[j].off_policy == IO_OUTPUT_BY_IF_DOWN){
					on = 0;
				}
			}

			if (on != -1) {
				LOG_IN("update io output %s", on? "on": "off");
				request_io_output_set(output->idx, on);
			}
			break;
		}
	}

	return ERR_OK;
}

int dtu_updown(int serial_id, char *ip_addr, int port, int status)
{
	char *argv[6];
	char strid[32];
	char strport[32], strstatus[32];
	int ret = -1;

	if(!ip_addr) {
		return ret; 
	}

	syslog(LOG_INFO, "run dtu updown: %d %s %d %d\n", serial_id, ip_addr, port, status);

	sprintf(strid, "%d", serial_id);
	sprintf(strport, "%d", port);
	if(status == DISCONNECT) {
		sprintf(strstatus, "disconnect");
	} else if(status == CONNECT) {
		sprintf(strstatus, "connect");
	} else {
		return ret; 
	}

	argv[0] = "dtu-updown";
	argv[1] = strid;
	argv[2] = ip_addr;
	argv[3] = strport;
	argv[4] = strstatus;
	argv[5] = NULL;

	ret = _eval(argv , NULL, 0, NULL);

	if (ret > 0 || ret == -1) {
		ret = -1;
	} else {
		ret = 0;
	}

	return ret;
}

static void unescape(char *s)
{
	unsigned int c;

	while ((s = strpbrk(s, "%+"))) {
		if (*s == '%') {
			sscanf(s + 1, "%02x", &c);
			*s++ = (char) c;
			strcpy(s, s + 2);
		}else if (*s == '+') {
			*s++ = ' ';
		}
	}
}

char *web_uri_query_find(const char *query, const char *key)
{
	char *query_start;
	char *para_start;
	char *query_value;
	char *start, *end, *name, *value;

	if ((query == NULL) || (key == NULL)) {
		return NULL;
	}
	
	if ((query_start = strstr(query, WEB_URI_QUERY)) != NULL) {
		query_start += strlen(WEB_URI_QUERY);
	}else {
		return NULL;
	}
	
	if (strlen(query_start) == 0) {
		return NULL;
	}

	para_start = (char *)malloc(strlen(query_start)+1);
	if (!para_start) {
		return NULL;
	}
	memset(para_start, 0, strlen(query_start)+1);
	strncpy(para_start, query_start, strlen(query_start));
	
	end = para_start + strlen(para_start);
	start = para_start;
	
	while (strsep(&start, "&"));
	for (start = para_start; start < end; ) {
		value = start;
		start += strlen(start) + 1;
		
		name = strsep(&value, "=");
		if (name && value) {
			unescape(name);
			unescape(value);
			if (!strcmp(name, key)) {
				query_value = (char *)malloc(strlen(value)+1);
				if (!query_value) {
					free(para_start);
					return NULL;
				}
				
				memset(query_value, 0, strlen(value)+1);
				strncpy(query_value, value, strlen(value));
				free(para_start);
				
				return query_value;
			}
		}
	}
	free(para_start);
	
	return NULL;
}

int check_port_interface(char *port)
{
	char cmd[128];
	FILE *fp = NULL;
	char buf[64] = {'\0',};

	if (!port) {
		return -1;
	}

	snprintf(cmd, sizeof(cmd), "netstat -ant 2>/dev/null |grep \"%s\"|grep \"LISTEN\"|awk '{print $4}'", port);
	fp = popen(cmd, "r");
	if(fp == NULL){
		return -1;
	}

	fgets(buf, sizeof(buf), fp);
	pclose(fp);

	if(strstr(buf, port) != NULL){
		return 0;
	}

	return -1;
}

char *get_nm_aws_thing_name(void)
{
	FILE *fp = NULL;
	static char buf[64];

	fp = fopen(NM_THINGNAME_FILE, "r");
	if(NULL == fp){
		syslog(LOG_ERR, "open %s failed.(%d:%s)", NM_THINGNAME_FILE, errno, strerror(errno));
		return NULL;
	}

	fgets(buf, sizeof(buf), fp);
	fclose(fp);

	return buf;
}

