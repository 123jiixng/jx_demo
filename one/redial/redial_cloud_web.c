#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <jansson.h>

#include "traffic_stat_ipc.h"
#include "ih_updown_ipc.h"
#include "redial_cloud_web.h"
#include "vif_shared.h"
#include "redial.h"
#include "operator.h"
#include "modem.h"
#include "sw_ipc.h"

#include "cloud_web.h"

//-----------------------------private macros------------------------
#define ENABLE_DUAL_APN 1

#define JSON_VALUE_ENUM(parent, obj_k, out_int, enum_array) \
	bzero(value, sizeof(value)); \
	JSON_STRING_VALUE(parent, obj_k, value); \
	if(value[0]){ \
		if((out_int = get_num_by_string(value, enum_array)) >= 0){ \
			;\
		}else{ \
			JSON_NODE_ERR_INVAL_ADD(obj_k, json_object_get(parent, obj_k)); \
		} \
	} 

//-------------------------------------------------------------------

static struct event gl_mosq_tmr;
SUB_TOPIC_INFO gl_sub_info = {'\0'};
BOOL gl_need_redial = FALSE;
BOOL gl_need_publish = FALSE;

extern MY_REDIAL_INFO gl_myinfo;
extern int gl_modem_idx;
extern BOOL gl_sim_switch_flag;
extern MSG_DATA_USAGE sim_data_usage[SIM2];
extern int do_redial_procedure(void);
extern int get_sub_deamon_type(int idx);
extern int get_pdp_type_setting(void);
extern BOOL dual_apn_is_enabled(void);

ENUM_STR gl_main_sim_enum[] = { 
	{"sim1", 	SIM1},
	{"sim2", 	SIM2},
	{NULL, 		-1}
};

ENUM_STR gl_sim1_profile_enum[] = { 
	{"auto", 	0},
	{"0", 		1},
	{NULL, 		-1}
};
#if ENABLE_DUAL_APN
ENUM_STR gl_sim1_profile2_enum[] = { 
	{"disable", 	0},
	{"2", 		3},
	{NULL, 		-1}
};
#endif
ENUM_STR gl_sim2_profile_enum[] = { 
	{"auto", 	0},
	{"1", 		2},
	{NULL, 		-1}
};
#if ENABLE_DUAL_APN
ENUM_STR gl_sim2_profile2_enum[] = { 
	{"disable", 	0},
	{"3", 		4},
	{NULL, 		-1}
};
#endif
ENUM_STR gl_pdp_type_enum[] = { 
	{"ipv4", 	PDP_TYPE_IPV4},
	{"ipv6", 	PDP_TYPE_IPV6},
	{"ipv4v6", 	PDP_TYPE_IPV4V6},
	{NULL, 		-1}
};

ENUM_STR gl_auth_type_enum[] = { 
	{"auto", 	AUTH_TYPE_AUTO},
	{"pap", 	AUTH_TYPE_PAP},
	{"chap", 	AUTH_TYPE_CHAP},
	{"ms-chap", 	AUTH_TYPE_MSCHAP},
	{"ms-chapv2", 	AUTH_TYPE_MSCHAPV2},
	{NULL, 		-1}
};

ENUM_STR gl_conn_mode_enum[] = { 
	{"always-online", 	PPP_ONLINE},
	{"on-demand", 		PPP_DEMAND},
	{"manual", 		PPP_MANUAL},
	{NULL, 			-1}
};

ENUM_STR gl_net_type_enum[] = { 
	{"auto", 	MODEM_NETWORK_AUTO},
	{"g2", 		MODEM_NETWORK_2G},
	{"g3", 		MODEM_NETWORK_3G},
	{"g4", 		MODEM_NETWORK_4G},
	{"g5", 		MODEM_NETWORK_5G},
	{"g5_g4", 	MODEM_NETWORK_5G4G},
	{NULL, 			-1}
};

