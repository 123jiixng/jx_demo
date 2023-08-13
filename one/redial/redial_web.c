#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <jansson.h>

#include "if_common.h"
#include "redial.h"
#include "redial_ipc.h"
#include "operator.h"
#include "ih_web.h"
#include "redial_web.h"

static struct event gl_mosq_tmr;
SUB_TOPIC_INFO gl_sub_info = {'\0'};

extern MY_REDIAL_INFO gl_myinfo;
extern int gl_modem_idx;
extern pid_t gl_ppp_pid;
extern REDIAL_DIAL_STATE gl_redial_dial_state;
extern char *gl_commport;
extern BOOL gl_sim_switch_flag;
extern int gl_ppp_redial_cnt;

extern uint64_t get_modem_code(int idx);
extern int get_sub_deamon_type(int idx); 
extern int32 stop_child_by_sub_daemon(void);
extern void my_restart_timer(int timeout);
extern int modem_enter_airplane_mode(char *commdev); 
extern void dualsim_handle_backuptimer(void);

static char *show_modem_status_to_json(char *query)
{
	MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;
	char *nets[] = {"","2G","3G","4G"};
	char *dualsim[] = {"","SIM 1","SIM 2"};
	char operator[BRAND_STR_SIZ] = {0};
	json_t *results = NULL;
	json_t *root  = NULL;
	char* s_repon = NULL;
	
	if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_PLS8
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS61
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS81
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_LARAR2
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ME909
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_TOBYL201
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_WPD200){
		if (info->carrier_str[0]) {
			memcpy(operator, info->carrier_str, sizeof(info->carrier_str));
		}
	}else {
		if(info->carrier_code) {
			find_operator(info->carrier_code,operator);
		}
	}

	//dbm, submode_name, nai_no, nai_pwd, band
	root = json_object();
	json_object_set_new(root, "active_sim", json_string(dualsim[info->current_sim]));
	json_object_set_new(root, "imei_code", json_string(info->imei));
	json_object_set_new(root, "imsi_code", json_string(info->imsi));
	json_object_set_new(root, "iccid_code", json_string(info->iccid));
	json_object_set_new(root, "phone_number", json_string(info->phonenum));
	json_object_set_new(root, "signal_level", json_integer(info->siglevel));
	json_object_set_new(root, "dbm", json_integer(info->dbm));
	json_object_set_new(root, "rerp", json_integer(info->rsrp));
	json_object_set_new(root, "rerq", json_integer(info->rsrq));
	json_object_set_new(root, "register_status", json_integer(info->regstatus));
	json_object_set_new(root, "operator", json_string(operator));
	json_object_set_new(root, "apns", json_string(info->apns));
	json_object_set_new(root, "network_type", json_string(nets[info->network]));
	json_object_set_new(root, "lac", json_string(info->lac));
	json_object_set_new(root, "cell_id", json_string(info->cellid));

	results = json_object();
	json_object_set_new(results, "results", root);
	s_repon = json_dumps(results, JSON_INDENT(0));

	json_decref(results);
	json_decref(root);

	return s_repon;
}

static char *show_network_status_to_json(char *query)
{
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	PPP_CONFIG *ppp_config = &gl_myinfo.priv.ppp_config;
	char dns_temp[64] = {0};
	json_t *results = NULL;
	json_t *array  = NULL;
	json_t *array_item  = NULL;
	char* s_repon = NULL;

	array = json_array();
	if (get_modem_code(gl_modem_idx) != IH_FEATURE_MODEM_NONE) {
		array_item = json_object();
		json_object_set_new(array_item, "status", json_integer(pinfo->status == VIF_UP));
		if(pinfo->status == VIF_UP) {
			json_object_set_new(array_item, "ip_addr", json_string(inet_ntoa(pinfo->local_ip)));
			json_object_set_new(array_item, "netmask", json_string(inet_ntoa(pinfo->netmask)));
			json_object_set_new(array_item, "gateway", json_string(inet_ntoa(pinfo->remote_ip)));
			sprintf(dns_temp, "%s ", inet_ntoa(pinfo->dns1));
			strcat(dns_temp, pinfo->dns2.s_addr?inet_ntoa(pinfo->dns2):"");
			json_object_set_new(array_item, "dns", json_string(dns_temp));
			json_object_set_new(array_item, "mtu", json_integer(ppp_config->mtu));
			json_object_set_new(array_item, "connect_time", json_integer(get_uptime() - gl_myinfo.svcinfo.uptime));
		}else {
			json_object_set_new(array_item, "ip_addr", json_string("0.0.0.0"));
			json_object_set_new(array_item, "netmask", json_string("0.0.0.0"));
			json_object_set_new(array_item, "gateway", json_string("0.0.0.0"));
			json_object_set_new(array_item, "dns", json_string("0.0.0.0"));
			json_object_set_new(array_item, "mtu", json_integer(ppp_config->mtu));
			json_object_set_new(array_item, "connect_time", json_integer(0));
		}
		
		json_array_append_new(array, array_item);
	}
	
	results = json_object();
	json_object_set_new(results, "results", array);
	s_repon = json_dumps(results, JSON_INDENT(0));

	json_array_clear(array);
	json_decref(results);

	return s_repon;
}

