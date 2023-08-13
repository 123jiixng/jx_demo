#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "ih_logtrace.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libyang/libyang.h>
#include <sysrepo.h>
#include "ih_sysrepo.h"
#include "shared.h"

#include "redial_ipc.h"
#include "modem.h"
#include "redial.h"
#include "operator.h"

extern int do_redial_procedure(void);
extern MY_REDIAL_INFO gl_myinfo;
extern BOOL gl_sim_switch_flag;
extern BOOL gl_modem_reset_flag;

#define CELLULAR_SYSREPO_DEBUG 1

//-------------------cellular sysrepo debug macros-----------------------------------
#if CELLULAR_SYSREPO_DEBUG
//----------------
#define DEBUG_PRINT(fmt, args...) do { \
	LOG_IN(fmt, ##args); \
}while(0)

//----------------
#define show_debug() \
IH_SYSREPO_NODE_ACTION_INFO(node, op); \
DEBUG_PRINT("operator node [%s]", node->schema->name);

#else
//----------------
#define DEBUG_PRINT(fmt, args...) do { \
}while(0)

//----------------
#define show_debug() do { \
}while(0)

#endif

//-------------------cellular private macros-----------------------------------------
#define SETTING_START(parent_str, key_str) \
	key_node = ih_sysrepo_get_root_key_node((struct lyd_node *)node, parent_str, key_str); \
	if(key_node){ \
		sim_id = ((struct lyd_node_leaf_list *)key_node)->value.enm->value; \
		sim_id = (sim_id==SIM1) ? SIM1 : SIM2;
//----------------
#define SIM_SETTING_START \
	SETTING_START("sim-basic-settings", "sim-id")
//----------------
#define PROFILE_SETTING_START \
	SIM_SETTING_START \
		if(sim_id == SIM1){ \
			profile = &ppp_config->profiles[0]; \
		}else{ \
			profile = &ppp_config->profiles[PPP_MAX_PROFILE>>1]; \
		} 
//----------------
#define CSQ_SETTING_START \
	SETTING_START("csq", "sim-id")
//----------------
#define SETTING_END \
	}
//----------------
#define GET_RPC_INPUT_VALUE_STR(input_name, value_string) \
	nodeset = lyd_find_path(input, input_name); \
	if(nodeset->number){ \
		snprintf(value_string, sizeof(value_string), "%s", ((struct lyd_node_leaf_list *)nodeset->set.d[0])->value_str); \
		DEBUG_PRINT("rpc input parameter [name:%s][value:%s]", nodeset->set.d[0]->schema->name, value_string); \
		ly_set_free(nodeset); \
	}else{ \
		ly_set_free(nodeset); \
		DEBUG_PRINT("%s can't get rpc input parameter: %s", __func__, input_name); \
		return SR_ERR_NOT_FOUND; \
	}
//----------------
#define NODE_LEAF_LIST(node) \
	((struct lyd_node_leaf_list *)node)
//----------------
#define GET_STRING(string_array, index_val) \
	(((index_val >= 0) && (ARRAY_COUNT(string_array) > index_val)) ? string_array[index_val] : "")
//---------------------------------------------------------------------------------

const INTEGER_MAP_STRING cellular_net_table[] = {
	{{MODEM_NETWORK_AUTO, 	AT_CMD_AUTO}, 	"auto"		},
	{{MODEM_NETWORK_2G,	AT_CMD_2G}, 	"net-2G-only"	},
	{{MODEM_NETWORK_3G,	AT_CMD_3G}, 	"net-3G-only"	},
	{{MODEM_NETWORK_3G2G,	AT_CMD_3G2G}, 	"net-3G2G"	},
	{{MODEM_NETWORK_4G,	AT_CMD_4G}, 	"net-4G-only"	},
	{{MODEM_NETWORK_5G,	AT_CMD_5G}, 	"net-5G-only"	},
	{{-1, -1}, NULL}
};

static char* int2str(int value, const char *fmt)
{
	static char value_str[64] = {0};
	char format[32]	= {"%d"};

	if(fmt){
		strlcpy(format, fmt, sizeof(format));
	}

	bzero(value_str, sizeof(value_str));
	snprintf(value_str, sizeof(value_str), format, value);
	
	return value_str;
}

static int get_cell_net_type(const char *str)
{
	int i = 0;

	if(!str){
		return MODEM_NETWORK_AUTO;
	}

	while(cellular_net_table[i].integer[0] != -1){
		if(strstr(str, cellular_net_table[i].str)){
			return cellular_net_table[i].integer[0];
		}
		i++;
	}

	return MODEM_NETWORK_AUTO;
}

static const char *
get_cell_net_type_string(int cell_net_type)
{
	int i = 0;

	while(cellular_net_table[i].integer[0] != -1){
		if(cellular_net_table[i].integer[0] == cell_net_type){
			return cellular_net_table[i].str;
		}
		i++;
	}

	return "auto";
}


