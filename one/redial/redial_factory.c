#include <time.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "ih_ipc.h"
#include "redial_ipc.h"
#include "ih_cmd.h"
#include "shared.h"
#include "ih_vif.h"

#include "modem.h"
#include "redial.h"
#include "redial_factory.h"

enum {
	FACTORY_DIAL_STATE_IDLE,
	FACTORY_DIAL_STATE_RUNNING,
	FACTORY_DIAL_STATE_SUSPEND,
};

typedef struct {
	int timeout;
}DIAL_CONTROL_CFG;
	
typedef struct {
	char imei[32];
	char manufacturer[32];
	char product_model[32];
	char software_version[64];
	char hardware_version[32];
	int active_sim;
}FACTORY_MODEM_INFO;

typedef struct {
	int status;
	int signal;
	char iccid[32];
	int dial_status;
	struct in_addr local_ip;
}FACTORY_SIM_INFO;

typedef struct {
	DIAL_CONTROL_CFG 	dial_cfg;
	FACTORY_MODEM_INFO 	modem;
	FACTORY_SIM_INFO 	sim[2];
}FACTORY_MODEM_TEST;

//-----------------------
extern char *gl_commport;
extern int gl_modem_code;
extern BOOL gl_sim_switch_flag;
extern BOOL gl_force_suspend;
//-----------------------
extern int get_redial_state(void);
extern void enable_dual_sim(void);
extern void change_redial_state(REDIAL_STATE state);
extern int check_modem_basic_info(MODEM_INFO *info);
extern AT_CMD* get_modem_imei_cmd(void);
extern AT_CMD* get_modem_signal_cmd(void);
//-----------------------
int gl_sim_switch_count = 0;
struct event g_ev_factory_modem_tmr;
int gl_factory_dial_state = FACTORY_DIAL_STATE_IDLE;

FACTORY_MODEM_TEST gl_factory = {
	.dial_cfg = {.timeout = 60},
};
//-----------------------

void restart_factory_modem_timer(int timeout)
{
	struct timeval tv = {timeout, 0};
	event_add(&g_ev_factory_modem_tmr, &tv);
}

void stop_factory_modem_timer()
{
	event_del(&g_ev_factory_modem_tmr);
}

int read_modem_imei(void)
{
	AT_CMD *pcmd = NULL;
	char buf[128] = {0};
	MODEM_CONFIG modem_config;
	MODEM_INFO modem_info;
	FACTORY_MODEM_TEST *pinfo = &gl_factory;
	int ret = -1;

	memset(&modem_config, 0, sizeof(MODEM_CONFIG));
	memset(&modem_info, 0, sizeof(MODEM_INFO));

	pcmd = get_modem_imei_cmd();
	
	if(pcmd){
		ret = send_at_cmd_sync(gl_commport, pcmd->atc, buf, sizeof(buf), 0);
		if(ret==0 && pcmd->at_cmd_handle){
			pcmd->at_cmd_handle(pcmd, buf, 0, &modem_config, &modem_info);
		}
	}

	if(modem_info.imei[0]){
		strlcpy(pinfo->modem.imei, modem_info.imei, sizeof(pinfo->modem.imei));
	}

	return ret;
}

int read_modem_version(void)
{
	MODEM_INFO modem_info;
	FACTORY_MODEM_TEST *pinfo = &gl_factory;
	int ret = -1;

	memset(&modem_info, 0, sizeof(MODEM_INFO));

	ret = check_modem_basic_info(&modem_info);

	if(modem_info.manufacturer[0]){
		strlcpy(pinfo->modem.manufacturer, modem_info.manufacturer, sizeof(pinfo->modem.manufacturer));
	}

	if(modem_info.product_model[0]){
		strlcpy(pinfo->modem.product_model, modem_info.product_model, sizeof(pinfo->modem.product_model));
	}

	if(modem_info.software_version[0]){
		strlcpy(pinfo->modem.software_version, modem_info.software_version, sizeof(pinfo->modem.software_version));
	}

	return ret;
}

int read_sim_status(int simid)
{
	AT_CMD *pcmd = NULL;
	char buf[128] = {0};
	MODEM_CONFIG modem_config;
	MODEM_INFO modem_info;
	FACTORY_MODEM_TEST *pinfo = &gl_factory;
	int ret = -1;

	memset(&modem_config, 0, sizeof(MODEM_CONFIG));
	memset(&modem_info, 0, sizeof(MODEM_INFO));
	
	pcmd = find_at_cmd(AT_CMD_CPIN);

	if(pcmd){
		ret = send_at_cmd_sync(gl_commport, pcmd->atc, buf, sizeof(buf), 0);
		if(ret==0 && pcmd->at_cmd_handle){
			pcmd->at_cmd_handle(pcmd, buf, 0, &modem_config, &modem_info);
		}
	}

	if(modem_info.simstatus){
		pinfo->sim[simid-1].status = modem_info.simstatus;
	}

	if(modem_info.iccid[0]){
		strlcpy(pinfo->sim[simid-1].iccid, modem_info.iccid, sizeof(pinfo->sim[pinfo->modem.active_sim-1].iccid));
	}

	return ret;
}