static char *show_config_to_json(char *query)
{
	int i = 0;
	PPP_CONFIG *ppp_config = &gl_myinfo.priv.ppp_config;
	DUALSIM_INFO *dualsim_info = &(gl_myinfo.priv.dualsim_info);
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	json_t *results = NULL;
	json_t *root  = NULL;
	json_t *profile  = NULL;
	json_t *profile_item  = NULL;
	char* s_repon = NULL;

	root = json_object();
	if (get_modem_code(gl_modem_idx) != IH_FEATURE_MODEM_NONE) {
		json_object_set_new(root, "enable", json_integer(gl_myinfo.priv.enable));
		//if (gl_myinfo.priv.enable) {
		if (1) {
			json_object_set_new(root, "enable_dual_sim", json_integer(modem_config->dualsim));
			if (modem_config->dualsim) {
				json_object_set_new(root, "main_sim", json_integer(modem_config->policy_main));
				json_object_set_new(root, "max_dial_times", json_integer(dualsim_info->retries));
				json_object_set_new(root, "min_dial_times", json_integer(dualsim_info->uptime));
				json_object_set_new(root, "backup_sim_timeout", json_integer(dualsim_info->backtime));
			}
			
			json_object_set_new(root, "network_type", json_integer(modem_config->network_type));
			json_object_set_new(root, "sim1_profile", json_integer(ppp_config->profile_idx));
			json_object_set_new(root, "sim1_roaming", json_integer(modem_config->roam_enable[0]));
			json_object_set_new(root, "sim1_pincode", json_string(modem_config->pincode[0]));
			
			if (modem_config->dualsim) {
				json_object_set_new(root, "sim2_profile", json_integer(ppp_config->profile_2nd_idx));
				json_object_set_new(root, "sim2_roaming", json_integer(modem_config->roam_enable[1]));
				json_object_set_new(root, "sim2_pincode", json_string(modem_config->pincode[1]));
				
				json_object_set_new(root, "sim1_csq_threshold", json_integer(modem_config->csq[0]));
				json_object_set_new(root, "sim2_csq_threshold", json_integer(modem_config->csq[1]));
				json_object_set_new(root, "sim1_csq_detect_interval", json_integer(modem_config->csq_interval[0]));
				json_object_set_new(root, "sim2_csq_detect_interval", json_integer(modem_config->csq_interval[1]));
				json_object_set_new(root, "sim1_csq_detect_retries", json_integer(modem_config->csq_retries[0]));
				json_object_set_new(root, "sim2_csq_detect_retries", json_integer(modem_config->csq_retries[1]));
			}
			
			json_object_set_new(root, "static_ip", json_integer(ppp_config->ppp_static));
			if (ppp_config->ppp_static) {
				json_object_set_new(root, "ip_addr", json_string(ppp_config->ppp_ip));
				json_object_set_new(root, "peer_addr", json_string(ppp_config->ppp_peer));
			}
			
			json_object_set_new(root, "connect_mode", json_integer(ppp_config->ppp_mode));
			if (ppp_config->ppp_mode == PPP_DEMAND) {
				json_object_set_new(root, "trig_data", json_integer(ppp_config->trig_data));
				json_object_set_new(root, "trig_sms", json_integer(ppp_config->trig_sms));
				json_object_set_new(root, "max_idle_time", json_integer(ppp_config->idle));
				json_object_set_new(root, "radial_interval", json_integer(gl_myinfo.priv.dial_interval));
			}else if (ppp_config->ppp_mode == PPP_ONLINE) {
				json_object_set_new(root, "radial_interval", json_integer(gl_myinfo.priv.dial_interval));
			}

			profile = json_array();
			for (i=0; i<PPP_MAX_PROFILE; i++) {
				if(ppp_config->profiles[i].type == PROFILE_TYPE_NONE) {
					continue;
				}
				
				profile_item = json_object();

				json_object_set_new(profile_item, "index", json_integer(i+1));
				json_object_set_new(profile_item, "network_type", json_integer(ppp_config->profiles[i].type));
				json_object_set_new(profile_item, "apn", json_string(ppp_config->profiles[i].apn));
				json_object_set_new(profile_item, "access_number", json_string(ppp_config->profiles[i].dialno));
				json_object_set_new(profile_item, "auth_method", json_integer(ppp_config->profiles[i].auth_type));
				json_object_set_new(profile_item, "username", json_string(ppp_config->profiles[i].username));
				json_object_set_new(profile_item, "password", json_string(ppp_config->profiles[i].password));

				json_array_append_new(profile, profile_item);
			}
			json_object_set_new(root, "profile", profile);

			json_object_set_new(root, "init_command", json_string(ppp_config->init_string));
			json_object_set_new(root, "rssi_poll_interval", json_integer(gl_myinfo.priv.signal_interval));
			json_object_set_new(root, "dial_timeout", json_integer(ppp_config->timeout));
			json_object_set_new(root, "mtu", json_integer(ppp_config->mtu));
			json_object_set_new(root, "mru", json_integer(ppp_config->mru));
			json_object_set_new(root, "use_default_asyncmap", json_integer(ppp_config->default_am));
			json_object_set_new(root, "use_peer_dns", json_integer(ppp_config->peerdns));
			json_object_set_new(root, "lcp_interval", json_integer(ppp_config->lcp_echo_interval));
			json_object_set_new(root, "lcp_max_retries", json_integer(ppp_config->lcp_echo_retries));
			json_object_set_new(root, "infinitely_dial_retry", json_integer(ppp_config->infinitely_dial_retry));
			json_object_set_new(root, "debug", json_integer(gl_myinfo.priv.ppp_debug));
			json_object_set_new(root, "expert_options", json_string(ppp_config->options));
		}
	}

	results = json_object();
	json_object_set_new(results, "results", root);
	s_repon = json_dumps(results, JSON_INDENT(0));

	json_decref(results);
	json_decref(root);

	return s_repon;
}