static void set_modem_supported_cell_net_types(struct lyd_node *cell_net_list)
{
	int i = 0;
	AT_CMD *custom = NULL;

	if(!cell_net_list || !g_modem_custom_cmds){
		return;
	}

	while(cellular_net_table[i].integer[1] != -1){
		for(custom=g_modem_custom_cmds; custom->index!=-1; custom++){
			if(cellular_net_table[i].integer[1] == custom->index){
				lyd_new_leaf(cell_net_list, NULL, "net-type", cellular_net_table[i].str);
				break;
			}
		}
		i++;
	}
}

int asu2dbm(int siglevel)
{
	if(100 < siglevel && siglevel <= 191){
		return (siglevel - 216);
	}
	
	return (2*siglevel - 113);
}

/*
 .mod_name   = "inhand-cellular",
 .xpath      = "/inhand-cellular:cellular",
 * */
IH_SYSREPO_CONFIG_CALLBACK(ih_sr_cellular_config_cb)
{
	sr_change_iter_t *iter;
	sr_change_oper_t op;
	const struct lyd_node *node, *key_node = NULL;
	const char *prev_val, *prev_list;
	char *xpath2;
	const char *value_str;
	bool prev_dflt;
	int ret;
	int sim_id;
	int network_type = MODEM_NETWORK_AUTO;
	MY_REDIAL_INFO *pinfo = &gl_myinfo;
	PPP_CONFIG *ppp_config = &gl_myinfo.priv.ppp_config;
	MODEM_CONFIG *modem_config = &gl_myinfo.priv.modem_config;
	DUALSIM_INFO *dualsim_info = &gl_myinfo.priv.dualsim_info;
	PPP_PROFILE *profile = NULL;
	BOOL need_redial = FALSE;

	IH_SYSREPO_CONFIG_CALL_INFO();

	if (asprintf(&xpath2, "%s//.", xpath) == -1) {
		EMEM;
		return SR_ERR_NOMEM;
	}

	ret = sr_get_changes_iter(session, xpath2, &iter);
	free(xpath2);
	if (ret != SR_ERR_OK) {
		LOG_ER("Getting changes iter failed (%s).", sr_strerror(ret));
		return ret;
	}

	while ((ret = sr_get_change_tree_next(session, iter, &op, &node, &prev_val, &prev_list, &prev_dflt)) == SR_ERR_OK) {

		if(IH_SYSREPO_NODE_IS(node, "cellular", "enabled")){
			show_debug();
			if(op == SR_OP_DELETED){
				pinfo->priv.enable = 0;
			}else{
				pinfo->priv.enable = NODE_LEAF_LIST(node)->value.bln;
			}
			need_redial = TRUE;
			DEBUG_PRINT("cellular enable status: %d", pinfo->priv.enable);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "dial-settings", "redial-interval")){
			show_debug();
			if(op == SR_OP_DELETED){
				pinfo->priv.dial_interval = 10;
			}else{
				pinfo->priv.dial_interval = NODE_LEAF_LIST(node)->value.uint16;
			}
			DEBUG_PRINT("redial interval: %d", pinfo->priv.dial_interval);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "dial-settings", "connection-mode")){
			show_debug();
			if(op == SR_OP_DELETED){
				ppp_config->ppp_mode = PPP_ONLINE;
			}else{
				ppp_config->ppp_mode = NODE_LEAF_LIST(node)->value.enm->value;
			}
			need_redial = TRUE;
			DEBUG_PRINT("connection-mode : %d", ppp_config->ppp_mode);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "sim-basic-settings", "roaming")){
			show_debug();
			SIM_SETTING_START
				if(op == SR_OP_DELETED){
					modem_config->roam_enable[sim_id-1] = 1;
				}else{
					modem_config->roam_enable[sim_id-1] = NODE_LEAF_LIST(node)->value.bln;
				}
				need_redial = TRUE;
				DEBUG_PRINT("sim%d roaming: %d", sim_id, modem_config->roam_enable[sim_id-1]);
			SETTING_END
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "sim-basic-settings", "pin-code")){
			show_debug();
			SIM_SETTING_START
				if(op == SR_OP_DELETED){
					bzero(modem_config->pincode[sim_id-1], sizeof(modem_config->pincode[0]));
				}else{
					strlcpy(modem_config->pincode[sim_id-1], NODE_LEAF_LIST(node)->value_str, sizeof(modem_config->pincode[0]));
				}
				need_redial = TRUE;
				DEBUG_PRINT("sim%d pincode: %s", sim_id, modem_config->pincode[sim_id-1]);
			SETTING_END
			continue;
		}
		
		if(IH_SYSREPO_NODE_IS(node, "sim-basic-settings", "net-search-mode")){
			show_debug();
			SIM_SETTING_START
				if(op == SR_OP_DELETED){
					network_type = MODEM_NETWORK_AUTO;
				}else{
					network_type = get_cell_net_type(NODE_LEAF_LIST(node)->value_str);
				}

				if(sim_id == SIM1){
					modem_config->network_type = network_type;
				}else{
					modem_config->network_type2 = network_type;
				}
				need_redial = TRUE;
				DEBUG_PRINT("sim%d net-search-mode: %d", sim_id, network_type);
			SETTING_END
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "profile1", "apn")){
			show_debug();
			PROFILE_SETTING_START
				if(op == SR_OP_DELETED){
					bzero(profile->apn, PPP_MAX_APN_LEN);
					profile->type = PROFILE_TYPE_NONE;
					if(sim_id == SIM1){ //configure APN automatically
						ppp_config->profile_idx = 0;
					}else{
						ppp_config->profile_2nd_idx = 0;
					}
				}else{
					profile->type = PROFILE_TYPE_GSM;
					strlcpy(profile->apn, NODE_LEAF_LIST(node)->value_str, PPP_MAX_APN_LEN);
					if(sim_id == SIM1){ //if APN is empty, configure APN automatically
						ppp_config->profile_idx = profile->apn[0] ? 1 : 0;
					}else{
						ppp_config->profile_2nd_idx = profile->apn[0] ? (PPP_MAX_PROFILE/2 + 1) : 0;
					}
				}
				need_redial = TRUE;
				DEBUG_PRINT("sim%d apn: %s", sim_id, profile->apn);
			SETTING_END
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "profile1", "access-number")){
			show_debug();
			PROFILE_SETTING_START
				if(op == SR_OP_DELETED){
					bzero(profile->dialno, PPP_MAX_DIALNO_LEN);
				}else{
					strlcpy(profile->dialno, NODE_LEAF_LIST(node)->value_str, PPP_MAX_DIALNO_LEN);
				}
				need_redial = TRUE;
				DEBUG_PRINT("sim%d access-number: %s", sim_id, profile->dialno);
			SETTING_END
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "profile1", "auth-method")){
			show_debug();
			PROFILE_SETTING_START
				if(op == SR_OP_DELETED){
					profile->auth_type = AUTH_TYPE_AUTO;
				}else{
					profile->auth_type = NODE_LEAF_LIST(node)->value.enm->value;
				}
				need_redial = TRUE;
				DEBUG_PRINT("sim%d auth-type: %d", sim_id, profile->auth_type);
			SETTING_END
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "profile1", "username")){
			show_debug();
			PROFILE_SETTING_START
				if(op == SR_OP_DELETED){
					bzero(profile->username, PPP_MAX_USR_LEN);
				}else{
					strlcpy(profile->username, NODE_LEAF_LIST(node)->value_str, PPP_MAX_USR_LEN);
				}
				need_redial = TRUE;
				DEBUG_PRINT("sim%d username: %s", sim_id, profile->username);
			SETTING_END
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "profile1", "password")){
			show_debug();
			PROFILE_SETTING_START
				if(op == SR_OP_DELETED){
					bzero(profile->password, PPP_MAX_PWD_LEN);
				}else{
					strlcpy(profile->password, NODE_LEAF_LIST(node)->value_str, PPP_MAX_PWD_LEN);
				}
				need_redial = TRUE;
				DEBUG_PRINT("sim%d password: %s", sim_id, profile->password);
			SETTING_END
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "dual-sim", "enabled")){
			show_debug();
			if(op == SR_OP_DELETED){
				modem_config->dualsim = 0;
			}else{
				modem_config->dualsim = NODE_LEAF_LIST(node)->value.bln;
			}
			DEBUG_PRINT("dual-sim enabled: %d", modem_config->dualsim);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "dual-sim", "main-sim")){
			show_debug(); 
			if(op == SR_OP_DELETED){
				modem_config->main_sim = SIM1;
				modem_config->policy_main = modem_config->main_sim;
				unlink(SEQ_SIM_FILE);
			}else{
				value_str = NODE_LEAF_LIST(node)->value_str;
				if(strstr(value_str, "sim1")){
					modem_config->main_sim = SIM1;
					modem_config->policy_main = modem_config->main_sim;
				}else if(strstr(value_str, "sim2")){
					modem_config->main_sim = SIM2;
					modem_config->policy_main = modem_config->main_sim;
				}else if(strstr(value_str, "random")){
					if(rand()%2 == 0) modem_config->main_sim = SIM1;
					else modem_config->main_sim = SIM2;
					modem_config->policy_main = SIMX;
					unlink(SEQ_SIM_FILE);
				}else if(strstr(value_str, "sequence")){
					char sim[2];
					if(f_read_string(SEQ_SIM_FILE, sim, sizeof(sim))>0) {
						modem_config->main_sim = SIM1+SIM2-atoi(sim);
					}else {
						modem_config->main_sim = SIM1;
					}
					sprintf(sim, "%d", modem_config->main_sim);
					f_write_string(SEQ_SIM_FILE, sim, 0, 0);
					modem_config->policy_main = SIM_SEQ;
				}
			}
			need_redial = TRUE;
			DEBUG_PRINT("dual-sim main-sim: %d policy_main: %d", modem_config->main_sim, modem_config->policy_main);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "dual-sim", "max-dial-times")){
			show_debug();
			if(op == SR_OP_DELETED){
				dualsim_info->retries = 5;
			}else{
				dualsim_info->retries = NODE_LEAF_LIST(node)->value.uint16;
			}
			DEBUG_PRINT("max-dial-times: %d", dualsim_info->retries);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "dual-sim", "min-connection-time")){
			show_debug();
			if(op == SR_OP_DELETED){
				dualsim_info->uptime = 0; //disabled
			}else{
				dualsim_info->uptime = NODE_LEAF_LIST(node)->value.uint32;
			}
			DEBUG_PRINT("min-connection-time: %d", dualsim_info->uptime);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "dual-sim", "sim-backup-time")){
			show_debug();
			if(op == SR_OP_DELETED){
				dualsim_info->backtime = 0; //disabled
			}else{
				dualsim_info->backtime = NODE_LEAF_LIST(node)->value.uint32;
			}
			DEBUG_PRINT("sim-backup-time: %d", dualsim_info->backtime);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "csq", "csq-threshold")){
			show_debug();
			CSQ_SETTING_START;
				if(op == SR_OP_DELETED){
					modem_config->csq[sim_id-1] = 0; //disabled
				}else{
					modem_config->csq[sim_id-1] = NODE_LEAF_LIST(node)->value.uint8;
				}
				DEBUG_PRINT("sim%d csq-threshold: %d", sim_id, modem_config->csq[sim_id-1]);
			SETTING_END
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "csq", "detect-interval")){
			show_debug();
			CSQ_SETTING_START;
				if(op == SR_OP_DELETED){
					modem_config->csq_interval[sim_id-1] = 0; //disabled
				}else{
					modem_config->csq_interval[sim_id-1] = NODE_LEAF_LIST(node)->value.uint32;
				}
				DEBUG_PRINT("sim%d detect-interval: %d", sim_id, modem_config->csq_interval[sim_id-1]);
			SETTING_END
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "csq", "detect-retries")){
			show_debug();
			CSQ_SETTING_START;
				if(op == SR_OP_DELETED){
					modem_config->csq_retries[sim_id-1] = 0; //disabled
				}else{
					modem_config->csq_retries[sim_id-1] = NODE_LEAF_LIST(node)->value.uint16;
				}
				DEBUG_PRINT("sim%d detect-retries: %d", sim_id, modem_config->csq_retries[sim_id-1]);
			SETTING_END
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "advanced", "initial-at-cmds")){
			show_debug();
			if(op == SR_OP_DELETED){
				bzero(ppp_config->init_string, sizeof(ppp_config->init_string));
			}else{
				strlcpy(ppp_config->init_string, NODE_LEAF_LIST(node)->value_str, sizeof(ppp_config->init_string));
			}
			need_redial = TRUE;
			DEBUG_PRINT("initial-at-cmds: %s", ppp_config->init_string);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "advanced", "dial-timeout")){
			show_debug();
			if(op == SR_OP_DELETED){
				ppp_config->timeout = 120; //seconds
			}else{
				ppp_config->timeout = NODE_LEAF_LIST(node)->value.uint16;
			}
			need_redial = TRUE;
			DEBUG_PRINT("dial-timeout: %d", ppp_config->timeout);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "advanced", "mru")){
			show_debug();
			if(op == SR_OP_DELETED){
				ppp_config->mru = 1500;
			}else{
				ppp_config->mru = NODE_LEAF_LIST(node)->value.uint16;
			}
			need_redial = TRUE;
			DEBUG_PRINT("mru: %d", ppp_config->mru);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "advanced", "default-asyncmap")){
			show_debug();
			if(op == SR_OP_DELETED){
				ppp_config->default_am = 0; //default disabled
			}else{
				ppp_config->default_am = NODE_LEAF_LIST(node)->value.bln;
			}
			need_redial = TRUE;
			DEBUG_PRINT("default-asyncmap: %d", ppp_config->default_am);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "advanced", "use-peer-dns")){
			show_debug();
			if(op == SR_OP_DELETED){
				ppp_config->peerdns = 1; //default enabled
			}else{
				ppp_config->peerdns = NODE_LEAF_LIST(node)->value.bln;
			}
			need_redial = TRUE;
			DEBUG_PRINT("use-peer-dns: %d", ppp_config->peerdns);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "advanced", "lcp-echo-interval")){
			show_debug();
			if(op == SR_OP_DELETED){
				ppp_config->lcp_echo_interval = 55; //seconds
			}else{
				ppp_config->lcp_echo_interval = NODE_LEAF_LIST(node)->value.uint32;
			}
			need_redial = TRUE;
			DEBUG_PRINT("lcp-echo-interval: %d", ppp_config->lcp_echo_interval);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "advanced", "lcp-echo-retries")){
			show_debug();
			if(op == SR_OP_DELETED){
				ppp_config->lcp_echo_retries = 5;
			}else{
				ppp_config->lcp_echo_retries = NODE_LEAF_LIST(node)->value.uint16;
			}
			need_redial = TRUE;
			DEBUG_PRINT("lcp-echo-retries: %d", ppp_config->lcp_echo_retries);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "advanced", "expert-options")){
			show_debug();
			if(op == SR_OP_DELETED){
				bzero(ppp_config->options, sizeof(ppp_config->options));
			}else{
				strlcpy(ppp_config->options, NODE_LEAF_LIST(node)->value_str, sizeof(ppp_config->options));
			}
			need_redial = TRUE;
			DEBUG_PRINT("expert-options: %s", ppp_config->options);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "advanced", "infinitely-dial-retry")){
			show_debug();
			if(op == SR_OP_DELETED){
				ppp_config->infinitely_dial_retry = 0; //default disabled
			}else{
				ppp_config->infinitely_dial_retry = NODE_LEAF_LIST(node)->value.bln;
			}
			DEBUG_PRINT("infinitely-dial-retry: %d", ppp_config->infinitely_dial_retry);
			continue;
		}
		//add code here
	}

	sr_free_change_iter(iter);
	
	if(need_redial){
		do_redial_procedure();
	}

	return SR_ERR_OK;
}