int read_modem_signal(int simid)
{
	AT_CMD *pcmd = NULL;
	char buf[512] = {0};
	MODEM_CONFIG modem_config;
	MODEM_INFO modem_info;
	FACTORY_MODEM_TEST *pinfo = &gl_factory;
	int ret = -1;

	memset(&modem_config, 0, sizeof(MODEM_CONFIG));
	memset(&modem_info, 0, sizeof(MODEM_INFO));
	
	pcmd = get_modem_signal_cmd();

	if(pcmd){
		ret = send_at_cmd_sync(gl_commport, pcmd->atc, buf, sizeof(buf), 0);
		if(ret==0 && pcmd->at_cmd_handle){
			pcmd->at_cmd_handle(pcmd, buf, 0, &modem_config, &modem_info);
		}
	}

	if(modem_info.siglevel){
		pinfo->sim[simid-1].signal = modem_info.siglevel;
	}

	return ret;
}

void redial_handle_enter(REDIAL_STATE *state, MODEM_INFO *modem_info, VIF_INFO *vif_info)
{
	int simid = (modem_info->current_sim == SIM2) ? SIM2 : SIM1;
	FACTORY_MODEM_TEST *pinfo = &gl_factory;
	
	//update active sim	
	pinfo->modem.active_sim = simid;

	if(*state >= REDIAL_STATE_MODEM_READY){
		//update sim status
		pinfo->sim[simid-1].status = modem_info->simstatus;

		//update modem imei
		if(modem_info->imei[0]){
			strlcpy(pinfo->modem.imei, modem_info->imei, sizeof(pinfo->modem.imei));
		}

		//update modem manufacturer
		if(modem_info->manufacturer[0]){
			strlcpy(pinfo->modem.manufacturer, modem_info->manufacturer, sizeof(pinfo->modem.manufacturer));
		}

		//update modem product model
		if(modem_info->product_model[0]){
			strlcpy(pinfo->modem.product_model, modem_info->product_model, sizeof(pinfo->modem.product_model));
		}

		//update modem software version
		if(modem_info->software_version[0]){
			strlcpy(pinfo->modem.software_version, modem_info->software_version, sizeof(pinfo->modem.software_version));
		}

		//update modem hardware version
		if(modem_info->hardware_version[0]){
			strlcpy(pinfo->modem.hardware_version, modem_info->hardware_version, sizeof(pinfo->modem.hardware_version));
		}

		//update sim iccid
		if(modem_info->iccid[0]){
			strlcpy(pinfo->sim[simid-1].iccid, modem_info->iccid, sizeof(pinfo->sim[simid-1].iccid));
		}

		//update sim dial-status
		pinfo->sim[simid-1].dial_status = *state;

		//update sim signal
		pinfo->sim[simid-1].signal = modem_info->siglevel;

		//update local ip
		memcpy(&(pinfo->sim[simid-1].local_ip), &(vif_info->local_ip), sizeof(struct in_addr));
	}
}


void redial_handle_leave(REDIAL_STATE *state, MODEM_INFO *modem_info)
{
	int simid = (modem_info->current_sim == SIM1) ? SIM1 : SIM2;
	FACTORY_MODEM_TEST *pinfo = &gl_factory;

	switch(gl_factory_dial_state){
		case FACTORY_DIAL_STATE_RUNNING:
			if(*state == REDIAL_STATE_SUSPEND){
				//leave suspend
				change_redial_state(REDIAL_STATE_MODEM_STARTUP);
			}else if(*state == REDIAL_STATE_CONNECTED){
				//update dial-status
				pinfo->sim[simid-1].dial_status = *state;

				if(gl_sim_switch_count > 0){
					gl_sim_switch_count--;
					//update signal
					read_modem_signal(simid);
					//stop timer
					stop_factory_modem_timer();
					//switch sim
					enable_dual_sim();
					gl_sim_switch_flag = TRUE;
					change_redial_state(REDIAL_STATE_MODEM_STARTUP);
				}else{
					//finish test
					gl_factory_dial_state = FACTORY_DIAL_STATE_IDLE;
				}
			}
			break;
		case FACTORY_DIAL_STATE_SUSPEND:
			//wait modem ready and change to suspend
			if(*state >= REDIAL_STATE_MODEM_READY){
				change_redial_state(REDIAL_STATE_SUSPEND);
			}
			break;
		case FACTORY_DIAL_STATE_IDLE:
		default:
			if(*state == REDIAL_STATE_SUSPEND && !gl_force_suspend){
				//leave suspend
				change_redial_state(REDIAL_STATE_MODEM_STARTUP);
			}
			break;
	}

	if(*state == REDIAL_STATE_SUSPEND){
		LOG_WA("redial is suspended!!");
	}
}