static char *show_interface_list_to_json(char *query)
{
	char iface_name[IF_CMD_NAME_SIZE] = {0};
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	json_t *results = NULL;
	json_t *array  = NULL;
	json_t *array_item  = NULL;
	char* s_repon = NULL;

	array = json_array();
	if (get_modem_code(gl_modem_idx) != IH_FEATURE_MODEM_NONE) {
		array_item = json_object();
		get_str_from_if_info(&(pinfo->if_info), iface_name);
		json_object_set_new(array_item, "interface", json_string(iface_name));
		json_object_set_new(array_item, "ip_addr", json_string(inet_ntoa(pinfo->local_ip)));
		
		json_array_append_new(array, array_item);
	}
	
	results = json_object();
	json_object_set_new(results, "results", array);
	s_repon = json_dumps(results, JSON_INDENT(0));

	json_array_clear(array);
	json_decref(results);

	return s_repon;
}

static char *set_redial_enable_for_json(char *post_json)
{
	int status = 0;
	int err_code = ERR_OK;
	char err_reason[MAX_NAME_LEN] = {0};
	json_t *root  = NULL;
	json_t *item  = NULL;
	json_error_t failed;
	char* s_repon = NULL;

	if (post_json == NULL) {
		LOG_ER("request json is NULL");
		MY_ERROR(ERR_REQUEST, ERR_REQUEST_NULL);
	}

	root = json_loads(post_json, JSON_INDENT(0), &failed);
	if (!root) {
		LOG_ER("redial loads json error,%d:%s", failed.line, failed.text);
		MY_ERROR(ERR_REQUEST, ERR_REQUEST_INVALID);
	}

	item = json_object_get(root, "status");
	if (item) {
		status = json_integer_value(item);
		if (status) {//up
			LOG_IN("receive connect command, Go!");
			if(get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP
				|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI
				|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_MIPC
					|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_NONE){
				if (PPP_ONLINE != gl_myinfo.priv.ppp_config.ppp_mode){
					gl_redial_dial_state = REDIAL_DIAL_STATE_NONE;
				}
			}else if(gl_ppp_pid >= 0) {
				kill(gl_ppp_pid, SIGUSR1);
			}
		}else {//down
			LOG_IN("receive disconnect command, hangup!");
			if(get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP
				|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI
				|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_MIPC
					|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_NONE){

				stop_child_by_sub_daemon();

				if (PPP_ONLINE != gl_myinfo.priv.ppp_config.ppp_mode){
					gl_redial_dial_state = REDIAL_DIAL_STATE_KILLED;
				}
			}else if(gl_ppp_pid >= 0) {
				kill(gl_ppp_pid, SIGHUP);
			}
		}
	}else {
		LOG_ER("redial status argument is not found");
		MY_ERROR(ERR_INVAL, ERR_INVAL_STR);
	}

	s_repon = show_network_status_to_json(NULL);

	return s_repon;

ERROR:
	root = json_pack("{s:s,s:i,s:s}", "result", "error", "error_code", err_code, "error", err_reason);
	if (root) {
		s_repon = json_dumps(root, JSON_INDENT(0));
		json_decref(root);
	}

	return s_repon;
}