/*
 .mod_name   = "ietf-interfaces",
 .xpath      = "/ietf-interfaces:interfaces/interface[name='cellular1']",
 * */
IH_SYSREPO_CONFIG_CALLBACK(ih_sr_cellular_if_config_cb)
{
	sr_change_iter_t *iter;
	sr_change_oper_t op;
	const struct lyd_node *node;
	const char *prev_val, *prev_list;
	char *xpath2;
	bool prev_dflt;
	int ret;
	struct in_addr netmask;
	PPP_CONFIG *ppp_config = &gl_myinfo.priv.ppp_config;

	IH_SYSREPO_CONFIG_CALL_INFO();

	if (asprintf(&xpath2, "%s//.", xpath) == -1) {
		EMEM;
		return SR_ERR_NOMEM;
	}

	ret = sr_get_changes_iter(session, xpath2, &iter);
	free(xpath2);
	if (ret != SR_ERR_OK) {
		LOG_ER("Getting changes iter failed (%s).", sr_strerror(ret));
		return ret;
	}

	while ((ret = sr_get_change_tree_next(session, iter, &op, &node, &prev_val, &prev_list, &prev_dflt)) == SR_ERR_OK) {
		if(IH_SYSREPO_NODE_IS(node, "ipv4", "mtu")){
			show_debug();
			if(op == SR_OP_DELETED){
				ppp_config->mtu = 1500; //default value
			}else{
				ppp_config->mtu = NODE_LEAF_LIST(node)->value.uint16;
			}
			DEBUG_PRINT("cellular1 mtu: %d", ppp_config->mtu);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "address", "ip")){
			show_debug();
			if(op == SR_OP_DELETED){
				bzero(ppp_config->ppp_ip, sizeof(ppp_config->ppp_ip));
			}else{
				strlcpy(ppp_config->ppp_ip, NODE_LEAF_LIST(node)->value_str, sizeof(ppp_config->ppp_ip));
			}
			DEBUG_PRINT("cellular1 static local ip: %s", ppp_config->ppp_ip);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "address", "prefix-length")){ //maybe we also need to parse network mask in IP format
			show_debug();
			if(op == SR_OP_DELETED){
				bzero(ppp_config->static_netmask, sizeof(ppp_config->static_netmask));
			}else{
                		masklen2ip(NODE_LEAF_LIST(node)->value.uint8, &netmask);
                		strlcpy(ppp_config->static_netmask, (const char*)inet_ntoa(netmask), sizeof(ppp_config->static_netmask));
			}
			DEBUG_PRINT("cellular1 static netmask: %s", ppp_config->static_netmask);
			continue;
		}

		if(IH_SYSREPO_NODE_IS(node, "inhand", "gateway")){
			show_debug();
			if(op == SR_OP_DELETED){
				bzero(ppp_config->ppp_peer, sizeof(ppp_config->ppp_peer));
			}else{
				strlcpy(ppp_config->ppp_peer, NODE_LEAF_LIST(node)->value_str, sizeof(ppp_config->ppp_peer));
			}
			DEBUG_PRINT("cellular1 static peer ip: %s", ppp_config->ppp_peer);
			continue;
		}
		//add code here
	}

	sr_free_change_iter(iter);

	return SR_ERR_OK;
}

