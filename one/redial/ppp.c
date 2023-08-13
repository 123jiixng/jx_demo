/*
 * $Id: $
 * $HeadURL: $
 * Created by: E.g. shandy(shandy98@inhand.com.cn)
 * Maintained by: E.g. shandy(shandy98@inhand.com.cn)
 *
 * Copyright (C) 2001-2012 Inhand Networks Corp.
 *         All rights reserved.
 * Website: www.inhand.com.cn
 *
 * Description: ppp routines
 *
 */


/*
 * History:
 * 2007-10-5 Shandy Created this file. 
 * 2012-01-12 Zhangly Revise this file. 
 */

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

#include "ih_ipc.h"
#include "shared.h"
#include "redial_ipc.h"

#include "ppp.h"
#include "modem.h"

const char dialup_options[] = "/etc/ppp/peers/wan%d";

/*
 *Convert '#' to "\#"
*/
static int escape_ppp_chars(char *n_str, char *o_str)
{
	char *p, *q;
	char r_str[] = "\\#";
	int size=0;

	p = o_str;
	while((q=strchr(p, '#'))) {
		memcpy(n_str+size, p, q-p);
		size += (q-p);
		memcpy(n_str+size, r_str, strlen(r_str));
		size += strlen(r_str);
		//skip char '#'
		p = q+1;
	}
	memcpy(n_str+size, p, strlen(p)+1);//contain '\0'

	return 0;
}