static int redial_json_parse(char *json, MY_REDIAL_INFO *myinfo)
{
	int i = 0;
	int index = 0;
	int profile_num = 0;
	PPP_CONFIG *ppp_config = NULL;
	DUALSIM_INFO *dualsim_info = NULL;
	MODEM_CONFIG *modem_config = NULL;
	json_t *root  = NULL;
	json_t *profile  = NULL;
	json_t *profile_item  = NULL;
	json_error_t failed;

	if ((json == NULL) || (myinfo == NULL)) {
		return -1;
	}

	ppp_config = &myinfo->priv.ppp_config;
	dualsim_info = &(myinfo->priv.dualsim_info);
	modem_config = &(myinfo->priv.modem_config);

	root = json_loads(json, JSON_INDENT(0), &failed);
	if (!root) {
		LOG_ER("redial loads json error,%d:%s", failed.line, failed.text);
		return -2;
	}

	JSON_INTEGER_VALUE_FORCE(root, "enable", myinfo->priv.enable);
	if (!myinfo->priv.enable) {
		return 0;
	}
	
	JSON_INTEGER_VALUE(root, "enable_dual_sim", modem_config->dualsim);
	if (modem_config->dualsim) {
		JSON_INTEGER_VALUE_FORCE(root, "main_sim", modem_config->policy_main);
		JSON_INTEGER_VALUE_FORCE(root, "max_dial_times", dualsim_info->retries);
		JSON_INTEGER_VALUE_FORCE(root, "min_dial_times", dualsim_info->uptime);
		JSON_INTEGER_VALUE_FORCE(root, "backup_sim_timeout", dualsim_info->backtime);

		JSON_INTEGER_VALUE_FORCE(root, "sim2_profile", ppp_config->profile_2nd_idx);
		JSON_INTEGER_VALUE_FORCE(root, "sim2_roaming", modem_config->roam_enable[1]);
		JSON_STRING_VALUE_FORCE(root, "sim2_pincode", modem_config->pincode[1]);

		JSON_INTEGER_VALUE_FORCE(root, "sim1_csq_threshold", modem_config->csq[0]);
		JSON_INTEGER_VALUE_FORCE(root, "sim2_csq_threshold", modem_config->csq[1]);
		JSON_INTEGER_VALUE_FORCE(root, "sim1_csq_detect_interval", modem_config->csq_interval[0]);
		JSON_INTEGER_VALUE_FORCE(root, "sim2_csq_detect_interval", modem_config->csq_interval[1]);
		JSON_INTEGER_VALUE_FORCE(root, "sim1_csq_detect_retries", modem_config->csq_retries[0]);
		JSON_INTEGER_VALUE_FORCE(root, "sim2_csq_detect_retries", modem_config->csq_retries[1]);
	}
	
	JSON_INTEGER_VALUE_FORCE(root, "network_type", modem_config->network_type);
	JSON_INTEGER_VALUE_FORCE(root, "sim1_profile", ppp_config->profile_idx);
	JSON_INTEGER_VALUE_FORCE(root, "sim1_roaming", modem_config->roam_enable[0]);
	JSON_STRING_VALUE_FORCE(root, "sim1_pincode", modem_config->pincode[0]);
	
	JSON_INTEGER_VALUE_FORCE(root, "static_ip", ppp_config->ppp_static);
	if (ppp_config->ppp_static) {
		JSON_STRING_VALUE_FORCE(root, "ip_addr", ppp_config->ppp_ip);
		JSON_STRING_VALUE_FORCE(root, "peer_addr", ppp_config->ppp_peer);
	}
	
	JSON_INTEGER_VALUE_FORCE(root, "connect_mode", ppp_config->ppp_mode);
	if (ppp_config->ppp_mode == PPP_DEMAND) {
		JSON_INTEGER_VALUE_FORCE(root, "trig_data", ppp_config->trig_data);
		JSON_INTEGER_VALUE_FORCE(root, "trig_sms", ppp_config->trig_sms);
		JSON_INTEGER_VALUE_FORCE(root, "max_idle_time", ppp_config->idle);
		JSON_INTEGER_VALUE_FORCE(root, "radial_interval", myinfo->priv.dial_interval);
	}else if (ppp_config->ppp_mode == PPP_ONLINE) {
		JSON_INTEGER_VALUE_FORCE(root, "radial_interval", myinfo->priv.dial_interval);
	}

	profile = json_object_get(root, "profile");
	if (!profile) {
		LOG_ER("profile item error");
		return -1;
	}
	for (i = 0; i < json_array_size(profile); i++) {
		profile_item = json_array_get(profile, i);
		if (!profile_item) {
			LOG_ER("profile item array error");
			return -1;
		}

		JSON_INTEGER_VALUE_FORCE(profile_item, "index", index);
		if ((index <= 0) || (index > PPP_MAX_PROFILE)) {
			continue;
		}
		
		JSON_INTEGER_VALUE_FORCE(profile_item, "network_type", ppp_config->profiles[index-1].type);
		JSON_STRING_VALUE_FORCE(profile_item, "apn", ppp_config->profiles[index-1].apn);
		JSON_STRING_VALUE_FORCE(profile_item, "access_number", ppp_config->profiles[index-1].dialno);
		JSON_INTEGER_VALUE_FORCE(profile_item, "auth_method", ppp_config->profiles[index-1].auth_type);
		JSON_STRING_VALUE_FORCE(profile_item, "username", ppp_config->profiles[index-1].username);
		JSON_STRING_VALUE_FORCE(profile_item, "password", ppp_config->profiles[index-1].password);
	}
	profile_num = i;

	JSON_STRING_VALUE_FORCE(root, "init_command", ppp_config->init_string);
	JSON_INTEGER_VALUE_FORCE(root, "rssi_poll_interval", myinfo->priv.signal_interval);
	JSON_INTEGER_VALUE_FORCE(root, "dial_timeout", ppp_config->timeout);
	JSON_INTEGER_VALUE_FORCE(root, "mtu", ppp_config->mtu);
	JSON_INTEGER_VALUE_FORCE(root, "mru", ppp_config->mru);
	JSON_INTEGER_VALUE_FORCE(root, "use_default_asyncmap", ppp_config->default_am);
	JSON_INTEGER_VALUE_FORCE(root, "use_peer_dns", ppp_config->peerdns);
	JSON_INTEGER_VALUE_FORCE(root, "lcp_interval", ppp_config->lcp_echo_interval);
	JSON_INTEGER_VALUE_FORCE(root, "lcp_max_retries", ppp_config->lcp_echo_retries);
	JSON_INTEGER_VALUE_FORCE(root, "infinitely_dial_retry", ppp_config->infinitely_dial_retry);
	JSON_INTEGER_VALUE_FORCE(root, "debug", myinfo->priv.ppp_debug);
	JSON_STRING_VALUE_FORCE(root, "expert_options", ppp_config->options);

	return profile_num;
}