IH_SYSREPO_RPC_CALLBACK(ih_sr_cellular_switch_sim_rpc_cb)
{
	struct ly_set *nodeset;
	char value[128] = {0};
	int sim_id = 0;
	int modem_id = MODEM1;
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	MODEM_CONFIG *modem_config = &gl_myinfo.priv.modem_config;
	
	GET_RPC_INPUT_VALUE_STR("sim-id", value);
	if(!strcmp(value, "sim1")){
		sim_id = SIM1;
	}else{
		sim_id = SIM2;
	}

	GET_RPC_INPUT_VALUE_STR("modem-id", value);
	if(strcmp(value, "modem1")){
		return SR_ERR_UNSUPPORTED;
	}

	if(modem_config->dualsim && (modem_info->current_sim != sim_id)){
		sim_id = (modem_info->current_sim==SIM1) ? SIM2 : SIM1;
		gl_sim_switch_flag = TRUE;
		do_redial_procedure();
	}else{
		sim_id = modem_info->current_sim;
	}

	lyd_new_output_leaf(output, NULL, "cur-sim", int2str(sim_id, "sim%d"));
	lyd_new_output_leaf(output, NULL, "modem-id", int2str(modem_id, "modem%d"));

	return SR_ERR_OK;
}

IH_SYSREPO_RPC_CALLBACK(ih_sr_cellular_reset_modem_rpc_cb)
{
	struct ly_set *nodeset;
	char value[128] = {0};
	
	GET_RPC_INPUT_VALUE_STR("modem-id", value);
	if(!strcmp(value, "modem1")){
		gl_modem_reset_flag = TRUE;
		do_redial_procedure();
	}else{
		return SR_ERR_UNSUPPORTED;
	}

	return SR_ERR_OK;
}