int at_ppp_build_config(char *dev, PPP_CONFIG *info, MODEM_CONFIG *config, MODEM_INFO *modem_info, BOOL debug)
{
	FILE *fp;
	char fname[64], *p, buf[128];
	int accept_local=1; //accept_remote;
	int mode=info->ppp_mode;
	//char *argv[4];
	int proto=PROTO_DIALUP, n=1;
	PPP_PROFILE *p_profile;
	int profile_idx = 0;
	
	//check profile if valid
	if(modem_info->current_sim==SIM1) profile_idx=info->profile_idx;
	else if(modem_info->current_sim==SIM2) profile_idx=info->profile_2nd_idx;
	if(profile_idx == 0){
		p_profile =  &info->auto_profile;
	}else {
		if(info->profiles[profile_idx-1].type==PROFILE_TYPE_NONE) {
			p_profile =  &info->default_profile;
		} else {
			p_profile = info->profiles + profile_idx -1;
		}
	}
	if(proto==PROTO_DIALUP){
		sprintf(fname, "/etc/ppp/wan%d-on-dialer", n);

		fp = fopen(fname, "w");
		if(!fp){
			syslog(LOG_ERR, "failed to start wan%d, cannot write to %s!", n, fname);
			return -1;
		}else{
			fprintf(fp, "ABORT \"NO CARRIER\"\n");
			fprintf(fp, "ABORT \"NO DIALTONE\"\n");
			fprintf(fp, "ABORT \"BUSY\"\n");
			fprintf(fp, "ABORT \"ERROR\"\n");
			fprintf(fp, "ABORT \"NO ANSWER\"\n");
			fprintf(fp, "ABORT \"Username/Password Incorrect\"\n");
			
			fprintf(fp, "TIMEOUT 8\n");
			//fprintf(fp, "\"\" \"\\d+++\\d\"\n");
			//fprintf(fp, "\"\" \"ATH\"\n");
			//fprintf(fp, "\"\" AT+CSQ\n");
			//fprintf(fp, "OK AT\n");
			fprintf(fp, "\"\" AT\n");
			//fprintf(fp, "OK ATZ\n");

			if(info->init_string[0])
				fprintf(fp, "OK %s\n", info->init_string);

			if(p_profile->apn[0] && strcmp(USE_BLANK_APN, p_profile->apn))
				fprintf(fp, "OK AT+CGDCONT=1,\"IP\",\"%s\"\n", p_profile->apn);

			fprintf(fp, "OK ATD%s\n", p_profile->dialno);

			fprintf(fp, "TIMEOUT %d\n", info->timeout);

			fprintf(fp, "CONNECT ''\n");
		
			fclose(fp);
		}

		/*add disconnect script*/
		sprintf(fname, "/etc/ppp/wan%d-off-dialer", n);

		fp = fopen(fname, "w");
		if(!fp){
			syslog(LOG_ERR, "failed to stop wan%d, cannot write to %s!", n, fname);
			return -1;
		}else{
			fprintf(fp, "ABORT \"NO DIALTONE\"\n");
			fprintf(fp, "ABORT \"BUSY\"\n");
			fprintf(fp, "ABORT \"ERROR\"\n");
			//fprintf(fp, "SAY \"Sending break to the modem\"\n");
			fprintf(fp, "\"\" \"\\K\"\n");
			fprintf(fp, "\"\" \"+++ATH\"\n");
			//fprintf(fp, "SAY \"PDP context detached\"\n");

			fclose(fp);
		}
	}

	sprintf(fname, dialup_options, n);

	fp = fopen(fname, "w");
	if(!fp){
		syslog(LOG_ERR, "failed to start wan%d, cannot write to %s!", n, fname);
		return -1;
	}else{
		fprintf(fp, "unit 1\n");//zly: specify pppx
		if(proto==PROTO_DIALUP){
			fprintf(fp, "%s%s\n", (dev[0]=='/') ? "" : "/dev/", dev);
			fprintf(fp, "%s\n", "230400");
		}
		fprintf(fp, "pidfile /var/run/wan%d.pid\n", n);
		fprintf(fp, "noauth\n");
		if(proto==PROTO_DIALUP){
			fprintf(fp, "comlock\n");//zly: lock com by pppd
			fprintf(fp, "crtscts\n");
			fprintf(fp, "noipdefault\n");//added by zly as pppd 2.4.3
		}

		if(proto==PROTO_DIALUP || proto==PROTO_PPPOE){
			fprintf(fp, "nodefaultroute\n");
		}

		//fprintf(fp, "name modem\n");
		/*comment it for +pap*/
		//fprintf(fp, "remotename wan%d\n", n);
		//fprintf(fp, "nodefaultroute\n");
		
		/*
		 * nothing -- asyncmap 0, not escape any ctrl char
		 * asyncmap <n> -- escape some ctrl char
		 * default-asyncmap(-am) -- disable negotiation, escape all ctrl chars
		*/
		/*pppoe did not support -am*/
		if(proto==PROTO_DIALUP && info->default_am){
			fprintf(fp, "-am\n");
		}

		if(mode!=PPP_ONLINE){//dial on demand or manual
			fprintf(fp, "demand\n");
			fprintf(fp, "nopersist\n");
			if(mode==PPP_DEMAND) {
				fprintf(fp, "idle %d\n", info->idle);
	
				if(info->trig_data) fprintf(fp, "local_activate\n");
	
				if(proto==PROTO_DIALUP) {
					if(info->trig_sms) fprintf(fp, "callin_activate\n");
				}
			} else {//manual
				fprintf(fp, "idle 0\n");
			}
		}
		/*set ppp authentication, 0--auto, 1--pap, 2--chap*/
		switch(p_profile->auth_type) {
		case 2:
			fprintf(fp, "-pap\n");
			fprintf(fp, "refuse-eap\n");
			break;
		case 1:
			fprintf(fp, "-chap\n");
			fprintf(fp, "-mschap\n");
			fprintf(fp, "-mschap-v2\n");
			fprintf(fp, "refuse-eap\n");
			break;
		case 0:
		default:
			break;
		}

		if(info->ppp_static){
			p = info->ppp_ip;
			if(*p && strcmp(p, "0.0.0.0")) accept_local = 0;
			else accept_local = 1;
			if(mode==PPP_ONLINE){
				fprintf(fp, "%s", (*p && strcmp(p, "0.0.0.0")) ? p : "");
			}else{
				fprintf(fp, "%s", (*p && strcmp(p, "0.0.0.0")) ? p : "0.0.0.0");
			}
		}
		p = info->ppp_peer;
		fprintf(fp, ":%s\n", (*p && strcmp(p, "0.0.0.0")) ? p : "");

		if(accept_local) fprintf(fp, "ipcp-accept-local\n");
		fprintf(fp, "ipcp-accept-remote\n");

		if(debug) {
			fprintf(fp, "debug\n");
			//if(proto==PROTO_PPPOE) //by zly
				//fprintf(fp, "pppoe_debug 0xffff\n");
		}

//		fprintf(fp, "chap-restart %d\n", i);
//		fprintf(fp, "chap-max-challenge %d\n", k);
//		fprintf(fp, "pap-restart %d\n", i);
//		fprintf(fp, "pap-max-authreq %d\n", k);
//		fprintf(fp, "ipcp-restart %d\n", i);
//		fprintf(fp, "ipcp-max-configure %d\n", k);
		/*useful if use ppp 2.4.3, or can not get dns. by zly*/
		fprintf(fp, "ipcp-restart %d\n", 20);
		fprintf(fp, "ipcp-max-configure %d\n", 20);
		fprintf(fp, "ipcp-max-failure %d\n", 20);
		//fprintf(fp, "lcp-max-terminate 0\n");
//		fprintf(fp, "ipcp-max-failure %d\n", k);
//		fprintf(fp, "lcp-restart %d\n", i);
//		fprintf(fp, "lcp-max-configure %d\n", k);
//		fprintf(fp, "lcp-max-failure %d\n", k);
		
		if(info->peerdns) fprintf(fp, "usepeerdns\n");

		if(info->nopcomp)
			fprintf(fp, "nopcomp\n");  //disable protocol compression
		if(info->noaccomp && proto!=PROTO_PPPOE)
			fprintf(fp, "noaccomp\n"); //disable address&control compression

		fprintf(fp, "nodetach\n");
		if(info->vj) {
			if(info->novjccomp)
				fprintf(fp, "novjccomp\n");
		} else {
			fprintf(fp, "novj\n");
			fprintf(fp, "novjccomp\n");
		}
		fprintf(fp, "hide-password\n");
		fprintf(fp, "ip-up /usr/bin/ppp-up\n");
		fprintf(fp, "ip-down /usr/bin/ppp-down\n");
		fprintf(fp, "ipparam wan%d\n", n);

		if(p_profile->username[0]) {
 			/*ppp option interpret '#', need convert to "\#"*/
			escape_ppp_chars(buf, p_profile->username);
			fprintf(fp, "user %s\n", buf);
		}

		fprintf(fp, "mtu %d\n", info->mtu);

		if(proto!=PROTO_PPPOE){
			fprintf(fp, "mru %d\n", info->mru);
		}

		//fprintf(fp, "lcp-echo-optimize\n");//disable lcp echo when have data flow

		if(info->lcp_echo_interval>0) {
			fprintf(fp, "lcp-echo-interval %d\n", info->lcp_echo_interval);

			if(info->lcp_echo_retries>0)
				fprintf(fp, "lcp-echo-failure %d\n", info->lcp_echo_retries);
		}

		//TODO
		if(info->ppp_comp==PPP_COMP_NONE) {
			fprintf(fp, "noccp\n");
		} else {
			if(info->ppp_comp==PPP_COMP_BSD)
				fprintf(fp, "bsdcomp\n");
			else if(info->ppp_comp==PPP_COMP_DEFLATE)
				fprintf(fp, "deflate\n");
			else if(info->ppp_comp==PPP_COMP_MPPC)
				fprintf(fp, "mppc nodeflate nobsdcomp\n");
				
		}
		//fprintf(fp, "nomppe nomppc nodeflate nobsdcomp\n");

#if 0
		if(proto==PROTO_PPPOE){
			/*if use ppp-2.4.3, comment two above, use one below by zly*/
			fprintf(fp, "plugin /usr/sbin/rp-pppoe.so\n");

			sprintf(nv, "wan%d_ppp_service", n);
			p = nvram_default_get(nv, NULL);
			if(*p) fprintf(fp, "rp_pppoe_service %s\n", p);

			sprintf(nv, "wan%d_ppp_ac", n);
			p = nvram_default_get(nv, NULL);
			if(*p) fprintf(fp, "rp_pppoe_ac %s\n", p);
			fprintf(fp, "wan%d\n", n); //interface name
		}

#endif
		if(info->options[0]) {
			fprintf(fp, "%s\n", info->options);
		}
		
		fclose(fp);
	}

	fp = fopen("/etc/ppp/chap-secrets", "w");
	if(!fp){
		syslog(LOG_ERR, "failed to start wan%d, cannot write to chap secrets file!", n);
		return -1;
	}else{
		fprintf(fp, "#client	server   	secret	IP addresses\n");
		fprintf(fp, "\"%s\"	*	\"%s\"	*\n", p_profile->username, p_profile->password);

		fclose(fp);
	}

	fp = fopen("/etc/ppp/pap-secrets", "w");
	if(!fp){
		syslog(LOG_ERR, "failed to start wan%d, cannot write to pap secrets file!", n);
		return -1;
	}else{
		fprintf(fp, "#client	server   	secret	IP addresses\n");
		fprintf(fp, "\"%s\"	*	\"%s\"	*\n", p_profile->username, p_profile->password);

		fclose(fp);
	}
	
	return 0;
}