/*
 * my timer for factory modem test
 */
void my_factory_modem_timer(int fd, short event, void *arg)
{
	if(gl_sim_switch_count > 0){
		gl_sim_switch_count--;
		//switch sim
		enable_dual_sim();
		gl_sim_switch_flag = TRUE;
		change_redial_state(REDIAL_STATE_MODEM_STARTUP);
	}
}

char *dial_status_to_string(int state)
{
	char *p = "unknown";

	if(state == REDIAL_STATE_SUSPEND){
		p = "suspended";
	}else if(state >= REDIAL_STATE_AT && state < REDIAL_STATE_CONNECTING){
		p = "executing at";
	}else if(state == REDIAL_STATE_CONNECTING){
		p = "connecting";
	}else if(state == REDIAL_STATE_CONNECTED){
		p = "connected";
	}

	return p;
}

int my_factory_cmd_handle(IH_CMD *cmd)
{
#define MY_LEAVE(x) {ret_code = x;goto leave;}
	int simid = SIM1;
	int value = 0;
	char *pcmd = NULL, *what = NULL;
	int32 ret_code = IH_ERR_INVALID;
	FACTORY_MODEM_TEST *pinfo = &gl_factory;
	char *sim_status_str[] = {"unknown", "sim pin", "ready"};

	MYTRACE_ENTER();

	pcmd = (cmd->len>0) ? cmd->args : "";

	/* modem <1-1> 
	 * modem <1-1> dial auto-test timeout <30-180>
	 * modem <1-1> dial start|stop|status
	 * modem <1-1> read active sim
	 * modem <1-1> read imei
	 * modem <1-1> read sim <1-2>
	 * modem <1-1> read sim <1-2> dial-status|iccid|signal|status
	 * modem <1-1> read version hw|sw
	 * modem <1-1> switch sim <1-2>
	 */
	if(strcmp(cmd->cmd, "modem") == 0) {		
#ifndef INHAND_ER3
		//read current sim
		gpio(GPIO_READ, GPIO_SIM_SWITCH, &value);
		pinfo->modem.active_sim = value ? SIM1 : SIM2;
#endif
		what = cmdsep(&pcmd);
		what = cmdsep(&pcmd);
		if(!what){
			if(get_redial_state() == REDIAL_STATE_SUSPEND
				|| get_redial_state() == REDIAL_STATE_CONNECTED){
				read_modem_imei();
				read_modem_version();
			}

			ih_cmd_rsp_print("manufacturer: %s\n"
					 "product model: %s\n"
					 "software version: %s\n"
					 "hardware version: %s\n"
					 "imei: %s", 
					 pinfo->modem.manufacturer, 
					 pinfo->modem.product_model,
					 pinfo->modem.software_version,
					 pinfo->modem.hardware_version,
					 pinfo->modem.imei);
			MY_LEAVE(ERR_OK);
		}else if(strncmp(what, "dial", 1) == 0) {
			what = cmdsep(&pcmd);
			if(strncmp(what, "auto-test", 1) == 0){
				what = cmdsep(&pcmd);
				what = cmdsep(&pcmd);
				pinfo->dial_cfg.timeout = atoi(what);

				gl_sim_switch_count = 1;
				restart_factory_modem_timer(pinfo->dial_cfg.timeout);

				if(pinfo->modem.active_sim == SIM1){
					//clear sim2 info
					memset(&(pinfo->sim[1]), 0, sizeof(FACTORY_SIM_INFO));
				}else{
					memset(&(pinfo->sim[0]), 0, sizeof(FACTORY_SIM_INFO));
				}
				gl_factory_dial_state = FACTORY_DIAL_STATE_RUNNING;
				ih_cmd_rsp_print("OK!");
				MY_LEAVE(ERR_OK);
			}else if(strncmp(what, "start", 4) == 0){
				stop_factory_modem_timer();
				change_redial_state(REDIAL_STATE_MODEM_STARTUP);

				memset(&(pinfo->sim[pinfo->modem.active_sim-1]), 0, sizeof(FACTORY_SIM_INFO));
				gl_factory_dial_state = FACTORY_DIAL_STATE_RUNNING;
				ih_cmd_rsp_print("OK!");
				MY_LEAVE(ERR_OK);
			}else if(strncmp(what, "stop", 3) == 0){
				stop_factory_modem_timer();
				if(get_redial_state() > REDIAL_STATE_SUSPEND){
					change_redial_state(REDIAL_STATE_MODEM_STARTUP);
				}
				gl_factory_dial_state = FACTORY_DIAL_STATE_SUSPEND;
				ih_cmd_rsp_print("OK!");
				MY_LEAVE(ERR_OK);
			}else if(strncmp(what, "status", 4) == 0){
				ih_cmd_rsp_print("sim1(%s): %s\n"
						 "sim2(%s): %s", 
						 pinfo->modem.active_sim == SIM1 ? "active" : "inactive",
						 dial_status_to_string(pinfo->sim[0].dial_status),
						 pinfo->modem.active_sim == SIM2 ? "active" : "inactive",
						 dial_status_to_string(pinfo->sim[1].dial_status));
				MY_LEAVE(ERR_OK);
			}
		}else if(strncmp(what, "read", 1) == 0) {
			what = cmdsep(&pcmd);
			if(strncmp(what, "active", 1) == 0){
				ih_cmd_rsp_print("%d", pinfo->modem.active_sim);
				MY_LEAVE(ERR_OK);
			}else if(strncmp(what, "imei", 1) == 0){

				if(get_redial_state() == REDIAL_STATE_SUSPEND
					|| get_redial_state() == REDIAL_STATE_CONNECTED){
					read_modem_imei();
				}

				ih_cmd_rsp_print("%s", pinfo->modem.imei);
				MY_LEAVE(ERR_OK);
			}else if(strncmp(what, "sim", 1) == 0){
				what = cmdsep(&pcmd);
				simid = atoi(what);
				what = cmdsep(&pcmd);

				if(get_redial_state() == REDIAL_STATE_SUSPEND
					|| get_redial_state() == REDIAL_STATE_CONNECTED){
					read_sim_status(pinfo->modem.active_sim);
					read_modem_signal(pinfo->modem.active_sim);
				}

				if(!what){
					ih_cmd_rsp_print("status: %s\n"
							 "iccid: %s\n"
							 "signal: %d\n"
							 "dial-status: %s\n"
							 "ip: %s", 
							 sim_status_str[pinfo->sim[simid-1].status],
							 pinfo->sim[simid-1].iccid, 
							 pinfo->sim[simid-1].signal, 
						 	 dial_status_to_string(pinfo->sim[simid-1].dial_status),
							 pinfo->sim[simid-1].local_ip.s_addr ? inet_ntoa(pinfo->sim[simid-1].local_ip) : "");
				}else if(strncmp(what, "dial-status", 1) == 0){
					ih_cmd_rsp_print("%s", dial_status_to_string(pinfo->sim[simid-1].dial_status));
				}else if(strncmp(what, "iccid", 2) == 0){
					ih_cmd_rsp_print("%s", pinfo->sim[simid-1].iccid);
				}else if(strncmp(what, "signal", 2) == 0){
					ih_cmd_rsp_print("%d", pinfo->sim[simid-1].signal);
				}else if(strncmp(what, "status", 2) == 0){
					ih_cmd_rsp_print("%s", sim_status_str[pinfo->sim[simid-1].status]);
				}else if(strncmp(what, "ip", 2) == 0){
					ih_cmd_rsp_print("%s", pinfo->sim[simid-1].local_ip.s_addr ? inet_ntoa(pinfo->sim[simid-1].local_ip) : "FAIL!");
				}

				MY_LEAVE(ERR_OK);
			}else if(strncmp(what, "version", 1) == 0){
				what = cmdsep(&pcmd);

				if(get_redial_state() == REDIAL_STATE_SUSPEND
					|| get_redial_state() == REDIAL_STATE_CONNECTED){
					read_modem_version();
				}
				
				if(strncmp(what, "hw", 1) == 0){
					ih_cmd_rsp_print("%s", pinfo->modem.hardware_version);
				}else if(strncmp(what, "sw", 1) == 0){
					ih_cmd_rsp_print("%s", pinfo->modem.software_version);
				}
				MY_LEAVE(ERR_OK);
			}
		}else if(strncmp(what, "switch", 1) == 0) {
			what = cmdsep(&pcmd);
			what = cmdsep(&pcmd);
			simid = atoi(what);
			if(pinfo->modem.active_sim != simid){
				enable_dual_sim();
				gl_sim_switch_flag = TRUE;	
				change_redial_state(REDIAL_STATE_MODEM_STARTUP);
			}
			ih_cmd_rsp_print("OK!");
			MY_LEAVE(ERR_OK);
		}
	}

leave:
	
	return ret_code;	
}