IH_SYSREPO_RPC_CALLBACK(ih_sr_cellular_renew_cellular_rpc_cb)
{
	struct ly_set *nodeset;
	char value[128] = {0};
	
	GET_RPC_INPUT_VALUE_STR("modem-id", value);
	if(!strcmp(value, "modem1")){
		do_redial_procedure();
	}else{
		return SR_ERR_UNSUPPORTED;
	}

	return SR_ERR_OK;
}

struct lyd_node *
get_cellular_oper_data(sr_conn_ctx_t *conn)
{
	struct lyd_node *root = NULL, *cont, *sim_basic_settings, *profile1, *dual_sim, *csq, *advanced, *modem, *cell_net_list, *operator, *cell_info;
	struct ly_ctx *ly_ctx;
	MY_REDIAL_INFO *pinfo = &gl_myinfo;
	MODEM_CONFIG *modem_config = &gl_myinfo.priv.modem_config;
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	PPP_CONFIG *ppp_config = &gl_myinfo.priv.ppp_config;
	DUALSIM_INFO *dualsim_info = &gl_myinfo.priv.dualsim_info;
	PPP_PROFILE *profile = NULL;
	int network_type = 0;
	char value[128];
	int i = 0;

	char *bool_str[] = {"false", "true"};
	char *auth_method_str[] = {"Auto", "PAP", "CHAP", "MS-CHAP", "MS-CHAPv2"};
    	char *conn_mode_str[] = {"always-online", "on-demand", "menual"};
    	char *main_sim_str[] = {"", "sim1", "sim2", "random", "sequence"};
    	char *sim_status_str[] = {"nosim", "simpin", "ready"};
    	char *net_type_str[] = {"NA", "2G", "3G", "4G", "5G"};
    	char *reg_status_str[] = {"registering", "registered", "searching", "denied", "unknown", "roaming"};

	ly_ctx = (struct ly_ctx *)sr_get_context(conn);

	root = lyd_new_path(NULL, ly_ctx, "/inhand-cellular:cellular", NULL, 0, 0);
	if (!root) {
		goto error;
	}
	//enable
	lyd_new_leaf(root, NULL, "enabled", GET_STRING(bool_str, pinfo->priv.enable));
	//dial-settings
	if((cont = lyd_new(root, NULL, "dial-settings"))){
		//sim-basic-settings
		for(i = SIM1; i < SIMX; i++){
        		sim_basic_settings = lyd_new(cont, NULL, "sim-basic-settings");
			lyd_new_leaf(sim_basic_settings, NULL, "sim-id", int2str(i, "sim%d"));
			lyd_new_leaf(sim_basic_settings, NULL, "roaming", GET_STRING(bool_str, modem_config->roam_enable[i-1]));
			if(modem_config->pincode[i-1][0])
				lyd_new_leaf(sim_basic_settings, NULL, "pin-code", modem_config->pincode[i-1]);

			network_type = (i==SIM1) ? modem_config->network_type : modem_config->network_type2;
			lyd_new_leaf(sim_basic_settings, NULL, "net-search-mode", get_cell_net_type_string(network_type));
			//profile1
        		profile1 = lyd_new(sim_basic_settings, NULL, "profile1");
			if(i == SIM1){
				profile = &ppp_config->profiles[0];
			}else{
				profile = &ppp_config->profiles[PPP_MAX_PROFILE>>1];
			}
			//if apn is empty, return auto profile
			if(profile->type == PROFILE_TYPE_NONE || !profile->apn[0]){
				profile = &ppp_config->auto_profile;
			}

			lyd_new_leaf(profile1, NULL, "apn", profile->apn);
			lyd_new_leaf(profile1, NULL, "access-number", profile->dialno);
			lyd_new_leaf(profile1, NULL, "auth-method", GET_STRING(auth_method_str, profile->auth_type));
			lyd_new_leaf(profile1, NULL, "username", profile->username);
			lyd_new_leaf(profile1, NULL, "password", profile->password);
		}
        	lyd_new_leaf(cont, NULL, "connection-mode", GET_STRING(conn_mode_str, ppp_config->ppp_mode));
		lyd_new_leaf(cont, NULL, "redial-interval", int2str(pinfo->priv.dial_interval, NULL));
		//dual-sim
        	dual_sim = lyd_new(cont, NULL, "dual-sim");
		lyd_new_leaf(dual_sim, NULL, "enabled", GET_STRING(bool_str, modem_config->dualsim));
		lyd_new_leaf(dual_sim, NULL, "main-sim", GET_STRING(main_sim_str, modem_config->policy_main));
		lyd_new_leaf(dual_sim, NULL, "max-dial-times", int2str(dualsim_info->retries, NULL));
		lyd_new_leaf(dual_sim, NULL, "min-connection-time", int2str(dualsim_info->uptime, NULL));
		//csq
		for(i = SIM1; i < SIMX; i++){
        		csq = lyd_new(dual_sim, NULL, "csq");
			lyd_new_leaf(csq, NULL, "sim-id", int2str(i, "sim%d"));
			lyd_new_leaf(csq, NULL, "csq-threshold", int2str(modem_config->csq[i-1], NULL));
			lyd_new_leaf(csq, NULL, "detect-interval", int2str(modem_config->csq_interval[i-1], NULL));
			lyd_new_leaf(csq, NULL, "detect-retries", int2str(modem_config->csq_retries[i-1], NULL));
		}
		lyd_new_leaf(dual_sim, NULL, "sim-backup-time", int2str(dualsim_info->backtime, NULL));
		//advanced
        	advanced = lyd_new(cont, NULL, "advanced");
		lyd_new_leaf(advanced, NULL, "initial-at-cmds", ppp_config->init_string);
		lyd_new_leaf(advanced, NULL, "dial-timeout", int2str(ppp_config->timeout, NULL));
		lyd_new_leaf(advanced, NULL, "mru", int2str(ppp_config->mru, NULL));
		lyd_new_leaf(advanced, NULL, "default-asyncmap", GET_STRING(bool_str, ppp_config->default_am));
		lyd_new_leaf(advanced, NULL, "use-peer-dns", GET_STRING(bool_str, ppp_config->peerdns));
		lyd_new_leaf(advanced, NULL, "lcp-echo-interval", int2str(ppp_config->lcp_echo_interval, NULL));
		lyd_new_leaf(advanced, NULL, "lcp-echo-retries", int2str(ppp_config->lcp_echo_retries, NULL));
		lyd_new_leaf(advanced, NULL, "expert-options", ppp_config->options);
		lyd_new_leaf(advanced, NULL, "infinitely-dial-retry", GET_STRING(bool_str, ppp_config->infinitely_dial_retry));
	}
	//modems
	cont = lyd_new(root, NULL, "modems");
	for(i = MODEM1; i < MODEM_MAX; i++){
        	modem = lyd_new(cont, NULL, "modem-entry");
		lyd_new_leaf(modem, NULL, "modem-id", int2str(i, "modem%d"));

		//support-net-types
        	cell_net_list = lyd_new(modem, NULL, "support-net-types");
		set_modem_supported_cell_net_types(cell_net_list);

		lyd_new_leaf(modem, NULL, "active-sim", int2str(modem_info->current_sim, "sim%d"));
		lyd_new_leaf(modem, NULL, "sim-status", GET_STRING(sim_status_str, modem_info->simstatus));
		lyd_new_leaf(modem, NULL, "imei", modem_info->imei);
		lyd_new_leaf(modem, NULL, "imsi", modem_info->imsi);
		lyd_new_leaf(modem, NULL, "iccid", modem_info->iccid);
		lyd_new_leaf(modem, NULL, "phone-number", modem_info->phonenum);
		lyd_new_leaf(modem, NULL, "register-status", GET_STRING(reg_status_str, modem_info->regstatus));
		lyd_new_leaf(modem, NULL, "current-net-type", GET_STRING(net_type_str, modem_info->network));
		lyd_new_leaf(modem, NULL, "signal-level", int2str(asu2dbm(modem_info->siglevel), NULL));
		
		//operator
        	operator = lyd_new(modem, NULL, "operator");
		if(modem_info->carrier_str[0]){
			strlcpy(value, modem_info->carrier_str, sizeof(value));
		}else{
			find_operator(modem_info->carrier_code, value);
		}
		lyd_new_leaf(operator, NULL, "operator-name", value);
		char mcc[8], mnc[8];
		if(modem_info->carrier_code/100 > 999){
			snprintf(mcc, sizeof(mcc), "%d", modem_info->carrier_code/1000);
			snprintf(mnc, sizeof(mnc), "%d", modem_info->carrier_code%1000);
		}else{
			snprintf(mcc, sizeof(mcc), "%d", modem_info->carrier_code/100);
			snprintf(mnc, sizeof(mnc), "%d", modem_info->carrier_code%100);
		}
		lyd_new_leaf(operator, NULL, "mcc", mcc);
		lyd_new_leaf(operator, NULL, "mnc", mnc);
		lyd_new_leaf(operator, NULL, "plmn", int2str(modem_info->carrier_code, NULL));

		//cell-info
        	cell_info = lyd_new(modem, NULL, "cell-info");
		lyd_new_leaf(cell_info, NULL, "rsrp", int2str(modem_info->rsrp, NULL));
		lyd_new_leaf(cell_info, NULL, "rsrq", int2str(modem_info->rsrq, NULL));
		lyd_new_leaf(cell_info, NULL, "sinr", modem_info->sinr);
		lyd_new_leaf(cell_info, NULL, "band", int2str(modem_info->band, NULL));
		lyd_new_leaf(cell_info, NULL, "lac", modem_info->lac);
		lyd_new_leaf(cell_info, NULL, "cell-id", modem_info->cellid);

		//add code here
	}

	if (lyd_validate(&root, LYD_OPT_NOSIBLINGS, NULL)) {
		goto error;
	}

	return root;

error:
	return NULL;
}