int ppp_build_config(char *dev, PPP_CONFIG *info, MODEM_CONFIG *config, MODEM_INFO *modem_info, int modem_code, BOOL debug)
{
	FILE *fp;
	char fname[64], *p, buf[128];
	int accept_local=1; //accept_remote;
	int mode=info->ppp_mode;
	//char *argv[4];
	int proto=PROTO_DIALUP, n=1;
	PPP_PROFILE *p_profile;
	int profile_idx = 0;
	
	//check profile if valid
	if(modem_info->current_sim==SIM1) profile_idx=info->profile_idx;
	else if(modem_info->current_sim==SIM2) profile_idx=info->profile_2nd_idx;	
	LOG_IN("profile_idx:%d", profile_idx);
	if(profile_idx == 0){
		p_profile =  &info->auto_profile;
	}else {
		if(info->profiles[profile_idx-1].type==PROFILE_TYPE_NONE) {
			p_profile =  &info->default_profile;
		} else {
			p_profile = info->profiles + profile_idx -1;
		}
	}

	if(proto==PROTO_DIALUP){
		sprintf(fname, "/etc/ppp/wan%d-on-dialer", n);

		fp = fopen(fname, "w");
		if(!fp){
			syslog(LOG_ERR, "failed to start wan%d, cannot write to %s!", n, fname);
			return -1;
		}else{
			fprintf(fp, "ABORT \"NO CARRIER\"\n");
			fprintf(fp, "ABORT \"NO DIALTONE\"\n");
			fprintf(fp, "ABORT \"BUSY\"\n");
			fprintf(fp, "ABORT \"ERROR\"\n");
			fprintf(fp, "ABORT \"NO ANSWER\"\n");
			fprintf(fp, "ABORT \"Username/Password Incorrect\"\n");
			
			fprintf(fp, "TIMEOUT 8\n");
			//fprintf(fp, "\"\" \"\\d+++\\d\"\n");
			//fprintf(fp, "\"\" \"ATH\"\n");
			//fprintf(fp, "\"\" AT+CSQ\n");
			//fprintf(fp, "OK AT\n");
			fprintf(fp, "\"\" AT\n");
			//fprintf(fp, "OK ATZ\n");

			if(info->init_string[0])
				fprintf(fp, "OK %s\n", info->init_string);

			if(p_profile->apn[0]) {
				switch (modem_code) {
				  case IH_FEATURE_MODEM_LE910:
				  case IH_FEATURE_MODEM_ELS31:
					  fprintf(fp, "OK AT+CGDCONT=3,\"IPV4V6\",\"%s\"\n", p_profile->apn);
					  break;
				  case IH_FEATURE_MODEM_U8300:
				  case IH_FEATURE_MODEM_U8300C: //liyb:区分默认承载
						  if(strcmp(p_profile->dialno, "*99***2#") == 0) {
							  fprintf(fp, "OK AT+CGDCONT=2,\"IP\",\"%s\"\n", p_profile->apn);
						  }else {
							  if(strcmp(USE_BLANK_APN, p_profile->apn)){
								  fprintf(fp, "OK AT+CGDCONT=1,\"IP\",\"%s\"\n", p_profile->apn);
							  }
						  } 
					  break;
				  case IH_FEATURE_MODEM_ELS61:
				  case IH_FEATURE_MODEM_LARAR2:
				  case IH_FEATURE_MODEM_TOBYL201:
					  break;
				  default:
					  fprintf(fp, "OK AT+CGDCONT=1,\"IP\",\"%s\"\n", p_profile->apn);
					  break;
				}
			}

			fprintf(fp, "OK ATD%s\n", p_profile->dialno);

			fprintf(fp, "TIMEOUT %d\n", info->timeout);

			fprintf(fp, "CONNECT ''\n");
		
			fclose(fp);
		}

		/*add disconnect script*/
		sprintf(fname, "/etc/ppp/wan%d-off-dialer", n);

		fp = fopen(fname, "w");
		if(!fp){
			syslog(LOG_ERR, "failed to stop wan%d, cannot write to %s!", n, fname);
			return -1;
		}else{
			fprintf(fp, "ABORT \"NO DIALTONE\"\n");
			fprintf(fp, "ABORT \"BUSY\"\n");
			fprintf(fp, "ABORT \"ERROR\"\n");
			//fprintf(fp, "SAY \"Sending break to the modem\"\n");
			fprintf(fp, "\"\" \"\\K\"\n");
			fprintf(fp, "\"\" \"+++ATH\"\n");
			//fprintf(fp, "SAY \"PDP context detached\"\n");

			fclose(fp);
		}
	}

	sprintf(fname, dialup_options, n);

	fp = fopen(fname, "w");
	if(!fp){
		syslog(LOG_ERR, "failed to start wan%d, cannot write to %s!", n, fname);
		return -1;
	}else{
		fprintf(fp, "unit 1\n");//zly: specify pppx
		if(proto==PROTO_DIALUP){
			fprintf(fp, "%s%s\n", (dev[0]=='/') ? "" : "/dev/", dev);
			fprintf(fp, "%s\n", "230400");
		}
		if(proto==PROTO_DIALUP) {
			fprintf(fp, "connect '/usr/sbin/chat -v -t 10 -f /etc/ppp/wan%d-on-dialer'\n", n);
			fprintf(fp, "disconnect '/usr/sbin/chat -v -f /etc/ppp/wan%d-off-dialer'\n", n);
		}
		fprintf(fp, "pidfile /var/run/wan%d.pid\n", n);
		fprintf(fp, "noauth\n");
		if(proto==PROTO_DIALUP){
			fprintf(fp, "comlock\n");//zly: lock com by pppd
			fprintf(fp, "crtscts\n");
			fprintf(fp, "noipdefault\n");//added by zly as pppd 2.4.3
		}

		if(proto==PROTO_DIALUP || proto==PROTO_PPPOE){
			fprintf(fp, "nodefaultroute\n");
		}

		//fprintf(fp, "name modem\n");
		/*comment it for +pap*/
		//fprintf(fp, "remotename wan%d\n", n);
		//fprintf(fp, "nodefaultroute\n");
		
		/*
		 * nothing -- asyncmap 0, not escape any ctrl char
		 * asyncmap <n> -- escape some ctrl char
		 * default-asyncmap(-am) -- disable negotiation, escape all ctrl chars
		*/
		/*pppoe did not support -am*/
		if(proto==PROTO_DIALUP && info->default_am){
			fprintf(fp, "-am\n");
		}

		if(mode!=PPP_ONLINE){//dial on demand or manual
			fprintf(fp, "demand\n");
			fprintf(fp, "nopersist\n");
			if(mode==PPP_DEMAND) {
				fprintf(fp, "idle %d\n", info->idle);
	
				if(info->trig_data) fprintf(fp, "local_activate\n");
	
				if(proto==PROTO_DIALUP) {
					if(info->trig_sms) fprintf(fp, "callin_activate\n");
				}
			} else {//manual
				fprintf(fp, "idle 0\n");
			}
		}
		/*set ppp authentication, 0--auto, 1--pap, 2--chap*/
		switch(p_profile->auth_type) {
		case 2:
			fprintf(fp, "-pap\n");
			fprintf(fp, "refuse-eap\n");
			break;
		case 1:
			fprintf(fp, "-chap\n");
			fprintf(fp, "-mschap\n");
			fprintf(fp, "-mschap-v2\n");
			fprintf(fp, "refuse-eap\n");
			break;
		case 0:
		default:
			break;
		}

		if(info->ppp_static){
			p = info->ppp_ip;
			if(*p && strcmp(p, "0.0.0.0")) accept_local = 0;
			else accept_local = 1;
			if(mode==PPP_ONLINE){
				fprintf(fp, "%s", (*p && strcmp(p, "0.0.0.0")) ? p : "");
			}else{
				fprintf(fp, "%s", (*p && strcmp(p, "0.0.0.0")) ? p : "0.0.0.0");
			}
		}
		p = info->ppp_peer;
		fprintf(fp, ":%s\n", (*p && strcmp(p, "0.0.0.0")) ? p : "");

		if(accept_local) fprintf(fp, "ipcp-accept-local\n");
		fprintf(fp, "ipcp-accept-remote\n");

		if(debug) {
			fprintf(fp, "debug\n");
			//if(proto==PROTO_PPPOE) //by zly
				//fprintf(fp, "pppoe_debug 0xffff\n");
		}

//		fprintf(fp, "chap-restart %d\n", i);
//		fprintf(fp, "chap-max-challenge %d\n", k);
//		fprintf(fp, "pap-restart %d\n", i);
//		fprintf(fp, "pap-max-authreq %d\n", k);
//		fprintf(fp, "ipcp-restart %d\n", i);
//		fprintf(fp, "ipcp-max-configure %d\n", k);
		/*useful if use ppp 2.4.3, or can not get dns. by zly*/
		fprintf(fp, "ipcp-restart %d\n", 20);
		fprintf(fp, "ipcp-max-configure %d\n", 20);
		fprintf(fp, "ipcp-max-failure %d\n", 20);
		//fprintf(fp, "lcp-max-terminate 0\n");
//		fprintf(fp, "ipcp-max-failure %d\n", k);
//		fprintf(fp, "lcp-restart %d\n", i);
//		fprintf(fp, "lcp-max-configure %d\n", k);
//		fprintf(fp, "lcp-max-failure %d\n", k);
		
		if(info->peerdns) fprintf(fp, "usepeerdns\n");

		if(info->nopcomp)
			fprintf(fp, "nopcomp\n");  //disable protocol compression
		if(info->noaccomp && proto!=PROTO_PPPOE)
			fprintf(fp, "noaccomp\n"); //disable address&control compression

		fprintf(fp, "nodetach\n");
		if(info->vj) {
			if(info->novjccomp)
				fprintf(fp, "novjccomp\n");
		} else {
			fprintf(fp, "novj\n");
			fprintf(fp, "novjccomp\n");
		}
		fprintf(fp, "hide-password\n");
		fprintf(fp, "ip-up /usr/bin/ppp-up\n");
		fprintf(fp, "ip-down /usr/bin/ppp-down\n");
		fprintf(fp, "ipparam wan%d\n", n);

		if(p_profile->username[0]) {
 			/*ppp option interpret '#', need convert to "\#"*/
			escape_ppp_chars(buf, p_profile->username);
			fprintf(fp, "user %s\n", buf);
		}

		fprintf(fp, "mtu %d\n", info->mtu);

		if(proto!=PROTO_PPPOE){
			fprintf(fp, "mru %d\n", info->mru);
		}

		//fprintf(fp, "lcp-echo-optimize\n");//disable lcp echo when have data flow

		if(info->lcp_echo_interval>0) {
			fprintf(fp, "lcp-echo-interval %d\n", info->lcp_echo_interval);

			if(info->lcp_echo_retries>0)
				fprintf(fp, "lcp-echo-failure %d\n", info->lcp_echo_retries);
		}

		//TODO
		if(info->ppp_comp==PPP_COMP_NONE) {
			fprintf(fp, "nomppe nomppc nodeflate nobsdcomp noccp\n");
		} else {
			if(info->ppp_comp==PPP_COMP_BSD)
				fprintf(fp, "bsdcomp\n");
			else if(info->ppp_comp==PPP_COMP_DEFLATE)
				fprintf(fp, "deflate\n");
			else if(info->ppp_comp==PPP_COMP_MPPC)
				fprintf(fp, "mppc nodeflate nobsdcomp\n");
				
		}
		//fprintf(fp, "nomppe nomppc nodeflate nobsdcomp\n");

#if 0
		if(proto==PROTO_PPPOE){
			/*if use ppp-2.4.3, comment two above, use one below by zly*/
			fprintf(fp, "plugin /usr/sbin/rp-pppoe.so\n");

			sprintf(nv, "wan%d_ppp_service", n);
			p = nvram_default_get(nv, NULL);
			if(*p) fprintf(fp, "rp_pppoe_service %s\n", p);

			sprintf(nv, "wan%d_ppp_ac", n);
			p = nvram_default_get(nv, NULL);
			if(*p) fprintf(fp, "rp_pppoe_ac %s\n", p);
			fprintf(fp, "wan%d\n", n); //interface name
		}

#endif
		if(info->options[0]) {
			fprintf(fp, "%s\n", info->options);
		}
		
		fclose(fp);
	}

	fp = fopen("/etc/ppp/chap-secrets", "w");
	if(!fp){
		syslog(LOG_ERR, "failed to start wan%d, cannot write to chap secrets file!", n);
		return -1;
	}else{
		fprintf(fp, "#client	server   	secret	IP addresses\n");
		fprintf(fp, "\"%s\"	*	\"%s\"	*\n", p_profile->username, p_profile->password);

		fclose(fp);
	}

	fp = fopen("/etc/ppp/pap-secrets", "w");
	if(!fp){
		syslog(LOG_ERR, "failed to start wan%d, cannot write to pap secrets file!", n);
		return -1;
	}else{
		fprintf(fp, "#client	server   	secret	IP addresses\n");
		fprintf(fp, "\"%s\"	*	\"%s\"	*\n", p_profile->username, p_profile->password);

		fclose(fp);
	}
	
	return 0;
}

