#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "ih_ipc.h"
#include "shared.h"
#include "redial_ipc.h"
#include "mbim.h"
#include "modem.h"
#include "ih_vif.h"
#include "redial.h"


#define MBIM_CFG_FILE_PATH	"/etc/mbim-network.conf"
#define MBIM_LOG_FILE  "/tmp/mbim_conn.log"

extern int send_at_cmd_sync_timeout(char *dev, char* cmd, char* rsp, int len, int verbose, char *check_value, int timeout);

char *auth[] = {"CHAP", "PAP", "CHAP"};

int build_mbim_config_file(PPP_CONFIG *info, MODEM_INFO *modem_info)
{
	FILE *fp = NULL;
	unlink(MBIM_CFG_FILE_PATH);

	int profile_idx = 0;
	PPP_PROFILE *p_profile = NULL;

	fp = fopen(MBIM_CFG_FILE_PATH, "w");
	if (!fp) {
		syslog(LOG_ERR, "faild to build mbim config file.");
		return -1;
	}

	if(modem_info->current_sim==SIM1) profile_idx=info->profile_idx;
	else if(modem_info->current_sim==SIM2) profile_idx=info->profile_2nd_idx;
	if(profile_idx == 0) {
		p_profile =  &info->auto_profile;
	}else {
		if(info->profiles[profile_idx-1].type==PROFILE_TYPE_NONE) {
			p_profile =  &info->default_profile;
		} else {
			p_profile = info->profiles + profile_idx -1;
		}
	}

	if (p_profile->apn && p_profile->apn[0]) {
		if(!strcmp(USE_BLANK_APN, p_profile->apn)){ //wucl: actually for blank apn which means profile1 is blank
			fprintf(fp, "APN=\n");//TODO
		}else{
			fprintf(fp, "APN=%s\n", p_profile->apn);//TODO
		}
	}


	if (p_profile->username && p_profile->username[0]) {
		fprintf(fp, "APN_USER=%s\n", p_profile->username);//TODO
		fprintf(fp, "APN_AUTH=%s\n", auth[p_profile->auth_type]);//TODO
	}
	
	if (p_profile->password && p_profile->password[0]) {
		fprintf(fp, "APN_PASS=%s\n", p_profile->password);//TODO
	}

	fprintf(fp, "PROXY=no\n");//TODO
	
	fclose(fp);
	
	return 0;
}