/*
 .mod_name   = "inhand-cellular",
 .xpath      = "/inhand-cellular:cellular",
 * */
IH_SYSREPO_OPER_CALLBACK(ih_sr_cellular_data_cb)
{
	struct lyd_node *data = NULL, *node;
	struct ly_set *set = NULL;
	int ret = SR_ERR_OK;

	data = get_cellular_oper_data(ih_sysrepo_srv.sr_conn);

	/* find the requested top-level subtree */
	set = lyd_find_path(data, path);
	if (!set || !set->number) {
		ret = SR_ERR_OPERATION_FAILED;
		goto cleanup;
	}
	node = set->set.d[0];

	if (node->parent || *parent) {
		EINT;
		ret = SR_ERR_OPERATION_FAILED;
		goto cleanup;
	}

	/* return the subtree */
	if (node == data) {
		data = data->next;
	}
	lyd_unlink(node);
	*parent = node;

	/* success */

cleanup:
	ly_set_free(set);
	lyd_free_withsiblings(data);
	return ret;    
}

IH_SYSREPO_SUB cellular_sr_subscribe[] = {
	{
		.type       = IH_SYSREPO_SUB_CONFIG,
		.mod_name   = "inhand-cellular",
		.xpath      = "/inhand-cellular:cellular",
		.callback   = ih_sr_cellular_config_cb,
	},
	{
		.type       = IH_SYSREPO_SUB_CONFIG,
		.mod_name   = "ietf-interfaces",
		.xpath      = "/ietf-interfaces:interfaces/interface[name='cellular1']",
		.callback   = ih_sr_cellular_if_config_cb,
	},
	{
		.type       = IH_SYSREPO_SUB_OPER,
		.mod_name   = "inhand-cellular",
		.xpath      = "/inhand-cellular:cellular",
		.callback   = ih_sr_cellular_data_cb,
	},
	{
		.type       = IH_SYSREPO_SUB_RPC,
		.mod_name   = "inhand-cellular",
		.xpath      = "/inhand-cellular:switch-sim",
		.callback   = ih_sr_cellular_switch_sim_rpc_cb,
	},
	{
		.type       = IH_SYSREPO_SUB_RPC,
		.mod_name   = "inhand-cellular",
		.xpath      = "/inhand-cellular:reset-modem",
		.callback   = ih_sr_cellular_reset_modem_rpc_cb,
	},
	{
		.type       = IH_SYSREPO_SUB_RPC,
		.mod_name   = "inhand-cellular",
		.xpath      = "/inhand-cellular:renew-cellular",
		.callback   = ih_sr_cellular_renew_cellular_rpc_cb,
	}
};

int redial_sysrepo_init(void)
{
	DEBUG_PRINT("redial sysrepo init");
	//clear profiles
	memset(&(gl_myinfo.priv.ppp_config.profiles[0]), 0, sizeof(gl_myinfo.priv.ppp_config.profiles));
	return ih_sysrepo_init(cellular_sr_subscribe, IH_SYSREPO_SUB_ARRAY_SIZE(cellular_sr_subscribe));
}