#if 0
/*
 *	argv: program interface tty speed local-ip remote-ip ipparam dns1 dns2
 *  ipparam: wanX, interface name
 */
int ipup_main(int argc, char **argv)
{
//	char *wan_ifname;
//	char *wan_proto;
	char *value;
	char buf[256];
	const char *p;
	char *wan, *iface, *local, *peer;
	char *dns1, *dns2;
	char nv[64];
	int idx;

	if(argc<7){
		syslog(LOG_ERR, "bad call to ipup, expecting 7 args instead of %d", argc);
		return -1;
	}

	iface = argv[1];
	local = argv[4];
	peer = argv[5];
	wan = argv[6];
	dns1 = argc>7 ? argv[7] : "";
	dns2 = argc>8 ? argv[8] : "";

	syslog(LOG_INFO, "%s up: %s %s <=> %s, dns: %s,%s", wan, iface, local, peer, dns1, dns2);

	idx = atoi(wan+3);

//	if (!wait_action_idle(10)) return -1;

	/*clear modem status*/
	sprintf(nv, "_%s_modem_status", wan);
	nvram_set(nv, "ok");

	sprintf(nv, "_%s_name", wan);
	nvram_set(nv, iface);	// ppp#

	sprintf(nv, "_%s_get_ip", wan);
	nvram_set(nv, local);

	sprintf(nv, "_%s_get_netmask", wan);
	nvram_set(nv, "255.255.255.255");

	sprintf(nv, "_%s_get_gateway", wan);
	nvram_set(nv, peer);

//	f_write_string("/tmp/ppp/link", argv[1], 0, 0);

	buf[0] = 0;
	if ((p = getenv("DNS1")) != NULL) strlcpy(buf, p, sizeof(buf));
	if ((p = getenv("DNS2")) != NULL) {
		if (buf[0]) strlcat(buf, " ", sizeof(buf));
		strlcat(buf, p, sizeof(buf));
	}
	if(!*buf){
		if(*dns1) strlcpy(buf, dns1, sizeof(buf));
		if(*dns2 && buf[0]) strlcat(buf, " ", sizeof(buf));
		if(*dns2) strlcat(buf, dns2, sizeof(buf));
	}

	sprintf(nv, "_%s_get_dns", wan);
	nvram_set(nv, buf);
	syslog(LOG_INFO, "%s get dns: %s", wan, buf);

	if ((value = getenv("AC_NAME"))){
		sprintf(nv, "_%s_get_ac", wan);
		nvram_set(nv, value);
	}
	if ((value = getenv("SRV_NAME"))){
		sprintf(nv, "_%s_get_srv", wan);
		nvram_set(nv, value);
	}

	if ((value = getenv("MTU"))){
		sprintf(nv, "_%s_run_mtu", wan);
		nvram_set(nv, value);
	}

	//FIXME: shandy, todo
	//eval("ip", "route", "del", "default");
	//eval("ip", "route", "add", "default", "dev", iface);

	sprintf(nv, "%s_ppp_txql", wan);
	strlcpy(buf, nvram_default_get(nv, NULL), sizeof(buf));
	eval("ip", "link", "set", "dev", iface, "txqueuelen", buf);

	start_wan_done(wan);
//	sprintf(nv,"up%s-start",wan);//zly
//	set_service_action(nv);
	syslog(LOG_INFO, "Clear connection table in ppp up...");
	clear_conntrack();

#if HAVE_TRAFFIC
        record_wan_traffic(TRAFFIC_FLAG_ONLINE, idx);
#endif

	return 0;
}