static char *set_redial_for_json(char *post_json)
{
	int profile_num = 0;
	char sim[2];
	MY_REDIAL_INFO myinfo = {0};
	PPP_CONFIG *ppp_config = &myinfo.priv.ppp_config;
	DUALSIM_INFO *dualsim_info = &(myinfo.priv.dualsim_info);
	MODEM_CONFIG *modem_config = &(myinfo.priv.modem_config);
	PPP_CONFIG *old_ppp_config = &gl_myinfo.priv.ppp_config;
	DUALSIM_INFO *old_dualsim_info = &(gl_myinfo.priv.dualsim_info);
	MODEM_CONFIG *old_modem_config = &(gl_myinfo.priv.modem_config);
	MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;
	int err_code = ERR_OK;
	char err_reason[MAX_NAME_LEN] = {0};
	json_t *root  = NULL;
	char* s_repon = NULL;

	if (post_json == NULL) {
		LOG_ER("request json is NULL");
		MY_ERROR(ERR_REQUEST, ERR_REQUEST_NULL);
	}

	profile_num = redial_json_parse(post_json, &myinfo);
	if (profile_num == -1) {
		LOG_ER("request json argument is invalid");
		MY_ERROR(ERR_INVAL, ERR_INVAL_STR);
	}else if (profile_num == -2) {
		LOG_ER("request json is invalid");
		MY_ERROR(ERR_REQUEST, ERR_REQUEST_INVALID);
	}

	if (gl_myinfo.priv.enable != myinfo.priv.enable) {
		gl_myinfo.priv.enable = myinfo.priv.enable;
	}

	if (!myinfo.priv.enable) {
		if ((gl_myinfo.priv.enable) && (gl_ppp_pid < 0) && (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS31)) {
			modem_enter_airplane_mode(gl_commport); 
		}
		
		gl_myinfo.priv.enable = FALSE;
	}else {
		gl_myinfo.priv.enable = TRUE;

		if (old_modem_config->dualsim != modem_config->dualsim) {
			old_modem_config->dualsim = modem_config->dualsim;
		}
		if (modem_config->dualsim) {
			if(old_dualsim_info->retries == 0) {
				old_dualsim_info->retries = 5;
			}
			if(info->current_sim != old_modem_config->main_sim){
				gl_sim_switch_flag = TRUE;
			}

			if (old_modem_config->policy_main != modem_config->policy_main) {
				if (modem_config->policy_main == SIMX) {//random
					if(rand()%2 == 0) {
						old_modem_config->main_sim = SIM1;
					}else {
						old_modem_config->main_sim = SIM2;
					}
					
					old_modem_config->policy_main = SIMX;
					unlink(SEQ_SIM_FILE);
				}else if (modem_config->policy_main == SIM_SEQ) {//sequence
					if(f_read_string(SEQ_SIM_FILE, sim, sizeof(sim))>0) {
						old_modem_config->main_sim = SIM1+SIM2-atoi(sim);
					}else {
						old_modem_config->main_sim = SIM1;
					}
					sprintf(sim, "%d", old_modem_config->main_sim);
					f_write_string(SEQ_SIM_FILE, sim, 0, 0);
					old_modem_config->policy_main = SIM_SEQ;
				}else {
					old_modem_config->main_sim = modem_config->policy_main;
					old_modem_config->policy_main = modem_config->policy_main;
					unlink(SEQ_SIM_FILE);
				}

				old_modem_config->backup_sim = SIM1+SIM2-old_modem_config->main_sim;
				if(old_modem_config->dualsim){
					if(info->current_sim != old_modem_config->main_sim){
						gl_sim_switch_flag = TRUE;
					}
				}
			}
			if (old_dualsim_info->retries != dualsim_info->retries) {
				old_dualsim_info->retries = dualsim_info->retries;
			}
			if (old_dualsim_info->uptime != dualsim_info->uptime) {
				old_dualsim_info->uptime = dualsim_info->uptime;
			}
			if (old_dualsim_info->backtime != dualsim_info->backtime) {
				old_dualsim_info->backtime = dualsim_info->backtime;
				if(old_modem_config->dualsim) {
					dualsim_handle_backuptimer();
				}
			}

			if (old_ppp_config->profile_2nd_idx != ppp_config->profile_2nd_idx) {
				if (ppp_config->profile_2nd_idx) {
					if(ppp_config->profiles[ppp_config->profile_2nd_idx-1].type != PROFILE_TYPE_NONE){
						old_ppp_config->profile_2nd_idx = ppp_config->profile_2nd_idx;
					}else {
						LOG_ER("profile secondary %d not found!", ppp_config->profile_2nd_idx);
						MY_ERROR(ERR_INVAL, "Profile Idx Not Found");
					}
				}else {//auto
					if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_LE910) {
						gl_ppp_redial_cnt=0;
					}
					
					old_ppp_config->profile_2nd_idx = 0;
				}
			}
			if (old_modem_config->roam_enable[1] != modem_config->roam_enable[1]) {
				old_modem_config->roam_enable[1] = modem_config->roam_enable[1];
			}
			if (strcmp(old_modem_config->pincode[1], modem_config->pincode[1])) {
				strcpy(old_modem_config->pincode[1], modem_config->pincode[1]);
			}

			if (old_modem_config->csq[0] != modem_config->csq[0]) {
				old_modem_config->csq[0] = modem_config->csq[0];
			}
			if (old_modem_config->csq[1] != modem_config->csq[1]) {
				old_modem_config->csq[1] = modem_config->csq[1];
			}
			if (old_modem_config->csq_interval[0] != modem_config->csq_interval[0]) {
				old_modem_config->csq_interval[0] = modem_config->csq_interval[0];
			}
			if (old_modem_config->csq_interval[1] != modem_config->csq_interval[1]) {
				old_modem_config->csq_interval[1]= modem_config->csq_interval[1];
			}
			if (old_modem_config->csq_retries[0] != modem_config->csq_retries[0]) {
				old_modem_config->csq_retries[0] = modem_config->csq_retries[0];
			}
			if (old_modem_config->csq_retries[1] != modem_config->csq_retries[1]) {
				old_modem_config->csq_retries[1] = modem_config->csq_retries[1];
			}
		}else {
			gl_sim_switch_flag = FALSE;
		}
		
		if (old_modem_config->network_type != modem_config->network_type) {
			old_modem_config->network_type = modem_config->network_type;
		}

		if (old_ppp_config->profile_idx != ppp_config->profile_idx) {
			if (ppp_config->profile_idx) {
				if(ppp_config->profiles[ppp_config->profile_idx-1].type != PROFILE_TYPE_NONE){
					old_ppp_config->profile_idx = ppp_config->profile_idx;
				}else {
					LOG_ER("profile %d not found!", ppp_config->profile_idx);
					MY_ERROR(ERR_INVAL, "Profile Idx Not Found");
				}
			}else {//auto
				if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_LE910) {
					gl_ppp_redial_cnt=0;
				}
				
				old_ppp_config->profile_idx = 0;
			}
		}

		if (old_modem_config->roam_enable[0] != modem_config->roam_enable[0]) {
			old_modem_config->roam_enable[0] = modem_config->roam_enable[0];
		}

		if (strcmp(old_modem_config->pincode[0], modem_config->pincode[0])) {
			strcpy(old_modem_config->pincode[0], modem_config->pincode[0]);
		}
		
		if (old_ppp_config->ppp_static != ppp_config->ppp_static) {
			old_ppp_config->ppp_static = ppp_config->ppp_static;
			
			if (ppp_config->ppp_static) {
				old_ppp_config->ppp_static = 1;
				
				if (strcmp(old_ppp_config->ppp_ip, ppp_config->ppp_ip)) {
					strcpy(old_ppp_config->ppp_ip, ppp_config->ppp_ip);
				}

				if (strcmp(old_ppp_config->ppp_peer, ppp_config->ppp_peer)) {
					strcpy(old_ppp_config->ppp_peer, ppp_config->ppp_peer);
				}
			}
		}

		if (old_ppp_config->ppp_mode != ppp_config->ppp_mode) {
			old_ppp_config->ppp_mode = ppp_config->ppp_mode;
		}
		if (ppp_config->ppp_mode == PPP_DEMAND) {
			if (old_ppp_config->trig_data != ppp_config->trig_data) {
				old_ppp_config->trig_data = ppp_config->trig_data;
			}

			if (old_ppp_config->trig_sms != ppp_config->trig_sms) {
				old_ppp_config->trig_sms = ppp_config->trig_sms;
			}

			if (old_ppp_config->idle != ppp_config->idle) {
				old_ppp_config->idle = ppp_config->idle;
			}

			if (gl_myinfo.priv.dial_interval != myinfo.priv.dial_interval) {
				gl_myinfo.priv.dial_interval = myinfo.priv.dial_interval;
			}
		}else if (ppp_config->ppp_mode == PPP_ONLINE) {
			if (gl_myinfo.priv.dial_interval != myinfo.priv.dial_interval) {
				gl_myinfo.priv.dial_interval = myinfo.priv.dial_interval;
			}
		}

		if (strcmp(old_ppp_config->init_string, ppp_config->init_string)) {
			strcpy(old_ppp_config->init_string, ppp_config->init_string);
		}
		
		if (gl_myinfo.priv.signal_interval != myinfo.priv.signal_interval) {
			gl_myinfo.priv.signal_interval = myinfo.priv.signal_interval;
		}
		
		if (old_ppp_config->timeout != ppp_config->timeout) {
			old_ppp_config->timeout = ppp_config->timeout;
		}
		
		if (old_ppp_config->mtu != ppp_config->mtu) {
			old_ppp_config->mtu = ppp_config->mtu;
		}
		
		if (old_ppp_config->mru != ppp_config->mru) {
			old_ppp_config->mru = ppp_config->mru;
		}
		
		if (old_ppp_config->default_am != ppp_config->default_am) {
			old_ppp_config->default_am = ppp_config->default_am;
		}
		
		if (old_ppp_config->peerdns != ppp_config->peerdns) {
			old_ppp_config->peerdns = ppp_config->peerdns;
		}
		
		if (old_ppp_config->lcp_echo_interval != ppp_config->lcp_echo_interval) {
			old_ppp_config->lcp_echo_interval = ppp_config->lcp_echo_interval;
		}
		
		if (old_ppp_config->lcp_echo_retries != ppp_config->lcp_echo_retries) {
			old_ppp_config->lcp_echo_retries = ppp_config->lcp_echo_retries;
		}
		
		if (old_ppp_config->infinitely_dial_retry != ppp_config->infinitely_dial_retry) {
			old_ppp_config->infinitely_dial_retry = ppp_config->infinitely_dial_retry;
		}
		
		if (gl_myinfo.priv.ppp_debug != myinfo.priv.ppp_debug) {
			gl_myinfo.priv.ppp_debug = myinfo.priv.ppp_debug;
		}
		
		if (strcmp(old_ppp_config->options, ppp_config->options)) {
			strcpy(old_ppp_config->options, ppp_config->options);
		}

		//delete profile
		memset(old_ppp_config->profiles, 0, sizeof(PPP_PROFILE)*PPP_MAX_PROFILE);
		
		//add profile
		memcpy(old_ppp_config->profiles,  ppp_config->profiles, sizeof(PPP_PROFILE)*PPP_MAX_PROFILE);
	}

	//only ppp run, handle ppp config
	if(gl_ppp_pid>0) {
		stop_child_by_sub_daemon();

		my_restart_timer(3);
	}

	s_repon = show_config_to_json(NULL);

	return s_repon;