int modem_get_net_params_by_at(char *commdev, BOOL init,  PPP_CONFIG *info, MODEM_INFO *modem_info, VIF_INFO *pinfo, BOOL verbose)
{
    char ret_buf[256] = {0};
	int ret = -1;
	PPP_PROFILE *p_profile = NULL;
	int profile_idx = 0;
	char ip_buf[16] = {0};
	char netmask_buf[16] = {0};
	char dns1_buf[16] = {0};
	char dns2_buf[16] = {0};
	unsigned int  ip[8];
	char *ip_str = NULL;
	char *pos = NULL;
	struct in_addr ipaddr;
    struct in_addr netmask;
    struct in_addr gateway;
	struct in_addr dns1;
	struct in_addr dns2;
	char buf[MAX_SYS_CMD_LEN] = {0};

	if(modem_info->current_sim==SIM1) profile_idx=info->profile_idx;
	else if(modem_info->current_sim==SIM2) profile_idx=info->profile_2nd_idx;

	if(profile_idx == 0) {
		p_profile =  &info->auto_profile;
	}else {
		if(info->profiles[profile_idx-1].type==PROFILE_TYPE_NONE) {
			p_profile =  &info->default_profile;
		} else {
			p_profile = info->profiles + profile_idx -1;
		}
	}

	//
	ret = send_at_cmd_sync_timeout(commdev, "AT+CGCONTRDP\r\n", ret_buf, sizeof(ret_buf), verbose, "OK", 5000);
	if(ret < 0){
		syslog(LOG_ERR, "Failed to execute AT+CGCONTRDP");
		//sleep(1000);
		goto error;
	}

	if ((p_profile->apn && p_profile->apn[0])
			&& (strcmp(USE_BLANK_APN, p_profile->apn))) {
		pos = strcasestr(ret_buf, p_profile->apn);
		if (!pos) {
			goto error;
		}
	} else {
		pos = strstr(ret_buf, "CGCONTRDP:");	
		if (!pos) {
			goto error;
		}

		pos = strchr(pos, '\"');
		if (!pos) {
			goto error;
		}
	}
	
	pos = strchr(pos, ',');
	if (!pos) {
		goto error;
	}

	pos = strchr(pos, '\"');
	if (!pos) {
		goto error;
	}

	pos++;

	ip_str = strsep(&pos, "\"");

	sscanf(ip_str, "%d.%d.%d.%d.%d.%d.%d.%d" , &ip[0], &ip[1], &ip[2], &ip[3], &ip[4], &ip[5], &ip[6], &ip[7]);
	snprintf(ip_buf, sizeof(ip_buf), "%d.%d.%d.%d",ip[0], ip[1], ip[2], ip[3]);
	if (verbose) {
		syslog(LOG_INFO, "local ip:%s", ip_buf);
	}
	inet_aton(ip_buf, &ipaddr);

	snprintf(netmask_buf, sizeof(netmask_buf), "%d.%d.%d.%d",ip[4], ip[5], ip[6], ip[7]);

	if (verbose) {
		syslog(LOG_INFO, "netmask:%s", netmask_buf);
	}

	inet_aton(netmask_buf, &netmask);

	pos = strchr(pos, ',');
	if (!pos) {
		goto error;
	}

	pos += 2;
	
	ip_str = strsep(&pos, "\"");
	if (!ip_str) {
		goto error;
	}

	if (verbose) {
		syslog(LOG_INFO, "gateway:%s", ip_str);
	}
	inet_aton(ip_str, &gateway);

	pos = strchr(pos, ',');
	if (!pos) {
		goto error;
	}

	pos += 2;
	
	ip_str = strsep(&pos, "\"");
	if (!ip_str) {
		goto error;
	}

	snprintf(dns1_buf, sizeof(dns1_buf), "%s", ip_str);
	inet_aton(ip_str, &dns1);
	if (verbose) {
		syslog(LOG_INFO, "dns1:%s", dns1_buf);
	}
	pos = strchr(pos, ',');
	if (!pos) {
		goto error;
	}

	pos += 2;
	
	ip_str = strsep(&pos, "\"");
	if (!ip_str) {
		goto error;
	}

	snprintf(dns2_buf, sizeof(dns2_buf), "%s", ip_str);

	if (verbose) {
		syslog(LOG_INFO, "dns2:%s", dns2_buf);
	}

	inet_aton(ip_str, &dns2);

	if (memcmp(&pinfo->local_ip, &ipaddr, sizeof(struct in_addr))
				|| memcmp(&pinfo->netmask, &netmask, sizeof(struct in_addr))
				|| memcmp(&pinfo->remote_ip, &gateway, sizeof(struct in_addr))
				|| memcmp(&pinfo->dns1, &dns1, sizeof(struct in_addr))
				|| memcmp(&pinfo->dns2, &dns2, sizeof(struct in_addr))) {
		if (!init) {
			eval("ip", "addr", "flush", "dev", USB0_WWAN);
			return -1;
		}

		memcpy(&pinfo->local_ip, &ipaddr, sizeof(struct in_addr));
		memcpy(&pinfo->remote_ip, &gateway, sizeof(struct in_addr));
		memcpy(&pinfo->netmask, &netmask, sizeof(struct in_addr));
		memcpy(&pinfo->dns1, &dns1, sizeof(struct in_addr));
		memcpy(&pinfo->dns2, &dns2, sizeof(struct in_addr));
#if 1
		eval("ip", "addr", "flush", "dev", USB0_WWAN);

		eval("ifconfig", USB0_WWAN, ip_buf, "netmask", netmask_buf);

		eval("ifconfig", USB0_WWAN, "-arp");

		snprintf(buf, sizeof(buf), "echo nameserver %s>>/etc/resolv.dnsmasq", dns1_buf);
		system(buf);

		snprintf(buf, sizeof(buf), "echo nameserver %s>>/etc/resolv.dnsmasq", dns2_buf);
		system(buf);
#endif

	}

	return 0;

error:
	eval("ip", "addr", "flush", "dev", USB0_WWAN);
	return -1;
}

#define MBIM_CONN_SUCCESS_RSP "Network started successfully"
#define MBIM_CONN_FAILED_RSP "Network start failed"
int mbim_start_data_connection_by_cli(char *dev)
{
	char cmd[256] = {0};
	char buf[256] = {0};
	FILE *fp;

	if (!dev) {
		
	}

	alarm(10);
	snprintf(cmd, sizeof(cmd), "mbim-network %s stop", dev);	

	syslog(LOG_INFO, "Stopping mbim-network\n");
	system(cmd);

	sleep(2);

	snprintf(cmd, sizeof(cmd), "mbim-network %s start > %s", dev, MBIM_LOG_FILE);	

	syslog(LOG_INFO, "Starting mbim-network\n");
	system(cmd);

	alarm(0);

	syslog(LOG_INFO, "Reading mbim-network output\n");
	fp = fopen(MBIM_LOG_FILE, "r");
	if (!fp) {
		return -1;
	}

	while(fgets(buf, sizeof(buf), fp)) {
		syslog(LOG_INFO, "mbim-network: %s\n", buf);
		if (strstr(buf, MBIM_CONN_SUCCESS_RSP)) {
			goto success;
		}
	}

	if (fp) {
		fclose(fp);
	}
	
	return -1;

success:
	fclose(fp);
	return 0;
}

int clear_mbim_child_processes(void)
{
	eval("killall", "mbim-network");
	eval("killall", "mbimcli");

	return 0;
}