/*
 *	argv: program interface tty speed local-ip remote-ip ipparam
 *  ipparam: wanX, interface name
 */
int ipdown_main(int argc, char **argv)
{
	char *wan, *iface, *local, *peer, *value;
	char nv[64];
	int idx;
	
	if(argc<7){
		syslog(LOG_ERR, "bad call to ipup, expecting 7 args instead of %d", argc);
		return -1;
	}
	
	//set_vrrpd_state(VRRPD_STATE_SLAVE);

	iface = argv[1];
	local = argv[4];
	peer = argv[5];
	wan = argv[6];

	idx = atoi(wan+3);

	syslog(LOG_INFO, "%s down: %s %s <=> %s", wan, iface, local, peer);

	if ((value = getenv("CONNECT_TIME"))){
		sprintf(nv, "_%s_connect_time", wan);
		nvram_set(nv, value);
	}
	if ((value = getenv("BYTES_SENT"))){
		sprintf(nv, "_%s_bytes_sent", wan);
		nvram_set(nv, value);
	}
	if ((value = getenv("BYTES_RCVD"))){
		sprintf(nv, "_%s_bytes_rcvd", wan);
		nvram_set(nv, value);
	}

	stop_wan_done(wan);

#if HAVE_TRAFFIC
        record_wan_traffic(TRAFFIC_FLAG_OFFLINE, idx);
#endif
/*
	ledcon(LEDMAN_CMD_ON, LED_WARN);

//	if (!wait_action_idle(10)) return -1;

	sprintf(nv, "_%s_name", wan);
	nvram_unset(nv);

	sprintf(nv, "_%s_get_ip", wan);
	nvram_unset(nv);
	
	sprintf(nv, "_%s_get_netmask", wan);
	nvram_unset(nv);
	
	sprintf(nv, "_%s_get_gateway", wan);
	nvram_unset(nv);

	sprintf(nv, "_%s_get_dns", wan);
	nvram_unset(nv);

#if HAVE_NETWATCHER
	stop_netwatcher(wan);
#endif//HAVE_NETWATCHER

#if HAVE_DDNS
	stop_ddns(idx);	// avoid to trigger DOD
#endif//HAVE_DDNS

#if HAVE_NTPC
	stop_ntpc();
#endif//HAVE_NTPC

//	unlink("/tmp/ppp/link");
*/
	return 0;
}