ENUM_STR gl_traffic_status_enum[] = { 
	{"disabled", 	DATA_USAGE_DISABLED},
	{"normal", 	DATA_USAGE_NORMAL},
	{"exceed", 	DATA_USAGE_EXCEED},
	{NULL, 		-1}
};

ENUM_STR gl_nr5g_mode_enum[] = { 
	{"sa-nsa", 	NR5G_MODE_AUTO},
	{"nsa", 	NR5G_MODE_NSA},
	{"sa", 		NR5G_MODE_SA},
	{NULL, 		-1}
};

ENUM_STR gl_ims_mode_enum[] = { 
	{"auto", 	IMS_MODE_AUTO},
	{"enable", 	IMS_MODE_ENABLE},
	{"disable", 	IMS_MODE_DISABLE},
	{NULL, 		-1}
};

/*
{
	"cellular": {
        "modem": {
            "enabled": true,
            "sim1": {
                "profile": "auto",
                "network_type": "auto"
            },
            "sim2": {
                "profile": "auto",
                "network_type": "auto"
            },
            "conn_mode": "always-online",
            "dual_sim": {
                "enabled": false,
                "main_sim": "sim1"
            },
        },

       "profile": {
            "0":{
                "type": "ipv4",
                "apn": "",
                "access_num": "*99***1#",
                "auth": "auto",
                "username": "",
                "password": "",
            },
	   "1":{
                "type": "ipv4",
                "apn": "",
                "access_num": "*99***1#",
                "auth": "auto",
                "username": "",
                "password": "",
            }
        }
    },
}*/