ERROR:
	root = json_pack("{s:s,s:i,s:s}", "result", "error", "error_code", err_code, "error", err_reason);
	if (root) {
		s_repon = json_dumps(root, JSON_INDENT(0));
		json_decref(root);
	}

	return s_repon;
}

const request_api reqapi[] = {
	{REDIAL_GET_MODEM_STATUS,	show_modem_status_to_json},
	{REDIAL_GET_NETWORK_STATUS,	show_network_status_to_json},
	{REDIAL_GET_CONFIG,			show_config_to_json},
	{REDIAL_GET_INTERFACE_LIST,	show_interface_list_to_json},
	{REDIAL_SET_ENABLE_CONFIG,	set_redial_enable_for_json},
	{REDIAL_SET_CONFIG,			set_redial_for_json},
	
	{ NULL,						NULL}
};

static void mosq_restart_timer(int timeout)
{
	struct timeval tv;

	evutil_timerclear(&tv);
	tv.tv_sec = timeout;
	event_add(&gl_mosq_tmr, &tv);
}

void mosq_client_init()
{
	SUB_TOPIC_INFO *sub_info = (SUB_TOPIC_INFO *)&gl_sub_info;

	/*sub service topic*/
	strlcpy(sub_info->topic[0], REDIAL_NOTICE_TOPIC, sizeof(sub_info->topic[0]));
	sub_info->topic_num = REDIAL_SUB_TOPIC_NUM;

	/*request api init*/
	gl_request_api = (request_api *)reqapi;

	/*init mosq client id*/
	snprintf(sub_info->clientid, sizeof(sub_info->clientid), "inos-%s", IDENT);

	/*monitor web api request*/
	sub_info->my_event_base = (struct event_base *)g_my_event_base;
	sub_info->restart_timer = mosq_restart_timer;

	/*monitor mosq client status*/
	evtimer_set(&gl_mosq_tmr,
				mosq_client_timer,
				&gl_sub_info);
	event_base_set(g_my_event_base, &gl_mosq_tmr);
	mosq_restart_timer(1);
}