/*
 *	argv: program interface tty speed local-ip remote-ip ipparam dns1 dns2
 *  ipparam: tunnelname(= "pptpc" or "l2tpc" + name)
 */
int vpn_ipup_main(int argc, char **argv)
{
	char *iface, *local, *peer;
	char nv[64];
	char *name;

	if(argc<7){
		syslog(LOG_ERR, "bad call to ipup, expecting 7 args instead of %d", argc);
		return -1;
	}

	iface = argv[1];
	local = argv[4];
	peer = argv[5];
	//ipparam = argv[6];
	name = argv[6];
	if(!name) return 0;
//	if(strcmp(tunnel_name, "xl2tpd")==0) {
//	}
//	else if(strcmp(tunnel_name, "pptpd")==0) {
//	}
//	else {
//	}
	
	//syslog(LOG_INFO, "%s up: %s <=> %s, get ipparam: %s", iface, local, peer, ipparam);

	/*save some parameters*/
	sprintf(nv, "_%s_iface", name);
	nvram_set(nv, iface);	// ppp#
	sprintf(nv, "_%s_ip", name);
	nvram_set(nv, local);
	sprintf(nv, "_%s_peerip", name);
	nvram_set(nv, peer);
	sprintf(nv, "_%s_status", name);
	nvram_set(nv, "1");
	/*set up time*/
	set_link_uptime(name, get_uptime());
	/*add static route*/
	do_static_routes(1);
	/*add nat*/
	reload_firewall();
	/*clear conntrack table*/
	//sleep(2);
	syslog(LOG_INFO, "Clear connection table in vpn up...");
	//eval("conntrack", "-F");//add by zly
	clear_conntrack();

	return 0;
}