static char *show_cellular_config_to_json(json_t *query)
{
	int i = 0, j = 0;
	json_t *root  = NULL;
	json_t *head  = NULL;
	json_t *modem  = NULL;
	json_t *sim  = NULL;
	json_t *ipv4  = NULL;
	json_t *dual_sim  = NULL;
	json_t *profile  = NULL;
	json_t *profile_idx  = NULL;
	char* s_repon = NULL;
	char key_str[128] = {0};
	struct in_addr netmask;

	MY_REDIAL_INFO *pinfo = &gl_myinfo;
	PPP_CONFIG *ppp_config = &gl_myinfo.priv.ppp_config;
	MODEM_CONFIG *modem_config = &gl_myinfo.priv.modem_config;

	MYTRACE_ENTER();

	if(!query) {
		LOG_ER("request json is NULL");
	}

	root = json_object();
	head = json_object();
	json_object_set_new(root, "cellular", head);
	
	for(i = MODEM1; i < MODEM_MAX; i++){
		modem = json_object();
		if(i == MODEM1){
			json_object_set_new(head, "modem", modem);
		}else{
			snprintf(key_str, sizeof(key_str), "modem%d", i);
			json_object_set_new(head, key_str, modem);
		}
#ifdef INHAND_WR3
		json_object_set_new(modem, "ant", json_integer(modem_config->ant));
#endif
		json_object_set_new(modem, "enabled", json_boolean(pinfo->priv.enable));
		json_object_set_new(modem, "nat", json_boolean(pinfo->priv.nat));

		for(j = SIM1; j < SIMX; j++){
			sim = json_object();
			snprintf(key_str, sizeof(key_str), "sim%d", j);
			json_object_set_new(modem, key_str, sim);
			//json_object_set_new(sim, "desc", json_string(modem_config->sim_desc[j-1]));
			if(j == SIM1){
				json_object_set_new(sim, "profile", json_string(get_string_by_num(ppp_config->profile_idx, gl_sim1_profile_enum)));
#if ENABLE_DUAL_APN
				json_object_set_new(sim, "profile2", json_string(get_string_by_num(ppp_config->profile_wwan2_idx, gl_sim1_profile2_enum)));
#endif
				json_object_set_new(sim, "network_type", json_string(get_string_by_num(modem_config->network_type, gl_net_type_enum)));
			}else{
				json_object_set_new(sim, "profile", json_string(get_string_by_num(ppp_config->profile_2nd_idx, gl_sim2_profile_enum)));
#if ENABLE_DUAL_APN
				json_object_set_new(sim, "profile2", json_string(get_string_by_num(ppp_config->profile_wwan2_2nd_idx, gl_sim2_profile2_enum)));
#endif
				json_object_set_new(sim, "network_type", json_string(get_string_by_num(modem_config->network_type2, gl_net_type_enum)));
			}
			json_object_set_new(sim, "nr5g_mode", json_string(get_string_by_num(modem_config->nr5g_mode[j-1], gl_nr5g_mode_enum)));
			json_object_set_new(sim, "pin_code", json_string(modem_config->pincode[j-1]));
			json_object_set_new(sim, "pdp_type", json_string(get_string_by_num(modem_config->pdp_type[j-1], gl_pdp_type_enum)));
			json_object_set_new(sim, "ims", json_string(get_string_by_num(modem_config->ims_mode[j-1], gl_ims_mode_enum)));
		}

		json_object_set_new(modem, "conn_mode", json_string(get_string_by_num(ppp_config->ppp_mode, gl_conn_mode_enum)));

		dual_sim = json_object();
		json_object_set_new(modem, "dual_sim", dual_sim);
		json_object_set_new(dual_sim, "enabled", json_boolean(modem_config->dualsim));
		json_object_set_new(dual_sim, "main_sim", json_string(get_string_by_num(modem_config->policy_main, gl_main_sim_enum)));

		json_object_set_new(modem, "mtu", json_integer(ppp_config->mtu));
		
		ipv4 = json_object();
		json_object_set_new(modem, "ipv4", ipv4);
		inet_aton(ppp_config->static_netmask, &netmask);
		json_object_set_new(ipv4, "prefix_len", json_integer(ppp_config->static_netmask[0] ? netmask_inaddr2len(netmask) : 0));
	}

	profile = json_object();
	json_object_set_new(head, "profile", profile);
#if ENABLE_DUAL_APN
	for(i = 0; i < SIM2*2; i++){
#else
	for(i = 0; i < SIM2; i++){
#endif
		profile_idx = json_object();
		snprintf(key_str, sizeof(key_str), "%d", i);
		json_object_set_new(profile, key_str, profile_idx);
		json_object_set_new(profile_idx, "type", json_string(get_string_by_num(ppp_config->profiles[i].pdp_type, gl_pdp_type_enum)));
		json_object_set_new(profile_idx, "apn", json_string(ppp_config->profiles[i].apn));
		json_object_set_new(profile_idx, "access_num", json_string(ppp_config->profiles[i].dialno));
		json_object_set_new(profile_idx, "auth", json_string(get_string_by_num(ppp_config->profiles[i].auth_type, gl_auth_type_enum)));
		json_object_set_new(profile_idx, "username", json_string(ppp_config->profiles[i].username));
		json_object_set_new(profile_idx, "password", json_string(ppp_config->profiles[i].password));
	}

	s_repon = in_json_get_config_resp_msg(query, root);
	json_decref(root);

	MYTRACE_LEAVE();
	return s_repon;
}

IPv6_SUBNET *get_cellular_primary_addr6(VIF_INFO *vif)
{
	int i;
	int num_need_set = CFG_PER_ADDR < DHCPV6C_LIST_NUM ? CFG_PER_ADDR:DHCPV6C_LIST_NUM;
	
	for(i = 0; i < num_need_set; i++){
		if(!IN6_IS_ADDR_UNSPECIFIED(&(vif->ip6_addr.dynamic_ifaddr[i].ip6)) && vif->ip6_addr.dynamic_ifaddr[i].prefix_len != 0){
			return &(vif->ip6_addr.dynamic_ifaddr[i]);
		}
	}
	
	return NULL;
}

static char *show_cellular_status_to_json(char *query)
{
	MY_REDIAL_INFO *pinfo = &gl_myinfo;
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	PPP_CONFIG *ppp_config = &gl_myinfo.priv.ppp_config;
	SRVCELL_INFO *cell = &gl_myinfo.svcinfo.modem_info.srvcell;
	VIF_INFO *vif_info = &gl_myinfo.svcinfo.vif_info;
	VIF_INFO *secondary_vif_info = &gl_myinfo.svcinfo.secondary_vif_info;
	char *net_str[] = {"", "2G", "3G", "4G", "5G"};
	char status_str[32] = {0};
	char operator[BRAND_STR_SIZ] = {0};
	char detect_target[64] = {0}, link_mode[32] = {0};
	IPv6_SUBNET *addr6 = NULL;
	//int status = pinfo->priv.enable;;
	time_t uptime = 0;
	int i;

	json_t *root = NULL;
	json_t *result = NULL;
	json_t *head  = NULL;
	json_t *modem = NULL;
	json_t *ipv4 = NULL;
	json_t *ipv6 = NULL;
	json_t *wwan2_ipv4 = NULL;
	json_t *simx = NULL;
	json_t *data_usage = NULL;
	json_t *monthly_data = NULL;
	char *s_repon = NULL;

	MYTRACE_ENTER();

	if(pinfo->priv.enable){
		//status = (vif_info->status == VIF_UP) ? 2 : 1;
		uptime = (vif_info->status == VIF_UP) ? (get_uptime() - pinfo->svcinfo.uptime) : 0;
		VIF_GET_STATUS_DESC(&gl_myinfo.svcinfo.vif_info.if_info, status_str, link_mode, detect_target);
	}else{
		snprintf(status_str, sizeof(status_str), "%s", "disabled");
	}
	
	if(modem_info->carrier_str[0]){
		strlcpy(operator, modem_info->carrier_str, sizeof(operator));
	}else if(modem_info->carrier_code){
		find_operator(modem_info->carrier_code, operator);
	}

	root = json_object();
	result = json_object();
	json_object_set_new(root, "result", result);
	head = json_object();
	json_object_set_new(result, "cellular", head);
	json_object_set_new(head, "status", json_string(status_str));
	json_object_set_new(head, "link_mode", json_string(link_mode));
	json_object_set_new(head, "detect_taget", json_string(detect_target));
	json_object_set_new(head, "uptime", json_integer(uptime));
	modem = json_object();
	json_object_set_new(head, "modem_info", modem);
	json_object_set_new(modem, "imei", json_string(modem_info->imei));
	json_object_set_new(modem, "imsi", json_string(modem_info->imsi));
	json_object_set_new(modem, "iccid", json_string(modem_info->iccid));
	json_object_set_new(modem, "phone_num", json_string(modem_info->phonenum));
	json_object_set_new(modem, "siglevel", json_integer(modem_info->asu));
	json_object_set_new(modem, "level", json_integer(modem_info->sigbar));
	json_object_set_new(modem, "dbm", json_integer(modem_info->dbm));
	json_object_set_new(modem, "reg_status", json_integer(modem_info->regstatus));
	json_object_set_new(modem, "network", json_string(net_str[modem_info->network]));
	json_object_set_new(modem, "submode", json_string(modem_info->submode_name));
	json_object_set_new(modem, "sim", json_integer(modem_info->current_sim));
	json_object_set_new(modem, "apn", json_string(modem_info->used_apn[0]));
	json_object_set_new(modem, "apn2", json_string(modem_info->used_apn[1]));
	json_object_set_new(modem, "operator", json_string(operator));
	json_object_set_new(modem, "lac", json_string(modem_info->lac));
	json_object_set_new(modem, "cell_id", json_string(modem_info->cellid));
	json_object_set_new(modem, "pci", json_string(cell->pci));
	json_object_set_new(modem, "arfcn", json_string(cell->arfcn));
	json_object_set_new(modem, "band", json_string(cell->band));

	if(get_pdp_type_setting() != PDP_TYPE_IPV6){
		ipv4 = json_object();
		json_object_set_new(head, "ipv4", ipv4);
		json_object_set_new(ipv4, "ip", json_string(ntoa(vif_info->local_ip)));
		json_object_set_new(ipv4, "prefix_len", json_integer(netmask_inaddr2len(vif_info->netmask)));
		json_object_set_new(ipv4, "gateway", json_string(ntoa(vif_info->remote_ip)));
		json_object_set_new(ipv4, "dns1", json_string(ntoa(vif_info->dns1)));
		json_object_set_new(ipv4, "dns2", json_string(ntoa(vif_info->dns2)));
		if(get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_DHCP
			|| get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_MIPC
			|| get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_QMI){
			json_object_set_new(ipv4, "mtu", json_integer(vif_info->mtu));
		}else{
			json_object_set_new(ipv4, "mtu", json_integer(ppp_config->mtu));
		}
#if ENABLE_DUAL_APN
		wwan2_ipv4 = json_object();
		json_object_set_new(head, "wwan2_ipv4", wwan2_ipv4);
		json_object_set_new(wwan2_ipv4, "ip", json_string(ntoa(secondary_vif_info->local_ip)));
		json_object_set_new(wwan2_ipv4, "prefix_len", json_integer(netmask_inaddr2len(secondary_vif_info->netmask)));
		json_object_set_new(wwan2_ipv4, "gateway", json_string(ntoa(secondary_vif_info->remote_ip)));
		json_object_set_new(wwan2_ipv4, "dns1", json_string(ntoa(secondary_vif_info->dns1)));
		json_object_set_new(wwan2_ipv4, "dns2", json_string(ntoa(secondary_vif_info->dns2)));
#endif
	}
	
	if(get_pdp_type_setting() != PDP_TYPE_IPV4){
		ipv6 = json_object();
		json_object_set_new(head, "ipv6", ipv6);
		addr6 = get_cellular_primary_addr6(vif_info);
		json_object_set_new(ipv6, "ip", json_string(addr6 ? inet6_ntoa(addr6->ip6) : ""));
		json_object_set_new(ipv6, "prefix_len", json_integer(addr6 ? addr6->prefix_len : 0));
	}

	data_usage = json_object();
	json_object_set_new(head, "data_usage", data_usage);
	for(i = SIM1; i < SIMX; i++){
		simx = json_object();
		json_object_set_new(data_usage, get_string_by_num(i, gl_main_sim_enum), simx);
		monthly_data = json_object();
		json_object_set_new(simx, "monthly_data", monthly_data);
		json_object_set_new(monthly_data, "status", json_string(get_string_by_num(sim_data_usage[i-1].status, gl_traffic_status_enum)));
		json_object_set_new(monthly_data, "used", json_integer(sim_data_usage[i-1].traffic));
	}

	s_repon = json_dumps(root, JSON_INDENT(0));
	json_decref(root);

	MYTRACE_LEAVE();

	return s_repon;	
}

static void parse_modem_config(MY_REDIAL_INFO *pinfo, const char *modem_name, json_t *modem)
{
	int i = 0;
	int prefix_len = 0;
	int err_code = ERR_OK;
	struct in_addr netmask;
	PPP_CONFIG *ppp_config = NULL;
	MODEM_CONFIG *modem_config = NULL;

	json_t *sim = NULL;
	json_t *ipv4 = NULL;
	json_t *dual_sim = NULL;
	char key_str[64] = {0};
	char value[256] = {0};
	char iface[32] = {0};
	char err_msg[MAX_BUFF_LEN] = {0};

	MYTRACE_ENTER();

	if(!pinfo || !modem_name || strcmp(modem_name, "modem") || !modem){
		//only support modem1 config now
		goto leave;
	}

	ppp_config = &(pinfo->priv.ppp_config);
	modem_config = &(pinfo->priv.modem_config);

#ifdef INHAND_WR3
	JSON_VALUE_INTEGER_RANGE(modem, "ant", modem_config->ant, 0, 1, 0);
#endif
	JSON_VALUE_BOOL(modem, "enabled", pinfo->priv.enable);
	pinfo->svcinfo.vif_info.enabled = pinfo->priv.enable;

	JSON_VALUE_BOOL_NAT(modem, "nat", pinfo->priv.nat);
	vif_get_sys_name(&(pinfo->svcinfo.vif_info.if_info), iface);
	uplink_interface_snat_set(iface, pinfo->priv.nat & pinfo->priv.enable);

	for(i = SIM1; i < SIMX; i++){
		snprintf(key_str, sizeof(key_str), "sim%d", i);
		if((sim = json_object_get(modem, key_str)) == NULL){
			continue;
		}
		/* start sim handle */
		IN_JSON_SUB_HANDLE_START(key_str);
		
		JSON_VALUE_DESC(sim, "desc", modem_config->sim_desc[i-1]);

		if(i == SIM1){
			JSON_VALUE_ENUM(sim, "profile", ppp_config->profile_idx, gl_sim1_profile_enum);
#if ENABLE_DUAL_APN
			JSON_VALUE_ENUM(sim, "profile2", ppp_config->profile_wwan2_idx, gl_sim1_profile2_enum);
			modem_config->dual_wwan_enable[0] = ppp_config->profile_wwan2_idx ? 1 : 0;
#endif
			JSON_VALUE_ENUM(sim, "network_type", modem_config->network_type, gl_net_type_enum);
		}else{
			JSON_VALUE_ENUM(sim, "profile", ppp_config->profile_2nd_idx, gl_sim2_profile_enum);
#if ENABLE_DUAL_APN
			JSON_VALUE_ENUM(sim, "profile2", ppp_config->profile_wwan2_2nd_idx, gl_sim2_profile2_enum);
			modem_config->dual_wwan_enable[1] = ppp_config->profile_wwan2_2nd_idx ? 1 : 0;
#endif
			JSON_VALUE_ENUM(sim, "network_type", modem_config->network_type2, gl_net_type_enum);
		}
		JSON_VALUE_ENUM(sim, "nr5g_mode", modem_config->nr5g_mode[i-1], gl_nr5g_mode_enum);
		JSON_VALUE_PINCODE(sim, "pin_code", modem_config->pincode[i-1]);
		JSON_VALUE_ENUM(sim, "pdp_type", modem_config->pdp_type[i-1], gl_pdp_type_enum);
		JSON_VALUE_ENUM(sim, "ims", modem_config->ims_mode[i-1], gl_ims_mode_enum);
		/* end sim handle */
		IN_JSON_HANDLE_END;
	}

	JSON_VALUE_ENUM(modem, "conn_mode", ppp_config->ppp_mode, gl_conn_mode_enum);
	
	snprintf(key_str, sizeof(key_str), "dual_sim");
	if((dual_sim = json_object_get(modem, key_str))){
		/* start dual_sim handle */
		IN_JSON_SUB_HANDLE_START(key_str);

		JSON_VALUE_BOOL(dual_sim, "enabled", modem_config->dualsim);
		JSON_VALUE_ENUM(dual_sim, "main_sim", modem_config->policy_main, gl_main_sim_enum);
		
		//doesn't support random and sequence mode
		unlink(SEQ_SIM_FILE);
		modem_config->main_sim = modem_config->policy_main;
		modem_config->backup_sim = SIM1+SIM2-modem_config->main_sim;
		if(pinfo->svcinfo.modem_info.current_sim != modem_config->main_sim){
			gl_sim_switch_flag = TRUE;
		}
		/* end dual_sim handle */
		IN_JSON_HANDLE_END;
	}
	
	JSON_VALUE_MTU2(modem, "mtu", ppp_config->mtu);

	snprintf(key_str, sizeof(key_str), "ipv4");
	if((ipv4 = json_object_get(modem, key_str))){
		/* start ipv4 handle */
		IN_JSON_SUB_HANDLE_START(key_str);
		
		JSON_VALUE_MASKLEN(ipv4, "prefix_len", prefix_len);
		if(err_code == ERR_OK){
			if(prefix_len > 0){
				masklen2ip(prefix_len, &netmask);
				memcpy(ppp_config->static_netmask, inet_ntoa(netmask), INET_ADDRSTRLEN);
			}else{
				memset(ppp_config->static_netmask, 0, sizeof(ppp_config->static_netmask));
			}
		}

		/* end ipv4 handle */
		IN_JSON_HANDLE_END;
	}

leave:
	MYTRACE_LEAVE();
}

static void parse_profile_config(MY_REDIAL_INFO *pinfo, json_t *profile)
{
	int idx = 0;
	int err_code = ERR_OK;
	char err_msg[MAX_BUFF_LEN] = {0};
	PPP_CONFIG *ppp_config = &(pinfo->priv.ppp_config);

	json_t *sub_obj = NULL;
	const char *sub_key = NULL;
	char value[256] = {0};

	MYTRACE_ENTER();

	IN_JSON_OBJECT_FOREACH(profile, sub_key, sub_obj)
		idx = atoi(sub_key);
		if(idx < 0 || idx > 3){
			IN_JSON_HANDLE_END;
			continue;
		}

		ppp_config->profiles[idx].type = PROFILE_TYPE_GSM;
		JSON_VALUE_ENUM(sub_obj, "type", ppp_config->profiles[idx].pdp_type, gl_pdp_type_enum);
		JSON_STRING_VALUE(sub_obj, "apn", ppp_config->profiles[idx].apn);
		JSON_STRING_VALUE(sub_obj, "access_num", ppp_config->profiles[idx].dialno);
		JSON_VALUE_ENUM(sub_obj, "auth", ppp_config->profiles[idx].auth_type, gl_auth_type_enum);
		JSON_STRING_VALUE(sub_obj, "username", ppp_config->profiles[idx].username);
		JSON_STRING_VALUE(sub_obj, "password", ppp_config->profiles[idx].password);

	IN_JSON_OBJECT_FOREACH_END

	MYTRACE_LEAVE();
}

static int check_cellular_config(MY_REDIAL_INFO *old_info, MY_REDIAL_INFO *new_info)
{
	VIF_INFO *pvif = &new_info->svcinfo.vif_info;
	PPP_CONFIG *old_ppp_config = &(old_info->priv.ppp_config);
	PPP_CONFIG *new_ppp_config = &(new_info->priv.ppp_config);
	MODEM_CONFIG *old_modem_config = &(old_info->priv.modem_config);
	MODEM_CONFIG *new_modem_config = &(new_info->priv.modem_config);

#define CONFIG_CMP(conf) \
	memcmp(&(old_##conf), &(new_##conf), sizeof(new_##conf))

	if(CONFIG_CMP(info->priv.enable)
#ifdef INHAND_WR3
		|| CONFIG_CMP(modem_config->ant)
#endif
		|| CONFIG_CMP(modem_config->network_type)
		|| CONFIG_CMP(modem_config->network_type2)
		|| CONFIG_CMP(modem_config->nr5g_mode[0])
		|| CONFIG_CMP(modem_config->nr5g_mode[1])
		|| CONFIG_CMP(modem_config->ims_mode[0])
		|| CONFIG_CMP(modem_config->ims_mode[1])
		|| CONFIG_CMP(modem_config->dual_wwan_enable[0])
		|| CONFIG_CMP(modem_config->dual_wwan_enable[1])
		|| CONFIG_CMP(modem_config->pdp_type[0])
		|| CONFIG_CMP(modem_config->pdp_type[1])
		|| CONFIG_CMP(modem_config->pincode[0])
		|| CONFIG_CMP(modem_config->pincode[1])
		|| CONFIG_CMP(modem_config->dualsim)
		|| CONFIG_CMP(modem_config->policy_main)
		|| CONFIG_CMP(ppp_config->profile_idx)
		|| CONFIG_CMP(ppp_config->profile_2nd_idx)
		|| CONFIG_CMP(ppp_config->ppp_mode)
		|| CONFIG_CMP(ppp_config->profiles[0])
		|| CONFIG_CMP(ppp_config->profiles[1])
		|| CONFIG_CMP(ppp_config->profiles[2])
		|| CONFIG_CMP(ppp_config->profiles[3])
		|| CONFIG_CMP(ppp_config->static_netmask)
		|| CONFIG_CMP(ppp_config->mtu)){

		gl_need_redial = TRUE;
	}

	if(CONFIG_CMP(info->priv.enable) && pvif->status == VIF_DOWN){
		gl_need_publish = TRUE;
	}

	return 0;
}

static char *cellular_config_for_json(char *query)
{
	MY_REDIAL_INFO gl_myinfo_tmp;
	MY_REDIAL_INFO *pinfo = &gl_myinfo_tmp;

	json_error_t failed;
	json_t *root  = NULL;
	json_t *sub_obj = NULL;
	const char *sub_key = NULL;
	char* s_repon = NULL;

	MYTRACE_ENTER();

	LOG_DB("query:%s", query ? query : "null");

	root = json_loads(query, JSON_INDENT(0), &failed);
	if (!root) {
		LOG_ER("cellular loads json error,%d:%s", failed.line, failed.text);
		goto leave;
	}

	gl_need_redial = FALSE;

	//copy config	
	memcpy(pinfo, &gl_myinfo, sizeof(MY_REDIAL_INFO));

	/* start root handle */
	IN_JSON_HANDLE_INIT("cellular");

	IN_JSON_OBJECT_FOREACH(root, sub_key, sub_obj)
		/* get modem config */	
		if(!strncmp(sub_key, "modem", 5)){
			parse_modem_config(pinfo, sub_key, sub_obj);
			/* end modem handle */
			IN_JSON_HANDLE_END;
			continue;
		}
		
		/* get profile config */
		if(!strcmp(sub_key, "profile")){
			parse_profile_config(pinfo, sub_obj);
			/* end profile handle */
			IN_JSON_HANDLE_END;
			continue;
		}
	IN_JSON_OBJECT_FOREACH_END

	/* end root handle */
	IN_JSON_HANDLE_END;
	
	check_cellular_config(&gl_myinfo, pinfo);

	//save config	
	memcpy(&gl_myinfo, pinfo, sizeof(MY_REDIAL_INFO));

	if(gl_need_redial){
		if(gl_need_publish){
			ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char*)&pinfo->svcinfo.vif_info, sizeof(VIF_INFO));
			gl_need_publish = FALSE;
		}
	
		do_redial_procedure();
		gl_need_redial = FALSE;
	}

leave:
	s_repon = show_cellular_config_to_json(root);
	json_decref(root);

	MYTRACE_LEAVE();
	return s_repon;
}

const request_api reqapi[] = {
	{CLOUD_WEB_SUB_REQ_CONFIG_TOPIC_OF(CLOUD_WEB_NODE_CELLULAR),	cellular_config_for_json},
	{CLOUD_WEB_SUB_REQ_STATUS_TOPIC_OF(CLOUD_WEB_NODE_CELLULAR),	show_cellular_status_to_json},
	
	{ NULL,				NULL}
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
	strlcpy(sub_info->topic[0], CLOUD_WEB_REQ_TOPIC_OF(CLOUD_WEB_NODE_CELLULAR), sizeof(sub_info->topic[0]));
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