/*
 *	argv: program interface tty speed local-ip remote-ip ipparam dns1 dns2
 *  ipparam: tunnelname 
 */
int vpn_ipdown_main(int argc, char **argv)
{
	char *iface;
	char nv[64];
	char *name;
	
	if(argc<7){
		syslog(LOG_ERR, "bad call to ipup, expecting 7 args instead of %d", argc);
		return -1;
	}

	iface = argv[1];
	//ipparam = argv[6];
	name = argv[6];

	sprintf(nv, "_%s_iface", name);
	nvram_unset(nv);
	sprintf(nv, "_%s_ip", name);
	nvram_unset(nv);
	sprintf(nv, "_%s_status", name);
	nvram_set(nv, "0");

	reload_firewall();

	/*clear conntrack table*/
	//sleep(2);
	syslog(LOG_INFO, "Clear connection table in vpn down...");
	//eval("conntrack", "-F");//add by zly
	clear_conntrack();

	//syslog(LOG_INFO, "%s down: %s <=> %s", iface, local, peer);
	return 0;
}

/*
 *	argv: program interface tty speed local-ip remote-ip ipparam dns1 dns2
 *  ipparam: pptpd or l2tpd
 */
int vpns_ipup_main(int argc, char **argv)
{
	char *iface, *local, *peer, *name;
	char *p, *prev, *next;
	char server_route[128];
	int found = 0;

	iface = argv[1];
	local = argv[4];
	peer = argv[5];
	name = argv[6];

	syslog(LOG_INFO, "%s -- %s up: %s <=> %s", name, iface, local, peer);
	/*add static route*/
	if(strcmp(name, "pptpd")==0)
		strlcpy(server_route, nvram_safe_get("pptps_route"), sizeof(server_route));
	else if(strcmp(name, "l2tpd")==0)
		strlcpy(server_route, nvram_safe_get("l2tps_route"), sizeof(server_route));
	else
		return 0;
	/*format: ip1:route1_1,route1_2;ip2:route2_1,route2_2*/
	if(server_route[0]) {
		p = server_route;
                p = strtok(p, ";");
                while(p) {
			next = p;
			prev = strsep(&next, ":");//ip1:route1
			//syslog(LOG_INFO, "prev=%s, next=%s", prev, next);
			if(strcmp(prev, peer)==0) {
				found = 1;
				break;
			}
                        p = strtok(NULL,";");
                }
		if(found && next) {
			/*parse route1_1,route1_2*/
			p = next;
                	p = strtok(p, ",");
                	while(p) {
				eval("ip", "route", "add", "dev", iface, p);
                        	p = strtok(NULL, ",");
			}
		}
	}

	return 0;
}

/*
 *	argv: program interface tty speed local-ip remote-ip ipparam dns1 dns2
 *  ipparam: pptpd or l2tpd
 */
int vpns_ipdown_main(int argc, char **argv)
{
	char *iface, *local, *peer, *name;

	iface = argv[1];
	local = argv[4];
	peer = argv[5];
	name = argv[6];

	syslog(LOG_INFO, "%s -- %s down: %s <=> %s", name, iface, local, peer);

	return 0;
}

/*
 *	argv: program tunnelname interface tun-mtu link-mtu local-ip remote-ip
 */
int openvpn_ipup_main(int argc, char **argv)
{
	char *name, *iface, *tun_mtu, *link_mtu, *local, *peer;
	char nv[32];

	if(argc<7){
		syslog(LOG_ERR, "bad call to openvpn ipup, expecting 7 args instead of %d", argc);
		return -1;
	}
	name = argv[1];
	iface = argv[2];
	tun_mtu = argv[3];
	link_mtu = argv[4];
	local = argv[5];
	peer = argv[6];

	syslog(LOG_INFO, "tunnel(%s),%s up: %s <=> %s, tun mtu:%s, link mtu:%s", name, iface, local, peer, tun_mtu, link_mtu);

	/*save some parameters*/
	sprintf(nv, "_openvpn_%s_iface", name);
	nvram_set(nv, iface);
	sprintf(nv, "_openvpn_%s_ip", name);
	nvram_set(nv, local);
	//sprintf(nv, "_%s_peerip", name);
	//nvram_set(nv, peer);
	sprintf(nv, "_openvpn_%s_status", name);
	nvram_set(nv, "1");
	/*set up time*/
	sprintf(nv, "openvpn_%s", name);
	set_link_uptime(nv, get_uptime());
	/*add nat*/
	reload_firewall();
	/*clear conntrack table*/
	//sleep(2);
	syslog(LOG_INFO, "Clear connection table in openvpn up...");
	//eval("conntrack", "-F");//add by zly
	clear_conntrack();

	return 0;
}

/*
 *	argv: program tunnelname interface tun-mtu link-mtu local-ip remote-ip
 */
int openvpn_ipdown_main(int argc, char **argv)
{
	char *name, *iface, *tun_mtu, *link_mtu, *local, *peer;
	char nv[32];

	if(argc<7){
		syslog(LOG_ERR, "bad call to openvpn ipdown, expecting 7 args instead of %d", argc);
		return -1;
	}
	name = argv[1];
	iface = argv[2];
	tun_mtu = argv[3];
	link_mtu = argv[4];
	local = argv[5];
	peer = argv[6];

	syslog(LOG_INFO, "tunnel(%s),%s down: %s <=> %s, tun mtu:%s, link mtu:%s", name, iface, local, peer, tun_mtu, link_mtu);

	sprintf(nv, "_openvpn_%s_iface", name);
	nvram_unset(nv);
	sprintf(nv, "_openvpn_%s_ip", name);
	nvram_unset(nv);
	sprintf(nv, "_openvpn_%s_status", name);
	nvram_set(nv, "0");
	sprintf(nv, "_openvpn_%s_uptime", name);
	nvram_unset(nv);
	/*del nat*/
	reload_firewall();
	/*clear conntrack table*/
	//sleep(2);
	syslog(LOG_INFO, "Clear connection table in openvpn down...");
	//eval("conntrack", "-F");//add by zly
	clear_conntrack();

	return 0;
}
#endif

