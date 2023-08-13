/*
 * $Id:
 *
 *   Redial service
 *
 * Copyright (c) 2001-2012 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 01/10/2012
 * Author: Zhangly
 * Description:
 * 		manage modem and pppd
 *
 */

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

#include "ih_ipc.h"
#include "redial_ipc.h"
#include "ih_cmd.h"
#include "sw_ipc.h"
#include "shared.h"
#include "ih_updown_ipc.h"
#include "mipc_wwan_ipc.h"
#include "ih_vif.h"
#include "netwatcher_ipc.h"
#include "traffic_stat_ipc.h"
#include "backup_ipc.h"
#include "sdwan_ipc.h"
#include "sms_ipc.h"
#include "validate.h"
#include "vif_shared.h"

#include "modem.h"
#include "ppp.h"
#include "redial.h"
#include "operator.h"
#include "sms.h"
#include "agent_ipc.h"
#include "redial_factory.h"

#include "build_info.h"
#include "mbim.h"
#include "nm_events.h"
#include "ih_events.h"

#include "ip_passth.h"

#if defined NEW_WEBUI
#include "redial_web.h"
#endif

#define MAX_PVS8_ACTIVATION_RETRY	3
#define MAX_MODEM_RESET	120
#define MAX_PPP_REDIAL	30
#define MAX_SIM_RETRY	30
#define MAX_OPEN_USB_RETRY	3
#define NAI_ENABLE              1
#define NAI_DISABLE             0
#define NETWORK_CONFIG_FILE_DIR		"/etc/networks.drv"
#define MAX_SIGNAL_RETRY	3
#define MAX_DHCP_RETRY		3
#define MAX_UPDATE_APN_RETRY		20
#define MAX_REG_RETRY		5
#define MAX_CHECK_MODEM_RETRY		5
#define OPT_CODE_LEN		6
#define DEFAULT_DIAL_NUMBER				"*99#"
#define LE910_DIAL_NUMBER				"*99***3#"
#define DEFAULT_EVDO_DIAL_NUMBER		"#777"
#define DEFAULT_APN_CM			"cmnet"
#define DEFAULT_APN_CU			"uninet"
#define DEFAULT_USERNAME		"gprs"
#define DEFAULT_PASSWORD		"gprs"
#define DEFAULT_EVDO_USERNAME	"card"
#define DEFAULT_EVDO_PASSWORD	"card"
#define DEFAULT_IPV6_PD_LABEL	"cell-1"
#define DEFAULT_WWAN2_IPV6_PD_LABEL	"cell-2"
#define DEFAULT_IPV6_SHARED_LABEL	"cellular-1"
#define DEFAULT_WWAN2_IPV6_SHARED_LABEL	"cellular-2"
#define DEFAULT_IPV6_IF_ID	"0:0:0:1"
#define U8300C_MORE_AT_OFFSET	8
#define ELS31_UPDATE_APN_MAX_RETRY	20

#define SYS_USB0_ADDR_LEN_PATH  "/sys/class/net/usb0/addr_len"

#define SUB_DAEMON_RUNNING \
 	((gl_ppp_pid > 0)  \
		|| (gl_2nd_ppp_pid > 0) \
		|| (gl_odhcp6c_pid > 0) \
		|| (gl_2nd_odhcp6c_pid > 0))

extern int gl_wcdma_at_lists[];
extern int gl_evdo_at_lists[];
extern int gl_evdo_nosim_at_lists[];
extern int gl_hspaplus_at_lists[];
extern int gl_hspaplus_mu609_at_lists[];
extern int gl_hspaplus_phs8_at_lists[];
extern int gl_lte_at_lists[];
extern int gl_lte_me909_at_lists[];
extern int gl_lte_me209_at_lists[];
extern int gl_lte_lua10_at_lists[];
extern int gl_evdo_pvs8_at_lists[];
extern int gl_lte_pls8_at_lists[];
extern int gl_lte_ec25_at_lists[];
extern int gl_nr_rm500_at_lists[];
extern int gl_nr_rm520n_at_lists[];
extern int gl_nr_rm500u_at_lists[];
extern int gl_lte_ep06_at_lists[];
extern int gl_lte_ec20_at_lists[];
extern int gl_lte_u8300_at_lists[];
extern int gl_lte_u8300c_at_lists[];
extern int gl_lte_mc7350_at_lists[];
extern int gl_wpd200_at_lists[];
extern int gl_telit_lte_at_lists[];
extern int gl_lp41_lte_at_lists[];
extern int gl_lte_els31_at_lists[];
extern int gl_lte_els61_at_lists[];
extern int gl_ublox_lara_cat1_at_lists[];
extern int gl_ublox_toby_lte_at_lists[];
extern int gl_nr_fm650_at_lists[];
extern int gl_nr_fg360_at_lists[];
extern int gl_lte_els81_at_lists[];
extern int gl_lte_nl668_at_lists[];

extern int check_modem_phonenum(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
extern int check_modem_vphonenum(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
extern int check_modem_sysinfo(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
extern int check_modem_cellid(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
extern int check_modem_smoni(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
extern int check_modem_network_sel(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
extern int check_pls8_connection_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
extern int check_modem_network(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
extern int check_modem_reg(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
extern int check_modem_qnetdevstatus(const char *dev, int cid, struct in_addr local_ip, int verbose);
extern int check_modem_nv_parameter(int verbose);
extern int check_modem_basic_info(MODEM_INFO *info);
extern int get_modem_preferred_operator_list(char *dev, int verbose);
extern int enable_ipv4v6_for_tmobile_profile(char *dev, int verbose);
extern int enable_modem_auto_time_zone_update(char *dev, int verbose);
extern int redial_sysrepo_init(void);
extern int check_nr5g_mode(char *dev, int verbose);
extern int check_rm500q_data_iface(char *dev, int iface_id, int verbose);
extern int check_rm500u_pcie_mode(char *dev, int mode, int verbose);
extern int check_fm650_data_iface(char *dev, const char *iface, int verbose);
extern int check_fm650_ippass(char *dev, int verbose);
extern int check_rm520n_eth_setting(char *dev, int verbose);

extern int write_fd(int fd, char* buf, size_t size, int timeout, int verbose);
int auto_select_net_apn(char *imsi_code, BOOL debug);
static int check_current_net_type(char *dev); 
extern int check_wpd200_rssi_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
extern void fixup_u8300c_at_list1(void);
//static void  set_sub_deamon_type(void);
static int execute_init_command(char *commdev, char *init_str); 
char *get_sub_deamon_name(void);
int get_sub_deamon_type(int idx); 
static int modem_force_ps_reattch(void);
static int ublox_bridge_mode_get_net_paras(char *dev, BOOL init, int verbose);
int modem_enter_airplane_mode(char *commdev); 
int redial_modem_set_cfun_mode(int mode, BOOL debug);
int redial_modem_re_register(BOOL debug); 
int redial_modem_de_register(BOOL debug); 
int redial_modem_force_register(BOOL debug); 
int redial_els_modem_function_reset(BOOL debug);
void show_data_usage(MSG_DATA_USAGE *msg);

#ifdef INHAND_WR3
static void redial_set_ant(bool ant);
#endif
#if 0
static void clear_at_state(void);
#endif
static char gl_trace_file[MAX_PATH];

///////////////////////////////////////////////////////////////////////////
// MODEM CODE
struct _modem_type{
	uint64_t modem_code;
	char commport[16];
	char dataport[16];
	int *at_lists;
	char pcieport[16];
	int pci_vid;
};

static struct _modem_type gl_modem_types[]={
	//code comm data at
	//{IH_FEATURE_MODEM_EXT, "", "", NULL},
	{IH_FEATURE_MODEM_NONE, "", "", NULL},
#if (defined INHAND_IP812)
	{IH_FEATURE_MODEM_MU609, "/dev/ttyUSB3", "/dev/ttyUSB1", gl_hspaplus_mu609_at_lists},
	{IH_FEATURE_MODEM_LUA10, "/dev/ttyUSB3", "/dev/ttyUSB2", gl_lte_lua10_at_lists},
	{IH_FEATURE_MODEM_U8300, "/dev/ttyUSB2", "/dev/ttyUSB1", gl_lte_u8300_at_lists},
	{IH_FEATURE_MODEM_MC2716, "/dev/ttyUSB1", "/dev/ttyUSB0", gl_evdo_at_lists},
#ifdef INHAND_IP812
	{IH_FEATURE_MODEM_ME909, "/dev/ttyUSB3", "/dev/ttyUSB1", gl_lte_me909_at_lists},
#else
	{IH_FEATURE_MODEM_ME909, "/dev/ttyUSB2", "/dev/ttyUSB0", gl_lte_me909_at_lists},
#endif
#else
	{IH_FEATURE_MODEM_EM770W, "/dev/ttyUSB2", "/dev/ttyUSB0", gl_wcdma_at_lists},
	{IH_FEATURE_MODEM_EM820W, "/dev/ttyUSB2", "/dev/ttyUSB0", gl_hspaplus_at_lists},
	//version: 11.103.58.00.00
	//{IH_FEATURE_MODEM_MU609, "/dev/ttyUSB1", "/dev/ttyUSB0", gl_hspaplus_mu609_at_lists},
	//version: 11.103.77.00.00
	{IH_FEATURE_MODEM_MU609, "/dev/ttyUSB2", "/dev/ttyUSB0", gl_hspaplus_mu609_at_lists},
	{IH_FEATURE_MODEM_PHS8, "/dev/ttyUSB2", "/dev/ttyUSB3", gl_hspaplus_phs8_at_lists},
	{IH_FEATURE_MODEM_PVS8, "/dev/ttyUSB2", "/dev/ttyUSB3", gl_evdo_pvs8_at_lists},
	//{IH_FEATURE_MODEM_PLS8, "/dev/ttyUSB2", "/dev/ttyUSB2", gl_lte_pls8_at_lists},
	//{IH_FEATURE_MODEM_PLS8, "/dev/ttyUSB3", "/dev/ttyUSB3", gl_lte_pls8_at_lists},
	{IH_FEATURE_MODEM_PLS8, "/dev/ttyACM1", "/dev/ttyACM1", gl_lte_pls8_at_lists},
	{IH_FEATURE_MODEM_ELS61, "/dev/ttyACM0", "/dev/ttyACM1", gl_lte_els61_at_lists},
	{IH_FEATURE_MODEM_ELS81, "/dev/ttyACM0", "/dev/cdc-wdm0", gl_lte_els81_at_lists},
	{IH_FEATURE_MODEM_MC7710, "/dev/ttyUSB4", "/dev/ttyUSB3", gl_lte_at_lists},
	{IH_FEATURE_MODEM_MC2716, "/dev/ttyUSB1", "/dev/ttyUSB0", gl_evdo_at_lists},
	{IH_FEATURE_MODEM_MC5635, "/dev/ttyUSB2", "/dev/ttyUSB1", gl_evdo_nosim_at_lists},
	{IH_FEATURE_MODEM_ME909, "/dev/ttyUSB2", "/dev/ttyUSB0", gl_lte_me909_at_lists},
	{IH_FEATURE_MODEM_LUA10, "/dev/ttyUSB3", "/dev/ttyUSB2", gl_lte_lua10_at_lists},
	{IH_FEATURE_MODEM_U8300, "/dev/ttyUSB2", "/dev/ttyUSB1", gl_lte_u8300_at_lists},
	{IH_FEATURE_MODEM_MC7350, "/dev/ttyUSB2", "/dev/ttyUSB0", gl_lte_mc7350_at_lists},
	{IH_FEATURE_MODEM_WPD200, "/dev/ttyUSB2", "/dev/ttyUSB0", gl_wpd200_at_lists},
	{IH_FEATURE_MODEM_U8300C, "/dev/ttyUSB2", "/dev/ttyUSB1", gl_lte_u8300c_at_lists},
	{IH_FEATURE_MODEM_LE910, "/dev/ttyUSB4", "/dev/ttyUSB5", gl_telit_lte_at_lists},
	{IH_FEATURE_MODEM_ME209, "/dev/ttyUSB2", "/dev/ttyUSB0", gl_lte_me209_at_lists},
	{IH_FEATURE_MODEM_LP41, "/dev/ttyACM0", "/dev/ttyACM0", gl_lp41_lte_at_lists},
	{IH_FEATURE_MODEM_ELS31, "/dev/ttyACM0", "/dev/ttyACM0", gl_lte_els31_at_lists},
	{IH_FEATURE_MODEM_LARAR2, "/dev/ttyUSB1", "/dev/ttyUSB3", gl_ublox_lara_cat1_at_lists},
	{IH_FEATURE_MODEM_TOBYL201, "/dev/ttyACM0", "/dev/ttyACM0", gl_ublox_toby_lte_at_lists},
	{IH_FEATURE_MODEM_EC25, "/dev/ttyUSB2", "/dev/ttyUSB3", gl_lte_ec25_at_lists},
	{IH_FEATURE_MODEM_RM500, "/dev/ttyUSB2", "/dev/ttyUSB2", gl_nr_rm500_at_lists, "/dev/mhi_DUN", 0x17cb},
	{IH_FEATURE_MODEM_RM520N, "/dev/ttyUSB2", "/dev/ttyUSB2", gl_nr_rm520n_at_lists},
	{IH_FEATURE_MODEM_RM500U, "/dev/ttyUSB2", "/dev/ttyUSB3", gl_nr_rm500u_at_lists, "/dev/stty_nr31", 0x16c3},
	{IH_FEATURE_MODEM_RG500, "/dev/ttyUSB2", "/dev/ttyUSB3", gl_nr_rm500_at_lists},
	{IH_FEATURE_MODEM_EP06, "/dev/ttyUSB2", "/dev/ttyUSB3", gl_lte_ep06_at_lists},
	{IH_FEATURE_MODEM_EC20, "/dev/ttyUSB2", "/dev/ttyUSB3", gl_lte_ec20_at_lists},
	{IH_FEATURE_MODEM_FM650, "/dev/ttyUSB0", "/dev/ttyUSB1", gl_nr_fm650_at_lists},
	{IH_FEATURE_MODEM_FG360, "17171", 	"17171", 	gl_nr_fg360_at_lists},
	{IH_FEATURE_MODEM_NL668, "/dev/ttyUSB2", "/dev/ttyUSB1", gl_lte_nl668_at_lists},
#endif
	{0}
};

char *gl_pdp_type_str[] = {"IPV4V6", "IP", "IPV6", "IPV4V6"};
char *gl_auth_type_str[] = {"auto", "pap", "chap", "mschap","mschapv2"};
static char dhcpc_evt_str[][16] = {"deconfig", "bound", "renew", "leasefail", "nak"};
/**
 * find modem type index by name
 * @param	modem_code	modem code
 * @return	>=0 for type index, <0 for error
 */
static int
find_modem_type_idx(const uint64_t modem_code)
{
	int i;

	for(i=0; gl_modem_types[i].modem_code; i++){
		if(gl_modem_types[i].modem_code==modem_code){
			return i;
		}
	}

	return -1;
}

/**
 * get modem code of this router
 * @param	idx		modem types idx
 * @return	modem code
 */
uint64_t
get_modem_code(int idx)
{
	struct _modem_type *ptype = &gl_modem_types[idx];

	return ptype->modem_code;
}

#if 0
///////////////////////////////////////////////////////////////////////////
// SERVICE DEFINITIONS
// service data
typedef struct {
	//public part
	REDIAL_SVCINFO svcinfo;//vif + modem

	//private part
	struct {
		//setup by cli 
		BOOL	debug;
		BOOL	ppp_debug;
		BOOL	enable;
		int	dial_interval;
		int	signal_interval;
		MODEM_CONFIG modem_config;
		PPP_CONFIG ppp_config;
		PPP_CONFIG wwan2_config;
		DUALSIM_INFO dualsim_info;
		char description[64];

		//backup interface
		BOOL backup;
		int backup_action;

		//sms config
		SMS_CONFIG sms_config;

		CELLULAR_SUB_IF sub_ifs[REDIAL_SUBIF_MAX];
	}priv;
}MY_INFO;
#endif

///////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
static struct event gl_ev_tmr;
static struct event gl_ev_at;
static struct event gl_ev_sms_tmr;
static struct event gl_ev_signal_tmr;
static struct event gl_ev_ppp_tmr;//monitor ppp dial
static struct event gl_ev_ppp_up_tmr;//added by baodn
static struct event gl_ev_dualsim_tmr;//added by baodn
static struct event gl_ev_wwan_connect_tmr;//added by liyb to monitor PLS8 WWAN0 connect.
static struct event gl_ev_redial_pending_task_tmr;//added by shenqw to run task which need to execute at commond.
static char gl_at_buf[512];
int gl_at_index = 0;
static int gl_at_try_n = 0;
static int gl_at_errno = ERR_AT_OK;
pid_t gl_ppp_pid = -1;
pid_t gl_odhcp6c_pid = -1;
pid_t gl_2nd_ppp_pid = -1;
pid_t gl_2nd_odhcp6c_pid = -1;
pid_t gl_qmi_proxy_pid = -1;
static pid_t gl_smsd_pid = -1;
static pid_t gl_npd_pid = -1;
int *gl_at_lists;
int gl_creg_at_lists[5] = {-1};
int gl_modem_idx = 0;
char *gl_commport = NULL;
static char *gl_dataport = NULL;
static unsigned gl_modem_reset_cnt = 0;
static int gl_modem_dev_state = 0;
static int gl_els31_update_apn_cnt = 0;
int gl_ppp_redial_cnt = 0;
static int gl_dualsim_retry_cnt = 0;
static int gl_dualsim_signal_retry_cnt = 0;
static int gl_pvs8_activation_retry_cnt = 0;
static int gl_open_usbserial_retry_cnt = 0;
static int gl_check_signal_retry_cnt = 0;
static int gl_check_modem_err_cnt = 0;
static int gl_wait_reconnect_cnt = 0;
static int gl_pending_task_retry_cnt = 0;
//static int gl_redial_sub_deamon_type = SUB_DEAMON_PPP;
static int gl_timeout = 0;
static BOOL gl_dual_wwan_enable = FALSE;
MSG_DATA_USAGE sim_data_usage[SIM2];

int gl_at_fd = -1;
uint64_t gl_modem_code = 0;

IH_SVC_ID gl_my_svc_id = IH_SVC_REDIAL;
MY_REDIAL_INFO gl_myinfo;
static MY_REDIAL_INFO myinfo_tmp;

static BOOL gl_debug = FALSE;
static BOOL gl_got_sysinfo = FALSE;
static BOOL gl_config_ready = FALSE;
static BOOL gl_got_usbinfo = FALSE;
static BOOL gl_at_ready = FALSE;
static time_t gl_init_timestamp = 0;
BOOL gl_sim_switch_flag = FALSE;
BOOL gl_modem_reset_flag = FALSE;
BOOL gl_modem_cfun_reset_flag = FALSE;
BOOL gl_force_suspend = FALSE;
static REDIAL_STATE gl_redial_state = REDIAL_STATE_INIT;
REDIAL_DIAL_STATE gl_redial_dial_state = REDIAL_DIAL_STATE_NONE;
static REDIAL_PENDING_TASK gl_redial_pending_task = REDIAL_PENDING_TASK_NONE;
//static BOOL gl_need_function_reset = FALSE;

MY_GLOBAL gl = {
	.default_cid = -1,
};

static const MY_REDIAL_INFO	gl_default_myinfo = {
	.svcinfo = {
		.modem_info = {
			.current_sim = SIM1,
		},
		.vif_info = {
			.if_info = {.type = IF_TYPE_CELLULAR, .slot = 0, .port = 1, .sid = 0},
			.status = VIF_DESTROY,
			.sim_id = SIM1,
		},
		.secondary_vif_info= {
			.if_info = {.type = IF_TYPE_CELLULAR, .slot = 0, .port = 2, .sid = 0},
			.status = VIF_DESTROY,
			.sim_id = SIM1,
		},
	},
	.priv = {
		.debug = FALSE,
		.ppp_debug = FALSE,
		.enable = TRUE,
		.nat = TRUE,
		.dial_interval = REDIAL_DEFAULT_DIAL_INTERVAL,
		.signal_interval = REDIAL_DEFAULT_CSQ_INTERVAL,
		.ppp_config = {
			.profile_idx = 0,//1,2...
			.profile_2nd_idx = 0,//1,2...
			.profiles[0] = {PROFILE_TYPE_GSM,"3gnet","*99***1#",PDP_TYPE_IPV4,AUTH_TYPE_AUTO,"gprs","gprs"},
			.default_profile = {PROFILE_TYPE_GSM,"3gnet","*99***1#",PDP_TYPE_IPV4,AUTH_TYPE_AUTO,"gprs","gprs"},
			.mtu = 1500,
			.mru = 1500,
			.timeout = REDIAL_DEFAULT_TIMEOUT,
			.peerdns = 0,
			.ppp_mode = PPP_ONLINE,
			.trig_data = 0,
			.trig_sms = 0,
			.idle = REDIAL_DEFAULT_IDLE,
			.ppp_static = 0,
			.ppp_peer = "1.1.1.3",
			.lcp_echo_interval = 55,
			.lcp_echo_retries = 5,
			.default_am = 0,
			.nopcomp = 0,
			.noaccomp = 0,
			.vj = 0,
			.novjccomp = 0,
			.ppp_comp = PPP_COMP_NONE,
			.prefix_share = FALSE,
			.shared_label = DEFAULT_IPV6_SHARED_LABEL,
			.pd_label = DEFAULT_IPV6_PD_LABEL,
			.ndp_enable = FALSE,
			.static_if_id_enable = FALSE,
			.static_interface_id = DEFAULT_IPV6_IF_ID,
#if defined INHAND_ER8 || defined INHAND_ER6 || defined INHAND_WR3 || defined INHAND_ER3 //ER8 & ER6 & WR3 redial infinitely
			.infinitely_dial_retry = 1,
#else
			.infinitely_dial_retry = 0,
#endif
		},
		.wwan2_config = {
			.profile_idx = 0,//1,2...
			.profile_2nd_idx = 0,//1,2...
			.profiles[0] = {PROFILE_TYPE_GSM,"3gnet","*99***1#",PDP_TYPE_IPV4,AUTH_TYPE_AUTO,"gprs","gprs"},
			.default_profile = {PROFILE_TYPE_GSM,"3gnet","*99***1#",PDP_TYPE_IPV4,AUTH_TYPE_AUTO,"gprs","gprs"},
			.mtu = 1500,
			.mru = 1500,
			.timeout = REDIAL_DEFAULT_TIMEOUT,
			.peerdns = 0,
			.ppp_mode = PPP_ONLINE,
			.trig_data = 0,
			.trig_sms = 0,
			.idle = REDIAL_DEFAULT_IDLE,
			.ppp_static = 0,
			.ppp_peer = "1.1.1.3",
			.lcp_echo_interval = 55,
			.lcp_echo_retries = 5,
			.default_am = 0,
			.nopcomp = 0,
			.noaccomp = 0,
			.vj = 0,
			.novjccomp = 0,
			.ppp_comp = PPP_COMP_NONE,
			.prefix_share = FALSE,
			.shared_label = DEFAULT_WWAN2_IPV6_SHARED_LABEL,
			.pd_label = DEFAULT_WWAN2_IPV6_PD_LABEL,
			.ndp_enable = FALSE,
			.static_if_id_enable = FALSE,
			.static_interface_id = DEFAULT_IPV6_IF_ID,
		},
		.modem_config={
			.network_type = MODEM_NETWORK_AUTO,
			.dualsim = 0,
			.main_sim = SIM1,
			.policy_main = SIM1,
			.backup_sim = SIM2,
			.csq[0] = 0,
			.csq[1] = 0,
			.csq_interval[0] = 0,
			.csq_interval[1] = 0,
			.csq_retries[0] = 0,
			.csq_retries[1] = 0,
			.roam_enable[0] = 1,
			.roam_enable[1] = 1,
			.dual_wwan_enable[0] = 0,
			.dual_wwan_enable[1] = 0,
			.pdp_type[0] = PDP_TYPE_IPV4,
			.pdp_type[1] = PDP_TYPE_IPV4,
			.activate_enable = 0,
			.activate_no = "*22899"
		},
		.dualsim_info={
			.retries = 5,
			.backtime = 0,
			.uptime = 0,
		},
		.sms_config={
			.enable = 0,
			.mode = SMS_MODE_TEXT,
			.interval = SMS_DEFAULT_INTERVAL,
		},
	}
};
#define PVS8_MAX_REDIAL_INTERVAL	15
#define FT10_AUTO_REDIAL_INTERVAL	20
static const int redial_interval[] ={0,0,0,1,2,8,15,15};
static BOOL pvs8_activation = FALSE;
int pvs8_throttling_net = MODEM_NETWORK_AUTO;

///////////////////////////////////////////////////////////////////////////
// GLOBAL DECLARATIONS
int32 stop_ppp(void);
int32 stop_dhcpc(int *pid);
int32 stop_ih_qmi(pid_t *qmi_pid);
int32 stop_mipc_wwan(pid_t *mipc_pid);
int32 stop_child_by_sub_daemon(void);
int check_sim_status(int sim);
void sim_traffic_status_update(MSG_DATA_USAGE *msg);
int do_redial_procedure(void);
static int32 start_ppp(void);
static int32 stop_smsd(void);
static int32 start_smsd(void);
static void start_redial_pending_task(REDIAL_PENDING_TASK task);
static void stop_redial_pending_task(REDIAL_PENDING_TASK task);
static void clear_at_state(void);
static void clear_sim_status(void);
static int get_pdp_context_id(int wwan_id);
int get_pdp_type_setting(void);
void my_restart_timer(int timeout);
static void my_stop_timer(struct event *evt);
static int active_pdp_text(char *commdev, PPP_CONFIG *info, MODEM_CONFIG *modem_config, MODEM_INFO *modem_info, BOOL debug);
static int get_profile_setting(PPP_PROFILE **p_profile, PPP_PROFILE **p_2nd_wwan_profile, PPP_CONFIG *info, MODEM_CONFIG *modem_config, MODEM_INFO *modem_info);
static int setup_modem_pdp_parameters(char *commdev, int pdp_index, int pdp_type, char *apn, BOOL debug);
static int setup_modem_auth_parameters(char *commdev, PPP_PROFILE *p_profile, int pdp_index, BOOL debug);
int redial_modem_function_reset(BOOL debug); 
BOOL dual_apn_is_enabled(void);

///////////////////////////////////////////////////////////////////////////
// RESTART
static BOOL	  gl_restart = FALSE;
static char **gl_xargv;
static int    gl_xargc;

/*
 * stop this service
 */
static void stop()
{
	gl_restart = FALSE;
	event_base_loopbreak(g_my_event_base);
}

/*
 * restart this service
 */
static void restart()
{
	gl_restart = TRUE;
	event_base_loopbreak(g_my_event_base);
}

static void  schedule_router_or_modem_reboot() 
{
	if (!gl_myinfo.priv.ppp_config.infinitely_dial_retry || gl_at_errno == ERR_AT_NOMODEM) {
		LOG_ER("redial service is rebooting the system.");
		system("reboot");
		while(1) sleep(1000);
	} else {
		LOG_IN("max redial times is infinite, don't reboot.");
	}

	return;
}

static char gl_running_state_file[MAX_PATH];
/*
 * load running state from dump file
 *
 * @return ERR_OK for ok, others for error
 */
static int32 my_load_running_state()
{
	FILE *fp;
	int ret;

	fp = fopen(gl_running_state_file, "rb");
	if (!fp) {
		MYTRACE("cannot load running state from %s!",
				gl_running_state_file);
		return ERR_NOENT;
	}

	ret = safe_fread(&g_sysinfo, sizeof(g_sysinfo), 1, fp);
	if (ret != 1) LOG_WA("failed to load global data");
	ret = safe_fread(&gl, sizeof(gl), 1, fp);
	if (ret != 1) LOG_WA("failed to load global data");
	ret = safe_fread(&gl_myinfo, sizeof(gl_myinfo), 1, fp);
	if (ret != 1) LOG_WA("failed to load service data");

	fclose(fp);

	//bzero(&(gl_myinfo.svcinfo), sizeof(gl_myinfo.svcinfo));
	gl_myinfo.svcinfo.vif_info.status = VIF_DOWN; 

#ifdef NETCONF
	redial_sysrepo_init();
#endif

	return ERR_OK;
}

/*
 * dump running state to file, so we can recover quickly from it in case of failure
 *
 * @return ERR_OK for ok, others for error
 */
static int32 my_dump_running_state()
{
	FILE *fp;
	int ret;

	fp = fopen(gl_running_state_file, "wb");
	if (!fp) {
		LOG_ER("cannot dump running state to %s, we will not be able to recover in case of a fatal error!",
				gl_running_state_file);
		return ERR_NOENT;
	}

	//FIXME: make sure write is completed and is not interrupted by signals!
	ret = safe_fwrite(&g_sysinfo, sizeof(g_sysinfo), 1, fp);
	if (ret != 1) LOG_WA("failed to save global data");
	ret = safe_fwrite(&gl, sizeof(gl), 1, fp);
	if (ret != 1) LOG_WA("failed to save global data");
	ret = safe_fwrite(&gl_myinfo, sizeof(gl_myinfo), 1, fp);
	if (ret!=1) LOG_WA("failed to save service data");

	fclose(fp);

	return ERR_OK;
}

//////////////////////////////////////////////////////////////////////////////
// MESSAGE EVENT
/*
 * handle SYSINFO broadcast
 *
 * @param peer_svc_id		cmd from which service (should be IH_SVC_SYSWATCHER)
 * @param msg				received IPC msg
 * @param rsp_reqd			whether to send response
 *
 * @return ERR_OK for ok, others for error
 */
static int32 my_sysinfo_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen) 
{
	BOOL ipv6_enable_changed = FALSE;
	MYTRACE_ENTER();

	if (((IH_SYSINFO *)msg)->ipv6_enable != g_sysinfo.ipv6_enable) {
		ipv6_enable_changed = TRUE;
	}

	if (update_sysinfo((IH_SYSINFO *)msg)) {
		my_dump_running_state();
	}
	
	LOG_DB("recieve sysinfo! I am ready!");
	gl_got_sysinfo = TRUE;
	ih_ipc_set_ready(TRUE);

	ih_license_init(g_sysinfo.family_name, g_sysinfo.model_name, g_sysinfo.product_number);	
	
	//set_sub_deamon_type();
	
	if (ipv6_enable_changed) {
		do_redial_procedure();
	}

	MYTRACE_LEAVE();

	return ERR_OK;
}

static void show_cellular_config(uns32 flag)
{
	int i,got;
	PPP_CONFIG *ppp_config = &gl_myinfo.priv.ppp_config;
	PPP_CONFIG *wwan2_config = &gl_myinfo.priv.wwan2_config;
	char *auth[] = {"auto", "pap", "chap", "ms-chap", "ms-chapv2"};
	char *comp[] = {"", "bsdcomp", "deflate", "mppc"};
	char *operator[] = {"auto", "verizon", "others"};
	char *nr5g_mode_str[] = {"auto", "nsa", "sa"};
	DUALSIM_INFO *dualsim_info = &(gl_myinfo.priv.dualsim_info);
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	SMS_CONFIG *sms_config = &(gl_myinfo.priv.sms_config);
	CELLULAR_SUB_IF *sub_ifs = gl_myinfo.priv.sub_ifs;
	char *pdp_type_str[] = {"", "ipv4", "ipv6", "ipv4v6"};
	//MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;
	char *action[] = {"deny", "permit"};
	char *retptr = NULL;
	char prefix_addr[INET6_ADDRSTRLEN] = {0};
	
	if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_NONE){
		return;
	}

	switch (flag) {
	case IH_CMD_FLAG_JSON:
	//case IH_CMD_FLAG_CLI:
		ih_cmd_rsp_print("\ncellular1_config = {\n"
						"\t'enable': %d,\n"
						"\t'dial_interval': %d,\n"
						"\t'network_type': %d,\n"
						"\t'signal_interval': %d,\n"
						"\t'sim1_profile': %d,\n"
						"\t'sim2_profile': %d,\n"
						"\t'mtu': %d,\n"
						"\t'mru': %d,\n"
						"\t'ppp_timeout': %d,\n"
						"\t'peerdns': %d,\n"
						"\t'ppp_mode': %d,\n"
						"\t'trig_data': %d,\n"
						"\t'trig_sms': %d,\n"
						"\t'ppp_idle': %d,\n"
						"\t'ppp_static': %d,\n"
						"\t'ppp_ip': '%s',\n"
						"\t'ppp_peer': '%s',\n"
						"\t'static_netmask': '%s',\n"
						"\t'ppp_lcp_interval': %d,\n"
						"\t'ppp_lcp_retries': %d,\n"
						"\t'infinitely_dial_retry': %d,\n"
						"\t'sim1_roaming': %d,\n"
						"\t'sim2_roaming': %d,\n"
						"\t'dualsim_redial': %d,\n"
						"\t'dualsim_uptime': %d,\n"
						"\t'dualsim_csq1': %d,\n"
						"\t'dualsim_csq2': %d,\n"
						"\t'dualsim_csq_interval1': %d,\n"
						"\t'dualsim_csq_interval2': %d,\n"
						"\t'dualsim_csq_retries1': %d,\n"
						"\t'dualsim_csq_retries2': %d,\n"
						"\t'dualsim_time': %d,\n"
						"\t'main_sim': %d,\n"
						"\t'sim1_pincode': '%s',\n"
						"\t'sim2_pincode': '%s',\n"
						"\t'sim1_operator': %d,\n"
						"\t'sim2_operator': %d,\n"
						"\t'dualsim_enable': %d,\n"
						"\t'ppp_init': '%s',\n"
						"\t'ppp_am': '%d',\n"
						"\t'ppp_debug': '%d',\n"
						"\t'ppp_options': '%s',\n"
						"\t'support_req_mtu': %d,\n"
						"\t'request_mtu': %d,\n"
						"\t'activate_enable': '%d',\n"	
						"\t'activate_number': '%s',\n"
						"\t'phone_number': '%s',\n"
						"\t'nr5g_mode': '%d',\n"
						"\t'nai_username': '%s',\n"
						"\t'nai_password': '%s',\n"
						"\t'sim1_dual_wwan': %d,\n"
						"\t'sim2_dual_wwan': %d,\n"
						"\t'sim1_wwan2_profile': %d,\n"
						"\t'sim2_wwan2_profile': %d,\n"
						"\t'ndp_enable': '%d',\n"
						"\t'prefix_share': '%d',\n"
						"\t'shared_label': '%s',\n",
						gl_myinfo.priv.enable,
						gl_myinfo.priv.dial_interval,
						modem_config->network_type,
						gl_myinfo.priv.signal_interval,
						ppp_config->profile_idx,
						ppp_config->profile_2nd_idx,
						ppp_config->mtu,
						ppp_config->mru,
						ppp_config->timeout,
						ppp_config->peerdns,
						ppp_config->ppp_mode,
						ppp_config->trig_data,
						ppp_config->trig_sms,
						ppp_config->idle,
						ppp_config->ppp_static,
						ppp_config->ppp_ip,
						ppp_config->ppp_peer,
						ppp_config->static_netmask,
						ppp_config->lcp_echo_interval,
						ppp_config->lcp_echo_retries,
						ppp_config->infinitely_dial_retry,
						modem_config->roam_enable[0],
						modem_config->roam_enable[1],
						dualsim_info->retries,
						dualsim_info->uptime,
						modem_config->csq[0],
						modem_config->csq[1],
						modem_config->csq_interval[0],
						modem_config->csq_interval[1],
						modem_config->csq_retries[0],
						modem_config->csq_retries[1],
						dualsim_info->backtime,
						modem_config->policy_main,
						modem_config->pincode[0],
						modem_config->pincode[1],
						modem_config->sim_operator[0],
						modem_config->sim_operator[1],
						modem_config->dualsim,
						ppp_config->init_string,
						ppp_config->default_am,
						gl_myinfo.priv.ppp_debug,
						ppp_config->options,
						modem_support_dhcp_option_mtu(),
						(gl_myinfo.priv.dhcp_options & DHCP_OPT_MTU),
						modem_config->activate_enable,
						modem_config->activate_no,
						modem_config->phone_no,
						modem_config->nr5g_mode[0],
						(gl_myinfo.priv.modem_config.nai_no[0] ? gl_myinfo.priv.modem_config.nai_no : ""),
						(gl_myinfo.priv.modem_config.nai_pwd[0] ? gl_myinfo.priv.modem_config.nai_pwd : ""),
						modem_config->dual_wwan_enable[0],
						modem_config->dual_wwan_enable[1],
						ppp_config->profile_wwan2_idx,
						ppp_config->profile_wwan2_2nd_idx,
						(ppp_config->ndp_enable ? 1:0),
						(ppp_config->prefix_share ? 1:0),
						(ppp_config->shared_label[0] ? ppp_config->shared_label: ""));
		ih_cmd_rsp_print("\t'profiles':[");
		
		got = 0;
		for (i=0;i<PPP_MAX_PROFILE;i++) {
			if(ppp_config->profiles[i].type != PROFILE_TYPE_NONE) {
				if (got++ != 0) {
					ih_cmd_rsp_print(",");
				}
				//id,type,apn,callno,auth,username,pass
				ih_cmd_rsp_print("[\'%d\',\'%d\',\'%s\',\'%s\',\'%d\',\'%s\',\'%s\']",
					i+1,ppp_config->profiles[i].type,
					ppp_config->profiles[i].apn,
					ppp_config->profiles[i].dialno,
					ppp_config->profiles[i].auth_type,
					ppp_config->profiles[i].username,
					ppp_config->profiles[i].password);
			}
		}

/*
		if (!got) {
			ih_cmd_rsp_print("[\'\',\'\',\'\',\'\',\'\',\'\',\'\']");
		}
*/

		//ih_cmd_rsp_print("]\n};\n");
		ih_cmd_rsp_print("],");

		ih_cmd_rsp_print("\n'sms_config': {\n"
					"\t'enable': '%d',\n"
					"\t'mode': '%d',\n"
					"\t'interval': '%d',\n",
					sms_config->enable,
					sms_config->mode,
					sms_config->interval);
		//[id,action,phone]
		ih_cmd_rsp_print("\t'sms_acl':[ ");
		for(i=0; i<SMS_ACL_MAX; i++) {
			if(sms_config->acl[i].id) {
				ih_cmd_rsp_print("['%d','%d','%s','%d'],",
					sms_config->acl[i].id, sms_config->acl[i].action,
					sms_config->acl[i].phone, sms_config->acl[i].io_msg_enable);
			}
		}
		ih_cmd_rsp_drop_tail(1);
                ih_cmd_rsp_print("]\n}\n");
                ih_cmd_rsp_print("};\n");
		break;
	case IH_CMD_FLAG_CLI:
	default:
		if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_NONE){
			return;
		}

		ih_cmd_rsp_print("!\n#cellular config\n");

		for(i=0; i<PPP_MAX_PROFILE; i++) {
			if(ppp_config->profiles[i].type == PROFILE_TYPE_GSM) {
				if(!g_sysinfo.encrypt_passwd){
					ih_cmd_rsp_print("cellular 1 gsm profile %d %s %s %s %s %s %s\n", i+1,
							ppp_config->profiles[i].apn,ppp_config->profiles[i].dialno,
							pdp_type_str[ppp_config->profiles[i].pdp_type],
							auth[ppp_config->profiles[i].auth_type],ppp_config->profiles[i].username,
							ppp_config->profiles[i].password);
				}else{
					ih_cmd_rsp_print("cellular 1 gsm profile %d %s %s %s %s %s %s\n", i+1,
							ppp_config->profiles[i].apn,ppp_config->profiles[i].dialno,
							pdp_type_str[ppp_config->profiles[i].pdp_type],
							auth[ppp_config->profiles[i].auth_type],ppp_config->profiles[i].username,
							strlen(ppp_config->profiles[i].password)?encrypt_passwd(ppp_config->profiles[i].password):"");
				}
			} else if(ppp_config->profiles[i].type == PROFILE_TYPE_CDMA) {
				if(!g_sysinfo.encrypt_passwd){
					ih_cmd_rsp_print("cellular 1 cdma profile %d %s %s %s %s %s\n", i+1,
							ppp_config->profiles[i].dialno,
							pdp_type_str[ppp_config->profiles[i].pdp_type],
							auth[ppp_config->profiles[i].auth_type],ppp_config->profiles[i].username,
							ppp_config->profiles[i].password);
				}else{
					ih_cmd_rsp_print("cellular 1 cdma profile %d %s %s %s %s %s\n", i+1,
							ppp_config->profiles[i].dialno,
							pdp_type_str[ppp_config->profiles[i].pdp_type],
							auth[ppp_config->profiles[i].auth_type],ppp_config->profiles[i].username,
							strlen(ppp_config->profiles[i].password)?encrypt_passwd(ppp_config->profiles[i].password):"");
				}
			}
		}

		ih_cmd_rsp_print("cellular 1 dial interval %d\n", gl_myinfo.priv.dial_interval);
		ih_cmd_rsp_print("cellular 1 signal interval %d\n", gl_myinfo.priv.signal_interval);
		switch(modem_config->network_type) {
		case MODEM_NETWORK_5G4G:
			ih_cmd_rsp_print("cellular 1 network 5g4g\n");
			break;
		case MODEM_NETWORK_5G:
			ih_cmd_rsp_print("cellular 1 network 5g\n");
			break;
		case MODEM_NETWORK_4G:
			ih_cmd_rsp_print("cellular 1 network 4g\n");
			break;
		case MODEM_NETWORK_3G:
			ih_cmd_rsp_print("cellular 1 network 3g\n");
			break;
		case MODEM_NETWORK_2G:
			ih_cmd_rsp_print("cellular 1 network 2g\n");
			break;
		case MODEM_NETWORK_3G2G:
			ih_cmd_rsp_print("cellular 1 network 3g2g\n");
			break;
		case MODEM_NETWORK_EVDO:
			ih_cmd_rsp_print("cellular 1 network evdo\n");
			break;
		case MODEM_NETWORK_AUTO:
		default:
			ih_cmd_rsp_print("cellular 1 network auto\n");
			break;
		}
		
		ih_cmd_rsp_print("cellular 1 nr5g-mode %s\n", nr5g_mode_str[modem_config->nr5g_mode[0]]);

		if(modem_config->dualsim){
			ih_cmd_rsp_print("cellular 1 dual-sim enable\n");
			if(modem_config->policy_main == SIMX) ih_cmd_rsp_print("cellular 1 dual-sim main random\n");
			else if(modem_config->policy_main == SIM_SEQ) ih_cmd_rsp_print("cellular 1 dual-sim main sequence\n");
			else ih_cmd_rsp_print("cellular 1 dual-sim main %d\n",modem_config->main_sim);
			if(dualsim_info->retries>0)ih_cmd_rsp_print("cellular 1 dual-sim policy redial %d\n",dualsim_info->retries);
			if(modem_config->csq[0] > 0){
				ih_cmd_rsp_print("cellular 1 dual-sim policy csq %d",modem_config->csq[0]);
				if(modem_config->csq_interval[0]>=0) ih_cmd_rsp_print(" interval %d",modem_config->csq_interval[0]);
				if(modem_config->csq_retries[0]>=0) ih_cmd_rsp_print(" retries %d",modem_config->csq_retries[0]);
				ih_cmd_rsp_print("\n");
			}
			if(modem_config->csq[1] > 0){
				ih_cmd_rsp_print("cellular 1 dual-sim policy csq %d",modem_config->csq[1]);
				if(modem_config->csq_interval[1]>=0) ih_cmd_rsp_print(" interval %d",modem_config->csq_interval[1]);
				if(modem_config->csq_retries[1]>=0) ih_cmd_rsp_print(" retries %d",modem_config->csq_retries[1]);
				ih_cmd_rsp_print(" secondary\n");
			}
			if(dualsim_info->backtime>0) ih_cmd_rsp_print("cellular 1 dual-sim policy backtime %d\n",dualsim_info->backtime);
			if(dualsim_info->uptime>0) ih_cmd_rsp_print("cellular 1 dual-sim policy uptime %d\n",dualsim_info->uptime);
		}

		for(i=0; i<2; i++) {
			if(modem_config->pincode[i][0]) {
				if(!g_sysinfo.encrypt_passwd) {
					ih_cmd_rsp_print("cellular 1 sim %d %s\n", i+1,modem_config->pincode[i]);
				}else {
					ih_cmd_rsp_print("cellular 1 sim %d %s\n", i+1, encrypt_passwd(modem_config->pincode[i]));
				}
			}

			if(!modem_config->roam_enable[i]) ih_cmd_rsp_print("cellular 1 sim %d roaming forbid\n",  i+1);
			
			ih_cmd_rsp_print("cellular 1 sim %d operator %s\n", i+1, operator[modem_config->sim_operator[i]]);
		}
		if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PVS8){
			if(modem_config->activate_enable){
				ih_cmd_rsp_print("cellular 1 activation enable\n");
				if(modem_config->activate_no[0]){
					ih_cmd_rsp_print("cellular 1 activate-module %s",modem_config->activate_no);
					if(modem_config->phone_no[0]){
						ih_cmd_rsp_print(" %s\n",modem_config->phone_no);
					}else {
						ih_cmd_rsp_print("\n");
					}
				}
			}
		}
		
		if(sms_config->enable) {
			ih_cmd_rsp_print("cellular 1 sms enable\n");
		}

		if(sms_config->mode==SMS_MODE_PDU)
			ih_cmd_rsp_print("cellular 1 sms mode pdu\n");
		else
			ih_cmd_rsp_print("cellular 1 sms mode text\n");

		if(sms_config->interval>0) ih_cmd_rsp_print("cellular 1 sms interval %d\n",sms_config->interval);

		for(i=0; i<SMS_ACL_MAX; i++) {
			if(sms_config->acl[i].id) {
				ih_cmd_rsp_print("cellular 1 sms access-list %d %s %s %s\n",
						sms_config->acl[i].id, action[sms_config->acl[i].action],
						sms_config->acl[i].phone, sms_config->acl[i].io_msg_enable ? "enable": "disable");
			}
		}

		ih_cmd_rsp_print("!\n");
		ih_cmd_rsp_print("interface cellular 1\n");

		if (gl_myinfo.priv.modem_config.nai_no[0] && gl_myinfo.priv.modem_config.nai_pwd[0]){
			ih_cmd_rsp_print("  dialer nai %s %s\n", gl_myinfo.priv.modem_config.nai_no, gl_myinfo.priv.modem_config.nai_pwd);
		}

		if(gl_myinfo.priv.description[0]) ih_cmd_rsp_print("  description %s\n", gl_myinfo.priv.description);
		//if(gl_myinfo.priv.enable==TRUE) ih_cmd_rsp_print("  no shutdown\n");
		//else ih_cmd_rsp_print("  shutdown\n");
		if(gl_myinfo.priv.enable==FALSE) ih_cmd_rsp_print("  shutdown\n");
		if (0 != ppp_config->profile_idx) {
			ih_cmd_rsp_print("  dialer profile %d\n", ppp_config->profile_idx);
		}else {
			ih_cmd_rsp_print("  dialer profile auto\n");
		}
		
		if(ppp_config->profile_2nd_idx) {
			ih_cmd_rsp_print("  dialer profile %d secondary\n", ppp_config->profile_2nd_idx);
		}else {
			ih_cmd_rsp_print("  dialer profile auto secondary\n");
		}

		ih_cmd_rsp_print("  dialer timeout %d\n", ppp_config->timeout);
		
		if (ppp_config->prefix_share) {
			if (ppp_config->shared_label[0]) {
				ih_cmd_rsp_print("  ipv6 address share-prefix %s\n", ppp_config->shared_label);
			}else {
				ih_cmd_rsp_print("  ipv6 address share-prefix %s\n", DEFAULT_IPV6_SHARED_LABEL);
			}
		}

		if (ppp_config->pd_label[0]) {
			ih_cmd_rsp_print("  ipv6 address dhcp-pd %s\n", ppp_config->pd_label);
		}else {
			ih_cmd_rsp_print("  ipv6 address dhcp-pd %s\n", DEFAULT_IPV6_PD_LABEL);
		}

		if (wwan2_config->pd_label[0]) {
			ih_cmd_rsp_print("  ipv6 address dhcp-pd %s secondary\n", wwan2_config->pd_label);
		}else {
			ih_cmd_rsp_print("  ipv6 address dhcp-pd %s secondary\n", DEFAULT_WWAN2_IPV6_PD_LABEL);
		}

		if (ppp_config->static_if_id_enable) {
			if (ppp_config->static_interface_id[0]) {
				ih_cmd_rsp_print("  ipv6 address static interface-id %s\n", ppp_config->static_interface_id);
			}else {
				ih_cmd_rsp_print("  ipv6 address static interface-id %s\n", DEFAULT_IPV6_IF_ID);
			}
		}
		if (wwan2_config->static_if_id_enable) {
			if (wwan2_config->static_interface_id[0]) {
				ih_cmd_rsp_print("  ipv6 address static interface-id %s secondary\n", wwan2_config->static_interface_id);
			}else {
				ih_cmd_rsp_print("  ipv6 address static interface-id %s secondary\n", DEFAULT_IPV6_IF_ID);
			}
		}

		if (ppp_config->static_iapd_prefix_len > 0 && ppp_config->static_iapd_prefix_len <= 64) {
			retptr = (char *) inet_ntop(AF_INET6, &ppp_config->static_iapd_prefix, prefix_addr, INET6_ADDRSTRLEN);
			if (retptr) {
				ih_cmd_rsp_print("  ipv6 address static dhcp-pd ia-pd prefix-addr %s/%d\n", prefix_addr, ppp_config->static_iapd_prefix_len); 
			}
		}

		if (wwan2_config->static_iapd_prefix_len > 0 && wwan2_config->static_iapd_prefix_len <= 64) {
			retptr = (char *) inet_ntop(AF_INET6, &wwan2_config->static_iapd_prefix, prefix_addr, INET6_ADDRSTRLEN);
			if (retptr) {
				ih_cmd_rsp_print("  ipv6 address static dhcp-pd ia-pd prefix-addr %s/%d secondary\n", prefix_addr, wwan2_config->static_iapd_prefix_len); 
			}
		}

		if (gl_myinfo.priv.ppp_config.static_iapd_prefix_strict) {
			ih_cmd_rsp_print("  ipv6 address static dhcp-pd ia-pd strict\n"); 
		}

		if (gl_myinfo.priv.wwan2_config.static_iapd_prefix_strict) {
			ih_cmd_rsp_print("  ipv6 address static dhcp-pd ia-pd strict secondary\n"); 
		}

		if (ppp_config->ndp_enable) {
			ih_cmd_rsp_print("  ipv6 nd proxy\n");
		}

		if (modem_config->dual_wwan_enable[0]) {
			if (0 == ppp_config->profile_wwan2_idx) {
				ih_cmd_rsp_print("  dialer secondary-wan auto\n");
			}else {
				ih_cmd_rsp_print("  dialer secondary-wan %d\n", ppp_config->profile_wwan2_idx);
			}
		}

		if (modem_config->dual_wwan_enable[1]) {
			if (0 == ppp_config->profile_wwan2_2nd_idx) {
				ih_cmd_rsp_print("  dialer secondary-wan auto secondary\n");
			}else {
				ih_cmd_rsp_print("  dialer secondary-wan %d secondary\n", ppp_config->profile_wwan2_2nd_idx);
			}
		}

		switch(ppp_config->ppp_mode) {
		case PPP_DEMAND:
			if(ppp_config->trig_data) ih_cmd_rsp_print("  dialer activate traffic\n");
			if(ppp_config->trig_sms) ih_cmd_rsp_print("  dialer activate sms\n");
			ih_cmd_rsp_print("  dialer idle-timeout %d\n", ppp_config->idle);
			break;
		case PPP_MANUAL:
			ih_cmd_rsp_print("  dialer activate manual\n");
			break;
		case PPP_ONLINE:
		default:
			ih_cmd_rsp_print("  dialer activate auto\n");
			break;
		}
		if(ppp_config->ppp_static) {
			if(ppp_config->ppp_ip[0] && ppp_config->ppp_peer[0]) {
				ih_cmd_rsp_print("  ip address static local %s peer %s\n", ppp_config->ppp_ip, ppp_config->ppp_peer);
			} else if(ppp_config->ppp_ip[0]) {
				ih_cmd_rsp_print("  ip address static local %s\n", ppp_config->ppp_ip);
			} else if(ppp_config->ppp_peer[0]) {
				ih_cmd_rsp_print("  ip address static peer %s\n", ppp_config->ppp_peer);
			} 

			if(ppp_config->static_netmask[0]) {
				ih_cmd_rsp_print("  ip address static netmask %s\n", ppp_config->static_netmask);
			}

			if(!ppp_config->ppp_peer[0]) {
				ih_cmd_rsp_print("  no ip address static peer\n");
			}
			if(!ppp_config->ppp_ip[0]) {
				ih_cmd_rsp_print("  no ip address static local\n");
			}

			if(!ppp_config->static_netmask[0]) {
				ih_cmd_rsp_print("  no ip address static netmask\n");
			}
		} else {
			ih_cmd_rsp_print("  ip address negotiated\n");
		}
		ih_cmd_rsp_print("  ip mru %d\n", ppp_config->mru);
		ih_cmd_rsp_print("  ip mtu %d\n", ppp_config->mtu);

		if(gl_myinfo.priv.dhcp_options & DHCP_OPT_MTU){
			ih_cmd_rsp_print("  dhcp option mtu request\n");
		}

		if (ppp_config->infinitely_dial_retry) {
			ih_cmd_rsp_print("  dialer infinitely-retry\n");
		}

		if(ppp_config->vj) {
			if(ppp_config->novjccomp)
				ih_cmd_rsp_print("  ip tcp header-compression special-vj\n");
			else
				ih_cmd_rsp_print("  ip tcp header-compression\n");
		}
		if(ppp_config->peerdns) ih_cmd_rsp_print("  ppp ipcp dns request\n");
		if(ppp_config->default_am) ih_cmd_rsp_print("  ppp accm default\n");
		if(ppp_config->noaccomp) ih_cmd_rsp_print("  ppp acfc forbid\n");
		if(ppp_config->nopcomp) ih_cmd_rsp_print("  ppp pfc forbid\n");
		ih_cmd_rsp_print("  ppp keepalive %d %d\n", ppp_config->lcp_echo_interval, ppp_config->lcp_echo_retries);
		if(ppp_config->ppp_comp) ih_cmd_rsp_print("  compress %s\n", comp[ppp_config->ppp_comp]);
		if(ppp_config->init_string[0]) ih_cmd_rsp_print("  ppp initial string %s\n", ppp_config->init_string);
		if(gl_myinfo.priv.ppp_debug) ih_cmd_rsp_print("  ppp debug\n");
		if(ppp_config->options[0]) ih_cmd_rsp_print("  ppp options \"%s\"\n", ppp_config->options);

		ih_cmd_rsp_print("!\n");
		for(i=0; i<REDIAL_SUBIF_MAX; i++) {
			if(sub_ifs[i].if_info.type) {
				ih_cmd_rsp_print("interface cellular 1.%d\n", sub_ifs[i].if_info.sid);
				ih_cmd_rsp_print("!\n");
			}
		}
		break;
	}
}

/*cellular module infomation, as: sim
*/
static void show_cellular_info(uns32 flag)
{
	MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;
	VIF_INFO *vif = &gl_myinfo.svcinfo.vif_info;
	char *regs[] = {"Registering","Registered","Not registered, searching","Registration denied","4","Registered, roaming"};
	char *sims[] = {"No SIM","SIM PIN","SIM Ready"};
	char *nets[] = {"<NA>","2G","3G","4G", "5G"};
	char *dualsim[] = {"<NA>","SIM 1","SIM 2"};
	char operator[BRAND_STR_SIZ] = {0};
	char info_rsrq[8] = {0};
	
	if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_PLS8
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS61
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS81
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_LARAR2
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ME909
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_TOBYL201
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_WPD200){
	   if (info->carrier_str[0]) memcpy(operator, info->carrier_str, sizeof(info->carrier_str));	
	}else {
		if(info->carrier_code) find_operator(info->carrier_code,operator);
	}
	
	//Rsrq may be float format
	if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_TOBYL201){
		if(ih_key_support("FB78")){
			if(info->rsrq_str[0])snprintf(info_rsrq, sizeof(info_rsrq), "%s", info->rsrq_str);
		}else{
			if(info->rsrq)snprintf(info_rsrq, sizeof(info_rsrq), "%d", info->rsrq);
		}
	}else{
		if(info->rsrq)snprintf(info_rsrq, sizeof(info_rsrq), "%d", info->rsrq);
	}

	switch (flag) {
	case IH_CMD_FLAG_JSON:
		ih_cmd_rsp_print("\nmodem_info = {\n"
					"\t'imei': '%s',\n"
					"\t'imsi': '%s',\n"
					"\t'iccid': '%s',\n"
					"\t'phonenum': '%s',\n"
					"\t'siglevel': '%d',\n"
					"\t'sigbar': '%d',\n"
					"\t'dbm': %d,\n"
					"\t'asu': %d,\n"
					"\t'regstatus': '%d',\n"
					"\t'network_type': '%s',\n"
					"\t'submode_name': '%s',\n"
					"\t'current_sim': '%s',\n"
					"\t'current_operator': '%s',\n"
					"\t'lac': '%s',\n"
					"\t'cellid': '%s',\n"
					"\t'apns': '%s',\n"
					"\t'nai_no': '%s',\n"
					"\t'nai_pwd': '%s',\n"
					"\t'pci': '%s',\n"
					"\t'band': '%s',\n"
					"\t'rssi': '%s',\n"
					"\t'ber': '%s',\n"
					"\t'rscp': '%s',\n"
					"\t'ecio': '%s',\n"
					"\t'rsrp': '%s',\n"
					"\t'rsrq': '%s',\n"
					"\t'sinr': '%s',\n"
					"\t'ss_rsrp': '%s',\n"
					"\t'ss_rsrq': '%s',\n"
					"\t'ss_sinr': '%s'\n",
					info->imei,
					info->imsi,
					info->iccid,
					info->phonenum,
					info->siglevel,
					info->sigbar,
					info->dbm ? info->dbm : -110,
					info->asu,
					info->regstatus,
					nets[info->network],
					info->submode_name,
					dualsim[info->current_sim],
					operator,
					info->srvcell.lac,
					info->srvcell.cellid,
					info->apns,
					(gl_myinfo.priv.modem_config.nai_no[0] ? gl_myinfo.priv.modem_config.nai_no : "-"),
					(gl_myinfo.priv.modem_config.nai_pwd[0] ? gl_myinfo.priv.modem_config.nai_pwd : "-"),
					info->srvcell.pci,
					info->srvcell.band,
					info->srvcell.rssi,
					info->srvcell.ber,
					info->srvcell.rscp,
					info->srvcell.ecio,
					info->srvcell.rsrp,
					info->srvcell.rsrq,
					info->srvcell.sinr,
					info->srvcell.ss_rsrp,
					info->srvcell.ss_rsrq,
					info->srvcell.ss_sinr
					);
		ih_cmd_rsp_print("};\n");

		break;
	case IH_CMD_FLAG_CLI:
	default:
		ih_cmd_rsp_print("%-20s: %s\n", "Active SIM", dualsim[info->current_sim]);
		ih_cmd_rsp_print("%-20s: %s\n", "SIM Status", sims[info->simstatus]);
		ih_cmd_rsp_print("%-20s: %s\n", "IMEI Code", info->imei);
		ih_cmd_rsp_print("%-20s: %s\n", "IMSI Code", info->imsi);
		ih_cmd_rsp_print("%-20s: %s\n", "ICCID Code", info->iccid);
		if(info->phonenum[0] && info->phonenum[0]!='-') {
			ih_cmd_rsp_print("%-20s: %s\n", "Phone Number", info->phonenum);
		}
		ih_cmd_rsp_print("%-20s: %d asu (%d dbm)\n", "Signal Level", info->siglevel, info->dbm ? info->dbm : -110);
		if (info->rsrp && info_rsrq[0] && info->sinr[0]) {
			ih_cmd_rsp_print("%-20s: %d dbm\n", "RSRP", info->rsrp);
			ih_cmd_rsp_print("%-20s: %s db\n", "RSRQ", info_rsrq);
			ih_cmd_rsp_print("%-20s: %s db\n", "SINR", info->sinr);
		}
		ih_cmd_rsp_print("%-20s: %s\n", "Register Status", regs[info->regstatus]);
		ih_cmd_rsp_print("%-20s: %s\n", "Operator", operator);
		ih_cmd_rsp_print("%-20s: %s", "Network Type", nets[info->network]);
		if(info->submode_name[0]) {
			ih_cmd_rsp_print(" (%s)\n", info->submode_name);
		} else {
			ih_cmd_rsp_print("\n");
		}
		if(info->lac[0]) {
			ih_cmd_rsp_print("%-20s: %s\n", "LAC", info->lac);
			ih_cmd_rsp_print("%-20s: %s\n", "Cell ID", info->cellid);
		}
		ih_cmd_rsp_print("%-20s: %s\n", "IP", vif->local_ip.s_addr ? inet_ntoa(vif->local_ip) : "");
		break;
	}
}
static void display_cellular_network(VIF_INFO *pinfo, PPP_CONFIG *ppp_config)
{
	char str[IF_CMD_NAME_SIZE];
	unsigned int uptime;

	get_str_from_if_info(&(pinfo->if_info), str);
	ih_cmd_rsp_print("['%s',", str);
	ih_cmd_rsp_print("'',");//description
	ih_cmd_rsp_print("%d,", (pinfo->status == VIF_UP));
	ih_cmd_rsp_print("'%s',", inet_ntoa(pinfo->local_ip));
	ih_cmd_rsp_print("'%s',", inet_ntoa(pinfo->netmask));
	ih_cmd_rsp_print("'%s',", inet_ntoa(pinfo->remote_ip));
	ih_cmd_rsp_print("'%s", inet_ntoa(pinfo->dns1));
	ih_cmd_rsp_print(" %s',", inet_ntoa(pinfo->dns2));
	if(get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_DHCP
		|| get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_QMI
		|| get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_MIPC){
		ih_cmd_rsp_print("%d,", pinfo->mtu);
	}else{
		ih_cmd_rsp_print("%d,", ppp_config->mtu);
	}
	if(pinfo->status==VIF_UP) {
		uptime = get_uptime() - gl_myinfo.svcinfo.uptime;
		ih_cmd_rsp_print("'%us'", uptime);
	}else  ih_cmd_rsp_print("''");
	ih_cmd_rsp_print(",");
	iface_showrun_ip6_addr(pinfo->iface, &pinfo->ip6_addr, IH_CMD_FLAG_JSON);
	ih_cmd_rsp_print("]");

	return;
}

static void show_cellular_interface(uns32 flag)
{
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	VIF_INFO *pinfo_2nd = &(gl_myinfo.svcinfo.secondary_vif_info);
	VIF_INFO *sub_vif_info = gl_myinfo.svcinfo.sub_vif_info;
	PPP_CONFIG *ppp_config = &gl_myinfo.priv.ppp_config;
	unsigned int uptime;
	char str[IF_CMD_NAME_SIZE];
	int i;

	if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_NONE){
		switch (flag) {
		case IH_CMD_FLAG_JSON:
			ih_cmd_rsp_print("\ncellular_interface = [];\n");
		case IH_CMD_FLAG_CLI:
			break;
		}	
		return ;
	}

	switch (flag) {
	case IH_CMD_FLAG_JSON:
		//format: [<type>,<slot>,<port>]
		ih_cmd_rsp_print("\ncellular_interface = [");
		display_cellular_network(pinfo, ppp_config);
		ih_cmd_rsp_print(",");
		display_cellular_network(pinfo_2nd, ppp_config);
#if 0
		get_str_from_if_info(&(pinfo->if_info), str);
		ih_cmd_rsp_print("['%s',", str);
		ih_cmd_rsp_print("'',");//description
		ih_cmd_rsp_print("%d,", (pinfo->status == VIF_UP));
		ih_cmd_rsp_print("'%s',", inet_ntoa(pinfo->local_ip));
		ih_cmd_rsp_print("'%s',", inet_ntoa(pinfo->netmask));
		ih_cmd_rsp_print("'%s',", inet_ntoa(pinfo->remote_ip));
		ih_cmd_rsp_print("'%s", inet_ntoa(pinfo->dns1));
		ih_cmd_rsp_print(" %s',", inet_ntoa(pinfo->dns2));
		if(get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_DHCP
			|| get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_QMI){
			ih_cmd_rsp_print("%d,", pinfo->mtu);
		}else{
			ih_cmd_rsp_print("%d,", ppp_config->mtu);
		}
		if(pinfo->status==VIF_UP) {
			uptime = get_uptime() - gl_myinfo.svcinfo.uptime;
			ih_cmd_rsp_print("'%us'", uptime);
		}else  ih_cmd_rsp_print("''");
		ih_cmd_rsp_print(",");
		iface_showrun_ip6_addr(pinfo->iface, &pinfo->ip6_addr, IH_CMD_FLAG_JSON);
		ih_cmd_rsp_print("]");
#endif
		ih_cmd_rsp_print("];\n");
		break;
	case IH_CMD_FLAG_CLI:
	default:
		if(pinfo->status==VIF_UP) {
			ih_if_basic_display(&pinfo->if_info);
			uptime = get_uptime() - gl_myinfo.svcinfo.uptime;
			ih_cmd_rsp_print("          Line protocol uptime:%us (%s)\n", uptime, second_to_day(uptime));
			ih_cmd_rsp_print(" \n");
		}

		if(pinfo_2nd->status==VIF_UP) {
			ih_if_basic_display(&pinfo_2nd->if_info);
			uptime = get_uptime() - gl_myinfo.svcinfo.uptime;
			ih_cmd_rsp_print("          Line protocol uptime:%us (%s)\n", uptime, second_to_day(uptime));
			ih_cmd_rsp_print(" \n");
		}

		for(i=0; i<REDIAL_SUBIF_MAX; i++) {
			if(sub_vif_info[i].status==VIF_UP) {
				ih_if_basic_display(&sub_vif_info[i].if_info);
				uptime = get_uptime() - gl_myinfo.svcinfo.uptime;
				ih_cmd_rsp_print("          Line protocol uptime:%us (%s)\n", uptime, second_to_day(uptime));
				ih_cmd_rsp_print(" \n");//add space to avoid eated by cli
			}
		}
		break;
	}
}

void clear_csq_policy(int idx1, int idx2){
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	
	modem_config->csq[idx1] = 0;
	modem_config->csq[idx2] = 0;
	modem_config->csq_interval[idx1] = 0;
	modem_config->csq_interval[idx2] = 0;
	modem_config->csq_retries[idx1] = 0;
	modem_config->csq_retries[idx2] = 0;
	return;
}

void dualsim_handle_backuptimer(void)
{
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	DUALSIM_INFO *dualsim_info = &(gl_myinfo.priv.dualsim_info);
	struct timeval tv;
	
	//FIXME: sometime will del inactive timer.
	if(dualsim_info->backtime>0) {
		if(modem_info->current_sim == modem_config->backup_sim){
			evutil_timerclear(&tv);
			tv.tv_sec = dualsim_info->backtime;
			event_add(&gl_ev_dualsim_tmr, &tv);	
		} else {
			event_del(&gl_ev_dualsim_tmr);
		}
	} else {
		event_del(&gl_ev_dualsim_tmr);
	}
}

/*
 * handle cmd request
 *
 * @param peer_svc_id		cmd from which service
 * @param msg				received IPC msg
 * @param rsp_reqd			whether to send response
 *
 * @return ERR_OK for ok, others for error
 */
static int32 my_cmd_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen)  
{
#define MY_LEAVE(x) {ret_code = x;goto leave;}
	IH_CMD *cmd;
	char *pcmd, *what;
	int32 ret_code = IH_ERR_INVALID;
	int n=0, config_type=REDIAL_CONFIG_NONE, ret, vid;
	PPP_PROFILE *p;
	DUALSIM_INFO *dualsim_info = &(gl_myinfo.priv.dualsim_info);
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	SMS_CONFIG *sms_config = &(gl_myinfo.priv.sms_config);
	CELLULAR_SUB_IF *sub_ifs = gl_myinfo.priv.sub_ifs;
	MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;
	int csq_value = 0,csq_interval = 0,csq_retries = 0;
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	VIF_INFO *pinfo_2nd = &(gl_myinfo.svcinfo.secondary_vif_info);
	VIF_INFO *sub_vif_info = gl_myinfo.svcinfo.sub_vif_info;
	struct timeval tv;
	int skip = 0;
#if 0
	MODEM_CONFIG modem_config_tmp;
	SMS_CONFIG sms_config_tmp;
	PPP_CONFIG ppp_config_tmp;
#endif

	static MY_REDIAL_INFO gl_myinfo_tmp;

	MYTRACE_ENTER();

#if 0
	memcpy(&modem_config_tmp, &(gl_myinfo.priv.modem_config), sizeof(MODEM_CONFIG));
	memcpy(&sms_config_tmp, &(gl_myinfo.priv.sms_config), sizeof(SMS_CONFIG));
	memcpy(&ppp_config_tmp, &(gl_myinfo.priv.ppp_config), sizeof(PPP_CONFIG)); 
#endif
	memcpy(&gl_myinfo_tmp.priv, &(gl_myinfo.priv), sizeof(gl_myinfo_tmp.priv));

	cmd = (IH_CMD*)msg;
	ih_cmd_dump(cmd);
	pcmd = (cmd->len>0) ? cmd->args : "";

	ih_cmd_rsp_start(msg_hdr->svc_id,cmd,obuf,olen);

	/* factory test
	 */
	if(strcmp(cmd->view, "infactory") == 0){
		if(my_factory_cmd_handle(cmd) == ERR_OK){
			return ih_cmd_rsp_finish(ERR_OK);
		}
	}

	/*[no] debug cellular
	 */
	if (strcmp(cmd->cmd, "debug") == 0) {
		if (cmd->type == CMD_TYPE_NO
			||cmd->type == CMD_TYPE_DEFAULT) {
			gl_myinfo.priv.debug = FALSE;
			gl_myinfo.priv.ppp_debug = FALSE;
		} else {
			gl_myinfo.priv.debug = TRUE;
			gl_myinfo.priv.ppp_debug = TRUE;
		}

		trace_category_set(gl_myinfo.priv.debug ? DEFAULT_TRACE_CAT : 0);
		MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
	}

#ifdef MEMWATCH
	if (strcmp(cmd->cmd, "memstat")==0) {
		//ih_cmd_memwatch_show(gl_my_svc_id);
		return ih_cmd_rsp_finish(ERR_OK);
	}
#endif		

	/*show running-config cellular
	 *show cellular
	 */
	if (strcmp(cmd->cmd, "show") == 0) {		
		what = cmdsep(&pcmd);
		if(strncmp(what, "running-config", 1) == 0) {
			show_cellular_config(cmd->flags);
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (strncmp(what, "debugging", 1)==0) {
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if(strncmp(what, "cellular", 1) == 0) {
			show_cellular_info(cmd->flags);
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if(strncmp(what, "interface", 1) == 0) {
			show_cellular_interface(cmd->flags);
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		}
	}
	
	gl_modem_idx = find_modem_type_idx(ih_license_get_modem());
	if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_NONE){
		ih_cmd_rsp_print("Command is not supported!");
		MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
	}

	/*interface cellular <1-1>
	 *[no] interface cellular <1.1-2>
	 */
	if (strcmp(cmd->cmd, "interface") == 0) {
		cmdsep(&pcmd);
		what = cmdsep(&pcmd);
		if (cmd->type == CMD_TYPE_NO
		    ||cmd->type == CMD_TYPE_DEFAULT) {
			if(strchr(what, '.')) {
				vid = atoi(what+2);
				if(sub_ifs[vid-1].if_info.type) {
					//FIXME: publish VIF_DOWN
					sub_vif_info[vid-1].status = VIF_DESTROY;
					ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)&sub_vif_info[vid-1], sizeof(VIF_INFO));
					if(ret) LOG_ER("broadcast msg is not ok.");

					memset(&sub_vif_info[vid-1], 0, sizeof(VIF_INFO));
					memset(&sub_ifs[vid-1], 0, sizeof(CELLULAR_SUB_IF));

					config_type = REDIAL_CONFIG_PPP;
				}
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else {
			if(strchr(what, '.')) {
				vid = atoi(what+2);
				if(sub_vif_info[vid-1].status==VIF_DESTROY) {
					sub_vif_info[vid-1].if_info.type = IF_TYPE_SUB_CELLULAR;
					sub_vif_info[vid-1].if_info.port = 1;
					sub_vif_info[vid-1].if_info.sid = vid;
					sub_vif_info[vid-1].status = VIF_CREATE;
					ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)&sub_vif_info[vid-1], sizeof(VIF_INFO));
					if(ret) LOG_ER("broadcast msg is not ok.");
					sub_ifs[vid-1].if_info = sub_vif_info[vid-1].if_info;

					config_type = REDIAL_CONFIG_PPP;
				}
			} else {
				if(pinfo->status==VIF_DESTROY) {
					pinfo->status = VIF_CREATE;
					ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)pinfo, sizeof(VIF_INFO));
					if(ret) LOG_ER("broadcast msg is not ok.");
				}
				if(pinfo_2nd->status==VIF_DESTROY) {
					pinfo_2nd->status = VIF_CREATE;
					ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)pinfo_2nd, sizeof(VIF_INFO));
					if(ret) LOG_ER("broadcast msg is not ok.");
				}
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		}
	}

	/*[no] cellular 1 gsm profile <profile number> <apn> <dial number>
	 *		auto|pap|chap|ms-chap|ms-chapv2 [<username> <password>]
	 *[no] cellular 1 cdma profile <profile number> <dial number>
	 *		auto|pap|chap|ms-chap|ms-chapv2 <username> <password>
	 *cellular 1 dial interval <seconds>
	 *cellular 1 network auto|2g|3g
	 *cellular 1 sim <1-2> <pincode>
	 *cellular 1 sim <1-2> operator verizon|others|auto
	 *cellular 1 sim dual 
	 *cellular 1 signal interval <0-3600>
	 *cellular 1 sms access-list <1-10> deny|permit <phonenumber>
	 *cellular 1 sms enable 
	 *cellular 1 sms interval <10-3600> 
	 *cellular 1 sms mode pdu|text 
	 *cellular 1 ppp up|down 
	 */
	if (strcmp(cmd->cmd, "cellular") == 0) {		
		cmdsep(&pcmd);
		what = cmdsep(&pcmd);//gsm|cdma|...
		//handle cmd
		if (cmd->type == CMD_TYPE_NO
		    ||cmd->type == CMD_TYPE_DEFAULT) {
			if (strncmp(what, "gsm", 1) == 0
				|| strncmp(what, "cdma", 1)==0) {
				//FIXME: only one profile
				cmdsep(&pcmd);
				n = atoi(cmdsep(&pcmd));
				//this profile in use, cannot del.
				if(gl_myinfo.priv.ppp_config.profiles[n-1].type == PROFILE_TYPE_NONE) {
					ih_cmd_rsp_print("%%error: profile %d not exist!", n);
					MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
				}else if((n==gl_myinfo.priv.ppp_config.profile_idx)||
						(n==gl_myinfo.priv.ppp_config.profile_2nd_idx)){
					ih_cmd_rsp_print("%%error: profile %d in used!", n);
					MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
				}
				p = gl_myinfo.priv.ppp_config.profiles+n-1;
				memset((char *)p, '\0', sizeof(PPP_PROFILE));
			} else if (strncmp(what, "activation", 1) == 0) {
				modem_config->activate_enable = 0;
			} else if (strncmp(what, "dial", 2) == 0) {
				gl_myinfo.priv.dial_interval = REDIAL_DEFAULT_DIAL_INTERVAL;
			} else if (strncmp(what, "modem", 1) == 0) {
				modem_config->debug = FALSE;	
			} else if (strncmp(what, "network", 1) == 0) {
				config_type = REDIAL_CONFIG_PPP;
				modem_config->network_type = MODEM_NETWORK_AUTO;
			} else if (strncmp(what, "signal", 3) == 0) {
				gl_myinfo.priv.signal_interval = REDIAL_DEFAULT_CSQ_INTERVAL;
			} else if (strncmp(what, "sim", 3) == 0) {
				what = cmdsep(&pcmd);
				n = atoi(what);
				what = cmdsep(&pcmd);
				if(!what){
					modem_config->pincode[n-1][0] = '\0';
					modem_config->roam_enable[n-1] = 1;
				}else{
					if(strncmp(what, "roaming", 1) == 0){
						what = cmdsep(&pcmd);
						modem_config->roam_enable[n-1] = 1;
					}else modem_config->pincode[n-1][0] = '\0';
				}
			} else if (strncmp(what, "sms", 2) == 0) {
				what = cmdsep(&pcmd);
				if(strncmp(what, "access-list", 1) == 0){
					int idx;
					idx = atoi(cmdsep(&pcmd));
					memset(&sms_config->acl[idx-1], 0, sizeof(SMS_ACL));
				} else if(strncmp(what, "enable", 1) == 0){
					sms_config->enable = 0;
					stop_smsd();
				} else if(strncmp(what, "interval", 1) == 0){
					sms_config->interval = SMS_DEFAULT_INTERVAL;
				} else if(strncmp(what, "mode", 1) == 0){
					sms_config->mode = SMS_MODE_TEXT;
				}
			} else if (strncmp(what, "dual-sim", 2) == 0) {
				what = cmdsep(&pcmd);
				if(strncmp(what, "main", 1) == 0) {
					modem_config->main_sim = SIM1;
					modem_config->backup_sim = SIM2;	
					modem_config->policy_main = SIM1;
					unlink(SEQ_SIM_FILE);
					if(info->current_sim != modem_config->main_sim){
						config_type = REDIAL_CONFIG_PPP;
						gl_sim_switch_flag = TRUE;
					}
				}else if(strncmp(what, "enable", 1) == 0){
					modem_config->dualsim = 0;
					gl_sim_switch_flag = FALSE;
				}else if(strncmp(what, "policy", 1) == 0){
					what = cmdsep(&pcmd);
					if(!what){
						clear_csq_policy(0,1);
						dualsim_info->backtime = 0;
						dualsim_info->retries = 5;
					}else if(strncmp(what, "redial", 1) == 0){
						what = cmdsep(&pcmd);
						dualsim_info->retries = 5;
					}else if(strncmp(what, "csq", 1) == 0){
						what = cmdsep(&pcmd);
						if(!what) clear_csq_policy(0,0);
						else if(strncmp(what, "secondary", 1) == 0)
							clear_csq_policy(1,1);
						else{
							what = cmdsep(&pcmd);
							if(!what) clear_csq_policy(0,0);
							else if(strncmp(what, "secondary", 1) == 0)
								clear_csq_policy(1,1);
							else if(strncmp(what, "interval", 1) == 0){
								what = cmdsep(&pcmd);
								what = cmdsep(&pcmd);
								what = cmdsep(&pcmd);
								what = cmdsep(&pcmd);
								if(!what) clear_csq_policy(0,0);
								else if(strncmp(what, "secondary", 1) == 0)
									clear_csq_policy(1,1);
							}
									
						}
					}else if(strncmp(what, "backtime", 1) == 0){
						dualsim_info->backtime = 0;
						if(modem_config->dualsim)
							dualsim_handle_backuptimer();
					}else if(strncmp(what, "uptime", 1) == 0){
						dualsim_info->uptime = 0;
					}
				}
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "gsm", 1) == 0) {
			cmdsep(&pcmd);
			n = atoi(cmdsep(&pcmd));
			p = gl_myinfo.priv.ppp_config.profiles+n-1;
			p->type = PROFILE_TYPE_GSM;
			strlcpy(p->apn, cmdsep(&pcmd), sizeof(p->apn));
			strlcpy(p->dialno, cmdsep(&pcmd), sizeof(p->dialno));
			
			skip = 0;
			what = cmdsep(&pcmd);
			if(strncmp(what, "ipv4v6", 6) == 0)
				p->pdp_type = PDP_TYPE_IPV4V6;
			else if(strncmp(what, "ipv4", 4) == 0)
				p->pdp_type = PDP_TYPE_IPV4;
			else if(strncmp(what, "ipv6", 4) == 0)
				p->pdp_type = PDP_TYPE_IPV6;
			else{
				p->pdp_type = PDP_TYPE_IPV4;
				skip = 1;
			}
			
			if(!skip) what = cmdsep(&pcmd);
			if(strncmp(what, "auto", 1) == 0)
				p->auth_type = AUTH_TYPE_AUTO;
			else if(strncmp(what, "pap", 1) == 0)
				p->auth_type = AUTH_TYPE_PAP;
			else if(strncmp(what, "chap", 1) == 0)
				p->auth_type = AUTH_TYPE_CHAP;
			else if(strncmp(what, "ms-chapv2", 8) == 0)
				p->auth_type = AUTH_TYPE_MSCHAPV2;
			else if(strncmp(what, "ms-chap", 7) == 0)
				p->auth_type = AUTH_TYPE_MSCHAP;
			what = cmdsep(&pcmd);
			if(what) strlcpy(p->username, what, sizeof(p->username));
			else p->username[0] = '\0';
			what = cmdsep(&pcmd);
			if(what){ 
				what = decrypt_passwd(what); //wucl decrypt paaswd
				strlcpy(p->password, what, sizeof(p->password));
			}else{ 
				p->password[0] = '\0';
			}

			//if profile in use, revise it will cause ppp restart.
			if((n==gl_myinfo.priv.ppp_config.profile_idx)
				|| (n == gl_myinfo.priv.ppp_config.profile_2nd_idx)
				|| (n == gl_myinfo.priv.ppp_config.profile_wwan2_idx)
				|| (n == gl_myinfo.priv.ppp_config.profile_wwan2_2nd_idx)){
				config_type = REDIAL_CONFIG_PPP;
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "cdma", 1) == 0) {
			cmdsep(&pcmd);
			n = atoi(cmdsep(&pcmd));
			p = gl_myinfo.priv.ppp_config.profiles+n-1;
			p->type = PROFILE_TYPE_CDMA;
			p->apn[0] = '\0';
			strlcpy(p->dialno, cmdsep(&pcmd), sizeof(p->dialno));
			
			skip = 0;
			what = cmdsep(&pcmd);
			if(strncmp(what, "ipv4v6", 6) == 0)
				p->pdp_type = PDP_TYPE_IPV4V6;
			else if(strncmp(what, "ipv4", 4) == 0)
				p->pdp_type = PDP_TYPE_IPV4;
			else if(strncmp(what, "ipv6", 4) == 0)
				p->pdp_type = PDP_TYPE_IPV6;
			else {
				p->pdp_type = PDP_TYPE_IPV4;
				skip = 1;
			}

			if(!skip) what = cmdsep(&pcmd);
			if(strncmp(what, "auto", 1) == 0)
				p->auth_type = AUTH_TYPE_AUTO;
			else if(strncmp(what, "pap", 1) == 0)
				p->auth_type = AUTH_TYPE_PAP;
			else if(strncmp(what, "chap", 1) == 0)
				p->auth_type = AUTH_TYPE_CHAP;
			else if(strncmp(what, "ms-chapv2", 8) == 0)
				p->auth_type = AUTH_TYPE_MSCHAPV2;
			else if(strncmp(what, "ms-chap", 7) == 0)
				p->auth_type = AUTH_TYPE_MSCHAP;

			what = cmdsep(&pcmd);
			if(what) strlcpy(p->username, what, sizeof(p->username));
			else p->username[0] = '\0';
			what = cmdsep(&pcmd);
			if(what){
				what = decrypt_passwd(what); //wucl decrypt paaswd
				strlcpy(p->password, what, sizeof(p->password));
			}else{
				p->password[0] = '\0';
			}

			//if profile in use, revise it will cause ppp restart.
			if((n==gl_myinfo.priv.ppp_config.profile_idx)
				|| (n == gl_myinfo.priv.ppp_config.profile_2nd_idx)
				|| (n == gl_myinfo.priv.ppp_config.profile_wwan2_idx)
				|| (n == gl_myinfo.priv.ppp_config.profile_wwan2_2nd_idx)){
				config_type = REDIAL_CONFIG_PPP;
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "dial", 2) == 0) {
			cmdsep(&pcmd);
			gl_myinfo.priv.dial_interval =  atoi(cmdsep(&pcmd));
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "modem", 1) == 0) {
			modem_config->debug = TRUE;
		} else if (what && strncmp(what, "nr5g-mode", 2) == 0) {
			config_type = REDIAL_CONFIG_PPP;
			what = cmdsep(&pcmd);
			if(strncmp(what, "auto", 1) == 0){
				modem_config->nr5g_mode[0] = NR5G_MODE_AUTO;
			}else if(strncmp(what, "nsa", 1) == 0){
				modem_config->nr5g_mode[0] = NR5G_MODE_NSA;
			}else if(strncmp(what, "sa", 1) == 0){
				modem_config->nr5g_mode[0] = NR5G_MODE_SA;
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "network", 2) == 0) {
			config_type = REDIAL_CONFIG_PPP;
			what = cmdsep(&pcmd);
			if(strncmp(what, "auto", 1) == 0)
				modem_config->network_type = MODEM_NETWORK_AUTO;
			if(strncmp(what, "3g2g", 3) == 0)
				modem_config->network_type = MODEM_NETWORK_3G2G;
			else if(strncmp(what, "5g4g", 3) == 0)
				modem_config->network_type = MODEM_NETWORK_5G4G;
			else if(strncmp(what, "2g", 2) == 0)
				modem_config->network_type = MODEM_NETWORK_2G;
			else if(strncmp(what, "3g", 2) == 0)
				modem_config->network_type = MODEM_NETWORK_3G;
			else if(strncmp(what, "4g", 2) == 0)
				modem_config->network_type = MODEM_NETWORK_4G;
			else if(strncmp(what, "5g", 2) == 0)
				modem_config->network_type = MODEM_NETWORK_5G;
			else if(strncmp(what, "evdo", 4) == 0)
				modem_config->network_type = MODEM_NETWORK_EVDO;
			//SIM2 network type equal to SIM1 network type	
			modem_config->network_type2 = modem_config->network_type;
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "ppp", 1) == 0) {
			what = cmdsep(&pcmd);
			if(strncmp(what, "down", 1) == 0) {
				LOG_IN("receive disconnect command, hangup!");
				if(get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP
					|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI
					|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_MIPC
						|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_NONE){
					
					stop_child_by_sub_daemon();

					if (PPP_ONLINE != gl_myinfo.priv.ppp_config.ppp_mode){
						gl_redial_dial_state = REDIAL_DIAL_STATE_KILLED;
					}
				}else if(gl_ppp_pid>=0) kill(gl_ppp_pid, SIGHUP);
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			} else if(strncmp(what, "up", 1) == 0) {
				//FIXME: if downline by exception??
				LOG_IN("receive connect command, Go!");
				if(get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP
					|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI
					|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_MIPC
						|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_NONE){
					if (PPP_ONLINE != gl_myinfo.priv.ppp_config.ppp_mode){
						gl_redial_dial_state = REDIAL_DIAL_STATE_NONE;
					}
				}else if(gl_ppp_pid>=0) kill(gl_ppp_pid, SIGUSR1);
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			}
		} else if (what && strncmp(what, "signal", 3) == 0) {
			cmdsep(&pcmd);//interval
			what = cmdsep(&pcmd);
			gl_myinfo.priv.signal_interval = atoi(what);
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "sim", 3) == 0) {
			what = cmdsep(&pcmd);
			n = atoi(what);
			what = cmdsep(&pcmd);
			if(strncmp(what, "roaming", 1) == 0){
				what = cmdsep(&pcmd);
				modem_config->roam_enable[n-1] = 0;
			}else if(strncmp(what, "operator", 1) == 0){
				config_type = REDIAL_CONFIG_PPP;
				what = cmdsep(&pcmd);
				if(strncmp(what, "verizon", 1) == 0){
					modem_config->sim_operator[n-1] = SIM_OPER_VERIZON;
				}else if(strncmp(what, "others", 1) == 0){
					modem_config->sim_operator[n-1] = SIM_OPER_OTHERS;
				}else{
					modem_config->sim_operator[n-1] = SIM_OPER_AUTO;
				}
			}else strlcpy(modem_config->pincode[n-1], decrypt_passwd(what), sizeof(modem_config->pincode[n-1]));
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "sms", 2) == 0) {
			what = cmdsep(&pcmd);
			if(strncmp(what, "access-list", 1) == 0){
				int idx;
				idx = atoi(cmdsep(&pcmd));
				sms_config->acl[idx-1].id = idx;
				what = cmdsep(&pcmd);
				if(strncmp(what, "deny", 1) == 0){
					sms_config->acl[idx-1].action = SMS_ACL_DENY;
				} else if(strncmp(what, "permit", 1) == 0){
					sms_config->acl[idx-1].action = SMS_ACL_PERMIT;
				}
				strlcpy(sms_config->acl[idx-1].phone, cmdsep(&pcmd), sizeof(sms_config->acl[idx-1].phone));
				
				what = cmdsep(&pcmd);
				if(what && (strncmp(what, "enable", 1) == 0)) {
					sms_config->acl[idx-1].io_msg_enable = 1;
				}else{
					sms_config->acl[idx-1].io_msg_enable = 0;
				}
			} else if(strncmp(what, "enable", 1) == 0){
				sms_config->enable = 1;
				if(gl_smsd_pid<0) {
					start_smsd();
					//FIXME: check redial status, add sms timer
					if(gl_redial_state==REDIAL_STATE_CONNECTED || gl_redial_state==REDIAL_STATE_CONNECTING
						|| (get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI && gl_redial_state==REDIAL_STATE_DHCP)
						|| (get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_MIPC && gl_redial_state==REDIAL_STATE_DHCP)
						|| (get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP && gl_redial_state==REDIAL_STATE_DHCP)) {
						sms_init(gl_commport, sms_config->mode);
						if(sms_config->interval) {
							evutil_timerclear(&tv);
							tv.tv_sec = sms_config->interval;
							event_add(&gl_ev_sms_tmr, &tv);
						}
					}
				}
			} else if(strncmp(what, "interval", 1) == 0){
				what = cmdsep(&pcmd);
				sms_config->interval = atoi(what);
				if (sms_config->enable==1){
					if(sms_config->interval){
						evutil_timerclear(&tv);
						tv.tv_sec = sms_config->interval;
						event_add(&gl_ev_sms_tmr, &tv);
					}else {
						evutil_timerclear(&tv);
					}
				}
			} else if(strncmp(what, "mode", 1) == 0){
				what = cmdsep(&pcmd);
				if(strncmp(what, "pdu", 1) == 0){
					sms_config->mode = SMS_MODE_PDU;
				} else if(strncmp(what, "text", 1) == 0){
					sms_config->mode = SMS_MODE_TEXT;
				}

				if(gl_redial_state==REDIAL_STATE_CONNECTED || gl_redial_state==REDIAL_STATE_CONNECTING
					|| (get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI && gl_redial_state==REDIAL_STATE_DHCP)
					|| (get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_MIPC && gl_redial_state==REDIAL_STATE_DHCP)
				  	|| (get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP && gl_redial_state==REDIAL_STATE_DHCP)){
					sms_init(gl_commport, sms_config->mode);
				}
			} else if (strncmp(what, "send", 1) == 0){
				SMS outSms;
				char smsBuf[MAX_SMS_DATA_LEN] = {'\0'};
				int cnt = 0;
				char *pos = NULL;

				bzero(&outSms, sizeof(outSms));

				what = cmdsep(&pcmd);
				if (!what) {
					MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
				}
			
				strlcpy(outSms.phone, what, sizeof(outSms.phone));

				what = cmdsep(&pcmd);

				if (!what) {
					MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
				}

				pos = what;

				for(cnt = 0;(cnt < (MAX_SMS_DATA_LEN -1)) && pos && (*pos != '\0'); pos++) {
					if (*pos == '\"') {
						continue;
					}else if (*pos == '\\') {
						if (*(pos +1) == 'r') {
							smsBuf[cnt] = '\r';
							cnt++;
							pos++;
							continue;
							
						}else if (*(pos +1) == 'n') {
							smsBuf[cnt] = '\n';
							cnt++;
							pos++;
							continue;
						}
					}
					smsBuf[cnt] = *pos;
					cnt++;
				}

				smsBuf[cnt+1] = '\0';
				
				strlcpy(outSms.data, smsBuf, sizeof(outSms.data));
				
				outSms.data_len = strlen(outSms.data);

				LOG_IN("sending a sms to %s", outSms.phone);
				sms_init(gl_commport, SMS_MODE_TEXT);
				sms_handle_outsms(gl_commport, SMS_MODE_TEXT, &outSms);
				if (SMS_MODE_TEXT != sms_config->mode) {
					sms_init(gl_commport, sms_config->mode);
				}
			}

			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "dual-sim", 2) == 0) {
			what = cmdsep(&pcmd);
			if(strncmp(what, "main", 1) == 0) {
				what = cmdsep(&pcmd);
				if(strncmp(what, "random", 1) == 0){
					if(rand()%2 == 0) modem_config->main_sim = SIM1;
					else modem_config->main_sim = SIM2;
					modem_config->policy_main = SIMX;
					unlink(SEQ_SIM_FILE);
				}else if(strncmp(what, "sequence", 1) == 0){
					char sim[2];
					if(f_read_string(SEQ_SIM_FILE, sim, sizeof(sim))>0) {
						modem_config->main_sim = SIM1+SIM2-atoi(sim);
					}else {
						modem_config->main_sim = SIM1;
					}
					sprintf(sim, "%d", modem_config->main_sim);
					f_write_string(SEQ_SIM_FILE, sim, 0, 0);
					modem_config->policy_main = SIM_SEQ;
				}else{
					modem_config->main_sim = atoi(what);
					modem_config->policy_main = modem_config->main_sim;
					unlink(SEQ_SIM_FILE);
				}
				modem_config->backup_sim = SIM1+SIM2-modem_config->main_sim;
				if(modem_config->dualsim){
					if(info->current_sim != modem_config->main_sim){
						config_type = REDIAL_CONFIG_PPP;
						gl_sim_switch_flag = TRUE;
					}
				}
			}else if(strncmp(what, "enable", 1) == 0){
				modem_config->dualsim = 1;
				if(dualsim_info->retries == 0) dualsim_info->retries = 5;

				if(info->current_sim != modem_config->main_sim){
					config_type = REDIAL_CONFIG_PPP;
					gl_sim_switch_flag = TRUE;
				}
			}else if(strncmp(what, "policy", 1) == 0) {
				what = cmdsep(&pcmd);
				if(strncmp(what, "redial", 1) == 0){
					what = cmdsep(&pcmd);
					dualsim_info->retries = atoi(what);
				}else if(strncmp(what, "csq", 1) == 0){
					what = cmdsep(&pcmd);
					csq_value = atoi(what);
					what = cmdsep(&pcmd);
					if(!what || !*what) modem_config->csq[0] = csq_value;
					else if(strncmp(what, "interval", 1) == 0){
						what = cmdsep(&pcmd);
						csq_interval = atoi(what);
						what = cmdsep(&pcmd);
						what = cmdsep(&pcmd);
						csq_retries = atoi(what);
						what = cmdsep(&pcmd);
						if(!what || !*what){
							modem_config->csq[0] = csq_value;
							modem_config->csq_interval[0] = csq_interval;
							modem_config->csq_retries[0] = csq_retries;
						}else if(strncmp(what, "secondary", 1) == 0){
							modem_config->csq[1] = csq_value;
							modem_config->csq_interval[1] = csq_interval;
							modem_config->csq_retries[1] = csq_retries;	
						}
					}else if(strncmp(what, "secondary", 1) == 0){
						modem_config->csq[1] = csq_value;
					}
				}else if(strncmp(what, "backtime", 1) == 0){
					what = cmdsep(&pcmd);
					dualsim_info->backtime = atoi(what);
					if(modem_config->dualsim)
						dualsim_handle_backuptimer();
				}else if(strncmp(what, "uptime", 1) == 0){
					what = cmdsep(&pcmd);
					dualsim_info->uptime = atoi(what);
				}
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "activate-module", 10) == 0) {
			strlcpy(modem_config->activate_no, cmdsep(&pcmd), sizeof(modem_config->activate_no));
			what = cmdsep(&pcmd);
			if(what != NULL && *what){
				strlcpy(modem_config->phone_no, what, sizeof(modem_config->phone_no));
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "activation", 10) == 0) {
			modem_config->activate_enable = 1;
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		}
	}
	/*
	 *[no] dialer activate auto|traffic|sms|manual
	 *[no] dialer idle-timeout <seconds>
	 *[no] dialer profile <1-10> [secondary <1-10>]
	 *[no] dialer timeout <seconds>
	 */
	if (strcmp(cmd->cmd, "dialer") == 0) {		
		config_type = REDIAL_CONFIG_PPP;
		what = cmdsep(&pcmd);
		if (what && strncmp(what, "activate", 1) == 0) {
			if (cmd->type == CMD_TYPE_NO
		        ||cmd->type == CMD_TYPE_DEFAULT) {
				if(pcmd && *pcmd) {
					what = cmdsep(&pcmd);
					if(strncmp(what, "traffic", 1) == 0) {
						gl_myinfo.priv.ppp_config.trig_data = 0;
						if(gl_myinfo.priv.ppp_config.trig_sms==0)
							gl_myinfo.priv.ppp_config.ppp_mode = PPP_ONLINE;
					} else if(strncmp(what, "sms", 1) == 0) {
						gl_myinfo.priv.ppp_config.trig_sms = 0;
						if(gl_myinfo.priv.ppp_config.trig_data==0){
							gl_myinfo.priv.ppp_config.ppp_mode = PPP_ONLINE;
							gl_redial_dial_state = REDIAL_DIAL_STATE_NONE;
						}
					} else if(strncmp(what, "manual", 1) == 0) {
						gl_myinfo.priv.ppp_config.ppp_mode = PPP_ONLINE;
						gl_redial_dial_state = REDIAL_DIAL_STATE_NONE;
					}
				} else {
					gl_myinfo.priv.ppp_config.ppp_mode = PPP_ONLINE;
					gl_myinfo.priv.ppp_config.trig_data = 0;
					gl_myinfo.priv.ppp_config.trig_sms = 0;
				}
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			} else {
				what = cmdsep(&pcmd);
				if(strncmp(what, "auto", 1) == 0) {
					gl_myinfo.priv.ppp_config.ppp_mode = PPP_ONLINE;
					gl_myinfo.priv.ppp_config.trig_data = 0;
					gl_myinfo.priv.ppp_config.trig_sms = 0;
					gl_redial_dial_state = REDIAL_DIAL_STATE_STARTED;
				} else if(strncmp(what, "traffic", 1) == 0) {
					gl_myinfo.priv.ppp_config.ppp_mode = PPP_DEMAND;
					gl_myinfo.priv.ppp_config.trig_data = 1;
				} else if(strncmp(what, "sms", 1) == 0) {
					gl_myinfo.priv.ppp_config.ppp_mode = PPP_DEMAND;
					gl_myinfo.priv.ppp_config.trig_sms = 1;
					gl_redial_dial_state = REDIAL_DIAL_STATE_KILLED;
				} else if(strncmp(what, "manual", 7) == 0) {
					gl_myinfo.priv.ppp_config.ppp_mode = PPP_MANUAL;
					gl_myinfo.priv.ppp_config.trig_data = 0;
					gl_myinfo.priv.ppp_config.trig_sms = 0;
					gl_redial_dial_state = REDIAL_DIAL_STATE_KILLED;
				}
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			}
		} else if (what && strncmp(what, "idle-timeout", 2) == 0) {
			if (cmd->type == CMD_TYPE_NO
		        ||cmd->type == CMD_TYPE_DEFAULT) {
				gl_myinfo.priv.ppp_config.idle = REDIAL_DEFAULT_IDLE;
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			} else {
				gl_myinfo.priv.ppp_config.idle = atoi(cmdsep(&pcmd));
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			}
		} else if (what && strncmp(what, "profile", 1) == 0) {
			char *profile_id = NULL; 
			profile_id = cmdsep(&pcmd);
			if(profile_id && strncmp(profile_id, "auto", 1) == 0) {
				n = 0;	
			}else if (profile_id) {
				n = atoi(profile_id);	
			}

			what = cmdsep(&pcmd);

			if (cmd->type == CMD_TYPE_NO
		        ||cmd->type == CMD_TYPE_DEFAULT) {
				if(!what) gl_myinfo.priv.ppp_config.profile_idx = 1;
				else if(strncmp(what, "secondary", 1) == 0) gl_myinfo.priv.ppp_config.profile_2nd_idx = 0;
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			} else {
				if(n == 0) {
					if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_LE910) {
						gl_ppp_redial_cnt=0;
					}

					if(!what) gl_myinfo.priv.ppp_config.profile_idx = n;
					else if(strncmp(what, "secondary", 1) == 0) gl_myinfo.priv.ppp_config.profile_2nd_idx = n;
				}else {
					if(gl_myinfo.priv.ppp_config.profiles[n-1].type != PROFILE_TYPE_NONE){
						if(!what) gl_myinfo.priv.ppp_config.profile_idx = n;
						else if(strncmp(what, "secondary", 1) == 0) gl_myinfo.priv.ppp_config.profile_2nd_idx = n;
					}else ih_cmd_rsp_print("%%error: profile %d not found!", n);
				}
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			}
		} else if (what && strncmp(what, "timeout", 1) == 0) {
			if (cmd->type == CMD_TYPE_NO
		        ||cmd->type == CMD_TYPE_DEFAULT) {
				gl_myinfo.priv.ppp_config.timeout = REDIAL_DEFAULT_TIMEOUT;
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			} else {
				gl_myinfo.priv.ppp_config.timeout = atoi(cmdsep(&pcmd));
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			}
		} else if (what && strncmp(what, "nai", 3) == 0) {
			//char *nai_no = NULL;
			//LOG_IN("%s", what);
			if (cmd->type == CMD_TYPE_NO
		        ||cmd->type == CMD_TYPE_DEFAULT) {
				bzero(gl_myinfo.priv.modem_config.nai_no, sizeof(gl_myinfo.priv.modem_config.nai_no));
				bzero(gl_myinfo.priv.modem_config.nai_pwd, sizeof(gl_myinfo.priv.modem_config.nai_pwd));
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			} else {
				what = cmdsep(&pcmd);
				if (!what) 
					MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
				//nai_no = what;
					
				bzero(gl_myinfo.priv.modem_config.nai_no, sizeof(gl_myinfo.priv.modem_config.nai_no));
				memcpy(gl_myinfo.priv.modem_config.nai_no, what, strlen(what)+1);

				what = cmdsep(&pcmd);
				if (!what) 
					MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));

				bzero(gl_myinfo.priv.modem_config.nai_pwd, sizeof(gl_myinfo.priv.modem_config.nai_pwd));
				memcpy(gl_myinfo.priv.modem_config.nai_pwd, what, strlen(what)+1);
				
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			}
		}else if (what && strncmp(what, "secondary-wan", 1) == 0) {
			char *profile_id = NULL; 

			if (cmd->type == CMD_TYPE_NO
		        ||cmd->type == CMD_TYPE_DEFAULT) {
				what = cmdsep(&pcmd);
				if(!what) {
				 	//gl_myinfo.priv.ppp_config.profile_wwan2_idx = 0;
					modem_config->dual_wwan_enable[0] = 0;
				}else if (strncmp(what, "secondary", 1) == 0) {
					//gl_myinfo.priv.ppp_config.profile_wwan2_2nd_idx = 0;
					modem_config->dual_wwan_enable[1] = 0;
				}
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			} else {
				profile_id = cmdsep(&pcmd);
				if(profile_id && strncmp(profile_id, "auto", 1) == 0) {
					n = 0;	
				}else {
					n = atoi(profile_id);	
				}

				what = cmdsep(&pcmd);
				if(n == 0) {
					if(!what) {
						gl_myinfo.priv.ppp_config.profile_wwan2_idx = n;
						modem_config->dual_wwan_enable[0] = 1;
					} else if(strncmp(what, "secondary", 1) == 0){
						gl_myinfo.priv.ppp_config.profile_wwan2_2nd_idx = n;
						modem_config->dual_wwan_enable[1] = 1;
					}
				}else if(gl_myinfo.priv.ppp_config.profiles[n-1].type != PROFILE_TYPE_NONE){
					if(!what) {
						gl_myinfo.priv.ppp_config.profile_wwan2_idx = n;
						modem_config->dual_wwan_enable[0] = 1;
					} else if(strncmp(what, "secondary", 1) == 0) {
						gl_myinfo.priv.ppp_config.profile_wwan2_2nd_idx = n;
						modem_config->dual_wwan_enable[1] = 1;
					} else {
						MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
					}					
				}else {
					ih_cmd_rsp_print("%%error: profile %d not found!", n);
				}

				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			}

		}else if (what && strncmp(what, "infinitely-retry", 2) == 0) {
			if (cmd->type == CMD_TYPE_NO
		        ||cmd->type == CMD_TYPE_DEFAULT) {
				gl_myinfo.priv.ppp_config.infinitely_dial_retry = 0;
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			} else {
				gl_myinfo.priv.ppp_config.infinitely_dial_retry = 1;
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			}
		}
	}
	/*[no] ip address negotiated
	 *[no] ip address static local <ip>
	 *[no] ip address static peer <ip>
	 *[no] ip address static local <ip> peer <ip>
	 *[no] ip mru <64-1500>
	 *[no] ip mtu <64-1500>
	 *[no] ip tcp header-compression [special-vj]
	 */
	if (strcmp(cmd->cmd, "ip") == 0) {		
		config_type = REDIAL_CONFIG_PPP;
		what = cmdsep(&pcmd);
		if (cmd->type == CMD_TYPE_NO
		    ||cmd->type == CMD_TYPE_DEFAULT) {
			//TODO
			if (what && strncmp(what, "address", 1) == 0) {
				what = cmdsep(&pcmd);
				if(strncmp(what, "static", 1) == 0) {
					what = cmdsep(&pcmd);
					if(!what){
						gl_myinfo.priv.ppp_config.ppp_static = 0;
					} else if(strncmp(what, "local", 1) == 0) {
						gl_myinfo.priv.ppp_config.ppp_ip[0] = '\0';
					} else if(strncmp(what, "peer", 1) == 0) {
						gl_myinfo.priv.ppp_config.ppp_peer[0] = '\0';
					} else if(strncmp(what, "netmask", 1) == 0) {
						gl_myinfo.priv.ppp_config.static_netmask[0] = '\0';
					}
					if((!gl_myinfo.priv.ppp_config.ppp_ip[0])&&(!gl_myinfo.priv.ppp_config.ppp_peer[0]))
						gl_myinfo.priv.ppp_config.ppp_static = 0;
				}
			} else if (what && strncmp(what, "mru", 2) == 0) {
				gl_myinfo.priv.ppp_config.mru = 1500;
			} else if (what && strncmp(what, "mtu", 2) == 0) {
				gl_myinfo.priv.ppp_config.mtu = 1500;
			} else if (what && strncmp(what, "tcp", 1) == 0) {
				gl_myinfo.priv.ppp_config.vj = 0;
				gl_myinfo.priv.ppp_config.novjccomp = 0;
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "address", 1) == 0) {
			what = cmdsep(&pcmd);
			if(strncmp(what, "negotiated", 1) == 0) {
				gl_myinfo.priv.ppp_config.ppp_static = 0;
			} else if(strncmp(what, "static", 1) == 0) {
				gl_myinfo.priv.ppp_config.ppp_static = 1;
				what = cmdsep(&pcmd);
				if(strncmp(what, "local", 1) == 0) {
					//gl_myinfo.priv.ppp_config.ppp_peer[0] = '\0';
					strlcpy(gl_myinfo.priv.ppp_config.ppp_ip, cmdsep(&pcmd), sizeof(gl_myinfo.priv.ppp_config.ppp_ip));
					what = cmdsep(&pcmd);
					if(what && strncmp(what, "peer", 1) == 0)
						strlcpy(gl_myinfo.priv.ppp_config.ppp_peer, cmdsep(&pcmd), sizeof(gl_myinfo.priv.ppp_config.ppp_peer));
				} else if(strncmp(what, "peer", 1) == 0) {
					//gl_myinfo.priv.ppp_config.ppp_ip[0] = '\0';
					strlcpy(gl_myinfo.priv.ppp_config.ppp_peer, cmdsep(&pcmd), sizeof(gl_myinfo.priv.ppp_config.ppp_peer));
				} else if(strncmp(what, "netmask", 1) == 0) {
					strlcpy(gl_myinfo.priv.ppp_config.static_netmask, cmdsep(&pcmd), sizeof(gl_myinfo.priv.ppp_config.static_netmask));
				}
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "mru", 2) == 0) {
			gl_myinfo.priv.ppp_config.mru = atoi(cmdsep(&pcmd));
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "mtu", 2) == 0) {
			gl_myinfo.priv.ppp_config.mtu = atoi(cmdsep(&pcmd));
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "tcp", 1) == 0) {
			what = cmdsep(&pcmd);
			if(strncmp(what, "header-compression", 1) == 0) {
				gl_myinfo.priv.ppp_config.vj = 1;
				if(pcmd)
					gl_myinfo.priv.ppp_config.novjccomp = 1;
				else
					gl_myinfo.priv.ppp_config.novjccomp = 0;
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		}	
	}

	if (strcmp(cmd->cmd, "ipv6") == 0) {		
		config_type = REDIAL_CONFIG_PPP;
		what = cmdsep(&pcmd);
		if (what && strncmp(what, "address", 1) == 0) {
			what = cmdsep(&pcmd);
			if (what && strncmp(what, "share-prefix", 2) == 0) {
				if(cmd->type == CMD_TYPE_NO
					||cmd->type == CMD_TYPE_DEFAULT){
					memset(gl_myinfo.priv.ppp_config.shared_label, 0, sizeof(gl_myinfo.priv.ppp_config.shared_label));
					strlcpy(gl_myinfo.priv.ppp_config.shared_label, DEFAULT_IPV6_SHARED_LABEL, sizeof(gl_myinfo.priv.ppp_config.shared_label));
					gl_myinfo.priv.ppp_config.prefix_share = FALSE;
					request_set_pd_label(pinfo->if_info, PD_NONE, gl_myinfo.priv.ppp_config.shared_label, 0);
				}else {
					what = cmdsep(&pcmd);
					memset(gl_myinfo.priv.ppp_config.shared_label, 0, sizeof(gl_myinfo.priv.ppp_config.shared_label));
					if (what) {
						strlcpy(gl_myinfo.priv.ppp_config.shared_label, what, sizeof(gl_myinfo.priv.ppp_config.shared_label));
					}else {
						strlcpy(gl_myinfo.priv.ppp_config.shared_label, DEFAULT_IPV6_SHARED_LABEL, sizeof(gl_myinfo.priv.ppp_config.shared_label));
					}
					gl_myinfo.priv.ppp_config.prefix_share = TRUE;
					request_set_pd_label(pinfo->if_info, PREFIX_SHARE, gl_myinfo.priv.ppp_config.shared_label, 0);
				}
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			}else if (what && strncmp(what, "dhcp-pd", 1) == 0) {
				char *pd_label;
				if(cmd->type == CMD_TYPE_NO
					||cmd->type == CMD_TYPE_DEFAULT){
					what = cmdsep(&pcmd);
					if (!what) {
						memset(gl_myinfo.priv.ppp_config.pd_label, 0, sizeof(gl_myinfo.priv.ppp_config.pd_label));
						strlcpy(gl_myinfo.priv.ppp_config.pd_label, DEFAULT_IPV6_PD_LABEL, sizeof(gl_myinfo.priv.ppp_config.pd_label));
						//gl_myinfo.priv.ppp_config.prefix_share = FALSE;
						request_set_pd_label(pinfo->if_info, PD_NONE, gl_myinfo.priv.ppp_config.pd_label, 0);
					}else if (strncmp(what, "secondary", 1) == 0) {
						memset(gl_myinfo.priv.wwan2_config.pd_label, 0, sizeof(gl_myinfo.priv.wwan2_config.pd_label));
						strlcpy(gl_myinfo.priv.wwan2_config.pd_label, DEFAULT_WWAN2_IPV6_PD_LABEL, sizeof(gl_myinfo.priv.wwan2_config.pd_label));
						//gl_myinfo.priv.wwan2_config.prefix_share = FALSE;
						request_set_pd_label(pinfo_2nd->if_info, PD_NONE, gl_myinfo.priv.wwan2_config.pd_label, 0);
					}
				}else {
					pd_label = cmdsep(&pcmd);
					//memset(gl_myinfo.priv.ppp_config.pd_label, 0, sizeof(gl_myinfo.priv.ppp_config.pd_label));
					if (pd_label) {
						what = cmdsep(&pcmd);
						if (!what) {
							memset(gl_myinfo.priv.ppp_config.pd_label, 0, sizeof(gl_myinfo.priv.ppp_config.pd_label));
							strlcpy(gl_myinfo.priv.ppp_config.pd_label, pd_label, sizeof(gl_myinfo.priv.ppp_config.pd_label));
							//gl_myinfo.priv.ppp_config.prefix_share = TRUE;
							request_set_pd_label(pinfo->if_info, PD_SOURCE, gl_myinfo.priv.ppp_config.pd_label, 0);
						}else if (strncmp(what, "secondary", 1) == 0){
							memset(gl_myinfo.priv.wwan2_config.pd_label, 0, sizeof(gl_myinfo.priv.wwan2_config.pd_label));
							strlcpy(gl_myinfo.priv.wwan2_config.pd_label, pd_label, sizeof(gl_myinfo.priv.wwan2_config.pd_label));
							//gl_myinfo.priv.wwan2_config.prefix_share = TRUE;
							request_set_pd_label(pinfo_2nd->if_info, PD_SOURCE, gl_myinfo.priv.wwan2_config.pd_label, 0);
						}
					}else {
						strlcpy(gl_myinfo.priv.ppp_config.pd_label, DEFAULT_IPV6_PD_LABEL, sizeof(gl_myinfo.priv.ppp_config.pd_label));
						strlcpy(gl_myinfo.priv.wwan2_config.pd_label, DEFAULT_WWAN2_IPV6_PD_LABEL, sizeof(gl_myinfo.priv.wwan2_config.pd_label));
					}
				}
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			}else if (what && strncmp(what, "static", 2) == 0) {
				char *if_id = NULL;
				struct in6_addr ip6_addr;
				int  prefix_len;

				what = cmdsep(&pcmd);
				if (what && strncmp(what, "interface-id", 1) == 0) {
					if (cmd->type == CMD_TYPE_NO
					||cmd->type == CMD_TYPE_DEFAULT) {
						what = cmdsep(&pcmd);
						if(!what) {
							gl_myinfo.priv.ppp_config.static_if_id_enable = FALSE;
						}else if (strncmp(what, "secondary", 1) == 0){
							gl_myinfo.priv.wwan2_config.static_if_id_enable = FALSE;
						}
					}else {
						if_id = cmdsep(&pcmd);
						if (if_id) {
							what = cmdsep(&pcmd); 
							if(!what) {
								memset(gl_myinfo.priv.ppp_config.static_interface_id, 0, sizeof(gl_myinfo.priv.ppp_config.static_interface_id));
								strlcpy(gl_myinfo.priv.ppp_config.static_interface_id, if_id, sizeof(gl_myinfo.priv.ppp_config.static_interface_id));
								gl_myinfo.priv.ppp_config.static_if_id_enable = TRUE;
							}else if (strncmp(what, "secondary", 1) == 0){
								memset(gl_myinfo.priv.wwan2_config.static_interface_id, 0, sizeof(gl_myinfo.priv.wwan2_config.static_interface_id));
								strlcpy(gl_myinfo.priv.wwan2_config.static_interface_id, if_id, sizeof(gl_myinfo.priv.wwan2_config.static_interface_id));
								gl_myinfo.priv.wwan2_config.static_if_id_enable = TRUE;
							}
						}else {
							MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
						}
					}
				}else if(what && strncmp(what, "dhcp-pd", 1) == 0){
					if (cmd->type == CMD_TYPE_NO
					||cmd->type == CMD_TYPE_DEFAULT) {
						what = cmdsep(&pcmd);
						if(!what) {
							bzero(&gl_myinfo.priv.ppp_config.static_iapd_prefix , sizeof(gl_myinfo.priv.ppp_config.static_iapd_prefix));
							gl_myinfo.priv.ppp_config.static_iapd_prefix_len = 0;
							gl_myinfo.priv.ppp_config.static_iapd_prefix_strict = FALSE;
						}else if (strncmp(what, "secondary", 1) == 0){
							bzero(&gl_myinfo.priv.wwan2_config.static_iapd_prefix , sizeof(gl_myinfo.priv.wwan2_config.static_iapd_prefix));
							gl_myinfo.priv.wwan2_config.static_iapd_prefix_len = 0;
							gl_myinfo.priv.wwan2_config.static_iapd_prefix_strict = FALSE;
						} else if (strncmp(what, "ia-pd", 1) == 0){
							what = cmdsep(&pcmd);
							if(!what) {
									MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
							}else if (strncmp(what, "strct", 1) == 0){
								what = cmdsep(&pcmd);
								if(!what) {
									gl_myinfo.priv.ppp_config.static_iapd_prefix_strict = FALSE;
								}else if (strncmp(what, "secondary", 1) == 0){
									gl_myinfo.priv.wwan2_config.static_iapd_prefix_strict = FALSE;
								}
							}
						}
					}else {
						what = cmdsep(&pcmd); 
						if (!what) {
							MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
						}else if (strncmp(what, "ia-pd", 1) == 0) {
							what = cmdsep(&pcmd); 
							if (!what) {
								MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
							}else if (strncmp(what, "prefix-addr", 1) == 0) {
								what = cmdsep(&pcmd);
								if(!what) {
									MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
								}

								if(!str_to_ip6addr_prefixlen(what, &ip6_addr, &prefix_len)) {
									ih_cmd_rsp_print("parse IPv6 address/prefix failed!");
									MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
								}

								what = cmdsep(&pcmd);
								if (!what) {
									memcpy(&gl_myinfo.priv.ppp_config.static_iapd_prefix, &ip6_addr, sizeof(gl_myinfo.priv.ppp_config.static_iapd_prefix));
									gl_myinfo.priv.ppp_config.static_iapd_prefix_len = prefix_len;
								}else if (strncmp(what, "secondary", 1) == 0) {
									memcpy(&gl_myinfo.priv.wwan2_config.static_iapd_prefix, &ip6_addr, sizeof(gl_myinfo.priv.wwan2_config.static_iapd_prefix));
									gl_myinfo.priv.wwan2_config.static_iapd_prefix_len = prefix_len;
								}else {
									MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
								}

							}else if (strncmp(what, "strict", 1) == 0) {
								what = cmdsep(&pcmd);
								if (!what) {
									gl_myinfo.priv.ppp_config.static_iapd_prefix_strict = TRUE;
								}else if (strncmp(what, "secondary", 1) == 0) {
									gl_myinfo.priv.wwan2_config.static_iapd_prefix_strict = TRUE;
								}else {
									MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
								}
							}

						}
					}
				}
				MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
			}
		}else if (what && strncmp(what, "nd", 1) == 0) {
			what = cmdsep(&pcmd);
			if (what && strncmp(what, "proxy", 1) == 0) {
				if (cmd->type == CMD_TYPE_NO
				||cmd->type == CMD_TYPE_DEFAULT) {
					gl_myinfo.priv.ppp_config.ndp_enable = FALSE;
				}else {
					gl_myinfo.priv.ppp_config.ndp_enable = TRUE;
				}
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		}
	}
	/*ppp ipcp dns request
	 *ppp keepalive <seconds> <retries>
	 *[no] ppp ipcp dns request
	 *[no] ppp keepalive
	 *[no] ppp accm default
	 *[no] ppp acfc forbid
	 *[no] ppp pfc forbid
	 *[no] ppp debug
	 *[no] ppp options <string>
	 */
	if (strcmp(cmd->cmd, "ppp") == 0) {		
		config_type = REDIAL_CONFIG_PPP;
		what = cmdsep(&pcmd);
		if (cmd->type == CMD_TYPE_NO
		    ||cmd->type == CMD_TYPE_DEFAULT) {
			//todo
			if(what && strncmp(what, "accm", 3) == 0) {
				gl_myinfo.priv.ppp_config.default_am = 0;
			} else if (what && strncmp(what, "acfc", 3) == 0) {
				gl_myinfo.priv.ppp_config.noaccomp = 0;
			} else if(what && strncmp(what, "debug", 1) == 0) {
				gl_myinfo.priv.ppp_debug = FALSE;
			} else if (what && strncmp(what, "initial", 2) == 0) {
				gl_myinfo.priv.ppp_config.init_string[0] = '\0';
			} else if(what && strncmp(what, "ipcp", 2) == 0) {
				gl_myinfo.priv.ppp_config.peerdns = 0;
			} else if (what && strncmp(what, "keepalive", 1) == 0) {
				gl_myinfo.priv.ppp_config.lcp_echo_interval = 0;
				gl_myinfo.priv.ppp_config.lcp_echo_retries = 0;
			} else if (what && strncmp(what, "options", 1) == 0) {
				gl_myinfo.priv.ppp_config.options[0] = '\0';
			} else if (what && strncmp(what, "pfc", 1) == 0) {
				gl_myinfo.priv.ppp_config.nopcomp = 0;
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "accm", 3) == 0) {
			what = cmdsep(&pcmd);
			if(strncmp(what, "default", 1)==0)
				gl_myinfo.priv.ppp_config.default_am = 1;
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "acfc", 3) == 0) {
			gl_myinfo.priv.ppp_config.noaccomp = 1;
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "debug", 1) == 0) {
			gl_myinfo.priv.ppp_debug = TRUE;
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "initial", 2) == 0) {
			cmdsep(&pcmd);
			strlcpy(gl_myinfo.priv.ppp_config.init_string, cmdsep(&pcmd),
				sizeof(gl_myinfo.priv.ppp_config.init_string));
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "ipcp", 2) == 0) {
			gl_myinfo.priv.ppp_config.peerdns = 1;
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "keepalive", 1) == 0) {
			gl_myinfo.priv.ppp_config.lcp_echo_interval = atoi(cmdsep(&pcmd));
			gl_myinfo.priv.ppp_config.lcp_echo_retries = atoi(cmdsep(&pcmd));
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "options", 1) == 0) {
			what = cmdsep(&pcmd);
			/*escape ""*/
			if(*what=='"') {
				if(*(what+1)) {
					what[strlen(what)-1] = '\0';
					strlcpy(gl_myinfo.priv.ppp_config.options, what+1,
						sizeof(gl_myinfo.priv.ppp_config.options));
				}
			} else {
				strlcpy(gl_myinfo.priv.ppp_config.options, what,
					sizeof(gl_myinfo.priv.ppp_config.options));
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "pfc", 1) == 0) {
			gl_myinfo.priv.ppp_config.nopcomp = 1;
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		}
	}
	/*dhcp option mtu request
	 *[no] dhcp option mtu request
	 */
	if (strcmp(cmd->cmd, "dhcp") == 0) {		
		config_type = REDIAL_CONFIG_DHCP;
		what = cmdsep(&pcmd);
		if (cmd->type == CMD_TYPE_NO
		    ||cmd->type == CMD_TYPE_DEFAULT) {
			if(what && strncmp(what, "option", 1) == 0) {
				what = cmdsep(&pcmd);
				if(strncmp(what, "mtu", 1) == 0){
					gl_myinfo.priv.dhcp_options &= ~DHCP_OPT_MTU;
				}
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else if (what && strncmp(what, "option", 1) == 0) {
			what = cmdsep(&pcmd);
			if(strncmp(what, "mtu", 1) == 0){
				gl_myinfo.priv.dhcp_options |= DHCP_OPT_MTU;
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		}
	}
	/*[no] shutdown
	 */
	if (strcmp(cmd->cmd, "shutdown") == 0) {		
		config_type = REDIAL_CONFIG_PPP;
		if (cmd->type == CMD_TYPE_NO
		    ||cmd->type == CMD_TYPE_DEFAULT) {
			pinfo->enabled = TRUE;
			gl_myinfo.priv.enable = TRUE;
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else {
			if (gl_ppp_pid < 0) {
				if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS31) {
					modem_enter_airplane_mode(gl_commport); 
				}
			}

			pinfo->enabled = FALSE;
			gl_myinfo.priv.enable = FALSE;
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		}
	}
	/*[no] compress bsdcomp|deflate|mppc
	 */
	if (strcmp(cmd->cmd, "compress") == 0) {		
		config_type = REDIAL_CONFIG_PPP;
		if (cmd->type == CMD_TYPE_NO
		    ||cmd->type == CMD_TYPE_DEFAULT) {
			gl_myinfo.priv.ppp_config.ppp_comp = PPP_COMP_NONE;
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else {
			what = cmdsep(&pcmd);
			if (strncmp(what, "bsdcomp", 1) == 0) {
				gl_myinfo.priv.ppp_config.ppp_comp = PPP_COMP_BSD;
			} else if (strncmp(what, "deflate", 1) == 0) {
				gl_myinfo.priv.ppp_config.ppp_comp = PPP_COMP_DEFLATE;
			} else if (strncmp(what, "mppc", 1) == 0) {
				gl_myinfo.priv.ppp_config.ppp_comp = PPP_COMP_MPPC;
			}
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		}
	}
	/*description <string>
	 *[no] description
	 */
	if (strcmp(cmd->cmd, "description") == 0) {		
		if (cmd->type == CMD_TYPE_NO
		    ||cmd->type == CMD_TYPE_DEFAULT) {
			gl_myinfo.priv.description[0] = '\0';
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		} else {
			strlcpy(gl_myinfo.priv.description, cmdsep(&pcmd), sizeof(gl_myinfo.priv.description));
			MY_LEAVE(ih_cmd_rsp_finish(ERR_OK));
		}
	}
	
	MY_LEAVE(ih_cmd_rsp_finish(ERR_INVAL));
	
leave:
	//handle config change
	if(config_type == REDIAL_CONFIG_PPP
		|| config_type == REDIAL_CONFIG_DHCP) {
#if 0
		if ((memcmp(&modem_config_tmp, modem_config, sizeof(MODEM_CONFIG))==0)
			 && (memcmp(&sms_config_tmp, sms_config, sizeof(SMS_CONFIG))==0)
			 && (memcmp(&ppp_config_tmp, &gl_myinfo.priv.ppp_config, sizeof(PPP_CONFIG))==0)) {
				return ret_code;
		}
#endif

		if (memcmp(&gl_myinfo_tmp.priv, &(gl_myinfo.priv), sizeof(gl_myinfo_tmp.priv))==0) {
				return ret_code;
		}

		do_redial_procedure();
	}
	return ret_code;
#undef MY_LEAVE
}

int do_redial_procedure(void)
{
	if(gl_redial_state < REDIAL_STATE_MODEM_INIT){
		return -1;
	}

	if(SUB_DAEMON_RUNNING) {
		stop_child_by_sub_daemon();
		goto end;
	}

	clear_at_state();
	event_del(&gl_ev_at);
	CLOSE_FD(gl_at_fd);
	
	//clear sim status	
	clear_sim_status();
	if(gl_redial_state == REDIAL_STATE_MODEM_SLEEP){
		gl_modem_cfun_reset_flag = TRUE;
	}
	
	if(gl_redial_state != REDIAL_STATE_CONNECTED){
		gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
	}
end:
	my_restart_timer(3);

	return 1;
}

int get_redial_state(void)
{
	return gl_redial_state;	
}

void enable_dual_sim(void)
{
	gl_myinfo.priv.modem_config.dualsim = TRUE;
}

void change_redial_state(REDIAL_STATE state)
{
	if(gl_redial_state == state){
		return;
	}

	if(SUB_DAEMON_RUNNING) {
		stop_child_by_sub_daemon();
	}

	clear_at_state();
	event_del(&gl_ev_at);
	CLOSE_FD(gl_at_fd);

	//clear sms, signal timer
	event_del(&gl_ev_sms_tmr);
	event_del(&gl_ev_signal_tmr);
	event_del(&gl_ev_ppp_up_tmr);
	event_del(&gl_ev_wwan_connect_tmr);
	event_del(&gl_ev_redial_pending_task_tmr);

	gl_redial_state = state;
	
	my_restart_timer(1);
}

//////////////////////////////////////////////////////////////////////////////
// SIGNAL EVENT
static struct event gl_ev_sigchld, gl_ev_sighup, gl_ev_sigterm, gl_ev_sigint, gl_ev_sigusr1, gl_ev_sigalarm;

/*
 * SIGHUP handle
 */
static void sighup_cb(int sig, short ev, void *data)
{
	MYTRACE_ENTER();
#if 0
	/* Restart and re-read configuration */
	LOG_WA("Received SIGHUP; restarting...");
	restart();
#else
	LOG_WA("Received SIGHUP");
	change_redial_state(REDIAL_STATE_INIT);
#endif

	MYTRACE_LEAVE();
}

/*
 * General signal handle
 */
static void gensig_cb(int sig, short ev, void *data)
{
	char *sigstr;

	MYTRACE_ENTER();

	switch (sig) {
	case SIGTERM:
		sigstr = "SIGTERM";
		break;
	case SIGINT:
		sigstr = "SIGINT";
		break;
	default:
		sigstr = "an unknown signal";
		break;
	}

	LOG_ER("Received %s; quitting...", sigstr);

#ifdef NETCONF
	ih_sysrepo_cleanup();
#endif

	stop();

	MYTRACE_LEAVE();
}

static void sigalrm_cb(int sig, short ev, void *data)
{
	MYTRACE_ENTER();

	LOG_ER("Received SIGALRM!");
	clear_mbim_child_processes();

	set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);

	gl_redial_state = REDIAL_STATE_MODEM_STARTUP;

	my_stop_timer(&gl_ev_tmr);
	my_restart_timer(15);

	MYTRACE_LEAVE();
}

/*
 * SIGUSR1 handle, toggle tracing
 */
static void sigusr1_cb(int sig, short ev, void *data)
{
	MYTRACE_ENTER();

	trace_category_set(trace_category_get() ? 0 : DEFAULT_TRACE_CAT);
	LOG_IN("Received SIGUSR1; %s tracing to %s",
			trace_category_get() ? "start" : "stop",
			gl_trace_file);

	MYTRACE_LEAVE();
}

/*
 * SIGCHLD handle
 */
static void sigchld_cb(int sig, short ev, void *data)
{
	int status;
	pid_t pid;

	MYTRACE_ENTER();

	//LOG_DB("call sigchld_cb");
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0 ||
	    (pid < 0 && errno == EINTR)) {
		if (pid < 0) continue;
		if (pid == gl_ppp_pid) {
			gl_ppp_pid = -1;
			LOG_IN("child %s%s exited!", get_sub_deamon_name(), gl_dual_wwan_enable ? "1" : "");
		} else if (pid == gl_2nd_ppp_pid) {
			gl_2nd_ppp_pid = -1;
			LOG_IN("child %s%s exited!", get_sub_deamon_name(), gl_dual_wwan_enable ? "2" : "");
		} else if (pid == gl_qmi_proxy_pid) {
			gl_qmi_proxy_pid = -1;
			LOG_IN("child ih-qmi-proxy exited!");
		} else if (pid == gl_odhcp6c_pid) {
			gl_odhcp6c_pid = -1;
			LOG_IN("child odhcp6c%s exited!", gl_dual_wwan_enable ? "1" : "");
		} else if (pid == gl_2nd_odhcp6c_pid) {
			gl_2nd_odhcp6c_pid = -1;
			LOG_IN("child odhcp6c%s exited!", gl_dual_wwan_enable ? "2" : "");
		} else if (pid == gl_smsd_pid) {
			gl_smsd_pid = -1;
			LOG_IN("child smsd exited!");
			//TODO: delay start_smsd
		}
	}

	MYTRACE_LEAVE();
}

/*
 * setup signal handle
 */
static void signal_setup(void)
{
	int sig;

	MYTRACE_ENTER();

	// reset signal handlers
	for (sig = 0; sig < (_NSIG - 1); sig++)
		signal(sig, SIG_IGN);

	signal_set(&gl_ev_sighup, SIGHUP, sighup_cb, &gl_ev_sighup);
	event_base_set(g_my_event_base, &gl_ev_sighup);
	if (signal_add(&gl_ev_sighup, NULL) == -1)
		LOG_ER("cannot add handle for SIGHUP");

	signal_set(&gl_ev_sigusr1, SIGUSR1, sigusr1_cb, &gl_ev_sigusr1);
	event_base_set(g_my_event_base, &gl_ev_sigusr1);
	if (signal_add(&gl_ev_sigusr1, NULL) == -1)
		LOG_ER("cannot add handle for SIGUSR1");

	signal_set(&gl_ev_sigchld, SIGCHLD, sigchld_cb, &gl_ev_sigchld);
	event_base_set(g_my_event_base, &gl_ev_sigchld);
	if (signal_add(&gl_ev_sigchld, NULL) == -1)
		LOG_ER("cannot add handle for SIGCHLD");

	signal_set(&gl_ev_sigterm, SIGTERM, gensig_cb, &gl_ev_sigterm);
	event_base_set(g_my_event_base, &gl_ev_sigterm);
	if (signal_add(&gl_ev_sigterm, NULL) == -1)
		LOG_ER("cannot add handle for SIGTERM");

	signal_set(&gl_ev_sigint, SIGINT, gensig_cb, &gl_ev_sigint);
	event_base_set(g_my_event_base, &gl_ev_sigint);
	if (signal_add(&gl_ev_sigint, NULL) == -1)
		LOG_ER("cannot add handle for SIGINT");

	signal_set(&gl_ev_sigalarm, SIGALRM, sigalrm_cb, &gl_ev_sigalarm);
	event_base_set(g_my_event_base, &gl_ev_sigalarm);
	if (signal_add(&gl_ev_sigalarm, NULL) == -1)
		LOG_ER("cannot add handle for SIGINT");



	MYTRACE_LEAVE();
}

/*
 * handle IPC msg
 *
 * @param msg				received IPC msg from ippassthrough
 *
 * @return ERR_OK for ok, others for error
 */
static int32 ippt_msg_handle(IPC_MSG_HDR *msg_hdr, char *msg, uns32 msg_len,char *obuf, uns32 olen)
{
	IP_PASSINFO* ippt_info = (IP_PASSINFO*)msg;
	IP_PASSADDRINFO waninfo;
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	char iface_name[IF_CMD_NAME_SIZE];
	char cmdbuff[64] = {0};
	IP_PASSRESULT s_result;

	MYTRACE_ENTER();

	switch(ippt_info->action){
		case IPPT_CONFIG:
			if(ippt_info->enable && !IF_INFO_CMP(&(ippt_info->wanif_info), &(gl_myinfo.svcinfo.vif_info.if_info))){
				LOG_DB("ippt wan iface is celluar,start ippt");
            	memcpy(&(waninfo.ip), &(pinfo->local_ip), sizeof(struct in_addr));
	            memcpy(&(waninfo.netmask), &(pinfo->netmask), sizeof(struct in_addr));
				memcpy(&(waninfo.gateway), &(pinfo->remote_ip), sizeof(struct in_addr));
				vif_get_sys_name(&ippt_info->wanif_info, iface_name);
	            handle_ip_passthrough_wanset(&waninfo, iface_name);
				
				s_result.result = TRUE;
				memcpy(&s_result.wanif_info, &ippt_info->wanif_info, sizeof(IF_INFO));
				memcpy(&s_result.lanif_info, &ippt_info->lanif_info, sizeof(IF_INFO));
				ih_ipc_send_no_ack(IH_SVC_IPPASSTH, IPC_MSG_IP_PASSTHROUGH_RESULT, &s_result, sizeof(IP_PASSRESULT));
        }
            break;
		case IPPT_DISABLE:
		case IPPT_LINK_RECOVERY:
			if(!IF_INFO_CMP(&(ippt_info->wanif_info),&(pinfo->if_info))){
				memset(iface_name, 0, IF_CMD_NAME_SIZE);
				vif_get_sys_name(&pinfo->if_info, iface_name);
				
				memset(cmdbuff, 0, sizeof(cmdbuff));
				snprintf(cmdbuff, sizeof(cmdbuff), "echo 0 > /proc/sys/net/ipv4/conf/%s/proxy_arp", iface_name);
				system(cmdbuff);

				if(SUB_DAEMON_RUNNING){
					//reload the server to get ip info again
					stop_child_by_sub_daemon();
				}
			}

			break;
		default:
			break;
	}

	MYTRACE_LEAVE();

	return ERR_OK;
}
/*
 * handle IPC msg
 *
 * @param peer_svc_id		cmd from which service
 * @param msg				received IPC msg
 * @param rsp_reqd			whether to send response
 *
 * @return ERR_OK for ok, others for error
 */
static int32 config_msg_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen)   
{
	uns32 peer_svc_id = msg_hdr->svc_id;
	BOOL need_rsp = IPC_TEST_FLAG(msg_hdr->flag,IPC_MSG_FLAG_NEED_RSP);
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);

	MYTRACE_ENTER();

	switch(msg_hdr->type) {
	case IPC_MSG_CONFIG_FINISH:
		LOG_DB("dispatch configure ok...");
		gl_config_ready = TRUE;
#ifdef NETCONF
		redial_sysrepo_init();
#endif
		if (gl_myinfo.priv.ppp_config.prefix_share == TRUE) {
			request_set_pd_label(pinfo->if_info, PREFIX_SHARE, gl_myinfo.priv.ppp_config.shared_label, 0);
		}
		break;
	case IPC_MSG_CONFIG_RESET:
		LOG_IN("reset to default config");
		break;
	default:
		LOG_IN("MSG: 0x%x from service %d", msg_hdr->type, peer_svc_id);
		LOG_IN("    Len: %d", msg_hdr->len);
		LOG_IN("    Content: %s", msg);
		if (need_rsp) {
			ih_ipc_send_nak(peer_svc_id);
		}
		break;
	}

	MYTRACE_LEAVE();

	return ERR_OK;
}

static int save_raw_name_to_ifalias(void)
{
	char raw_name[32] = {0};
	char buff[32] = {0};
	char file[128] = {0};
	int i, qmap_mode = 0;	

	if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500){
		snprintf(file, sizeof(file), "/sys/class/net/"RMNET_MHI0_BASE"/qmap_mode");
		f_read(file, buff, sizeof(buff));
		qmap_mode = atoi(buff);
		for(i = 1; i <= qmap_mode; i++){
			snprintf(raw_name, sizeof(raw_name), RMNET_MHI0_BASE".%d", i);
			snprintf(file, sizeof(file), "/sys/class/net/%s/ifalias", raw_name);
			if(access(file, F_OK)){
				continue;
			}
			f_write(file, raw_name, strlen(raw_name), 0, 0);
		}
	}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EP06){
		snprintf(file, sizeof(file), "/sys/class/net/"WWAN0_BASE"/qmap_mode");
		f_read(file, buff, sizeof(buff));
		qmap_mode = atoi(buff);
		for(i = 1; i <= qmap_mode; i++){
			snprintf(raw_name, sizeof(raw_name), WWAN0_BASE".%d", i);
			snprintf(file, sizeof(file), "/sys/class/net/%s/ifalias", raw_name);
			if(access(file, F_OK)){
				continue;
			}
			f_write(file, raw_name, strlen(raw_name), 0, 0);
		}
	}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_FG360){
		for(i = 0; i <= MIPC_IF_MAX; i++){
			snprintf(raw_name, sizeof(raw_name), MIPC_IF_PREFIX"%d", i);
			snprintf(file, sizeof(file), "/sys/class/net/%s/ifalias", raw_name);
			if(access(file, F_OK)){
				continue;
			}
			f_write(file, raw_name, strlen(raw_name), 0, 0);
			eval("ip", "link", "set", raw_name, "down");
		}
	}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500U){
		if(gl_modem_dev_state & MODEM_DEV_PCIE_ALIVE){
			return ERR_OK;
		}
		
		if(ih_license_support(IH_FEATURE_PCIE2ETH_RTL8125B)){
			snprintf(raw_name, sizeof(raw_name), "%s", APN1_WWAN);
		}else{
			snprintf(raw_name, sizeof(raw_name), "%s", USB0_WWAN);
		}

		snprintf(file, sizeof(file), "/sys/class/net/%s/ifalias", raw_name);
		if(access(file, F_OK)){
			return ERR_FAILED;
		}
		f_write(file, raw_name, strlen(raw_name), 0, 0);
	}
	
	return ERR_OK;
}

static int rm500_rename_iface(char *sys_name, char *raw_name)
{
	char file[128] = {0};
	char buff[128] = {0};
	int i, qmap_mode = 0;	

	if(!sys_name || !raw_name){
		return ERR_FAILED;
	}

	snprintf(file, sizeof(file), "/sys/class/net/"RMNET_MHI0_BASE"/qmap_mode");
	f_read(file, buff, sizeof(buff));
	qmap_mode = atoi(buff);
	for(i = 1; i <= qmap_mode; i++){
		snprintf(file, sizeof(file), "/sys/class/net/"RMNET_MHI0_BASE".%d/ifalias", i);
		f_read(file, buff, sizeof(buff));
		if(strncmp(buff, raw_name, strlen(raw_name))){
			continue;
		}
		
		//rename
		eval("ip", "link", "set", raw_name, "down");
		eval("ip", "link", "set", raw_name, "name", sys_name);
		//eval("ip", "link", "set", sys_name, "up");
		return ERR_OK;
	}

	return ERR_FAILED;
}

static int ep06_rename_iface(char *sys_name, char *raw_name)
{
	char file[128] = {0};
	char buff[128] = {0};
	int i, qmap_mode = 0;	

	if(!sys_name || !raw_name){
		return ERR_FAILED;
	}

	snprintf(file, sizeof(file), "/sys/class/net/"WWAN0_BASE"/qmap_mode");
	f_read(file, buff, sizeof(buff));
	qmap_mode = atoi(buff);
	for(i = 1; i <= qmap_mode; i++){
		snprintf(file, sizeof(file), "/sys/class/net/"WWAN0_BASE".%d/ifalias", i);
		f_read(file, buff, sizeof(buff));
		if(strncmp(buff, raw_name, strlen(raw_name))){
			continue;
		}
		
		//rename
		eval("ip", "link", "set", raw_name, "down");
		eval("ip", "link", "set", raw_name, "name", sys_name);
		//eval("ip", "link", "set", sys_name, "up");
		return ERR_OK;
	}

	return ERR_FAILED;
}

static int fg360_rename_iface(char *sys_name, char *raw_name)
{
	char file[128] = {0};
	char buff[128] = {0};
	int i;	

	if(!sys_name || !raw_name){
		return ERR_FAILED;
	}

	for(i = 0; i <= MIPC_IF_MAX; i++){
		snprintf(file, sizeof(file), "/sys/class/net/"MIPC_IF_PREFIX"%d/ifalias", i);
		f_read(file, buff, sizeof(buff));
		if(strncmp(buff, raw_name, strlen(raw_name))){
			continue;
		}
		
		//rename
		eval("ip", "link", "set", raw_name, "down");
		eval("ip", "link", "set", raw_name, "name", sys_name);
		eval("ip", "link", "set", sys_name, "up");
		return ERR_OK;
	}

	return ERR_FAILED;
}

static int generic_rename_iface(char *sys_name, char *raw_name)
{
	if(!sys_name || !raw_name){
		return ERR_FAILED;
	}
	
	//rename
	eval("ip", "link", "set", raw_name, "down");
	eval("ip", "link", "set", raw_name, "name", sys_name);
	eval("ip", "link", "set", sys_name, "up");

	return ERR_OK;
}

static int check_cell_sys_name(int wwan_id, int id)
{
	VIF_INFO *pinfo = NULL;
	char sys_name[32] = {0}; //vlanxxx
	char raw_name[32] = {0}; //eth0.xxx
	char file[128] = {0};
	char buff[128] = {0};

#define REVERT_RAW_NAME(_sys, _raw) \
	eval("ip", "-4", "addr", "flush", "dev", _sys); \
	eval("ip", "link", "set", _sys, "down"); \
	eval("ip", "link", "set", _sys, "name", _raw);

	if(wwan_id == WWAN_ID2){
		pinfo = &(gl_myinfo.svcinfo.secondary_vif_info);
	}else{
		pinfo = &(gl_myinfo.svcinfo.vif_info);
	}

	vif_get_sys_name(&(pinfo->if_info), sys_name);
	if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500){
		if(modem_support_dual_apn()){
			snprintf(raw_name, sizeof(raw_name), RMNET_MHI0_BASE".%d", id);
		}else{
			snprintf(raw_name, sizeof(raw_name), RMNET_MHI0_BASE".1");
		}
		snprintf(file, sizeof(file), "/sys/class/net/%s/ifalias", sys_name);
		if(!access(file, F_OK)){
			f_read(file, buff, sizeof(buff));
			trim_str(buff); //delete wihtespace
			if(!strncmp(buff, raw_name, strlen(raw_name))){
				return ERR_OK;
			}else{
				REVERT_RAW_NAME(sys_name, buff);
				return rm500_rename_iface(sys_name, raw_name);
			}
		}else{
			return rm500_rename_iface(sys_name, raw_name);
		}
	}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EP06){
		if(modem_support_dual_apn()){
			snprintf(raw_name, sizeof(raw_name), WWAN0_BASE".%d", id);
		}else{
			snprintf(raw_name, sizeof(raw_name), WWAN0_BASE);
		}
		snprintf(file, sizeof(file), "/sys/class/net/%s/ifalias", sys_name);
		if(!access(file, F_OK)){
			f_read(file, buff, sizeof(buff));
			trim_str(buff); //delete wihtespace
			if(!strncmp(buff, raw_name, strlen(raw_name))){
				return ERR_OK;
			}else{
				REVERT_RAW_NAME(sys_name, buff);
				return ep06_rename_iface(sys_name, raw_name);
			}
		}else{
			return ep06_rename_iface(sys_name, raw_name);
		}
	}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_FG360){
		snprintf(raw_name, sizeof(raw_name), MIPC_IF_PREFIX"%d", id);
		snprintf(file, sizeof(file), "/sys/class/net/%s/ifalias", sys_name);
		if(!access(file, F_OK)){
			f_read(file, buff, sizeof(buff));
			trim_str(buff); //delete wihtespace
			if(!strncmp(buff, raw_name, strlen(raw_name))){
				return ERR_OK;
			}else{
				REVERT_RAW_NAME(sys_name, buff);
				return fg360_rename_iface(sys_name, raw_name);
			}
		}else{
			return fg360_rename_iface(sys_name, raw_name);
		}
	}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500U){
		if(gl_modem_dev_state & MODEM_DEV_PCIE_ALIVE){
			return ERR_OK;
		}

		//rename usb0 or apn1 to pcie0
		if(ih_license_support(IH_FEATURE_PCIE2ETH_RTL8125B)){
			snprintf(raw_name, sizeof(raw_name), "%s", APN1_WWAN);
		}else{
			snprintf(raw_name, sizeof(raw_name), "%s", USB0_WWAN);
		}

		snprintf(file, sizeof(file), "/sys/class/net/%s/ifalias", sys_name);
		if(!access(file, F_OK)){
			f_read(file, buff, sizeof(buff));
			trim_str(buff); //delete wihtespace
			if(!strncmp(buff, raw_name, strlen(raw_name))){
				return ERR_OK;
			}else{
				REVERT_RAW_NAME(sys_name, buff);
				return generic_rename_iface(sys_name, raw_name);
			}
		}else{
			return generic_rename_iface(sys_name, raw_name);
		}
	}

	return ERR_OK;
}

static VIF_INFO* get_svc_vif_info_by_name(char *ifname) 
{
	char sys_name1[32] = {0}; //vlanxxx
	char sys_name2[32] = {0}; //vlanxxx
	
	if (!ifname || !ifname[0]) {
		return NULL;
	}

	if(gl_dual_wwan_enable){
		vif_get_sys_name(&(gl_myinfo.svcinfo.vif_info.if_info), sys_name1);
		if(!strcmp(ifname, sys_name1)){
			return &(gl_myinfo.svcinfo.vif_info);
		}
		
		vif_get_sys_name(&(gl_myinfo.svcinfo.secondary_vif_info.if_info), sys_name2);
		if(!strcmp(ifname, sys_name2)){
			return &(gl_myinfo.svcinfo.secondary_vif_info);
		}
	}else{
		return  &(gl_myinfo.svcinfo.vif_info);
	}

	return NULL;
}

static PPP_CONFIG *get_modem_iface_config_by_name(char *ifname) 
{
	char sys_name1[32] = {0}; //vlanxxx
	char sys_name2[32] = {0}; //vlanxxx
	
	if (!ifname || !ifname[0]) {
		return NULL;
	}

	if(gl_dual_wwan_enable){
		vif_get_sys_name(&(gl_myinfo.svcinfo.vif_info.if_info), sys_name1);
		if(!strcmp(ifname, sys_name1)){
			return &(gl_myinfo.priv.ppp_config);
		}
		
		vif_get_sys_name(&(gl_myinfo.svcinfo.secondary_vif_info.if_info), sys_name2);
		if(!strcmp(ifname, sys_name2)){
			return &(gl_myinfo.priv.wwan2_config);
		}
	}else{
		return &(gl_myinfo.priv.ppp_config);
	}

	return NULL;
}

static void mipc_wwan_event_handle(MIPC_WWAN_EVT_INFO *info)
{
	struct timeval tv;
	char ifname[32] = {0};
	char buf[MAX_SYS_CMD_LEN] = {0};
	DUALSIM_INFO *dualsim_info = &(gl_myinfo.priv.dualsim_info);
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	PPP_CONFIG *ppp_config = &(gl_myinfo.priv.ppp_config);
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);

	if(check_cell_sys_name(WWAN_ID1, info->ifid)){
		LOG_ER("failed to check cellualr interface sys_name!");
		return;
	}

	vif_get_sys_name(&(pinfo->if_info), ifname);

	LOG_IN("MIPC_WWAN event %d, interface %s", info->evt, ifname);
	memset(buf, 0, sizeof(buf));
	switch(info->evt){
	case MIPC_WWAN_EVT_DOWN:
		eval("ip", "-4", "addr", "flush", "dev", ifname);
		pinfo->status = VIF_DOWN;
		gl_myinfo.svcinfo.uptime = 0;
		if(gl_redial_state == REDIAL_STATE_CONNECTED){
			my_restart_timer(1);
		}
		break;
	case MIPC_WWAN_EVT_UP:
		eval("ip", "-4", "addr", "flush", "dev", ifname);
		snprintf(pinfo->iface, sizeof(pinfo->iface), ifname);
		inet_aton(info->v4_ip, &pinfo->local_ip);
		if (ppp_config->static_netmask[0]) { 
			strlcpy(info->v4_subnet, ppp_config->static_netmask, sizeof(info->v4_subnet));
			inet_aton(ppp_config->static_netmask, &pinfo->netmask);
			snprintf(buf, sizeof(buf), "%s/%d", info->v4_ip, netmask_aton(ppp_config->static_netmask));
		}else {
			inet_aton(info->v4_subnet, &pinfo->netmask);
			snprintf(buf, sizeof(buf), "%s/%d", info->v4_ip, netmask_aton(info->v4_subnet));
		}

		LOG_IN("%s get ip address %s, netmask %s", ifname, info->v4_ip, info->v4_subnet);

		if (info->v4_gw[0]) {
			inet_aton(info->v4_gw, &pinfo->remote_ip);
			LOG_IN("%s get default gateway %s", ifname, info->v4_gw);
		}

		if(!strcmp(info->v4_subnet, "255.255.255.255")){
			eval("ifconfig", ifname, info->v4_ip, "netmask", info->v4_subnet, "pointopoint", info->v4_gw, "up");
		}else{
			eval("ip", "addr", "add", "dev", ifname, "local", buf);
		}

		if(info->v4_dns1[0]){
			inet_aton(info->v4_dns1, &pinfo->dns1);
			snprintf(buf, sizeof(buf), "echo nameserver %s>>/etc/resolv.dnsmasq", info->v4_dns1);
			system(buf);
		}
		
		if(info->v4_dns2[0]){
			inet_aton(info->v4_dns2, &pinfo->dns2);
			snprintf(buf, sizeof(buf), "echo nameserver %s>>/etc/resolv.dnsmasq", info->v4_dns2);
			system(buf);
		}

		if (gl_myinfo.priv.ppp_config.mtu > 0){
			sprintf(buf, "ifconfig %s mtu %d", ifname, gl_myinfo.priv.ppp_config.mtu);
			system(buf);
			pinfo->mtu = gl_myinfo.priv.ppp_config.mtu;
		}else if (info->v4_mtu > 0){
			LOG_IN("%s get mtu %d", ifname, info->v4_mtu);
			sprintf(buf, "ifconfig %s mtu %d", ifname, info->v4_mtu);
			system(buf);
			pinfo->mtu = info->v4_mtu;
		}else{
			pinfo->mtu = 1500;
			sprintf(buf, "ifconfig %s mtu %d", ifname, pinfo->mtu);
			system(buf);
		}
		
		pinfo->status = VIF_UP;
		gl_myinfo.svcinfo.uptime = get_uptime();

		evutil_timerclear(&tv);
		if(modem_config->dualsim)
			tv.tv_sec = dualsim_info->uptime;
		else
			tv.tv_sec = REDIAL_PPP_UP_TIME;
		event_add(&gl_ev_ppp_up_tmr, &tv);

		break;
	default:
		break;
	}
}

static void wwan0_dhcpc_event_handle(DHCPC_EVT_INFO *info)
{
	char buf[MAX_SYS_CMD_LEN] = {0};
	int ret = -1;
	struct timeval tv;
	DUALSIM_INFO *dualsim_info = &(gl_myinfo.priv.dualsim_info);
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	PPP_CONFIG *ppp_config = &(gl_myinfo.priv.ppp_config);

#if 0
	struct in_addr ip, mask;
#endif
	char *dns, *p;
	VIF_INFO *pinfo = NULL;

	pinfo = get_svc_vif_info_by_name(info->ifname); 
	if(!pinfo){
		LOG_ER("%s pinfo is null", __func__);
		return;
	}

	LOG_DB("DHCP Client event %s, interface %s", dhcpc_evt_str[info->dhcpc_evt], info->ifname);
	memset(buf, 0, sizeof(buf));
	switch(info->dhcpc_evt){
	case DHCPC_EVT_DECONFIG:
		eval("ip", "-4", "addr", "flush", "dev", info->ifname);
		pinfo->status = VIF_DOWN;
		gl_myinfo.svcinfo.uptime = 0;
		LOG_IN("DHCP Client event %s, interface %s", dhcpc_evt_str[info->dhcpc_evt], info->ifname);
		if(gl_redial_state == REDIAL_STATE_CONNECTED){
			if(get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI){
				ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, pinfo, sizeof(VIF_INFO));
			}else if(get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP){
				//LOG_WA("%s lease lost, try to redial.", get_sub_deamon_name());
				do_redial_procedure();
			}
			my_restart_timer(1);
		}
		break;
	case DHCPC_EVT_RENEW:
#if 0	//liyb:防止丢包.
		inet_aton(info->ip, &ip);
		inet_aton(info->subnet, &mask);
		eval("ip", "addr", "flush", "dev", info->ifname);
		snprintf(buf, sizeof(buf), "%s/%d", info->ip, netmask_aton(info->subnet));
		eval("ip", "addr", "add", "dev", info->ifname, "local", buf);

		if (info->gateway[0]) 
			eval("route", "add","default","gw",info->gateway);	
#endif
		break;
	case DHCPC_EVT_BOUND:
		LOG_IN("DHCP Client event %s, interface %s", dhcpc_evt_str[info->dhcpc_evt], info->ifname);
		eval("ip", "-4", "addr", "flush", "dev", info->ifname);
		snprintf(pinfo->iface, sizeof(pinfo->iface), info->ifname);
		inet_aton(info->ip,&pinfo->local_ip);
		if (ppp_config->static_netmask[0]) { 
			strlcpy(info->subnet, ppp_config->static_netmask, sizeof(info->subnet));
			inet_aton(ppp_config->static_netmask, &pinfo->netmask);
			snprintf(buf, sizeof(buf), "%s/%d", info->ip, netmask_aton(ppp_config->static_netmask));
		}else {
			inet_aton(info->subnet,&pinfo->netmask);
			snprintf(buf, sizeof(buf), "%s/%d", info->ip, netmask_aton(info->subnet));
		}

		LOG_IN("%s get ip address %s,netmask %s",info->ifname, info->ip, info->subnet);

		if (info->gateway[0]) {
		//	eval("route", "add","default","gw",info->gateway);	
			inet_aton(info->gateway,&pinfo->remote_ip);
			LOG_IN("%s get default gateway %s", info->ifname, info->gateway);
		}

		if(!strcmp(info->subnet, "255.255.255.255")){
			eval("ifconfig", info->ifname, info->ip, "netmask", info->subnet, "pointopoint", info->gateway, "up");
		}else{
			eval("ip", "addr", "add", "dev", info->ifname, "local", buf);
		}

		if(ih_license_support(IH_FEATURE_MODEM_RM520N)){
			//if we have got dnses from AT, use it.
			if(pinfo->dns1.s_addr){
				snprintf(info->dns, sizeof(info->dns), "%s", inet_ntoa(pinfo->dns1));
				if(pinfo->dns2.s_addr){
					snprintf(info->dns + strlen(info->dns), sizeof(info->dns) - strlen(info->dns), " %s", inet_ntoa(pinfo->dns2));
				}
			}
		}

        if (info->dns[0]) {
			LOG_IN("%s get dns %s", info->ifname, info->dns);
			dns = info->dns;
			p = strsep(&dns, " ");
			if (p) {
				inet_aton(p, &pinfo->dns1);
				bzero(buf, sizeof(buf));
				snprintf(buf, sizeof(buf), "echo nameserver %s>>/etc/resolv.dnsmasq", p);
				system(buf);
				
				p = strsep(&dns, " ");
				if(p){
					inet_aton(p, &pinfo->dns2);
					bzero(buf, sizeof(buf));
					snprintf(buf, sizeof(buf), "echo nameserver %s>>/etc/resolv.dnsmasq", p);
					system(buf);
				}
			}
		}
		
		if (gl_myinfo.priv.ppp_config.mtu > 0){
			bzero(buf, sizeof(buf));
			if (ih_license_support(IH_FEATURE_MODEM_LP41)
				|| (ih_license_support(IH_FEATURE_MODEM_ME909) 
					&& (ih_key_support("FH09")
					|| ih_key_support("FH19")))) {
					if (ih_check_interface_exist(USB0_WWAN)) {
						sprintf(buf, "ifconfig %s mtu %d", USB0_WWAN, gl_myinfo.priv.ppp_config.mtu);
					} else {	
						sprintf(buf, "ifconfig %s mtu %d", ETH2_WWAN, gl_myinfo.priv.ppp_config.mtu);
					}
			}else {
				sprintf(buf, "ifconfig %s mtu %d", info->ifname, gl_myinfo.priv.ppp_config.mtu);
			}
			system(buf);
			pinfo->mtu = gl_myinfo.priv.ppp_config.mtu;
		}else if(atoi(info->mtu) > 0){
			LOG_IN("%s get mtu %s", info->ifname, info->mtu);
			sprintf(buf, "ifconfig %s mtu %s", info->ifname, info->mtu);
			system(buf);
			pinfo->mtu = atoi(info->mtu);
		}else{
			pinfo->mtu = 1500;
			sprintf(buf, "ifconfig %s mtu %d", info->ifname, pinfo->mtu);
			system(buf);
		}

		pinfo->status = VIF_UP;
		gl_myinfo.svcinfo.uptime = get_uptime();
		//gl_ppp_redial_cnt = 0;

		evutil_timerclear(&tv);
		if(modem_config->dualsim)
			tv.tv_sec = dualsim_info->uptime;
		else
			tv.tv_sec = REDIAL_PPP_UP_TIME;
		event_add(&gl_ev_ppp_up_tmr, &tv);

		if(gl_redial_state == REDIAL_STATE_CONNECTED){
			if(get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI){
				ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, pinfo, sizeof(VIF_INFO));
			}
			my_restart_timer(0);
		}
		break;
	case DHCPC_EVT_LEASEFAIL:
		  if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_PLS8){
			  snprintf(buf, sizeof(buf),  "AT^SWWAN=1,%d\r\n", ih_key_support("FS39") ? get_pdp_context_id(WWAN_ID1) : 1);
			  ret = send_at_cmd_sync(gl_commport, buf, NULL, 0, 0);
			  if(ret<0) {
				  LOG_ER("AT DHCPC %s is not capable to connect",gl_commport);
			  }
		  }
		  gl_myinfo.svcinfo.uptime = 0;
	case DHCPC_EVT_NAK:
		break;
	default:
		break;
	}
}

#define ADDR_OPT		0x0001
#define DNS_OPT     	0x0002
#define RA_DNS_OPT      0x0004
#define RA_MTU_OPT      0x0008
#define HOPLIMIT_OPT    0x0010
#define RA_ROUTE_OPT	0x0020

static int need_update(ODHCP6C_EVT_INFO *info, VIF_INFO *redl_iface, int num_need_set){
	struct in6_addr address, dns;
	time_t preferred_time = 0;
	int prefix_len = 0;
	int ret = 0;
	int i=0,j=0;
	BOOL addr_find_flag = FALSE;
	BOOL mtu_find_flag = FALSE;
	BOOL dns_find_flag = FALSE;
	BOOL ra_dns_find_flag = FALSE;
	BOOL ra_route_find_flag = FALSE;

	for(i=0;i<num_need_set;i++){
		addr_find_flag = FALSE;
		mtu_find_flag = FALSE;
		dns_find_flag = FALSE;
		ra_dns_find_flag = FALSE;	
		ra_route_find_flag = FALSE;

		if(!(ret & ADDR_OPT)){
			preferred_time = 0;
			prefix_len = 0;
			memset(&address, 0, sizeof(address));
			if(info->address[i].addr_or_prefix[0]){
				inet_pton(AF_INET6, info->address[i].addr_or_prefix, &address);
				prefix_len = atoi(info->address[i].prefix_len);
			}else if(info->ra_address[i].addr_or_prefix[0]){
				inet_pton(AF_INET6, info->ra_address[i].addr_or_prefix, &address);
				prefix_len = atoi(info->ra_address[i].prefix_len);		
			}else{

			}
			if(IN6_IS_ADDR_UNSPECIFIED(&address)){
				addr_find_flag = TRUE;
			}else{
				if (info->address[i].preferred_time){
					preferred_time = atoi(info->address[i].preferred_time);
				}
				if (info->ra_address[i].preferred_time){
					preferred_time = atoi(info->ra_address[i].preferred_time);
				}
				for(j = 0; j <num_need_set; j++){
					if (IN6_ARE_ADDR_EQUAL(&(redl_iface->ip6_addr.dynamic_ifaddr[j].ip6), &address) && 
								redl_iface->ip6_addr.dynamic_ifaddr[j].prefix_len == prefix_len){
						redl_iface->ip6_addr.dynamic_ifaddr[j].preferred_lifetime = preferred_time;
						redl_iface->ip6_addr.dynamic_ifaddr[j].preferred_got_time = get_uptime();
						addr_find_flag = TRUE;
						break;
					}
				}
			}
		}
		if(!(ret & RA_MTU_OPT)){
			if(IN6_IS_ADDR_UNSPECIFIED(&(redl_iface->ip6_addr.dynamic_ifaddr[i].ip6))){
				mtu_find_flag = TRUE;
			}else{
				if(redl_iface->ip6_addr.dynamic_ifaddr[i].mtu == atoi(info->ra_mtu)){
					mtu_find_flag = TRUE;
				}
			}
		}
		if(!(ret & DNS_OPT)){
			if(1 != inet_pton(AF_INET6, info->rdnss[i], &dns)){
				memset(&dns, 0, sizeof(dns));
			}
			if(IN6_IS_ADDR_UNSPECIFIED(&dns)){
				dns_find_flag = TRUE;
			}else{
				for(j=0; j<num_need_set; j++){
					if(IN6_ARE_ADDR_EQUAL(&(redl_iface->ip6_addr.dhcp_dns[j]), &dns)){
						dns_find_flag = TRUE;
						break;
					}
				}
			}
		}
		if(!(ret & RA_DNS_OPT)){
			if(1 != inet_pton(AF_INET6, info->ra_dns[i], &dns)){
				memset(&dns, 0, sizeof(dns));
			}
			if(IN6_IS_ADDR_UNSPECIFIED(&dns)){
				ra_dns_find_flag = TRUE;
			}else{
				for(j=0; j<num_need_set; j++){
					if(IN6_ARE_ADDR_EQUAL(&(redl_iface->ip6_addr.ra_dns[j]), &dns)){
						ra_dns_find_flag = TRUE;
						break;
					}
				}
			}
		}

		if(!(ret & RA_ROUTE_OPT)){
			struct in6_addr prefix;
			struct in6_addr gateway;
			int prefix_len;
			memset(&prefix, 0, sizeof(prefix));
			memset(&gateway, 0, sizeof(gateway));
			prefix_len = 0;
			inet_pton(AF_INET6, info->ra_routes[i].prefix, &prefix);
			inet_pton(AF_INET6, info->ra_routes[i].gateway, &gateway);
			prefix_len = atoi(info->ra_routes[i].prefix_len);
			if(IN6_IS_ADDR_UNSPECIFIED(&prefix) && IN6_IS_ADDR_UNSPECIFIED(&gateway)){
				ra_route_find_flag = TRUE;
			}else{
				for(j=0;j<num_need_set; j++){
					if(IN6_ARE_ADDR_EQUAL(&(redl_iface->ip6_addr.ra_route[j].prefix), &prefix) &&
								IN6_ARE_ADDR_EQUAL(&(redl_iface->ip6_addr.ra_route[j].gateway), &gateway) &&
								redl_iface->ip6_addr.ra_route[j].prefix_len == prefix_len){
						ra_route_find_flag = TRUE;
						break;
					}
				}
			}
		}

		if(!(addr_find_flag && mtu_find_flag && dns_find_flag && ra_dns_find_flag && ra_route_find_flag)){
			if(!addr_find_flag){
				ret |= ADDR_OPT;
			}
			if(!mtu_find_flag){
				ret |= RA_MTU_OPT;
			}
			if(!dns_find_flag){
				ret |= DNS_OPT;
			}
			if(!ra_dns_find_flag){
				ret |= RA_DNS_OPT;
			}
			if(!ra_route_find_flag){
				ret |= RA_ROUTE_OPT;
			}
		}
	}
	for(i = 0; i <num_need_set; i++){
		addr_find_flag = FALSE;
		mtu_find_flag = FALSE;
		dns_find_flag = FALSE;
		ra_dns_find_flag = FALSE;
		ra_route_find_flag = FALSE;

		if(!(ret & ADDR_OPT)){
			if(IN6_IS_ADDR_UNSPECIFIED(&(redl_iface->ip6_addr.dynamic_ifaddr[i].ip6))){
				addr_find_flag = TRUE;
			}else{		
				for(j=0;j<num_need_set;j++){
					prefix_len = 0;
					memset(&address, 0, sizeof(address));
					if(info->address[j].addr_or_prefix[0]){
						inet_pton(AF_INET6, info->address[j].addr_or_prefix, &address);
						prefix_len = atoi(info->address[j].prefix_len);
					}	
					if(info->ra_address[j].addr_or_prefix[0]){
						inet_pton(AF_INET6, info->ra_address[j].addr_or_prefix, &address);
						prefix_len = atoi(info->ra_address[j].prefix_len);		
					}				

					if (IN6_ARE_ADDR_EQUAL(&(redl_iface->ip6_addr.dynamic_ifaddr[i].ip6), &address) && 
								redl_iface->ip6_addr.dynamic_ifaddr[i].prefix_len == prefix_len){
						addr_find_flag = TRUE;
						break;
					}
				}
			}
		}
		if(!(ret & RA_MTU_OPT)){
			if(IN6_IS_ADDR_UNSPECIFIED(&(redl_iface->ip6_addr.dynamic_ifaddr[i].ip6))){
				mtu_find_flag = TRUE;
			}else{
				if(redl_iface->ip6_addr.dynamic_ifaddr[i].mtu == atoi(info->ra_mtu)){
					mtu_find_flag = TRUE;
				}
			}
		}
		if(!(ret & DNS_OPT)){
			if(IN6_IS_ADDR_UNSPECIFIED(&(redl_iface->ip6_addr.dhcp_dns[i]))){
				dns_find_flag = TRUE;
			}else{		
				for(j=0; j<num_need_set; j++){
					if(1 != inet_pton(AF_INET6, info->rdnss[j], &dns)){
						memset(&dns, 0, sizeof(dns));
					}
					if(IN6_ARE_ADDR_EQUAL(&(redl_iface->ip6_addr.dhcp_dns[i]), &dns)){
						dns_find_flag = TRUE;
						break;
					}
				}
			}
		}
		if(!(ret & RA_DNS_OPT)){
			if(IN6_IS_ADDR_UNSPECIFIED(&(redl_iface->ip6_addr.ra_dns[i]))){
				ra_dns_find_flag = TRUE;	
			}else{
				for(j=0; j<num_need_set; j++){
					if(1 != inet_pton(AF_INET6, info->ra_dns[j], &dns)){
						memset(&dns, 0, sizeof(dns));
					}
					if(IN6_ARE_ADDR_EQUAL(&(redl_iface->ip6_addr.ra_dns[i]), &dns)){
						ra_dns_find_flag = TRUE;
						break;
					}
				}
			}
		}
		if(!(ret & RA_ROUTE_OPT)){
			if(IN6_IS_ADDR_UNSPECIFIED(&(redl_iface->ip6_addr.ra_route[i].prefix)) &&
						IN6_IS_ADDR_UNSPECIFIED(&(redl_iface->ip6_addr.ra_route[i].gateway))){
				ra_route_find_flag = TRUE;
			}else{
				for(j=0; j<num_need_set; j++){
					struct in6_addr prefix;
					struct in6_addr gateway;
					int prefix_len = 0;
					memset(&prefix, 0, sizeof(prefix));
					memset(&gateway, 0, sizeof(gateway));
					inet_pton(AF_INET6, info->ra_routes[j].prefix, &prefix);
					inet_pton(AF_INET6, info->ra_routes[j].gateway, &gateway);
					prefix_len = atoi(info->ra_routes[j].prefix_len);
					if(IN6_ARE_ADDR_EQUAL(&(redl_iface->ip6_addr.ra_route[i].prefix), &prefix) &&
								IN6_ARE_ADDR_EQUAL(&(redl_iface->ip6_addr.ra_route[i].gateway), &gateway) &&
								redl_iface->ip6_addr.ra_route[i].prefix_len == prefix_len){
						ra_route_find_flag = TRUE;
						break;
					}
				}
			}
		}

		if(!(addr_find_flag && mtu_find_flag && dns_find_flag && ra_dns_find_flag && ra_route_find_flag)){
			if(!addr_find_flag){
				ret |= ADDR_OPT;
			}
			if(!mtu_find_flag){
				ret |= RA_MTU_OPT;
			}
			if(!dns_find_flag){
				ret |= DNS_OPT;
			}
			if(!ra_dns_find_flag){
				ret |= RA_DNS_OPT;
			}
			if(!ra_route_find_flag){
				ret |= RA_ROUTE_OPT;
			}	
		}
	}
	return ret;
}

static void del_dhcpv6_info2inf(VIF_INFO *redl_iface, int num_need_set, int del_flag){
	int i = 0;
	if(ADDR_OPT & del_flag){
		for(i=0;i<num_need_set;i++){
			memset(&(redl_iface->ip6_addr.dynamic_ifaddr[i].ip6), 0,sizeof(redl_iface->ip6_addr.dynamic_ifaddr[i].ip6));
			memset(&(redl_iface->ip6_addr.dynamic_ifaddr[i].gateway), 0,sizeof(redl_iface->ip6_addr.dynamic_ifaddr[i].gateway));
			redl_iface->ip6_addr.dynamic_ifaddr[i].prefix_len = 0;
			redl_iface->ip6_addr.dynamic_ifaddr[i].valid_lifetime = 0;
			redl_iface->ip6_addr.dynamic_ifaddr[i].preferred_lifetime = 0;
			redl_iface->ip6_addr.dynamic_ifaddr[i].valid_got_time = 0;
			redl_iface->ip6_addr.dynamic_ifaddr[i].preferred_got_time = 0;
		}
	}
	if(DNS_OPT & del_flag){
		memset(redl_iface->ip6_addr.dhcp_dns, 0, sizeof(redl_iface->ip6_addr.dhcp_dns));
	}
	if(RA_DNS_OPT & del_flag){
		memset(redl_iface->ip6_addr.ra_dns, 0, sizeof(redl_iface->ip6_addr.ra_dns));
	}
	if(RA_ROUTE_OPT & del_flag){
		memset(redl_iface->ip6_addr.ra_route, 0, sizeof(redl_iface->ip6_addr.ra_route));
	}

	if(RA_MTU_OPT & del_flag){
		for(i=0;i<num_need_set;i++){
			redl_iface->ip6_addr.dynamic_ifaddr[i].mtu = 0;
		}
	}

	if(HOPLIMIT_OPT & del_flag){
		for(i=0;i<num_need_set;i++){
			redl_iface->ip6_addr.dynamic_ifaddr[i].hop_limit = 0;
		}
	}
	return;
}

static int add_dhcpv6_info2inf(ODHCP6C_EVT_INFO *info, VIF_INFO *redl_iface, int num_need_set, PPP_CONFIG config, int save_flag){
	struct in6_addr address, dns, gateway, s_address;
	time_t preferred_time = 0;
	int prefix_len = 0;
	int metric = 0;
	int i = 0;

	for(i=0;i<num_need_set;i++){
		prefix_len = 0;
		preferred_time = 0;
		metric = 0;
		memset(&address, 0, sizeof(address));	
		memset(&dns, 0, sizeof(dns));
		memset(&gateway, 0, sizeof(gateway));
		if(ADDR_OPT & save_flag){
			if(info->address[i].addr_or_prefix[0]){
				inet_pton(AF_INET6, info->address[i].addr_or_prefix, &address);

				if (gl_myinfo.priv.ppp_config.prefix_share){
					prefix_len = 128;
				}else if (0 == prefix_len){
					prefix_len = atoi(info->address[i].prefix_len);
				}

				if(config.static_if_id_enable) {
					inet_pton(AF_INET6, info->address[i].addr_or_prefix, &s_address);
					if (add_ipv6_str_interface_id_to_address(&s_address, config.static_interface_id)) {
						LOG_ER("failed to add interface id :%s to ipv6 address", config.static_interface_id);
						//return ERR_INVAL;
					} 

					
				}
			}
			if(info->ra_address[i].addr_or_prefix[0]){
				inet_pton(AF_INET6, info->ra_address[i].addr_or_prefix, &address);

				if (gl_myinfo.priv.ppp_config.prefix_share){
					prefix_len = 128;
				}else if (0 == prefix_len){
					prefix_len = atoi(info->ra_address[i].prefix_len);		
				}

				if(config.static_if_id_enable) {
					inet_pton(AF_INET6, info->ra_address[i].addr_or_prefix, &s_address);

					if (add_ipv6_str_interface_id_to_address(&s_address, config.static_interface_id)) {
						LOG_ER("failed to add interface id :%s to ipv6 address", config.static_interface_id);
						//return ERR_INVAL;
					} 
				}
			}

			if(config.static_if_id_enable && (!IN6_IS_ADDR_UNSPECIFIED(&s_address)) && (prefix_len != 0)){
				iface_set_dynamic_ip6_addr(info->ifname, &(redl_iface->ip6_addr), s_address, prefix_len);
			}

			if(!IN6_IS_ADDR_UNSPECIFIED(&address)){
				if (info->address[i].preferred_time){
					preferred_time = atoi(info->address[i].preferred_time);
				}
				if (info->ra_address[i].preferred_time){
					preferred_time = atoi(info->ra_address[i].preferred_time);
				}		
				if(preferred_time){
					redl_iface->ip6_addr.dynamic_ifaddr[i].preferred_lifetime = preferred_time;
					redl_iface->ip6_addr.dynamic_ifaddr[i].preferred_got_time = get_uptime();
				}		
				iface_set_dynamic_ip6_addr(info->ifname, &(redl_iface->ip6_addr), address, prefix_len);
			}

		}
		if(DNS_OPT & save_flag){
			inet_pton(AF_INET6, info->rdnss[i], &dns);
			redl_iface->ip6_addr.dhcp_dns[i] = dns;
		}
		if(RA_DNS_OPT & save_flag){
			inet_pton(AF_INET6, info->ra_dns[i], &dns);
			redl_iface->ip6_addr.ra_dns[i] = dns;
		}	
		if(RA_ROUTE_OPT & save_flag){
			inet_pton(AF_INET6, info->ra_routes[i].prefix, &address);
			inet_pton(AF_INET6, info->ra_routes[i].gateway, &gateway);
			if(!IN6_IS_ADDR_UNSPECIFIED(&gateway)){
				prefix_len = atoi(info->ra_routes[i].prefix_len);
				preferred_time = atoi(info->ra_routes[i].preferred_time);
				metric = atoi(info->ra_routes[i].metric);
				redl_iface->ip6_addr.ra_route[i].prefix = address;
				redl_iface->ip6_addr.ra_route[i].gateway = gateway;
				redl_iface->ip6_addr.ra_route[i].prefix_len = prefix_len;
				redl_iface->ip6_addr.ra_route[i].metric = metric;
				/*if(IN6_IS_ADDR_UNSPECIFIED(&address) && (0 == prefix_len)){ pick up default gateway 
				  if(metric < redl_iface->ip6_addr.ra_route[i].ra_route_df_gw.metric){
				  redl_iface->ip6_addr.ra_route[i].ra_route_df_gw.prefix = address;		
				  redl_iface->ip6_addr.ra_route[i].ra_route_df_gw.gateway = gateway;
				  redl_iface->ip6_addr.ra_route[i].ra_route_df_gw.prefix_len = prefix_len;
				  redl_iface->ip6_addr.ra_route[i].ra_route_df_gw.preferred_time = preferred_time;
				  redl_iface->ip6_addr.ra_route[i].ra_route_df_gw.metric = metric;
				  }*/
			}
		}
		if(RA_MTU_OPT & save_flag){
			redl_iface->ip6_addr.dynamic_ifaddr[i].mtu = atoi(info->ra_mtu);
		}
	}
	

	return 0; 
}

void redl_iface_clear_addresses(char *ifname, VIF_INFO *pinfo) 
{
	if (!ifname || !pinfo) {
		return;
	}

	iface_clear_all_ip6_addr(ifname, &(pinfo->ip6_addr));
	flush_ipv6_scope_addr(ifname, IPV6_ADDR_GLOBAL);
	iface_clear_all_ip4_addr(ifname, pinfo);
	flush_ip4_addr(ifname);
	return;
}
	
void redl_iface_update_state(VIF_INFO* redl_iface){
	char  prefix_str[48] = {0};
	struct timeval tv;
	int ret;
	struct in6_addr	npd_prefix;
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	DUALSIM_INFO *dualsim_info = &(gl_myinfo.priv.dualsim_info);

	if(NULL == redl_iface){
		return;
	}
	if (gl_myinfo.priv.ppp_config.ndp_enable) {
		//inet_ntop(AF_INET6, &redl_iface->ip6_addr.dynamic_ifaddr[0].ip6, prefix_str, sizeof(prefix_str));

		in6_addr_2prefix(redl_iface->ip6_addr.dynamic_ifaddr[0].ip6, 64, &npd_prefix);
		inet_ntop(AF_INET6, &npd_prefix, prefix_str, sizeof(prefix_str));
		if (prefix_str[0]) {
			if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PLS8){
				start_npd6(&gl_npd_pid, prefix_str, USB0_WWAN, gl_myinfo.priv.ppp_debug);
			}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC25 && ih_key_support("FQ78")){
				start_npd6(&gl_npd_pid, prefix_str, WWAN0_WWAN, gl_myinfo.priv.ppp_debug);
			}
		}else {
			LOG_ER("get prefix error!");
		}
	}else {
		if (gl_npd_pid > 0) {
			stop_npd6(&gl_npd_pid);
		}
	}

	if(!iface_has_valid_ip6_address(&redl_iface->ip6_addr)){
		//redl_iface->status = VIF_DOWN;
		flush_ip6_addr(redl_iface->iface);
                return;
	}else{
		redl_iface->status = VIF_UP;
		flush_ip6_addr(redl_iface->iface);
		iface_add_ip6_addr(redl_iface->iface, &(redl_iface->ip6_addr));
	}

	gl_myinfo.svcinfo.uptime = get_uptime();

	ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)redl_iface, sizeof(VIF_INFO));
	if(ret) {
		LOG_ER("broadcast link change msg is not ok.");
	}
	clear_conntrack();
	evutil_timerclear(&tv);
	if(modem_config->dualsim)
	  tv.tv_sec = dualsim_info->uptime;
	else
	  tv.tv_sec = REDIAL_PPP_UP_TIME;
	event_add(&gl_ev_ppp_up_tmr, &tv);
	return;
}
/*
States:
 * started		The DHCPv6 client has been started
 * bound		A suitable server was found and addresses or prefixes acquired		
 * informed		A stateless information request returned updated information
 * updated		Updated information was received from the DHCPv6 server
 * ra-updated	Updated information was received from via Router Advertisement
 * rebound		The DHCPv6 client switched to another server
 * unbound		The DHCPv6 client lost all DHCPv6 servers and will restart
 * stopped		The DHCPv6 client has been stopped
 */
static char odhcp6c_evt_str[][16] = {"started", "bound", "informed", "updated", "ra_updated", "rebound", "unbound", "stoped"};
static void wwan0_odhcp6c_event_handle(ODHCP6C_EVT_INFO *info)
{
	int num_need_set = 0;
	int opt_flag = 0;
	int i = 0;
	BOOL  find = FALSE;
	VIF_INFO *redl_iface = NULL;
	PPP_CONFIG *config = NULL;
	struct in6_addr address;

	redl_iface = get_svc_vif_info_by_name(info->ifname); 
	config = get_modem_iface_config_by_name(info->ifname);

	if (!info || !redl_iface){
		return;
	}
	
	if (config->static_iapd_prefix_strict) {
		if ((config->static_iapd_prefix_len > 0) && info->pd_prefix[0].addr_or_prefix[0]) {
			for (i = 0; i < DHCPV6C_LIST_NUM; i++) {
				if (!info->pd_prefix[i].addr_or_prefix[0]) {
					continue;
				}

				inet_pton(AF_INET6, info->pd_prefix[i].addr_or_prefix, &address);
				if (memcmp(&config->static_iapd_prefix, &address, sizeof(config->static_iapd_prefix)) == 0) {
					find = TRUE;
					break;
				}
			}
			
			if (!find) {
				LOG_WA ("We got a wrong delagated prefix, discard it");
				return;			
			}

			LOG_IN("We got a right delagated prefix");
		}
	}

	vif_get_sys_name(&redl_iface->if_info, redl_iface->iface);

	num_need_set = CFG_PER_ADDR < DHCPV6C_LIST_NUM ? CFG_PER_ADDR:DHCPV6C_LIST_NUM;
	switch(info->odhcp6c_evt){
		case ODHCP6C_EVT_STARTED:
			opt_flag = ADDR_OPT|DNS_OPT|RA_DNS_OPT|RA_MTU_OPT|RA_ROUTE_OPT;
			del_dhcpv6_info2inf(redl_iface, num_need_set, opt_flag);
		    //	iface_down_up(redl_iface->iface);
			break;
		case ODHCP6C_EVT_REBOUND:	/* switch to another dhcp server  */
		case ODHCP6C_EVT_BOUND:
			opt_flag = ADDR_OPT|DNS_OPT|RA_DNS_OPT|RA_MTU_OPT|RA_ROUTE_OPT;
			if(opt_flag & RA_ROUTE_OPT){/* do route del option before l3 route del */
				//linux_ipv6_if_route_opt(redl_iface->iface, &redl_iface->ip6_addr, num_need_set, FALSE);
			}
			del_dhcpv6_info2inf(redl_iface, num_need_set, opt_flag);
			//redl_iface_update_state(redl_iface);
			add_dhcpv6_info2inf(info ,redl_iface, num_need_set, *config, opt_flag);
			redl_iface_update_state(redl_iface);

			if(opt_flag & RA_ROUTE_OPT){/* do route add option after address added*/
				//linux_ipv6_if_route_opt(redl_iface->iface, &redl_iface->ip6_addr, num_need_set, TRUE);
			}
			break;
		case ODHCP6C_EVT_INFORMED:
			opt_flag = need_update(info ,redl_iface, num_need_set);
			if(opt_flag){
				if(opt_flag & RA_ROUTE_OPT){/* do route del option before l3 route del */
					//linux_ipv6_if_route_opt(redl_iface->iface, &redl_iface->ip6_addr, num_need_set, FALSE);
				}
				del_dhcpv6_info2inf(redl_iface, num_need_set, opt_flag);			
				if(opt_flag & ADDR_OPT){
					//redl_iface_update_state(redl_iface);
				}
				add_dhcpv6_info2inf(info ,redl_iface, num_need_set, *config, opt_flag);
				if(opt_flag & ADDR_OPT){
					redl_iface_update_state(redl_iface);
				}
				if(opt_flag & RA_ROUTE_OPT){/* do route add option after address added*/
					//linux_ipv6_if_route_opt(redl_iface->iface, &redl_iface->ip6_addr, num_need_set, TRUE);
				}			
			}		
			break;
		case ODHCP6C_EVT_UPDATED:
			opt_flag = need_update(info ,redl_iface, num_need_set);
			if(opt_flag){
				if(opt_flag & RA_ROUTE_OPT){/* do route del option before l3 route del */
					//linux_ipv6_if_route_opt(redl_iface->iface, &redl_iface->ip6_addr, num_need_set, FALSE);
				}			
			        del_dhcpv6_info2inf(redl_iface, num_need_set, opt_flag);			
				if(opt_flag & ADDR_OPT){
					//redl_iface_update_state(redl_iface);
				}
				add_dhcpv6_info2inf(info ,redl_iface, num_need_set, *config, opt_flag);
				if(opt_flag & ADDR_OPT){
					redl_iface_update_state(redl_iface);
				}	
				if(opt_flag & RA_ROUTE_OPT){/* do route add option after address added*/
					//linux_ipv6_if_route_opt(redl_iface->iface, &redl_iface->ip6_addr, num_need_set, TRUE);
				}			
			}
			break;
		case ODHCP6C_EVT_RA_UPDATED:
			opt_flag = need_update(info ,redl_iface, num_need_set);
			if(opt_flag){
				if(opt_flag & RA_ROUTE_OPT){/* do route del option before l3 route del */
					//linux_ipv6_if_route_opt(redl_iface->iface, &redl_iface->ip6_addr, num_need_set, FALSE);
				}			
				del_dhcpv6_info2inf(redl_iface, num_need_set, opt_flag);			
				if(opt_flag & ADDR_OPT){
					//redl_iface_update_state(redl_iface);
				}
				add_dhcpv6_info2inf(info ,redl_iface, num_need_set, *config, opt_flag);
				if(opt_flag & ADDR_OPT){
					redl_iface_update_state(redl_iface);
				}
				if(opt_flag & RA_ROUTE_OPT){/* do route add option after address added*/
					//linux_ipv6_if_route_opt(redl_iface->iface, &redl_iface->ip6_addr, num_need_set, TRUE);
				}			
			}
			break;
		case ODHCP6C_EVT_UNBOUND:
		case ODHCP6C_EVT_STOPED:
			opt_flag = ADDR_OPT|RA_DNS_OPT|DNS_OPT|DNS_OPT|RA_MTU_OPT|RA_ROUTE_OPT;
			if(opt_flag & RA_ROUTE_OPT){/* do route del option before l3 route del */
				//linux_ipv6_if_route_opt(redl_iface->iface, &redl_iface->ip6_addr, num_need_set, FALSE);
			}		
			del_dhcpv6_info2inf(redl_iface, num_need_set, opt_flag);
			break;
		default:/*error*/
			break;
	}
	LOG_IN("Dhcpv6 Client interface %s event %s opt %x", info->ifname, odhcp6c_evt_str[info->odhcp6c_evt], opt_flag);
	return;
}


#define MAX_SAVED_SMS	20
SMS g_saved_unsent_sms[MAX_SAVED_SMS] = {0};

int save_unsent_sms(SMS *psms)
{
	int i = 0;

	if(!psms){
		return -1;
	}

	for(i = 0; i < MAX_SAVED_SMS; i++){
		if(!g_saved_unsent_sms[i].data_len){
			memcpy(&g_saved_unsent_sms[i], psms, sizeof(SMS));
			LOG_IN("save_unsent_sms at %d", i);
			return i;
		}
	}

	return -1;
}

int purge_saved_sms(int idx)
{
	if(idx < 0 || idx >= MAX_SAVED_SMS){
		return -1;
	}

	memset(&g_saved_unsent_sms[idx], 0, sizeof(SMS));

	return 0;
}

int resend_unsent_sms(void)
{
	int i = 0;
	int cnt = 0;
	int ret = 0;
	int sent_ok = 0;
	int sent_cnt = 0;
	SMS sms;
	SMS_CONFIG *sms_config = &(gl_myinfo.priv.sms_config);

	if(gl_redial_state != REDIAL_STATE_CONNECTED){
		return 0;
	}

	if(!gl_myinfo.priv.sms_config.enable) {
		return 0;
	}

	sms_init(gl_commport, SMS_MODE_TEXT);

	for(i = 0; i < MAX_SAVED_SMS; i++){
		if(!g_saved_unsent_sms[i].data_len){
			continue;
		}

		sent_ok = 0;
		memcpy(&sms, &g_saved_unsent_sms[i], sizeof(SMS));
		for(cnt=0; cnt<SMS_ACL_MAX; cnt++) {
			if(sms_config->acl[cnt].id) {
				strcpy(sms.phone, sms_config->acl[cnt].phone);
				LOG_IN("resend a sms to %s", sms.phone);
				ret = sms_handle_outsms(gl_commport, SMS_MODE_TEXT, &sms);		
				if(ret == 0){
					sent_ok++;
				}
			}
		}

		if(sent_ok){
			memset(&g_saved_unsent_sms[i], 0, sizeof(SMS));
			sent_cnt++;
		}
	}

	if(sent_cnt){
		LOG_IN("resent %d saved sms", sent_cnt);
	}

	return sent_cnt;
}

int handle_sim_data_usage(MSG_DATA_USAGE *msg)
{
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	MODEM_INFO *modem_info = &(gl_myinfo.svcinfo.modem_info);
	int cur_sim = modem_info->current_sim;

	if(!msg || ((msg->slot != SIM1) && msg->slot != SIM2)){
		return -1;
	}
	
	sim_traffic_status_update(msg);

	if(msg->type == MONTHLY_DATA_USAGE && msg->slot == cur_sim){
		if((msg->action == STOP_FORWARD || msg->action == SWITCH_SIM)
			&& (msg->status == DATA_USAGE_EXCEED)){

			if(modem_config->dualsim && !gl_sim_switch_flag && !check_sim_status(SIM1+SIM2-cur_sim)){
				LOG_DB("do switch to sim%d", SIM1+SIM2-msg->slot);
				gl_sim_switch_flag = TRUE;
				do_redial_procedure();
			}
		}
	}

	memcpy(&sim_data_usage[msg->slot-1], msg, sizeof(MSG_DATA_USAGE));
	show_data_usage(msg);

	return 0;
}

/*
 * handle IPC msg with ih_updown, firewall, routed, ...
 *
 * @param peer_svc_id		cmd from which service
 * @param msg				received IPC msg
 * @param rsp_reqd			whether to send response
 *
 * @return ERR_OK for ok, others for error
 */
static int32 my_msg_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen)   
{
	uns32 peer_svc_id = msg_hdr->svc_id;
	BOOL need_rsp = IPC_TEST_FLAG(msg_hdr->flag,IPC_MSG_FLAG_NEED_RSP);
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	VIF_INFO *pinfo_2nd = &(gl_myinfo.svcinfo.secondary_vif_info);
	struct timeval tv;
	DUALSIM_INFO *dualsim_info = &(gl_myinfo.priv.dualsim_info);
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	SMS_CONFIG *sms_config = &(gl_myinfo.priv.sms_config);
	BACKUP_SVCINFO *backup;
	VIF_STATUS vif_status;
    SDWAN_STATUS_INFO *sdwan_status = NULL;
	SMS *psms;
	int ret;
	DHCPC_EVT_INFO *info;
	ODHCP6C_EVT_INFO *odhcp6c_info = NULL;
	int cnt = 0;
	int sms_saved = 0;
	int sms_sent_ok = 0;
	int action = 0;
	VIF_INFO vif_tmp;


   	MYTRACE_ENTER();

	switch(msg_hdr->type) {
	case IPC_MSG_UPDOWN_PPP_UP:
		LOG_DB("pppd up...");
		gl_myinfo.svcinfo.uptime = get_uptime();
		memcpy((char *)pinfo, msg, sizeof(VIF_INFO));
        //adjust tx queue
		eval("ip", "link", "set", "dev", pinfo->iface, "txqueuelen", "64");
		//add ppp up timer
		evutil_timerclear(&tv);
		if(modem_config->dualsim)
			tv.tv_sec = dualsim_info->uptime;
		else
			tv.tv_sec = REDIAL_PPP_UP_TIME;
		event_add(&gl_ev_ppp_up_tmr, &tv);
	
		break;
	case IPC_MSG_UPDOWN_PPP_DOWN:
		LOG_DB("pppd down...");
		gl_myinfo.svcinfo.uptime = 0;
		memcpy((char *)pinfo, msg, sizeof(VIF_INFO));
		//TODO: ATH
		ret = send_at_cmd_sync(gl_commport, "ATH\r\n", NULL, 0, 0);
		if(ret<0) {
			send_at_cmd_sync(gl_commport, "ATH\r\n", NULL, 0, 0);
			//send_at_cmd_sync(gl_commport, "AT+CHUP\r\n", NULL, 0, 0);
		}
		//TODO: call timer
		break;
	case IPC_MSG_VIF_LINK_CHANGE_REQ:
		LOG_DB("service %d requests vif info", peer_svc_id);
		if(pinfo->status!=VIF_DESTROY){
			memcpy(&vif_tmp, pinfo, sizeof(VIF_INFO));
			
			vif_get_status(&(pinfo->if_info), &vif_status);
			if(VIF_IS_LINK(vif_status) && VIF_IS_LINK(pinfo->status)){
				vif_tmp.status = vif_status;
			}

			ih_ipc_send_response(peer_svc_id, IPC_MSG_VIF_LINK_CHANGE, &vif_tmp, sizeof(VIF_INFO));
		}

		if(pinfo_2nd->status!=VIF_DESTROY && gl_dual_wwan_enable){
			memcpy(&vif_tmp, pinfo_2nd, sizeof(VIF_INFO));
			
			vif_get_status(&(pinfo_2nd->if_info), &vif_status);
			if(VIF_IS_LINK(vif_status) && VIF_IS_LINK(pinfo_2nd->status)){
				vif_tmp.status = vif_status;
			}

			ih_ipc_send_response(peer_svc_id, IPC_MSG_VIF_LINK_CHANGE, &vif_tmp, sizeof(VIF_INFO));
		}
		break;
	case IPC_MSG_NETWATCHER_LINKDOWN:
		LOG_DB("rcved linkdown MSG from service netwatcher");
		if(SUB_DAEMON_RUNNING) {
			stop_child_by_sub_daemon();

			sleep(3);
			modem_force_ps_reattch();
		}
		break;
	case IPC_MSG_REDIAL_SVCINFO_REQ:
		LOG_DB("service %d requests svc info", peer_svc_id);
		ih_ipc_send_response(peer_svc_id, IPC_MSG_REDIAL_SVCINFO, &gl_myinfo, sizeof(MY_REDIAL_INFO));
		break;
	case IPC_MSG_REDIAL_TIME_SYNC_REQ:
		action = *((int*)msg);
		if(action == START_REDIAL_PENDING_TASK){
			start_redial_pending_task(REDIAL_PENDING_TASK_UPDATE_TIME);
		}else{
			stop_redial_pending_task(REDIAL_PENDING_TASK_UPDATE_TIME);
		}
		break;
	case IPC_MSG_REDIAL_SUSPEND_REQ:
		action = *((BOOL*)msg);
		gl_force_suspend = FALSE;
		if(action == TRUE){
			gl_force_suspend = TRUE;
			change_redial_state(REDIAL_STATE_SUSPEND);
		}else if(gl_redial_state == REDIAL_STATE_SUSPEND){
			change_redial_state(REDIAL_STATE_MODEM_STARTUP);
			if(send_at_cmd_sync(gl_commport, "AT\r\n", NULL, 0, 0) == ERR_OK){
				my_restart_timer(1);
			}else{
				my_restart_timer(30);
			}
		}
		break;
	case IPC_MSG_BACKUPD_SVCINFO:
		backup = (BACKUP_SVCINFO *) msg;
		LOG_DB("receive backup service info: type %d port %d", backup->if_info.type, backup->if_info.port);
		if(backup->if_info.type == IF_TYPE_CELLULAR) {
			switch(backup->status) {
			case BACKUP_UP:
				gl_myinfo.priv.backup_action = BACKUP_UP;
				break;
			case BACKUP_DOWN:
				gl_myinfo.priv.backup_action = BACKUP_DOWN;
				break;
			case BACKUP_ENABLE:
				gl_myinfo.priv.backup = TRUE;
				gl_myinfo.priv.backup_action = BACKUP_DOWN;
				break;
			case BACKUP_DISABLE:
			default:
				gl_myinfo.priv.backup = FALSE;
				gl_myinfo.priv.backup_action = 0;
				break;
			}
			if(SUB_DAEMON_RUNNING) {
				stop_child_by_sub_daemon();
			}
		}
		break;
	case IPC_MSG_SDWAN_STATUS_INFO:
		sdwan_status = (SDWAN_STATUS_INFO *) msg;
		LOG_DB("receive sdwan service info: type %d state %d", sdwan_status->svc_type, sdwan_status->state);
        if (sdwan_status->svc_type != SDWAN_SVC_IF) {
            break;
        }

		if(sdwan_status->if_status.if_info.type == IF_TYPE_CELLULAR) {
			switch(sdwan_status->state) {
			case SDWAN_SWITCH:
				if(modem_config->dualsim) {
					gl_sim_switch_flag = TRUE;
					LOG_IN("switch cellular SIM card by sdwan.");
				}

			case SDWAN_RESTART:
				LOG_IN("restart cellular by sdwan.");
				do_redial_procedure();
				break;
			default:
				break;
			}
		}
		break;
	case IPC_MSG_DATA_USAGE:
		LOG_DB("received data usage info form service %d", peer_svc_id);
		handle_sim_data_usage((MSG_DATA_USAGE*)msg);
		break;
	case IPC_MSG_SMS_SEND:
		//send sms msg
		if(gl_myinfo.priv.sms_config.enable) {
			psms = (SMS *) msg;
			if(strncmp(psms->phone, "01010101010", 11) == 0){
				for(cnt=0; cnt<SMS_ACL_MAX; cnt++) {
					if(sms_config->acl[cnt].id && sms_config->acl[cnt].io_msg_enable) {
						strcpy(psms->phone, sms_config->acl[cnt].phone);
						LOG_IN("send a sms to %s", psms->phone);
						sms_handle_outsms(gl_commport, gl_myinfo.priv.sms_config.mode, psms);		
					}
				}
			}else if(strncmp(psms->phone, "88888888888", 11) == 0){

				resend_unsent_sms();
				sms_saved = save_unsent_sms(psms);

				if(gl_redial_state == REDIAL_STATE_CONNECTED){
					sms_init(gl_commport, SMS_MODE_TEXT);
					for(cnt=0; cnt<SMS_ACL_MAX; cnt++) {
						if(sms_config->acl[cnt].id) {
							strcpy(psms->phone, sms_config->acl[cnt].phone);
							LOG_IN("send sms to %s", psms->phone);
							ret = sms_handle_outsms(gl_commport, SMS_MODE_TEXT, psms);		
							if(ret == 0){
								sms_sent_ok++;
							}
						}
					}

					if(sms_sent_ok && sms_saved >= 0){
						purge_saved_sms(sms_saved);
					}
				}
			}else{
				LOG_IN("send a sms to %s", psms->phone);
				sms_handle_outsms(gl_commport, gl_myinfo.priv.sms_config.mode, psms);		
			}
		}
		break;
	case IPC_MSG_PYSMS_SEND:
		//send sms msg
		if(gl_myinfo.priv.sms_config.enable) {
			psms = (SMS *) msg;
			LOG_IN("send a python sms to %s， sms len %d", psms->phone, psms->data_len);
			pysms_handle_outsms(gl_commport, psms);		
		}
		break;
	case IPC_MSG_UPDOWN_DHCPC_EVENT:
	  	info = (DHCPC_EVT_INFO *)msg;
		wwan0_dhcpc_event_handle(info);
		break;
	case IPC_MSG_UPDOWN_ODHCP6C_EVENT:
	  	odhcp6c_info = (ODHCP6C_EVT_INFO *)msg;
		wwan0_odhcp6c_event_handle(odhcp6c_info);
		break;
	case IPC_MSG_MIPC_WWAN_EVENT:
		mipc_wwan_event_handle((MIPC_WWAN_EVT_INFO *)msg);
		break;
	case IPC_MSG_AGENT_PREFIX_USER_NOTIFY:
		if (g_sysinfo.ipv6_enable 
			&& (pinfo->status==VIF_UP || pinfo_2nd->status==VIF_UP)){
			if (SUB_DAEMON_RUNNING){
				stop_child_by_sub_daemon();
			}
		}
		break;
	default:
		LOG_IN("MSG: 0x%x from service %d", msg_hdr->type, peer_svc_id);
		LOG_IN("    Len: %d", msg_hdr->len);
		LOG_IN("    Content: %s", msg);
		if (need_rsp) {
			ih_ipc_send_nak(peer_svc_id);
		}
		break;
	}

	MYTRACE_LEAVE();

	return ERR_OK;
}

/*
static int32 test_msg_handle(IPC_MSG_HDR *msg_hdr,char *msg,uns32 msg_len,char *obuf,uns32 olen)   
{
	uns32 peer_svc_id = msg_hdr->svc_id;

	LOG_IN("MSG: 0x%x from service %d", msg_hdr->type, peer_svc_id);
	LOG_IN("    Len: %d", msg_hdr->len);

	return ERR_OK;
}
*/

//////////////////////////////////////////////////////////////////////////////
// TIMER EVENT
void my_restart_timer(int timeout)
{
	struct timeval tv;

	evutil_timerclear(&tv);
	tv.tv_sec = timeout;
	event_add(&gl_ev_tmr, &tv);
}

static void my_stop_timer(struct event *evt)
{
	if(NULL == evt){
		return;
	}
	
	event_del(evt);
}

/*
 * my timer for check sms
 *
 * @param fd		not used
 * @param event		not used
 * @param args		timer callback data
 */
static void my_sms_timer(int fd, short event, void *arg)
{
	struct timeval tv;

	//receive sms msg
	sms_handle_insms(gl_commport, gl_myinfo.priv.sms_config.mode, gl_myinfo.priv.sms_config.acl);

	if(gl_myinfo.priv.sms_config.interval) {
		evutil_timerclear(&tv);
		tv.tv_sec = gl_myinfo.priv.sms_config.interval;
		event_add(&gl_ev_sms_tmr, &tv);
	}

	resend_unsent_sms();
}

int handle_at_cmd(const char *dev, AT_CMD *pcmd, int verbose)
{
	char buf[1024] = {0};
	int ret = ERR_AT_OK;
	
	if(pcmd) {
		ret = send_at_cmd_sync3(dev, pcmd->atc, buf, sizeof(buf), verbose);
		if(ret==0 && pcmd->at_cmd_handle) {
			pcmd->at_cmd_handle(pcmd, buf, 0, &gl_myinfo.priv.modem_config, &gl_myinfo.svcinfo.modem_info);			
		}
	}
	
	return ret;	
}

int handle_at_cmd_by_id(const char *dev, int cmd, int verbose)
{
	return handle_at_cmd(dev, find_at_cmd(cmd), verbose);
}

/*
 * my timer for check signal
 * note: first we should refresh network info, because if network type changed, related signal info should be cleared.
 *
 * @param fd		not used
 * @param event		not used
 * @param args		timer callback data
 */
static void my_signal_timer(int fd, short event, void *arg)
{
	struct timeval tv;
	char buf[1024] = {0};
	AT_CMD *pcmd; 
	MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	int ret, timeout, sim_idx;
	uint64_t modem_code = get_modem_code(gl_modem_idx);
	BOOL is_cdma = FALSE;

	if(modem_config->debug){
		evutil_timerclear(&tv);
		tv.tv_sec = gl_myinfo.priv.signal_interval;
		event_add(&gl_ev_signal_tmr, &tv);
		return;
	}

	memcpy(&myinfo_tmp, &gl_myinfo, sizeof(MY_REDIAL_INFO));

	LOG_DB("refresh signal.");

	//-------------------------refresh serving cell info------------------------
	if(modem_code==IH_FEATURE_MODEM_EM820W
	    || modem_code==IH_FEATURE_MODEM_MU609
	    || modem_code==IH_FEATURE_MODEM_ME909) {
		
		handle_at_cmd_by_id(gl_commport, AT_CMD_SYSINFOEX, 0);
		//cellid
		pcmd = find_at_cmd(AT_CMD_CGREG);
		if(pcmd) {
			//send cgreg command
			ret = send_at_cmd_sync(gl_commport, pcmd->atc, buf, sizeof(buf), 0);
			if(ret==0) {
				check_modem_cellid(pcmd, buf, 0, NULL, info);			
			}
		}
	} else if(modem_code==IH_FEATURE_MODEM_EM770W) {
		handle_at_cmd_by_id(gl_commport, AT_CMD_SYSINFO, 0);
	} else if(modem_code==IH_FEATURE_MODEM_PHS8 || 
		modem_code==IH_FEATURE_MODEM_PVS8 ||
		modem_code==IH_FEATURE_MODEM_ELS61 ||
		modem_code==IH_FEATURE_MODEM_PLS8) {

		handle_at_cmd_by_id(gl_commport, AT_CMD_SMONI, 0);
	} else if(modem_code==IH_FEATURE_MODEM_U8300
		||modem_code==IH_FEATURE_MODEM_U8300C) {
		handle_at_cmd_by_id(gl_commport, AT_CMD_PSRAT, 0);
	}else if(modem_code==IH_FEATURE_MODEM_MC2716
			|| modem_code==IH_FEATURE_MODEM_MC5635) {

		handle_at_cmd_by_id(gl_commport, AT_CMD_VSYSINFO, 0);
	} else if (modem_code==IH_FEATURE_MODEM_ELS81) {
		
		handle_at_cmd_by_id(gl_commport, AT_CMD_COPS, 0);
		if (info->network==3) { //LTE
			pcmd = find_at_cmd(AT_CMD_CEREG);
		} else {
			pcmd = find_at_cmd(AT_CMD_CREG);
		}

		handle_at_cmd(gl_commport, pcmd, 0);
	} else if (modem_code==IH_FEATURE_MODEM_EC25
			|| modem_code==IH_FEATURE_MODEM_EC20) {
		
		handle_at_cmd_by_id(gl_commport, AT_CMD_QENG_SRVCELL2, 0);
		if (info->network==3) { //LTE
			pcmd = find_at_cmd(AT_CMD_CEREG);
		} else {
			pcmd = find_at_cmd(AT_CMD_CREG);
		}

		handle_at_cmd(gl_commport, pcmd, 0);
	} else if (modem_code==IH_FEATURE_MODEM_RM500
			|| modem_code==IH_FEATURE_MODEM_RM520N
			|| modem_code==IH_FEATURE_MODEM_RM500U
			|| modem_code==IH_FEATURE_MODEM_RG500
			|| modem_code==IH_FEATURE_MODEM_FM650
			|| modem_code==IH_FEATURE_MODEM_FG360
			|| modem_code==IH_FEATURE_MODEM_EP06
			|| modem_code==IH_FEATURE_MODEM_NL668) {

		if(modem_code==IH_FEATURE_MODEM_FM650
			|| modem_code==IH_FEATURE_MODEM_FG360
			|| modem_code==IH_FEATURE_MODEM_NL668){
			pcmd = find_at_cmd(AT_CMD_GTCCINFO);
		}else{
			pcmd = find_at_cmd(AT_CMD_QENG_SRVCELL);
		}

		handle_at_cmd(gl_commport, pcmd, 0);

		if(info->network==4 && !strcmp(info->submode_name, "NSA")){ //5G-NSA
			pcmd = find_at_cmd(AT_CMD_CEREG);
		}else if(info->network==4){ //5G-SA
			pcmd = find_at_cmd(AT_CMD_C5GREG);
		}else if(info->network==3){//4G
			pcmd = find_at_cmd(AT_CMD_CEREG);
		}else if(info->network==2){//3G
			pcmd = find_at_cmd(AT_CMD_CGREG);
		}
		
		handle_at_cmd(gl_commport, pcmd, 0);
	}else if(modem_code==IH_FEATURE_MODEM_TOBYL201){
		if(ih_key_support("FB78") && info->network==3){
			// get fb78 rsrp rsrq sinr
			handle_at_cmd_by_id(gl_commport, AT_CMD_UCGED2, 0);
			handle_at_cmd_by_id(gl_commport, AT_CMD_UCGED, 0);
		}
	}
	
	if (modem_code==IH_FEATURE_MODEM_ELS31) {
		handle_at_cmd_by_id(gl_commport, AT_CMD_CGDCONT, 0);
	}

	//read phone number
	if(!info->phonenum[0]) {
		handle_at_cmd_by_id(gl_commport, AT_CMD_CNUM, 0);
	}
	
	//-------------------------refresh rssi----------------------------
	//vs08,AT+CSQ? is diff from AT+CSQ
	if (modem_code==IH_FEATURE_MODEM_U8300C) {
			if ((strcmp(info->submode_name, "EVDO") == 0) 
				|| (strcmp(info->submode_name, "HDR") == 0)
				|| (strcmp(info->submode_name, "CDMA") == 0)) {
				  is_cdma = TRUE;
			}else {
				  is_cdma = FALSE;
			}
	}

	if(modem_code==IH_FEATURE_MODEM_PVS8){
		pcmd = find_at_cmd(AT_CMD_VCSQ);
	}else if (modem_code==IH_FEATURE_MODEM_WPD200){
		pcmd = find_at_cmd(AT_CMD_HSTATE);
	}else if (modem_code==IH_FEATURE_MODEM_LP41){
		pcmd = find_at_cmd(AT_CMD_MEAS8);
	}else if (modem_code==IH_FEATURE_MODEM_U8300C && is_cdma){
		pcmd = find_at_cmd(AT_CMD_CCSQ);
	}else if (modem_code==IH_FEATURE_MODEM_LARAR2
			|| modem_code==IH_FEATURE_MODEM_TOBYL201){
		pcmd = find_at_cmd(AT_CMD_CESQ);
	}else {
		pcmd = find_at_cmd(AT_CMD_CSQ);
	}	
	
	handle_at_cmd(gl_commport, pcmd, 0);

	sim_idx = info->current_sim-1;
	timeout = gl_myinfo.priv.signal_interval;
	
	if (get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP
		|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_MIPC
		|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI){
		if(info->siglevel == 0) {
			if (gl_check_signal_retry_cnt ++ >= MAX_SIGNAL_RETRY){
				LOG_WA("No signal for a long time, try redial!");
				gl_check_signal_retry_cnt = 0;
				stop_child_by_sub_daemon();
			}
		}else {
			gl_check_signal_retry_cnt = 0;
		}
	}

	if(modem_config->dualsim && modem_config->csq[sim_idx] 
		&& modem_config->csq_interval[sim_idx]){
		if(info->siglevel < modem_config->csq[sim_idx]){
			LOG_IN("SIM%d Low signal is %d",info->current_sim,info->siglevel);
			if(gl_dualsim_signal_retry_cnt++ < modem_config->csq_retries[sim_idx])
				timeout = modem_config->csq_interval[sim_idx];
			else {
				LOG_IN("Signal is too low, try another SIM card.");
				if(SUB_DAEMON_RUNNING){
					stop_child_by_sub_daemon();
				}
				gl_sim_switch_flag = TRUE;
			}
		}else gl_dualsim_signal_retry_cnt = 0;
	}
	
#ifdef INHAND_ER3
	//update net led status
	if(gl_redial_state == REDIAL_STATE_CONNECTED){
		if(info->network == CELL_NET_STATUS_5G){
			cellular_ledcon(CELL_CONNECTED_5G);
		}else{
			cellular_ledcon(CELL_CONNECTED);
		}
	}
#endif
	if(timeout>0) {
		evutil_timerclear(&tv);
		tv.tv_sec = timeout;
		event_add(&gl_ev_signal_tmr, &tv);
	}

	if (memcmp(&gl_myinfo, &myinfo_tmp, sizeof(MY_REDIAL_INFO)) != 0) {
		//broadcast svcinfo
		ret = ih_ipc_publish(IPC_MSG_REDIAL_SVCINFO, (char *)&gl_myinfo, sizeof(MY_REDIAL_INFO));
		if(ret) LOG_ER("signal refresh broadcast msg is not ok.");
	}
}

/*
 * my timer for avoid ppp hangup when dialing.
 * timeout is 10 mins
 *
 * @param fd		not used
 * @param event		not used
 * @param args		not used
 */
static void my_ppp_timer(int fd, short event, void *arg)
{	
	//ppp has dialed a long time, kill it
	if(SUB_DAEMON_RUNNING) {
		LOG_NO("child %s is inactive for a long time, kill it.", get_sub_deamon_name());
		stop_child_by_sub_daemon();
	}
}

/*
 * my timer for check if ppp up for 10 mins
 */
static void my_ppp_up_timer(int fd, short event, void *arg)
{
	gl_ppp_redial_cnt = 0;
	gl_dualsim_retry_cnt = 0;
	gl_dualsim_signal_retry_cnt = 0;
}

void sim_traffic_status_update(MSG_DATA_USAGE *msg)
{
	SIM_STATUS *sim_status = gl_myinfo.svcinfo.sim_status;

	if(!msg || ((msg->slot != SIM1) && msg->slot != SIM2)){
		return;
	}

	//update sim traffic status
	sim_status[msg->slot-1].traffic_status = msg->status;
	sim_status[msg->slot-1].traffic_action = msg->action;
}

//@sim: SIM1(1) or SIM2(2)
//type: DAILY_DATA_USAGE or MONTHLY_DATA_USAGE
int get_data_usage_info(int sim, int type, MSG_DATA_USAGE *data_usage)
{
	IPC_MSG *rsp = NULL;
	DATA_USAGE_REQ req;
	int ret = 0;

	if(!data_usage) {
		return ERR_FAILED;
	}

	req.slot = sim;
	req.type = type;

	ret = ih_ipc_send_wait_rsp(IH_SVC_TRAFFIC, 1500, IPC_MSG_DATA_USAGE_REQ, (char *)&req, sizeof(req), &rsp);
	if (ret) {
		LOG_IN("Request sim%d data usage failed[%d]!", sim, ret);
		return ERR_FAILED;
	}
	
	memcpy(data_usage, (MSG_DATA_USAGE*)rsp->body, sizeof(MSG_DATA_USAGE));

	sim_traffic_status_update(data_usage);

	show_data_usage(data_usage);

	ih_ipc_free_msg(rsp);

	return ERR_OK;
}

void show_data_usage(MSG_DATA_USAGE *msg)
{
	char *action_s[] = {"alert", "cloud", "stop-iface", "switch-sim"};
	char *status_s[] = {"disabled", "normal", "exceed"};

	char buf1[64] = {0};
	char buf2[64] = {0};

	traffic_to_string(msg->limit, buf1, sizeof(buf1));
	traffic_to_string(msg->traffic, buf2, sizeof(buf2));
	
	LOG_DB("sim%d => action:%s limit:%s traffic:%s status: %s", 
			msg->slot, action_s[msg->action], buf1, buf2, status_s[msg->status]);
}

//return: 0: ready -1: no sim, -2: not ready, because of traffic policy
int check_sim_status(int sim)
{
	int idx = sim-1;
	SIM_STATUS *sim_status = gl_myinfo.svcinfo.sim_status;

	if(sim != SIM1 && sim != SIM2){
		return ERR_INVAL;
	}

	if(sim_status[idx].at_errno == ERR_AT_NOSIM){
		return -1;	
	}
	
	if((sim_status[idx].traffic_status == DATA_USAGE_EXCEED)
		&& (sim_status[idx].traffic_action == STOP_FORWARD || sim_status[idx].traffic_action == SWITCH_SIM)){
		return -2;
	}
	
	return 0;
}

static BOOL should_enter_sleep_state(void)
{
	BOOL ret = FALSE;
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	SIM_STATUS *sim_status = gl_myinfo.svcinfo.sim_status;

	if(modem_config->dualsim){
		if((sim_status[0].at_errno == ERR_AT_NOSIM && sim_status[0].retries >= MAX_SIM_RETRY)
			&& (sim_status[1].at_errno == ERR_AT_NOSIM && sim_status[1].retries >= MAX_SIM_RETRY)){
			ret = TRUE;
		}
	}else{
		if(sim_status[modem_info->current_sim-1].at_errno == ERR_AT_NOSIM 
			&& sim_status[modem_info->current_sim-1].retries >= MAX_SIM_RETRY){
			ret = TRUE;
		}
	}

	return ret;
}

static void do_dualsim_switch(void)
{
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	char sim[2];
	char data[2][MAX_EVENT_DATA_LEN];
	
	LOG_IN("Switching SIM: current SIM%d,switch to SIM%d",modem_info->current_sim, SIM1+SIM2-modem_info->current_sim);
	snprintf(data[0], MAX_EVENT_DATA_LEN, "sim%d", modem_info->current_sim);
	snprintf(data[1], MAX_EVENT_DATA_LEN, "sim%d", SIM1+SIM2-modem_info->current_sim);
	/*send_events_for_report(EVENT_UPLINK_STATUS_SIM_SWITCH, (char **)data, 2);*/
	if (modem_info->current_sim == SIM2) {
		EV_RECORD(EV_CODE_UPLINK_SWITCH, UPLINK_SIM2, UPLINK_SIM1);
	} else {
		EV_RECORD(EV_CODE_UPLINK_SWITCH, UPLINK_SIM1, UPLINK_SIM2);
	}

	if(modem_info->current_sim == modem_config->main_sim)
		modem_info->current_sim = modem_config->backup_sim;
	else
		modem_info->current_sim = modem_config->main_sim;

	pinfo->sim_id = modem_info->current_sim;

	if(modem_config->policy_main == SIM_SEQ) {
		sprintf(sim, "%d", modem_info->current_sim);
		f_write_string(SEQ_SIM_FILE, sim, 0, 0);
	}

	dualsim_handle_backuptimer();

	gl_dualsim_retry_cnt = 1;
	gl_dualsim_signal_retry_cnt = 0;
	modem_dualsim_switch(get_modem_code(gl_modem_idx), modem_info->current_sim);
}

static int check_dualsim_switch(MODEM_CONFIG *cfg, DUALSIM_INFO *info)
{
	int ret=-1;
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	
	if(gl_sim_switch_flag){
		gl_sim_switch_flag = FALSE;
		ret = 0;
	}else{
		/*register fail = dial fail*/
		switch(gl_at_errno){
			case ERR_AT_LOWSIGNAL:
				if(cfg->csq) ret = 0;
				//passthrough
			case ERR_AT_NOREG:
			case ERR_AT_OK:
				if(gl_dualsim_retry_cnt++ >= info->retries){
					if(!check_sim_status(SIM1+SIM2-modem_info->current_sim)){
						LOG_IN("ppp redial reach max times:%d, switch sim...", info->retries);
						ret = 0;
					}
				}
				break;
			case ERR_AT_NOSIGNAL:
			//case ERR_AT_NOREG:
			case ERR_AT_NOSIM:
			case ERR_AT_ROAM:
				ret = 0;
				break;
			default:
				break;
		}
	}
	
	return ret;
}

static BOOL current_sim_is_right(void)
{
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	SIM_STATUS *sim_status = gl_myinfo.svcinfo.sim_status;
	
	if(modem_config->dualsim){
		if(sim_status[modem_info->current_sim-1].traffic_status == DATA_USAGE_EXCEED 
			&& !check_sim_status(SIM1+SIM2-modem_info->current_sim)){
			gl_sim_switch_flag = TRUE;
			LOG_IN("SIM%d traffic is exceed", modem_info->current_sim);
		}
	}else{
		if(modem_info->current_sim != modem_config->policy_main){
			gl_sim_switch_flag = TRUE;
			LOG_IN("SIM%d isn't main sim", modem_info->current_sim);
		}
	}
		
	return gl_sim_switch_flag == FALSE;
}

/*
 * my timer for handle sim switch
 */
static void my_dualsim_timer(int fd, short event, void *arg)
{
	if(SUB_DAEMON_RUNNING){
		stop_child_by_sub_daemon();
	}
	LOG_IN("Dualsim backtime reached, switch sim...");
	gl_sim_switch_flag = TRUE;
}

static void start_redial_pending_task(REDIAL_PENDING_TASK task)
{
	struct timeval tv = {0, 0};

	if(task != REDIAL_PENDING_TASK_NONE){
		gl_redial_pending_task |= task;
		gl_pending_task_retry_cnt = 0;
		event_add(&gl_ev_redial_pending_task_tmr, &tv);
	}
}

static void stop_redial_pending_task(REDIAL_PENDING_TASK task)
{
	if(task != REDIAL_PENDING_TASK_NONE){
		gl_redial_pending_task &= ~task;
	}

	if(gl_redial_pending_task == REDIAL_PENDING_TASK_NONE){
		event_del(&gl_ev_redial_pending_task_tmr);
	}
}

//redial pending task should use communication port of modem
static void run_redial_pending_task(REDIAL_PENDING_TASK task_mask, int verbose)
{
	char buf[256] = {0};
	AT_CMD *pcmd = NULL;
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	
	if(modem_config->debug){
		return;
	}

	task_mask |= gl_redial_pending_task;

	//update system time by modem
	if(task_mask & REDIAL_PENDING_TASK_UPDATE_TIME){
		if(gl_modem_code == IH_FEATURE_MODEM_PLS8
			|| gl_modem_code == IH_FEATURE_MODEM_PHS8
			|| gl_modem_code == IH_FEATURE_MODEM_EC20
			|| gl_modem_code == IH_FEATURE_MODEM_EC25
			|| gl_modem_code == IH_FEATURE_MODEM_RM500
			|| gl_modem_code == IH_FEATURE_MODEM_RM520N
			|| gl_modem_code == IH_FEATURE_MODEM_RM500U
			|| gl_modem_code == IH_FEATURE_MODEM_RG500
			|| gl_modem_code == IH_FEATURE_MODEM_EP06
			|| gl_modem_code == IH_FEATURE_MODEM_LARAR2
			|| gl_modem_code == IH_FEATURE_MODEM_LE910
			|| gl_modem_code == IH_FEATURE_MODEM_ME909){

			enable_modem_auto_time_zone_update(gl_commport, 0);
			
			if(gl_modem_code == IH_FEATURE_MODEM_RM500U){
				pcmd = find_at_cmd(AT_CMD_QLTS);
			}else{
				pcmd = find_at_cmd(AT_CMD_CCLK);
			}

			if(pcmd){
				send_at_cmd_sync(gl_commport, pcmd->atc, buf, sizeof(buf), verbose);
				if(pcmd->at_cmd_handle(pcmd, buf, 0, NULL, NULL) >= 0){
					stop_redial_pending_task(REDIAL_PENDING_TASK_UPDATE_TIME);
					ih_ipc_send_no_ack(IH_SVC_SNTPC, IPC_MSG_REDIAL_TIME_SYNC, NULL, 0);
				}
			}
		}else{
			stop_redial_pending_task(REDIAL_PENDING_TASK_UPDATE_TIME);
		}
	}

	//read modem preferred operator list
	if(task_mask & REDIAL_PENDING_TASK_UPDATE_POL){
		if((strstr(g_sysinfo.oem_name, "welotec")) && 
			(gl_modem_code == IH_FEATURE_MODEM_PLS8)){
			if(get_modem_preferred_operator_list(gl_commport, 0) >= 0){
				stop_redial_pending_task(REDIAL_PENDING_TASK_UPDATE_POL);
			}
		}else{
			stop_redial_pending_task(REDIAL_PENDING_TASK_UPDATE_POL);
		}
	}
}

/*
 * my timer for handle request task
 */
static void my_redial_pending_task_timer(int fd, short event, void *arg)
{
	struct timeval tv = {0, 0};
	BOOL can_use_commport = FALSE;
	
	//get commport usable status
	if((gl_redial_state >= REDIAL_STATE_MODEM_READY) && (gl_redial_state != REDIAL_STATE_AT)){
		can_use_commport = TRUE;
		if((get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_PPP)
			&& (strcmp(gl_commport, gl_dataport) == 0)){
			can_use_commport = (gl_redial_state > REDIAL_STATE_AT) ? FALSE : TRUE;
		}
	}else{
		can_use_commport = FALSE;
	}

	if(can_use_commport){
		run_redial_pending_task(gl_redial_pending_task, gl_myinfo.priv.ppp_debug);
	}

	if(gl_redial_pending_task != REDIAL_PENDING_TASK_NONE){
		gl_pending_task_retry_cnt++;
		tv.tv_sec = LIMIT(gl_pending_task_retry_cnt * 2, 10, 3600);
		event_add(&gl_ev_redial_pending_task_tmr, &tv);
	}else{
		gl_pending_task_retry_cnt = 0;
	}
}

/*
 * liyb :my timer to check wwan0 connection status.
 */
static void my_wwan_connect_timer(int fd, short event, void *arg)
{
	AT_CMD *pcmd = NULL; 
	AT_CMD *scmd = NULL; 
	int ret = -1;
	char buf[256] = {0};
	struct timeval tv;
	MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	//PPP_CONFIG *ppp_config = &(gl_myinfo.priv.ppp_config);
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	int verbose = gl_myinfo.priv.ppp_debug;

	if(modem_config->debug){
		evutil_timerclear(&tv);
		tv.tv_sec = REDIAL_CHECK_CONN_STATE_INTERVAL;
		event_add(&gl_ev_wwan_connect_tmr, &tv);
		return;
	}

	switch (gl_modem_code) {
		case IH_FEATURE_MODEM_PLS8:
		case IH_FEATURE_MODEM_ELS61:
		case IH_FEATURE_MODEM_ELS81:
			pcmd = find_at_cmd(AT_CMD_SWWAN);
			scmd = find_at_cmd(AT_CMD_CGATT);
			break;
		case IH_FEATURE_MODEM_LP41:
			pcmd = find_at_cmd(AT_CMD_CGPADDR);
			scmd = find_at_cmd(AT_CMD_CEREG);
			break;
		case IH_FEATURE_MODEM_ME909:
			pcmd = find_at_cmd(AT_CMD_NDISSTATQRY);
			scmd = find_at_cmd(AT_CMD_CGATT);
			break;
		case IH_FEATURE_MODEM_ELS31:
			pcmd = find_at_cmd(AT_CMD_SICA);
			scmd = find_at_cmd(AT_CMD_CGATT);
			break;
		case IH_FEATURE_MODEM_FM650:
		case IH_FEATURE_MODEM_NL668:
			pcmd = find_at_cmd(AT_CMD_GTRNDIS);
			scmd = find_at_cmd(AT_CMD_CGATT);
			break;
		//case IH_FEATURE_MODEM_ELS81:
		case IH_FEATURE_MODEM_TOBYL201:
			//if (gl_modem_code==IH_FEATURE_MODEM_ELS81) {
			//	ret = modem_get_net_params_by_at(gl_commport, FALSE, ppp_config , info, pinfo, gl_myinfo.priv.ppp_debug);
			//} else { 
				ret = ublox_bridge_mode_get_net_paras(gl_commport, FALSE, gl_myinfo.priv.ppp_debug);
			//}
			if (ret < 0) {
				syslog(LOG_WARNING, "network is disconnected, try to reconnect it.");
				gl_ppp_pid = -1;
				goto wwan_try_redial;
			}else if(ret > 0) {
				ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)pinfo, sizeof(VIF_INFO));
				if(ret) LOG_ER("broadcast msg is not ok.");
				gl_ppp_pid = -1;
				goto wwan_try_redial;
			}

			pcmd = find_at_cmd(AT_CMD_CGATT);

			//goto restart_timer;
			break;
		case IH_FEATURE_MODEM_RM500U:
			ret = check_modem_qnetdevstatus(gl_commport, 1, pinfo->local_ip, gl_myinfo.priv.ppp_debug);
			if(ret == 0){
				LOG_IN("Connection is terminated, maybe the link is idle for a long time,reconnect to network!");
				goto wwan_try_redial;	
			}else if(ret < 0){
				gl_open_usbserial_retry_cnt++;
				if (gl_open_usbserial_retry_cnt >= MAX_OPEN_USB_RETRY){
					LOG_ER("Unknown error!");
					goto wwan_try_redial;
				}
				evutil_timerclear(&tv);
				tv.tv_sec = 5 ;
				event_add(&gl_ev_wwan_connect_tmr, &tv);
				return;
			}

			scmd = find_at_cmd(AT_CMD_CGATT);
			break;
		case IH_FEATURE_MODEM_RM520N:
			verbose = 0;
			pcmd = find_at_cmd(AT_CMD_MPDN_STATUS);
			if(ih_key_support("NRQ1")){
				scmd = find_at_cmd(AT_CMD_CGPADDR);
			}
			break;
		default:
			return;
	}

	if (pcmd){
		//send request command
		ret = send_at_cmd_sync(gl_commport, pcmd->atc, buf, sizeof(buf), verbose);
		if (ret==0){
			ret = pcmd->at_cmd_handle(pcmd, buf, 0, modem_config, info);			
			if (ERR_AT_OK != ret){
				LOG_IN("Connection is terminated, maybe the link is idle for a long time,reconnect to network!");
				goto wwan_try_redial;
			}
		}else {
			gl_open_usbserial_retry_cnt++;
			if (gl_open_usbserial_retry_cnt >= MAX_OPEN_USB_RETRY){
				LOG_ER("Unknown error!");
				//gl_redial_state = REDIAL_STATE_DHCP;		
				goto wwan_try_redial;
			}
			evutil_timerclear(&tv);
			tv.tv_sec = 5 ;
			event_add(&gl_ev_wwan_connect_tmr, &tv);
			return;
		}
	}

	if (scmd) {
		if(scmd->index == AT_CMD_CGPADDR && gl.default_cid > 0){
			snprintf(scmd->atc, sizeof(scmd->atc), "AT+CGPADDR=%d\r\n", gl.default_cid);
		}

		ret = send_at_cmd_sync(gl_commport, scmd->atc, buf, sizeof(buf), verbose);
		if (ret==0) {
			ret = scmd->at_cmd_handle(scmd, buf, 0, modem_config, info);
			if (ERR_AT_OK != ret) {
			  LOG_IN("Modem is disattached from network,maybe the device has no signal, try redial!");
			  goto wwan_try_redial;
			}
		}
	}

//restart_timer:
	gl_open_usbserial_retry_cnt = 0;
	evutil_timerclear(&tv);
	tv.tv_sec = REDIAL_CHECK_CONN_STATE_INTERVAL;
	event_add(&gl_ev_wwan_connect_tmr, &tv);
	return;

wwan_try_redial:
	stop_child_by_sub_daemon();

	gl_open_usbserial_retry_cnt = 0;
	my_restart_timer(1);
	return;
}

static int check_current_net_type(char *dev) 
{

	int fd = -1;

	AT_CMD *pcmd;
	char buf[256];

	if (gl_modem_code != IH_FEATURE_MODEM_U8300C) {
		return IH_ERR_OK;
	}

	fd = open_dev(dev);
	if(fd<0) return -1;

	pcmd = find_at_cmd(AT_CMD_PSRAT_C);

	send_at_cmd_sync(gl_commport, pcmd->atc, buf, sizeof(buf), 0);

	pcmd->at_cmd_handle(pcmd, buf, gl_at_try_n, &gl_myinfo.priv.modem_config, &(gl_myinfo.svcinfo.modem_info));

	close(fd);
	return IH_ERR_OK;


}

AT_CMD* find_at_cmd2(int index, uns16 retry_cnt)
{
	int num = 0;
	int *plist = NULL;
	AT_CMD *pcmd1 = NULL;
	AT_CMD *pcmd2 = NULL;

	if(index == AT_CMD_CREG_AUTO){
		plist = gl_creg_at_lists;
	}

	pcmd1 = find_at_cmd(index);
	if(plist && pcmd1){
		//calculate total number of the AT
		for(num = 0; plist[num] > 0; num++);
		//modify at cmd
		if(num && (pcmd2 = find_at_cmd(plist[retry_cnt % num])) != NULL){
			strlcpy(pcmd1->atc, pcmd2->atc, sizeof(pcmd1->atc));
			strlcpy(pcmd1->resp, pcmd2->resp, sizeof(pcmd1->resp));
			pcmd1->at_cmd_handle = pcmd2->at_cmd_handle;
			//LOG_DB("%s(%d) cmd:%s", pcmd1->name, index, pcmd1->atc);
		}
	}

	return pcmd1;
}

static void redial_wait_at_callback(int fd, short event, void *arg)
{
	int ret, reload=1, space;
	char buf[256] = {0};

	ret = read(fd, buf, sizeof(buf)-1);
	if(ret>0) {
		//avoid mem override
		space = sizeof(gl_at_buf) - strlen(gl_at_buf) -1;
		if(strlen(buf) > space) {
			LOG_IN("Too many data, eat it.");
			gl_at_buf[0] = '\0';
		} else {
			strcat(gl_at_buf, buf);
			if(strstr(gl_at_buf, "OK")) {
				reload = 0;
				gl_at_ready = TRUE;
				my_restart_timer(0);
				LOG_DB("got OK");
			}
		}
	}

	/*reload event*/
	if(reload) event_add(&gl_ev_at, NULL);		
}

static void redial_wait_sim_callback(int fd, short event, void *arg)
{
	int ret;
	char buf[512] = {0};

	ret = read(fd, buf, sizeof(buf)-1);
	if(ret>0) {
		if(strstr(buf, "+SIM: Inserted")){
			do_redial_procedure();
		}
	}else if(ret == 0){
		LOG_IN("AT port disconnected");
		CLOSE_FD(gl_at_fd);
	}
}

static void redial_at_callback(int fd, short event, void *arg)
{
	int ret, reload=1, space;
	AT_CMD *pcmd;
	char buf[256];
	gl_timeout = 0;
	ret = modem_read(fd, buf, sizeof(buf));
	if(ret>0) {
		//avoid mem override
		space = sizeof(gl_at_buf) - strlen(gl_at_buf) -1;
		if(strlen(buf) > space) {
			LOG_IN("Too many data, eat it.");
			gl_at_buf[0] = '\0';
		} else {
			strcat(gl_at_buf, buf);
			if(strstr(gl_at_buf, "OK")) {
				if(gl_at_lists[gl_at_index]!=-1) {
					pcmd = find_at_cmd2(gl_at_lists[gl_at_index], gl_at_try_n-1);
					//if((pcmd == NULL) || ((pcmd->atc)[0] == '\0'))			
					if(pcmd->at_cmd_handle)
						gl_at_errno = pcmd->at_cmd_handle(pcmd, gl_at_buf, gl_at_try_n, &gl_myinfo.priv.modem_config, &(gl_myinfo.svcinfo.modem_info));				
					else
						gl_at_errno = ERR_AT_OK;
					reload = 0;
					//reschedule next command
					if(!gl_at_errno) {
						if(pcmd->index == AT_CMD_CIMI
							|| pcmd->index == AT_CMD_QCIMI) {
							auto_select_net_apn(gl_myinfo.svcinfo.modem_info.imsi, 1);
							//if (gl_myinfo.priv.modem_config.network_type == MODEM_NETWORK_AUTO || gl_myinfo.priv.modem_config.network_type == MODEM_NETWORK_4G){

								if (!(ih_license_support(IH_FEATURE_MODEM_LARAR2) 
										|| ih_license_support(IH_FEATURE_MODEM_TOBYL201))) {
								active_pdp_text(gl_commport, &gl_myinfo.priv.ppp_config,&gl_myinfo.priv.modem_config, &gl_myinfo.svcinfo.modem_info, gl_myinfo.priv.ppp_debug);
								}
							//}
							
							if (gl_myinfo.priv.ppp_config.init_string[0] && (get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP || get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI || get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_MIPC)) {
								sleep(1);
								execute_init_command(gl_commport, gl_myinfo.priv.ppp_config.init_string);
							}
							
						} else if (pcmd->index == AT_CMD_CFUN0) {
								if (ih_license_support(IH_FEATURE_MODEM_LARAR2) 
										|| ih_license_support(IH_FEATURE_MODEM_TOBYL201)) {
										active_pdp_text(gl_commport, &gl_myinfo.priv.ppp_config,&gl_myinfo.priv.modem_config, &gl_myinfo.svcinfo.modem_info, gl_myinfo.priv.ppp_debug);
								}

						}

#if 0
						if (ih_license_support(IH_FEATURE_MODEM_LARAR2) 
								|| ih_license_support(IH_FEATURE_MODEM_TOBYL201)) {
								if (pcmd->index == AT_CMD_UMNOCONF_3_15
									|| pcmd->index == AT_CMD_UMNOCONF_1_20) {
									//gl_at_lists[gl_at_index + 1] = AT_CMD_AT;
									gl_at_lists[gl_at_index + 2] = AT_CMD_AT;
								}
#if 0
								if (pcmd->index == AT_CMD_UMNOCONF0_3_15
										||pcmd->index == AT_CMD_UMNOCONF0_1_20) { 
									gl_need_function_reset = TRUE;
									close(gl_at_fd);
									gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
									gl_timeout = 10;		
									my_restart_timer(gl_timeout);
								}
#endif
	
						}
#endif
						if (ih_license_support(IH_FEATURE_MODEM_ELS31)) {
							if (gl_at_lists[gl_at_index] == AT_CMD_CGPADDR2) {
								if (gl_els31_update_apn_cnt< ELS31_UPDATE_APN_MAX_RETRY) {	
									gl_at_index -= 2;
									gl_els31_update_apn_cnt++; 
								}else {
									gl_at_index++;
									gl_els31_update_apn_cnt = 0; 
								}
							} else if (gl_at_lists[gl_at_index] == AT_CMD_CGPADDR3){
								gl_at_index += 2;
								gl_els31_update_apn_cnt = 0; 
							} else {
								gl_at_index++;
							}
						}else {
							gl_at_index++;
						}

						gl_at_try_n = 0;
						//reschedule next cmd at once
						my_restart_timer(gl_timeout);
					}
				}
				gl_at_buf[0] = '\0';
#if (defined INHAND_IDTU9 || defined INHAND_IG5)
			} else if(gl_at_lists[gl_at_index]!=-1) {
				pcmd = find_at_cmd2(gl_at_lists[gl_at_index], gl_at_try_n-1);

				if(pcmd->index == AT_CMD_CPIN
						|| pcmd->index == AT_CMD_QCPIN) {

					if (strstr(gl_at_buf, "SIM failure") 
							|| strstr(gl_at_buf, "SIM not inserted")
							|| strstr(gl_at_buf, "ERROR") ) {
						ledcon_by_event(EV_DAIL_NOSIM);
					}
				}
#endif
			}
		}
	}

	/*reload event*/
	if(reload) event_add(&gl_ev_at, NULL);		

}

static void clear_cellular_net_info() 
{
	int num_need_set = CFG_PER_ADDR < DHCPV6C_LIST_NUM ? CFG_PER_ADDR:DHCPV6C_LIST_NUM;
	int opt_flag = ADDR_OPT|DNS_OPT|RA_DNS_OPT|RA_MTU_OPT|RA_ROUTE_OPT;
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	VIF_INFO *pinfo_2nd = &(gl_myinfo.svcinfo.secondary_vif_info);
#define BZERO(e) bzero(&(e), sizeof(e))

	BZERO(pinfo->local_ip);
	BZERO(pinfo->remote_ip);
	BZERO(pinfo->netmask);
	BZERO(pinfo->dns1);
	BZERO(pinfo->dns2);
	del_dhcpv6_info2inf(pinfo, num_need_set, opt_flag);

	BZERO(pinfo_2nd->local_ip);
	BZERO(pinfo_2nd->remote_ip);
	BZERO(pinfo_2nd->netmask);
	BZERO(pinfo_2nd->dns1);
	BZERO(pinfo_2nd->dns2);
	del_dhcpv6_info2inf(pinfo_2nd, num_need_set, opt_flag);
}


static void clear_modem_info(MODEM_INFO *info)
{
	int sim = info->current_sim;

	memset(info, 0, sizeof(MODEM_INFO));
	//current_sim not clear
	info->current_sim = sim;

	if (ih_license_support(IH_FEATURE_MODEM_ELS81)) {
		//eval("ifconfig", WWAN0_WWAN, "down");
		eval("ifconfig", USB0_WWAN, "down");
	} else if (ih_license_support(IH_FEATURE_MODEM_TOBYL201)) {
		eval("ifconfig", USB0_WWAN, "down");
	}

	clear_cellular_net_info();
}

static void clear_at_state(void)
{
	gl_at_buf[0] = '\0';
	gl_at_index = 0;
	gl_at_try_n = 0;
	gl_at_errno = ERR_AT_OK;
}

static void clear_sim_status(void)
{
	int i;
	SIM_STATUS *sim_status = gl_myinfo.svcinfo.sim_status;
	
	for(i = 0; i < SIM2; i++){
		sim_status[i].at_errno = ERR_AT_OK;
		sim_status[i].retries = 0;
	}
}

static int32 start_ppp(void)
{
	char nv[64];
	int n=1;

//	stop_ppp();

	sprintf(nv, "wan%d", n);
	if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_PVS8){
		start_daemon(&gl_ppp_pid, "pppd", "pvs8","call", nv, NULL);
	}else {
		start_daemon(&gl_ppp_pid, "pppd", "call", nv, NULL);
	}
	return ERR_OK;
}

int32 start_dhcpc(char *interface, int wwan_id, BOOL debug)
{	
	int *udhcpc_pid = NULL;
	int *odhcp6c_pid = NULL;
	char udhcpc_pidfile[64] = {0};
	char iface[64] = {0};

	VIF_INFO *pinfo = NULL;
	PPP_CONFIG *dialup_config;
	int pdp_type = get_pdp_type_setting();
	
	if (!interface || !interface[0]) {
		return ERR_FAILED;	
	}

	if (wwan_id==WWAN_ID2) {
		udhcpc_pid = &gl_2nd_ppp_pid;
		odhcp6c_pid = &gl_2nd_odhcp6c_pid;
		pinfo = &(gl_myinfo.svcinfo.secondary_vif_info);
		dialup_config = &(gl_myinfo.priv.wwan2_config); 
	}else {
		udhcpc_pid = &gl_ppp_pid;
		odhcp6c_pid = &gl_odhcp6c_pid;
		pinfo = &(gl_myinfo.svcinfo.vif_info);
		dialup_config = &(gl_myinfo.priv.ppp_config); 
	}

	if(ih_license_support(IH_FEATURE_PCIE2ETH_RTL8125B)){
		vif_get_sys_name(&pinfo->if_info, iface);
		f_write_string("/sys/class/net/eth0/queues/rx-0/rps_cpus", "e", 0, 0);
	}else if(gl_modem_code==IH_FEATURE_MODEM_FM650
		|| gl_modem_code==IH_FEATURE_MODEM_RM500U){
		if(gl_modem_dev_state & MODEM_DEV_PCIE_ALIVE){
			eval("ifconfig", "sipa_dummy0", "up");
		}
		vif_get_sys_name(&pinfo->if_info, iface);
#ifdef INHAND_ER6
		f_write_string("/proc/irq/25/smp_affinity", "3", 0, 0);
		f_write_string("/sys/class/net/pcie0/queues/tx-0/xps_cpus", "e", 0, 0);
		f_write_string("/sys/class/net/pcie0/queues/rx-0/rps_cpus", "e", 0, 0);
#endif
	}else{
		strlcpy(iface, interface, sizeof(iface));
	}
	
	if(check_cell_sys_name(wwan_id, get_pdp_context_id(wwan_id))){
		LOG_ER("failed to check cellualr interface sys_name!");
		return ERR_FAILED;
	}

	eval("ifconfig", iface, "up");//liyb:usb0/wwan0默认是down的.

	if (gl_modem_code == IH_FEATURE_MODEM_RM500U) {
		if (*odhcp6c_pid > 0) {
			stop_odhcp6c(pinfo->iface, &pinfo->ip6_addr, odhcp6c_pid);
		}
		if (g_sysinfo.ipv6_enable && (pdp_type != PDP_TYPE_IPV4 )) {
			LOG_IN("IPv6 is enabled, starting IPv6 stateless configuration.");
			start_odhcp6c2(iface, odhcp6c_pid, STATEFUL_PD, dialup_config->static_iapd_prefix, dialup_config->static_iapd_prefix_len, ODHCP6C_WWAN);
		}
	}

	if (pdp_type != PDP_TYPE_IPV6) {
		if (*udhcpc_pid > 0) {
			stop_dhcpc(udhcpc_pid);
		}

		snprintf(udhcpc_pidfile, sizeof(udhcpc_pidfile), "/var/run/%s_udhcpc.pid", iface);

		if (debug) {
			start_daemon(udhcpc_pid, "udhcpc", "-f", "-i", iface,"-S", "-R", "-O", "mtu", "-s", "/usr/bin/wwan-dhcpc-event", "-p", udhcpc_pidfile, NULL);
		}else {
			start_daemon(udhcpc_pid, "udhcpc", "-f", "-i", iface, "-R", "-O", "mtu", "-s", "/usr/bin/wwan-dhcpc-event", "-p", udhcpc_pidfile, NULL);
		}
	}
	return ERR_OK;
}

int32 qmi_proxy_is_running(void)
{
	return (read_pidfile(QMI_PROXY_PID_FILE) > 0) || (gl_qmi_proxy_pid > 0);
}

int32 start_ih_qmi_proxy(pid_t *ppid, int debug)
{
	int argc = 0;
	char *argv[16] = {NULL};
	
	argv[argc++] = "ih-qmi-proxy";
	argv[argc++] = "-p";
	argv[argc++] = QMI_PROXY_PID_FILE;
	if(debug){
		argv[argc++] = "-d";
	}
	
	return _eval(argv, NULL, 0, ppid);
}

void stop_ih_qmi_proxy(void)
{
	pid_t pid = -1;

	if(gl_qmi_proxy_pid > 0){
		stop_or_kill(gl_qmi_proxy_pid, 3000);
		gl_qmi_proxy_pid = -1;
	}else if((pid = read_pidfile(QMI_PROXY_PID_FILE)) > 0){
		stop_or_kill(pid, 3000);
	}

	unlink(QMI_PROXY_PID_FILE);
}

int32 start_ih_qmi(int wwan_id, BOOL debug)
{
	int  argc = 0, prefix_len;
	char *argv[32] = {NULL};
	char cid[8] = {0};
	char *retptr = NULL;
	char addr[INET6_ADDRSTRLEN] = {0};
	char length[8] = {0};
	char *pidfile = PPP_PID_FILE;
	VIF_INFO *pinfo = NULL;
	char sys_name[32] = {0};

	pid_t *qmi_pid = NULL;
	PPP_CONFIG *dialup_config = NULL;
	int pdp_type = get_pdp_type_setting();

	if(wwan_id == WWAN_ID2){
		pinfo = &(gl_myinfo.svcinfo.secondary_vif_info);
		qmi_pid = &gl_2nd_ppp_pid;
		dialup_config = &(gl_myinfo.priv.wwan2_config); 
		pidfile = PPP_PID_FILE2;
	}else{
		pinfo = &(gl_myinfo.svcinfo.vif_info);
		qmi_pid	= &gl_ppp_pid;
		dialup_config = &(gl_myinfo.priv.ppp_config); 
		pidfile = PPP_PID_FILE;
	}

	if(*qmi_pid > 0){
		stop_ih_qmi(qmi_pid);
	}

	if(gl_dual_wwan_enable){
		if(!qmi_proxy_is_running()){
			start_ih_qmi_proxy(&gl_qmi_proxy_pid, debug);
			sleep(3);
		}
	}else{
		if(qmi_proxy_is_running()){
			stop_ih_qmi_proxy();
			sleep(3);
		}
	}

	snprintf(cid, sizeof(cid), "%d", get_pdp_context_id(wwan_id));
	if(check_cell_sys_name(wwan_id, atoi(cid))){
		LOG_ER("failed to check cellualr interface sys_name!");
		return ERR_FAILED;
	}

	argv[argc++] = "ih-qmi";
	argv[argc++] = "-p";
	argv[argc++] = pidfile;
	argv[argc++] = "-n";
	argv[argc++] = cid;
	
	if(pdp_type != PDP_TYPE_IPV6){
		argv[argc++] = "-4";
		argv[argc++] = "-s";
		argv[argc++] = "/usr/bin/wwan-dhcpc-event";
	}

	if(gl_modem_code == IH_FEATURE_MODEM_RM500
		|| gl_modem_code == IH_FEATURE_MODEM_EP06){
		vif_get_sys_name(&(pinfo->if_info), sys_name);
		argv[argc++] = "-i";
		argv[argc++] = sys_name;
	}

	argv[argc++] = "-k"; //keep online

	if(g_sysinfo.ipv6_enable && (pdp_type != PDP_TYPE_IPV4)){
		argv[argc++] = "-6";
		argv[argc++] = "-S";
		argv[argc++] = "/usr/bin/wwan-odhcp6c-event";
		
		prefix_len = dialup_config->static_iapd_prefix_len;
		retptr = (char*)inet_ntop(AF_INET6, &dialup_config->static_iapd_prefix, addr, INET6_ADDRSTRLEN);
		if(retptr && prefix_len > 0 && prefix_len <= 64){
			snprintf(length, sizeof(length), "%d", prefix_len);
			argv[argc++] = "-a";
			argv[argc++] = addr;
			argv[argc++] = "-l";
			argv[argc++] = length;
		}
	}else if(ih_key_support("NRQ1")){
		argv[argc++] = "-6";
		argv[argc++] = "-N"; // Start IPv6 data call, but without odhcp6c.
		argv[argc++] = "6";
	}

	if(gl_dual_wwan_enable){
		argv[argc++] = "-u";
	}

	if(debug){
		argv[argc++] = "-d";
	}

	return _eval(argv, NULL, 0, qmi_pid);
}

int32 start_mipc_wwan(int wwan_id, BOOL debug)
{
	int  argc = 0;
	char *argv[32] = {NULL};
	char cid[8] = {0};
	char *pidfile = PPP_PID_FILE;
	PPP_PROFILE *p_profile = NULL;
	PPP_PROFILE *p_2nd_wwan_profile = NULL;

	pid_t *mipc_pid = NULL;

	get_profile_setting(&p_profile, &p_2nd_wwan_profile, &gl_myinfo.priv.ppp_config, &gl_myinfo.priv.modem_config, &gl_myinfo.svcinfo.modem_info); 

	if(wwan_id == WWAN_ID2){
		mipc_pid = &gl_2nd_ppp_pid;
		pidfile = PPP_PID_FILE2;
	}else{
		mipc_pid = &gl_ppp_pid;
		pidfile = PPP_PID_FILE;
	}

	if(*mipc_pid > 0){
		stop_mipc_wwan(mipc_pid);
	}

	snprintf(cid, sizeof(cid), "%d", get_pdp_context_id(wwan_id));

	argv[argc++] = "mipc_wwan";
	argv[argc++] = "-n";
	argv[argc++] = cid;
	argv[argc++] = "-t";
	argv[argc++] = "ipv4v6";
	if(p_profile && p_profile->apn[0]){
		argv[argc++] = "-s";
		argv[argc++] = p_profile->apn;
		if(p_profile->username[0] && p_profile->password[0]){
			argv[argc++] = p_profile->username;
			argv[argc++] = p_profile->password;
			argv[argc++] = gl_auth_type_str[p_profile->auth_type];
		}
	}
	
	if(debug){
		argv[argc++] = "-d";
	}
	
	argv[argc++] = "-p";
	argv[argc++] = pidfile;

	return _eval(argv, NULL, 0, mipc_pid);
}

int32 stop_ppp(void)
{
	if (gl_ppp_pid<0) return ERR_OK;

	LOG_IN("stop ppp service");
	stop_or_kill(gl_ppp_pid, 3000);
	gl_ppp_pid = -1;

	return ERR_OK;
}

int32 stop_dhcpc(int *pid)
{
	if (*pid<0){
		return ERR_OK;
	}

	//LOG_IN("stop dhcp service");
	stop_or_kill(*pid, 3000);
	*pid = -1;

	return ERR_OK;
}

int32 stop_ih_qmi(pid_t *qmi_pid)
{
	if(!qmi_pid || *qmi_pid < 0){
		return ERR_OK;
	}

	stop_or_kill(*qmi_pid, 3000);
	*qmi_pid = -1;
	
	return ERR_OK;
}

int32 stop_mipc_wwan(pid_t *mipc_pid)
{
	if(!mipc_pid || *mipc_pid < 0){
		return ERR_OK;
	}

	stop_or_kill(*mipc_pid, 3000);
	*mipc_pid = -1;
	
	return ERR_OK;
}

int32 stop_child_by_sub_daemon(void)
{
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	VIF_INFO *pinfo_2nd = &(gl_myinfo.svcinfo.secondary_vif_info);

	switch(get_sub_deamon_type(gl_modem_idx)){
		case SUB_DEAMON_DHCP:
			stop_dhcpc(&gl_ppp_pid);
			stop_dhcpc(&gl_2nd_ppp_pid);
			stop_odhcp6c(pinfo->iface, &(pinfo->ip6_addr), &gl_odhcp6c_pid);
			stop_odhcp6c(pinfo_2nd->iface, &(pinfo_2nd->ip6_addr), &gl_2nd_odhcp6c_pid);
			break;
		case SUB_DEAMON_QMI:
			stop_ih_qmi(&gl_ppp_pid);
			stop_ih_qmi(&gl_2nd_ppp_pid);
			break;
		case SUB_DEAMON_MIPC:
			stop_mipc_wwan(&gl_ppp_pid);
			break;
		case SUB_DEAMON_NONE:
			gl_ppp_pid = -1;
			break;
		case SUB_DEAMON_PPP:
		default:
			stop_ppp();
			break;
	}

	return ERR_OK;
}

int32 stop_all_child_by_pidfile(void)
{
	pid_t pid = -1;
	char sys_name[32] = {0};
	char pid_file[64] = {0};
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	VIF_INFO *pinfo_2nd = &(gl_myinfo.svcinfo.secondary_vif_info);

	if((pid = read_pidfile(PPP_PID_FILE)) > 0){
		stop_or_kill(pid, 3000);
		unlink(PPP_PID_FILE);
	}

	if((pid = read_pidfile(PPP_PID_FILE2)) > 0){
		stop_or_kill(pid, 3000);
		unlink(PPP_PID_FILE2);
	}

	if((pid = read_pidfile(QMI_PROXY_PID_FILE)) > 0){
		stop_or_kill(pid, 3000);
		unlink(QMI_PROXY_PID_FILE);
	}

	vif_get_sys_name(&(pinfo->if_info), sys_name);
	snprintf(pid_file, sizeof(pid_file), "/var/run/%s_odhcp6c.pid", sys_name);
	if((pid = read_pidfile(pid_file)) > 0){
		stop_or_kill(pid, 3000);
		unlink(pid_file);
	}

	vif_get_sys_name(&(pinfo_2nd->if_info), sys_name);
	snprintf(pid_file, sizeof(pid_file), "/var/run/%s_odhcp6c.pid", sys_name);
	if((pid = read_pidfile(pid_file)) > 0){
		stop_or_kill(pid, 3000);
		unlink(pid_file);
	}

	return ERR_OK;
}

static int32 start_smsd(void)
{
	SMS_CONFIG *sms_config = &(gl_myinfo.priv.sms_config);

	stop_smsd();

	if(sms_config->enable) {
		start_daemon(&gl_smsd_pid, "smsd", "-d", NULL);	
	}

	return 0;
}

static int32 stop_smsd(void)
{
	if (gl_smsd_pid<0) return ERR_OK;

	LOG_IN("stop smsd service");
	stop_or_kill(gl_smsd_pid, 3000);
	gl_smsd_pid = -1;
	
	return 0;
}

MODEM_CONFIG *get_modem_config(void)
{
	return &(gl_myinfo.priv.modem_config);
}

VIF_INFO *my_get_reidal_vif_info(void)
{
	return &(gl_myinfo.svcinfo.vif_info);
}

BOOL ppp_debug(void)
{
	return gl_myinfo.priv.ppp_debug;
}

BOOL dual_apn_is_enabled(void)
{
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	MODEM_INFO *modem_info = &(gl_myinfo.svcinfo.modem_info);

	if(modem_info->current_sim == SIM1){
		return modem_config->dual_wwan_enable[0] ? TRUE : FALSE;
	}else if(modem_info->current_sim == SIM2){
		return modem_config->dual_wwan_enable[1] ? TRUE : FALSE;
	}

	return FALSE;
}

BOOL allow_reboot(void)
{
	return gl_myinfo.priv.ppp_config.infinitely_dial_retry == 0;
}

#if 0
static void set_sub_deamon_type(void) 
{
	if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_PLS8
		|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ME909
		|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS31
		|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_LP41){
		gl_redial_sub_deamon_type = SUB_DEAMON_DHCP;
	}else {
		gl_redial_sub_deamon_type = SUB_DEAMON_PPP;
	}
	
	return;
}
#endif

char *get_sub_deamon_name(void)
{
	char *pname = "";
	int sub_deamon = get_sub_deamon_type(gl_modem_idx);
	
	switch(sub_deamon){
		case SUB_DEAMON_PPP:
			pname = "pppd";
			break;
		case SUB_DEAMON_DHCP:
			pname = "udhcpc";
			break;
		case SUB_DEAMON_QMI:
			pname = "ih-qmi";
			break;
		case SUB_DEAMON_MIPC:
			pname = "mipc_wwan";
			break;
		case SUB_DEAMON_NONE:
		default:
			break;
	}
	
	return pname;
}

int get_sub_deamon_type(int idx) 
{
	if (get_modem_code(idx)==IH_FEATURE_MODEM_PLS8
		|| get_modem_code(idx)==IH_FEATURE_MODEM_ME909
		|| get_modem_code(idx)==IH_FEATURE_MODEM_ELS31
		|| get_modem_code(idx)==IH_FEATURE_MODEM_RM500U
		|| get_modem_code(idx)==IH_FEATURE_MODEM_RM520N
		|| get_modem_code(idx)==IH_FEATURE_MODEM_FM650
	//	|| get_modem_code(idx)==IH_FEATURE_MODEM_ELS61
		//|| get_modem_code(idx)==IH_FEATURE_MODEM_ELS81
		|| get_modem_code(idx)==IH_FEATURE_MODEM_LP41){
		return SUB_DEAMON_DHCP;
	}else if (get_modem_code(idx)==IH_FEATURE_MODEM_EC25
			|| get_modem_code(idx)==IH_FEATURE_MODEM_EC20
			|| get_modem_code(idx)==IH_FEATURE_MODEM_EP06
			|| get_modem_code(idx)==IH_FEATURE_MODEM_RM500
			|| get_modem_code(idx)==IH_FEATURE_MODEM_RG500
			|| get_modem_code(idx)==IH_FEATURE_MODEM_NL668){
		return SUB_DEAMON_QMI;
	}else if (get_modem_code(idx)==IH_FEATURE_MODEM_FG360){
		return SUB_DEAMON_MIPC;
	}else if (get_modem_code(idx)==IH_FEATURE_MODEM_TOBYL201
				|| get_modem_code(idx)==IH_FEATURE_MODEM_ELS81) {
		return SUB_DEAMON_NONE;
	}

	return SUB_DEAMON_PPP;
}

static void fixup_at_list(void)
{
	int network_type = MODEM_NETWORK_AUTO;
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	uint64_t modem_code = get_modem_code(gl_modem_idx);
	int i = 0, skip_net_pos = 0;

	if (modem_code==IH_FEATURE_MODEM_U8300C) {
		fixup_u8300c_at_list1();
		return;
	}

	//workaround
	if(modem_code==IH_FEATURE_MODEM_MC7710
		|| modem_code==IH_FEATURE_MODEM_MC7350
		|| modem_code==IH_FEATURE_MODEM_LE910
		|| modem_code==IH_FEATURE_MODEM_ME209
		|| modem_code==IH_FEATURE_MODEM_LP41
		|| modem_code==IH_FEATURE_MODEM_ELS31
		//|| modem_code==IH_FEATURE_MODEM_PVS8
		//|| modem_code==IH_FEATURE_MODEM_PHS8
		) {
			return;
	}

	if(modem_code==IH_FEATURE_MODEM_EC25
		|| modem_code==IH_FEATURE_MODEM_EC20
		|| modem_code==IH_FEATURE_MODEM_EP06
		|| modem_code==IH_FEATURE_MODEM_RM500U
		|| modem_code==IH_FEATURE_MODEM_RM500
		|| modem_code==IH_FEATURE_MODEM_RM520N
		|| modem_code==IH_FEATURE_MODEM_RG500){
		skip_net_pos = 1;
	}

	//FIXME: 6th position
#define AT_NET_POS	6
#define AT_NET_POS2	6
#define AT_NET_POS3	8
#define AT_NET_POS4	7
#define AT_NET_POS9 9
/*Workaround for LUA10: AT+CEREG? for LTE network checking; 
	AT+CREG? for 2G/3G network checking */
#define AT_TW_REG1_POS	9
#define AT_TW_REG2_POS	10
	if(modem_info->current_sim == SIM1){
		network_type = modem_config->network_type;
	}else{
		network_type = modem_config->network_type2;
	}

	for(i = 0; i < ARRAY_COUNT(gl_creg_at_lists); i++){
		gl_creg_at_lists[i] = -1;
	}

	switch(network_type) {
	case MODEM_NETWORK_5G4G:
		if(modem_code==IH_FEATURE_MODEM_FG360){
			gl_at_lists[AT_NET_POS4] = AT_CMD_5G4G;
		}else if(!skip_net_pos){
			gl_at_lists[AT_NET_POS] = AT_CMD_5G4G;
		}

		if(modem_code==IH_FEATURE_MODEM_RM500
			|| modem_code==IH_FEATURE_MODEM_RM520N
			|| modem_code==IH_FEATURE_MODEM_FM650
			|| modem_code==IH_FEATURE_MODEM_FG360
			|| modem_code==IH_FEATURE_MODEM_RM500U
			|| modem_code==IH_FEATURE_MODEM_RG500) {
			gl_creg_at_lists[0] = AT_CMD_C5GREG;
			gl_creg_at_lists[1] = AT_CMD_CEREG;
		}

		break;
	case MODEM_NETWORK_5G:
		if(modem_code==IH_FEATURE_MODEM_FG360){
			gl_at_lists[AT_NET_POS4] = AT_CMD_5G;
		}else if(!skip_net_pos){
			gl_at_lists[AT_NET_POS] = AT_CMD_5G;
		}

		if(modem_code==IH_FEATURE_MODEM_RM500
			|| modem_code==IH_FEATURE_MODEM_RM520N
			|| modem_code==IH_FEATURE_MODEM_FM650
			|| modem_code==IH_FEATURE_MODEM_FG360
			|| modem_code==IH_FEATURE_MODEM_RM500U
			|| modem_code==IH_FEATURE_MODEM_RG500) {
			gl_creg_at_lists[0] = AT_CMD_C5GREG;
			gl_creg_at_lists[1] = AT_CMD_CEREG;
		}

		break;
	case MODEM_NETWORK_4G:
		if(modem_code==IH_FEATURE_MODEM_PLS8) {
			gl_at_lists[AT_NET_POS2] = AT_CMD_4G;
		}else if(modem_code==IH_FEATURE_MODEM_PHS8
			|| modem_code==IH_FEATURE_MODEM_ME909) {
			gl_at_lists[AT_NET_POS3] = AT_CMD_4G;
		}else if(modem_code==IH_FEATURE_MODEM_FG360) {
			gl_at_lists[AT_NET_POS4] = AT_CMD_4G;
		}else if(!skip_net_pos){
			gl_at_lists[AT_NET_POS] = AT_CMD_4G;
		}

		if(modem_code==IH_FEATURE_MODEM_LUA10) {
			gl_at_lists[AT_TW_REG1_POS] = AT_CMD_CEREG;
			gl_at_lists[AT_TW_REG2_POS] = AT_CMD_AT;
		}else if(modem_code==IH_FEATURE_MODEM_U8300) {
			gl_at_lists[9] = AT_CMD_CEREG;
			gl_at_lists[11] = AT_CMD_AT;
		}else if(modem_code==IH_FEATURE_MODEM_RM500
				|| modem_code==IH_FEATURE_MODEM_RM520N
				|| modem_code==IH_FEATURE_MODEM_RM500U
				|| modem_code==IH_FEATURE_MODEM_RG500
				|| modem_code==IH_FEATURE_MODEM_EC20
				|| modem_code==IH_FEATURE_MODEM_EC25
				|| modem_code==IH_FEATURE_MODEM_FM650
				|| modem_code==IH_FEATURE_MODEM_FG360
				|| modem_code==IH_FEATURE_MODEM_EP06
				|| modem_code==IH_FEATURE_MODEM_NL668) {
			gl_creg_at_lists[0] = AT_CMD_CEREG;
		}else if(modem_code==IH_FEATURE_MODEM_MC7350) {
			gl_at_lists[6] = AT_CMD_CEREG;
		}else if(modem_code==IH_FEATURE_MODEM_PLS8
					|| modem_code==IH_FEATURE_MODEM_LARAR2
					|| modem_code==IH_FEATURE_MODEM_ELS81
					|| modem_code==IH_FEATURE_MODEM_ELS61) {
			gl_at_lists[10] = AT_CMD_CEREG;
			if(modem_code != IH_FEATURE_MODEM_LARAR2) {
				gl_at_lists[11] = AT_CMD_AT;
			}
			gl_at_lists[12] = AT_CMD_AT;
		}else if(modem_code==IH_FEATURE_MODEM_TOBYL201) {
			gl_at_lists[13] = AT_CMD_CEREG;
			gl_at_lists[15] = AT_CMD_AT;
		}
		break;
	case MODEM_NETWORK_3G:
		if(modem_code==IH_FEATURE_MODEM_PLS8) {
			gl_at_lists[AT_NET_POS2] = AT_CMD_3G;
		}else if(modem_code==IH_FEATURE_MODEM_PHS8
			|| modem_code==IH_FEATURE_MODEM_ME909) {
			gl_at_lists[AT_NET_POS3] = AT_CMD_3G;
		}else if(modem_code==IH_FEATURE_MODEM_FG360){
			gl_at_lists[AT_NET_POS4] = AT_CMD_3G;
		}else if(!skip_net_pos){
			gl_at_lists[AT_NET_POS] = AT_CMD_3G;
		}

		if(modem_code==IH_FEATURE_MODEM_LUA10) {
			gl_at_lists[AT_NET_POS] = AT_CMD_AUTO;
			gl_at_lists[AT_TW_REG1_POS] = AT_CMD_AT;
			gl_at_lists[AT_TW_REG2_POS] = AT_CMD_CREG;
		}else if(modem_code==IH_FEATURE_MODEM_U8300) {
			gl_at_lists[9] = AT_CMD_AT;
			gl_at_lists[11] = AT_CMD_CGREG;
		}else if(modem_code==IH_FEATURE_MODEM_RM500
				|| modem_code==IH_FEATURE_MODEM_RM520N
				|| modem_code==IH_FEATURE_MODEM_RM500U
				|| modem_code==IH_FEATURE_MODEM_RG500
				|| modem_code==IH_FEATURE_MODEM_EC20
				|| modem_code==IH_FEATURE_MODEM_EC25
				|| modem_code==IH_FEATURE_MODEM_FM650
				|| modem_code==IH_FEATURE_MODEM_FG360
				|| modem_code==IH_FEATURE_MODEM_EP06
				|| modem_code==IH_FEATURE_MODEM_NL668) {
			gl_creg_at_lists[0] = AT_CMD_CGREG;
		}else if(modem_code==IH_FEATURE_MODEM_PLS8
				|| modem_code==IH_FEATURE_MODEM_LARAR2
				|| modem_code==IH_FEATURE_MODEM_ELS81
				|| modem_code==IH_FEATURE_MODEM_ELS61) {
			gl_at_lists[10] = AT_CMD_AT;

			if(modem_code != IH_FEATURE_MODEM_LARAR2) {
				gl_at_lists[11] = AT_CMD_AT;
			}

			gl_at_lists[12] = AT_CMD_CREG;
		}else if(modem_code==IH_FEATURE_MODEM_TOBYL201) {
			gl_at_lists[13] = AT_CMD_AT;
			gl_at_lists[15] = AT_CMD_CREG;
		}
		break;
	case MODEM_NETWORK_EVDO:
		if(!skip_net_pos){
			gl_at_lists[AT_NET_POS] = AT_CMD_EVDO;
		}

		if(modem_code==IH_FEATURE_MODEM_EC20) {
			gl_creg_at_lists[0] = AT_CMD_CGREG;
			gl_creg_at_lists[1] = AT_CMD_CREG;
		}
		break;
	case MODEM_NETWORK_2G:
		if(modem_code==IH_FEATURE_MODEM_PLS8) {
			gl_at_lists[AT_NET_POS2] = AT_CMD_2G;
		}else if(modem_code==IH_FEATURE_MODEM_PHS8
			|| modem_code==IH_FEATURE_MODEM_ME909) {
			gl_at_lists[AT_NET_POS3] = AT_CMD_2G;
		}else if(!skip_net_pos){
			gl_at_lists[AT_NET_POS] = AT_CMD_2G;
		}

		if(modem_code==IH_FEATURE_MODEM_LUA10) {
			gl_at_lists[AT_TW_REG1_POS] = AT_CMD_AT;
			gl_at_lists[AT_TW_REG2_POS] = AT_CMD_CREG;
		}else if(modem_code==IH_FEATURE_MODEM_U8300) {
			gl_at_lists[9] = AT_CMD_AT;
			gl_at_lists[11] = AT_CMD_CGREG;
		}else if(modem_code==IH_FEATURE_MODEM_EC20
			|| modem_code==IH_FEATURE_MODEM_EC25) {
			gl_creg_at_lists[0] = AT_CMD_CREG;
		}else if(modem_code==IH_FEATURE_MODEM_PLS8
			|| modem_code==IH_FEATURE_MODEM_LARAR2
			|| modem_code==IH_FEATURE_MODEM_ELS81
			|| modem_code==IH_FEATURE_MODEM_ELS61) {
			gl_at_lists[10] = AT_CMD_AT;

			if(modem_code != IH_FEATURE_MODEM_LARAR2) {
				gl_at_lists[11] = AT_CMD_AT;
			}

			gl_at_lists[12] = AT_CMD_CREG;
		}else if(modem_code==IH_FEATURE_MODEM_TOBYL201) {
			gl_at_lists[13] = AT_CMD_AT;
			gl_at_lists[15] = AT_CMD_CREG;
		}
		break;
	case MODEM_NETWORK_3G2G:
		if (modem_code==IH_FEATURE_MODEM_PLS8){
			gl_at_lists[AT_NET_POS2] = AT_CMD_3G2G;
			if(modem_code==IH_FEATURE_MODEM_PLS8) {
				gl_at_lists[10] = AT_CMD_AT;
				gl_at_lists[11] = AT_CMD_AT;
				gl_at_lists[12] = AT_CMD_CREG;
			}
		}else if(modem_code==IH_FEATURE_MODEM_ME909) {
			gl_at_lists[AT_NET_POS3] = AT_CMD_3G2G;
		}
		break;
	case MODEM_NETWORK_AUTO:
	default:
		if(modem_code==IH_FEATURE_MODEM_PLS8) {
			gl_at_lists[AT_NET_POS2] = AT_CMD_AUTO;
		}else if(modem_code==IH_FEATURE_MODEM_PHS8
			|| modem_code==IH_FEATURE_MODEM_ME909) {
			gl_at_lists[AT_NET_POS3] = AT_CMD_AUTO;
		}else if(modem_code==IH_FEATURE_MODEM_FG360){
			gl_at_lists[AT_NET_POS4] = AT_CMD_AUTO;
		}else if(!skip_net_pos){
			gl_at_lists[AT_NET_POS] = AT_CMD_AUTO;
		}

		if(modem_code==IH_FEATURE_MODEM_LUA10) {
			gl_at_lists[AT_TW_REG1_POS] = AT_CMD_CEREG;
			gl_at_lists[AT_TW_REG2_POS] = AT_CMD_CREG;
		}else if(modem_code==IH_FEATURE_MODEM_U8300) {
			gl_at_lists[9] = AT_CMD_CEREG;
			gl_at_lists[11] = AT_CMD_AT;
		}else if(modem_code==IH_FEATURE_MODEM_RM500
				|| modem_code==IH_FEATURE_MODEM_RM520N
				|| modem_code==IH_FEATURE_MODEM_FM650
				|| modem_code==IH_FEATURE_MODEM_FG360
				|| modem_code==IH_FEATURE_MODEM_RM500U
				|| modem_code==IH_FEATURE_MODEM_RG500) {
			gl_creg_at_lists[0] = AT_CMD_C5GREG;
			gl_creg_at_lists[1] = AT_CMD_CEREG;
			gl_creg_at_lists[2] = AT_CMD_CGREG;
		}else if(modem_code==IH_FEATURE_MODEM_EP06
			|| modem_code==IH_FEATURE_MODEM_NL668) {
			gl_creg_at_lists[0] = AT_CMD_CEREG;
			gl_creg_at_lists[1] = AT_CMD_CGREG;
		}else if(modem_code==IH_FEATURE_MODEM_EC20
			|| modem_code==IH_FEATURE_MODEM_EC25) {
			gl_creg_at_lists[0] = AT_CMD_CEREG;
			gl_creg_at_lists[1] = AT_CMD_CGREG;
			gl_creg_at_lists[2] = AT_CMD_CREG;
		}else if(modem_code==IH_FEATURE_MODEM_PLS8
					|| modem_code==IH_FEATURE_MODEM_LARAR2
					|| modem_code==IH_FEATURE_MODEM_ELS81
					|| modem_code==IH_FEATURE_MODEM_ELS61) {
			gl_at_lists[10] = AT_CMD_CEREG;

			if(modem_code != IH_FEATURE_MODEM_LARAR2) {
				gl_at_lists[11] = AT_CMD_AT;
			}

			gl_at_lists[12] = AT_CMD_AT;
		}else if(modem_code==IH_FEATURE_MODEM_TOBYL201) {
			gl_at_lists[13] = AT_CMD_CEREG;
			gl_at_lists[15] = AT_CMD_AT;
		}

		break;
	}

	if(modem_code==IH_FEATURE_MODEM_ME909) {
		if (ih_key_support("FH19") 
				|| ih_key_support("FH20")
				|| ih_key_support("TH09")) {
			gl_at_lists[3] = AT_CMD_CURC;
		}
	}

	if(modem_code==IH_FEATURE_MODEM_TOBYL201) {
		//if (!ih_key_support("FB38") ) {
			gl_at_lists[2] = AT_CMD_AT;
			gl_at_lists[18] = AT_CMD_CGACT1;
		//}
	}

	if(modem_code==IH_FEATURE_MODEM_PLS8) {
		if (ih_key_support("FS59") 
				|| ih_key_support("FS39")) {
			gl_at_lists[2] = AT_CMD_AT;
		}
	}
	
}

static void pvs8_activation_action(AT_CMD *pcmd, char *buf, MODEM_INFO *modem_info, MODEM_CONFIG *modem_config)
{
	int ret;
	char at_cmd[64]={0};

	ret = check_modem_vphonenum(pcmd, buf, 0, NULL, modem_info);	
	if(ret <0){
		if(modem_config->activate_enable && modem_config->activate_no[0]){
			if(gl_pvs8_activation_retry_cnt >= MAX_PVS8_ACTIVATION_RETRY) return;
			snprintf(at_cmd, sizeof(at_cmd), "AT+CDV%s\r\n",modem_config->activate_no);
			LOG_DB("PVS8 module active cmd %s,rety (%d/%d)", at_cmd, gl_pvs8_activation_retry_cnt++, MAX_PVS8_ACTIVATION_RETRY);
			ret = send_at_cmd_sync_timeout(gl_commport, at_cmd, buf, sizeof(buf), 
				1, "ACTIVATION:", 100000);	
			if(ret<0){
				LOG_IN("PVS8 module activate err");
			}else {
				if(strstr(buf, "Success")){
					buf[0]='\0';
					send_at_cmd_sync_timeout(gl_commport, pcmd->atc, buf, sizeof(buf), 0,"OK",3000);
					ret = check_modem_vphonenum(pcmd, buf, 0, NULL, modem_info);
					if(ret<0){
						LOG_IN("PVS8 module activate err, get phonoe number Failure");
					}else {
						pvs8_activation = TRUE;
						LOG_IN("PVS8 module activate sucessfully! phone number %s",modem_info->phonenum);
						strlcpy(modem_config->phone_no, modem_info->phonenum, sizeof(modem_config->phone_no));
					}
				}else {
					LOG_IN("PVS8 module activation Failure");
				}
			}
		}else {
			LOG_IN("PVS8 module activation disable");
		}
	}else {	
		pvs8_activation = TRUE;
		LOG_IN("PVS8 module has already activated, phone number %s",modem_info->phonenum);
		strlcpy(modem_config->phone_no, modem_info->phonenum, sizeof(modem_config->phone_no));
	}
}

static int check_pvs8_tetheredNai(void)
{
	char buf[2048]={0};
	char *p = NULL;
	int retval = -1;

	retval = send_at_cmd_sync_timeout(gl_commport, "AT^SCFG?\r\n", buf, sizeof(buf), 1, "SCFG", 10000);	
	if (retval < 0) {
		LOG_IN("PVS8 get TetheredNai status failed!");
		return -1;
	}

	p = strstr(buf, "CDMA/TetheredNai");
	if (p){
		p = strchr(p, ',');
		p = strchr(p, '"') +1;
		p = strtok(p, "\"");
		if (p) {
			LOG_IN("NAI : %s", p);
			if (strstr(p, "enable")) {
				LOG_IN("CDMA/TetheredNai is enabled, should disable it first.");
				return NAI_ENABLE;
			}else if (strstr(p, "disable")) {
				LOG_IN("CDMA/TetheredNai is disabled.");
				return NAI_DISABLE;
			}else {
				LOG_ER("PVS8 get TetheredNai status failed!");
				return -1;
			}
		}
	}else {
		LOG_IN("PVS8 check TetheredNai status failed!");
		return -1;
	}
	return -1;
}

static int disable_pvs8_tetheredNai(void) 
{
	int retval = -1;
	char buf[64]={0}, at_cmd[64];

	retval = check_pvs8_tetheredNai();
	if (NAI_ENABLE == retval) {
		sprintf(at_cmd, "AT^SCFG=\"cdma/tetheredNai\", \"disabled\"\r\n");
		retval = send_at_cmd_sync_timeout(gl_commport, at_cmd, buf, sizeof(buf), 1, "SCFG", 10000);	
		if (retval < 0) {
			LOG_IN("PVS8 set TetheredNai value failed!");
			return -1;
		}

		retval = check_pvs8_tetheredNai();
		if (NAI_DISABLE != retval) {
			return -1;
		}else {
			return 0;
		}
	} else if (NAI_DISABLE == retval){
		return 0;
	}
	return -1;
}

BOOL is_vzw_sim(void)
{
	char buf[32] = {0};
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	
	snprintf(buf, sizeof(buf), "%s", modem_info->imsi);
	buf[6] = '\0';
	return match_vzw_plmn(atoi(buf)) ? TRUE : FALSE;
}

static int get_pdp_context_id(int wwan_id)
{
	char buf[32] = {0};
	int cid = DEFAULT_CID, operator = SIM_OPER_AUTO;

	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	MODEM_CONFIG *modem_config = &gl_myinfo.priv.modem_config;

	if(modem_info->current_sim == SIM1){
		operator = modem_config->sim_operator[0];
	}else{
		operator = modem_config->sim_operator[1];
	}

	if(wwan_id == WWAN_ID2){
		cid = VZW_CLASS6_CID;
	}else{
		//verizon sims use cid3 to access internet
		switch(operator){
			case SIM_OPER_VERIZON: 
				cid = VZW_CLASS3_CID; 
				break;
			case SIM_OPER_OTHERS: 
				cid = DEFAULT_CID; 
				break;
			default:
				//make sure IMSI have been obtained
				snprintf(buf, sizeof(buf), "%s", modem_info->imsi);
				buf[6] = '\0';
				cid = match_vzw_plmn(atoi(buf)) ? VZW_CLASS3_CID : DEFAULT_CID;
				break;
		}
	}

	return cid;
}

int get_pdp_type_setting(void)
{
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);

	if(modem_support_ipv6()){
		return (modem_info->current_sim == SIM2) ? modem_config->pdp_type[1] : modem_config->pdp_type[0];
	}
	
	return PDP_TYPE_IPV4;
}

/*
use AT ppp 
*/
static int at_ppp(char *commdev, char *datadev, PPP_CONFIG *info, MODEM_CONFIG *config, MODEM_INFO *modem_info, BOOL debug)
{
	int ret = -1;
	char buf[1024]={0};
	char at_cmd[128]={0};
	PPP_PROFILE *p_profile;
	int profile_idx = 0;

	//check profile if valid
	if(modem_info->current_sim==SIM1) profile_idx=info->profile_idx;
	else if(modem_info->current_sim==SIM2) profile_idx=info->profile_2nd_idx;	
	if(info->profiles[profile_idx-1].type==PROFILE_TYPE_NONE) {
		p_profile =  &info->default_profile;
	} else {
		p_profile = info->profiles + profile_idx -1;
	}
	ret = send_at_cmd_sync(commdev, "AT\r\n", NULL, 0, 0);
	if(ret<0) {
		send_at_cmd_sync(commdev, "AT\r\n", NULL, 0, 0);
		LOG_IN("AT PPP %s is not capable to exec AT,ret %d",commdev, ret);
		return ret;
	}
	if(p_profile->apn[0])
		snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1,\"IP\",\"%s\"\n", p_profile->apn);

	ret = send_at_cmd_sync_timeout(commdev, at_cmd, buf, sizeof(buf), 0,"OK", 8000);
	if(ret<0) {
		LOG_IN("AT PPP %s is not capable to exec AT_CGDCONT ret %d",commdev, ret);
		return ret;
	}
	memset(at_cmd, 0, sizeof(at_cmd));
	snprintf(at_cmd, sizeof(at_cmd),  "ATD%s\n", p_profile->dialno);
	ret = send_at_cmd_sync_timeout(commdev, at_cmd, buf, sizeof(buf), 0, "CONNECT", info->timeout);
	if(ret<0) {
		LOG_IN("AT PPP %s is not capable to CONNECT,ret %d",commdev, ret);
		return ret;
	}
	
	at_ppp_build_config(datadev, info, config, modem_info, debug);
	return ret;
}

static int modem_swwan_data_connect(char *commdev, int adapter, int cid, int connect, BOOL debug)
{
	char at_cmd[64]={'\0'};
	int ret = -1;

	snprintf(at_cmd, sizeof(at_cmd), "AT^SWWAN=%d,%d,%d\r\n", connect, cid, adapter);
	ret = send_at_cmd_sync_timeout(commdev, at_cmd, NULL, 0, debug, "OK", 5000);
	if(ret<0) {
		if (connect) {
			LOG_IN("%s execute AT command %s faild, ret :%d",commdev, at_cmd, ret);
		}
		return ret; 
	}    

	return ERR_OK;
}

static int modem_rndis_data_connect(char *commdev, int adapter, int cid, int connect, BOOL debug)
{
	AT_CMD *pcmd = NULL; 
	char buf[256] = {0};
	char at_cmd[64]={0};
	int i = 0;
	int ret = -1;
	
	pcmd = find_at_cmd(AT_CMD_GTRNDIS);
	
	snprintf(at_cmd, sizeof(at_cmd), "AT+GTRNDIS=%d,%d\r\n", connect, cid);

	if(connect){
retry:
		ret = send_at_cmd_sync_timeout(commdev, at_cmd, NULL, 0, debug, "OK", 5000);

		bzero(buf, sizeof(buf));
		ret = send_at_cmd_sync(commdev, pcmd->atc, buf, sizeof(buf), debug);
		if(ret == 0 && pcmd->at_cmd_handle){
			ret = pcmd->at_cmd_handle(pcmd, buf, 0, NULL, NULL);
			if(ERR_AT_OK != ret){
				if(i++ < 3){
					goto retry;
				}else{
					LOG_IN("%s execute AT command %s faild",commdev, at_cmd);
				}
			}
		}
	}else{
		ret = send_at_cmd_sync_timeout(commdev, at_cmd, NULL, 0, debug, "OK", 5000);
	}

	return ret;
}

static int wait_linkup_event(const char *iface, int timeout)
{
	char old_s[32] = {0};
	char file[128] = {0};
	char buff[32] = {0};
	time_t tm_start = time(NULL);

	if(!iface || !iface[0]){
		return ERR_INVAL;
	}

	snprintf(file, sizeof(file), "/sys/class/net/%s/operstate", iface);
	if(f_read_string(file, old_s, sizeof(old_s)) <= 0){
		return ERR_INVAL;
	}

	while((time(NULL) - tm_start) < timeout){
		if(f_read_string(file, buff, sizeof(buff)) <= 0){
			break;
		}

		if(strncmp(old_s, "up", 2) && !strncmp(buff, "up", 2)){
			return ERR_OK;
		}

		strlcpy(old_s, buff, sizeof(old_s));
		usleep(100 * 1000); //wait 100ms
	}

	return ERR_FAILED;
}

static int check_modem_cgpaddr_timeout(char *commdev, int cid, int timeout, BOOL debug)
{
	char at_rsp[512]={0};
	time_t tm_start = time(NULL);
	AT_CMD *pcmd = find_at_cmd(AT_CMD_CGPADDR);

	while(pcmd && pcmd->at_cmd_handle && (time(NULL) - tm_start) < timeout){
		memset(at_rsp, 0, sizeof(at_rsp));
		snprintf(pcmd->atc, sizeof(pcmd->atc), "AT+CGPADDR=%d\r\n", cid);
		if(send_at_cmd_sync(commdev, pcmd->atc, at_rsp, sizeof(at_rsp), debug)){
			sleep(1);
			continue;
		}

		if(pcmd->at_cmd_handle(pcmd, at_rsp, 0, NULL, NULL) == ERR_OK){
			return ERR_OK;
		}

		sleep(1);
	}

	return ERR_FAILED;
}

static int check_rm520n_cgcontrdp(char *commdev, int cid, struct in_addr *dns1, struct in_addr *dns2, BOOL debug)
{
	int i, ret;
	char at_cmd[256] = {0};
	char at_rsp[1024] = {0};
	char *p = NULL;
	char *q = NULL;
	struct in_addr v4_addr;

	//+CGCONTRDP: 1,0,"3gnet","10.163.224.65",,"119.7.7.7","119.6.6.6"
	//+CGCONTRDP: 1,0,"internet","10.98.135.30","36.8.132.105.3.16.25.30.23.100.31.54.251.42.144.254", "254.128.0.0.0.0.0.0.0.0.0.0.0.0.0.1","119.7.7.7" "36.8.128.1.112.0.0.0.0.0.0.0.0.0.0.1","119.6.6.6"
	snprintf(at_cmd, sizeof(at_cmd), "AT+CGCONTRDP=%d\r\n", cid);
	ret = send_at_cmd_sync_timeout(commdev, at_cmd, at_rsp, sizeof(at_rsp), debug, "OK", 3000);
	if(ret < 0){
		LOG_IN("can not get ip and dns.");
		return ret;
	}

	//skip ipv4 address
	p = strstr(at_rsp, "+CGCONTRDP:");
	for(i = 0; i < 4 && p; i++){
		strsep(&p, ",");
	}

	i = 0;
	while(i < 2 && p && strsep(&p, "\"")){
		if(p == NULL){
			break;
		}

		q = strsep(&p, "\"");
		if(q && inet_pton(AF_INET, q, &v4_addr) == 1){
			i++;
			if(dns1 && i == 1){
				memcpy(dns1, &v4_addr, sizeof(v4_addr));
			}else if(dns2 && i == 2){
				memcpy(dns2, &v4_addr, sizeof(v4_addr));
			}
		}
	}

	return i;
}

static int modem_qmap_connect(char *commdev, int cid, BOOL debug)
{
	int ret = -1;
	char at_cmd[64]={0};
	char buff[128] = {0};
	char mac_address[32] = {0};
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);

	//clear dnses
	memset(&(pinfo->dns1), 0, sizeof(pinfo->dns1));
	memset(&(pinfo->dns2), 0, sizeof(pinfo->dns2));

	//we need to configure "MPDN_rule" again if cid is changed.
	if(gl.default_cid != cid){
		if(gl.default_cid < 0){
			//disable IPv6 Stateless Autoconfigration
			f_write_string("/proc/sys/net/ipv6/conf/"APN1_WWAN"/accept_ra", "0", 0, 0);

			get_random_hw_addr(mac_address, sizeof(mac_address), 1);
			eval("ip", "link", "set", "dev", APN1_WWAN, "address", mac_address);
		}else{
			f_read("/sys/class/net/"APN1_WWAN"/address", buff, sizeof(buff));
			sscanf(buff, "%[0-9:a-f]", mac_address);
		}

		send_at_cmd_sync_timeout(commdev, "AT+QMAP=\"MPDN_rule\",0\r\n", NULL, 0, debug, "OK", 8000);
	
		if(ih_key_support("NRQ1")){
			snprintf(at_cmd, sizeof(at_cmd), "AT+QMAP=\"MPDN_rule\",0,%d,0,1,0,\"%s\"\r\n", cid, mac_address);
		}else{ //NRQ1
			snprintf(at_cmd, sizeof(at_cmd), "AT+QMAP=\"MPDN_rule\",0,%d,0,1,0,\"FF:FF:FF:FF:FF:FF\"\r\n", cid);
		}

		ret = send_at_cmd_sync_timeout(commdev, at_cmd, NULL, 0, debug, "OK", 8000);
		if(ret){
			LOG_IN("%s execute AT command %s faild, ret: %d", commdev, at_cmd, ret);
			return ret;
		}

		gl.default_cid = cid;
	}else{
		snprintf(at_cmd, sizeof(at_cmd), "AT+QMAP=\"connect\",0,0\r\n");
		send_at_cmd_sync_timeout(commdev, at_cmd, NULL, 0, debug, "OK", 8000);
	}

	snprintf(at_cmd, sizeof(at_cmd), "AT+QMAP=\"connect\",0,1\r\n");
	ret = send_at_cmd_sync_timeout(commdev, at_cmd, NULL, 0, debug, "OK", 8000);
	if(ret<0) {
		LOG_IN("%s execute AT command %s faild, ret: %d", commdev, at_cmd, ret);
		return ret;
	}

	if(ih_license_support(IH_FEATURE_PCIE2ETH_RTL8125B)){
		wait_linkup_event(CELL_SWPORT, 8);
	}

	ret = check_modem_cgpaddr_timeout(commdev, cid, 10, debug);
	if(ret == ERR_OK){
		check_rm520n_cgcontrdp(commdev, cid, &(pinfo->dns1), &(pinfo->dns2), 0);
	}

	return ret;
}

/*
liyb:Before use port wwan0 to connetct to network,we should config the modem and activate the wwan ip connection.
*/
static int active_modem_data_connection(char *commdev, PPP_CONFIG *info, MODEM_CONFIG *config, MODEM_INFO *modem_info, BOOL debug) 
{
	int ret = -1;
	char at_cmd[512]={0};
	PPP_PROFILE *p_profile = NULL;
	PPP_PROFILE *p_2nd_wwan_profile = NULL;
	uint64_t my_modem_code = 0;
	int pdp_type = PDP_TYPE_IPV4;
	int cid = DEFAULT_CID;

	my_modem_code = get_modem_code(gl_modem_idx);
	
	get_profile_setting(&p_profile, &p_2nd_wwan_profile, info, config, modem_info); 
	
	if(ih_key_support("FS39")
		|| ih_key_support("NAVA")
		|| ih_key_support("NATO")
		|| ih_key_support("NRQ1")
		|| ih_key_support("FQ39")
		|| ih_key_support("FQ38")){
		pdp_type = PDP_TYPE_IPV4V6;
		cid = get_pdp_context_id(WWAN_ID1);
	}else if(modem_support_ipv6()){
		pdp_type = get_pdp_type_setting();
	}

	ret = send_at_cmd_sync(commdev, "AT\r\n", NULL, 0, debug);
	if(ret<0) {
		sleep(3);
		ret = send_at_cmd_sync(commdev, "AT\r\n", NULL, 0, debug);
		if(ret < 0){
			LOG_IN("AT dhcpc %s is not capable to exec AT",commdev);
			return ret;
		}
	}

#if 0
	ret = send_at_cmd_sync_timeout(commdev, "AT+CGATT=1\r\n", NULL, 0, debug,"OK", 5000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+CGATT ret %d.",commdev, ret);
	}
#endif
	
	strlcpy(modem_info->used_apn[0], p_profile->apn, sizeof(modem_info->used_apn[0]));
	if(p_2nd_wwan_profile){
		strlcpy(modem_info->used_apn[1], dual_apn_is_enabled() ? p_2nd_wwan_profile->apn : "", sizeof(modem_info->used_apn[1]));
	}
	
	if (my_modem_code==IH_FEATURE_MODEM_FG360) {
		return ERR_OK; //skip
	}

	if(p_profile->apn[0]) {
		if(!strcmp(USE_BLANK_APN, p_profile->apn)/* && my_modem_code==IH_FEATURE_MODEM_PLS8*/) { 
			setup_modem_pdp_parameters(commdev, cid, pdp_type, NULL, debug);
		}else {
			setup_modem_pdp_parameters(commdev, cid, pdp_type, p_profile->apn, debug);
		}
	}
	
	if(p_2nd_wwan_profile){
		setup_modem_pdp_parameters(commdev, get_pdp_context_id(WWAN_ID2), pdp_type, p_2nd_wwan_profile->apn, debug);
	}

#if 0
	switch (my_modem_code) {
		case IH_FEATURE_MODEM_PLS8:
			snprintf(at_cmd, sizeof(at_cmd),  "AT^SGAUTH=1,%d,%s,%s\r\n",\
				(p_profile->auth_type == AUTH_TYPE_PAP) ? 1:2,\
				p_profile->password[0] ? p_profile->password : "\" \"",\
				p_profile->username[0] ? p_profile->username : "\" \"");
			break;
		case IH_FEATURE_MODEM_LP41:
			snprintf(at_cmd, sizeof(at_cmd),  "AT%%PDNSET=1,\"%s\",\"IPv4v6\",\"%s\",\"%s\",\"%s\"\r\n",\
				p_profile->apn,\
				(p_profile->auth_type == 1) ? "PAP":"CHAP",\
				p_profile->username[0] ? p_profile->username : "",\
				p_profile->password[0] ? p_profile->password : "");
			break;
		case IH_FEATURE_MODEM_ME909:
			if ((ih_key_support("FH09") || ih_key_support("FH19"))) {
				snprintf(at_cmd, sizeof(at_cmd),  "AT^NDISDUP=1,1,\"%s\",\"%s\",\"%s\",%s\r\n",\
					p_profile->apn,\
					p_profile->username[0] ? p_profile->username : " ",\
					p_profile->password[0] ? p_profile->password : " ",\
					(p_profile->auth_type==AUTH_TYPE_PAP) ? "1":"2");
			}else {
				snprintf(at_cmd, sizeof(at_cmd),  "AT^NDISDUP=1,1,\"%s\",\"%s\",\"%s\",%s\r\n",\
					p_profile->apn,\
					p_profile->username[0] ? p_profile->username : " ",\
					p_profile->password[0] ? p_profile->password : " ",\
					(p_profile->auth_type==AUTH_TYPE_PAP) ? "1":((p_profile->auth_type==AUTH_TYPE_CHAP) ? "2":"3"));
			}
			break;
		case IH_FEATURE_MODEM_ELS61:
			break;
		default:
			return -1;
	}

	ret = send_at_cmd_sync(commdev, at_cmd, NULL, 0, debug);
	if(ret<0) {
		LOG_IN("AT DHCPC %s is not capable to exec %s!",commdev, at_cmd);
	}
#endif	

	setup_modem_auth_parameters(commdev, p_profile, cid, debug);
	if(p_2nd_wwan_profile){
		setup_modem_auth_parameters(commdev, p_2nd_wwan_profile, get_pdp_context_id(WWAN_ID2), debug);
	}

	//sleep(1);//liyb:需要等1s,使刚刚配置的参数起作用后再做激活操作.
	
	memset(at_cmd, 0, sizeof(at_cmd));
	switch (my_modem_code) {
		case IH_FEATURE_MODEM_PLS8:
		case IH_FEATURE_MODEM_ELS81:
		case IH_FEATURE_MODEM_ELS61:
			modem_swwan_data_connect(commdev, WWAN_PORT_CELLULAR1, 1, CELLULAR_DATA_DISCONNECT, debug);
			sleep(1);
			ret = modem_swwan_data_connect(commdev, WWAN_PORT_CELLULAR1, 1, CELLULAR_DATA_CONNECT, debug);
			return ret;
#if 0
		case IH_FEATURE_MODEM_ELS31:
			snprintf(at_cmd, sizeof(at_cmd),  "AT^SICA=1,3\r\n");
			break;
#endif
		case IH_FEATURE_MODEM_FM650:
			modem_rndis_data_connect(commdev, WWAN_PORT_CELLULAR1, cid, CELLULAR_DATA_DISCONNECT, debug);
			sleep(1);
			ret = modem_rndis_data_connect(commdev, WWAN_PORT_CELLULAR1, cid, CELLULAR_DATA_CONNECT, debug);
			return ret;
		case IH_FEATURE_MODEM_LP41:
			snprintf(at_cmd, sizeof(at_cmd), "AT%%PDNACT=1,1\r\n");
			break;
		case IH_FEATURE_MODEM_ME909:
			if ((ih_key_support("FH09") || ih_key_support("FH19"))) {
				snprintf(at_cmd, sizeof(at_cmd),  "AT^NDISDUP=1,1,\"%s\",\"%s\",\"%s\",%s\r\n",\
					p_profile->apn,\
					p_profile->username[0] ? p_profile->username : " ",\
					p_profile->password[0] ? p_profile->password : " ",\
					(p_profile->auth_type==AUTH_TYPE_PAP) ? "1":"2");
			}else {
				snprintf(at_cmd, sizeof(at_cmd),  "AT^NDISDUP=1,1,\"%s\",\"%s\",\"%s\",%s\r\n",\
					p_profile->apn,\
					p_profile->username[0] ? p_profile->username : " ",\
					p_profile->password[0] ? p_profile->password : " ",\
					(p_profile->auth_type==AUTH_TYPE_PAP) ? "1":((p_profile->auth_type==AUTH_TYPE_CHAP) ? "2":"3"));
			}
			break;
		case IH_FEATURE_MODEM_RM500U:
			snprintf(at_cmd, sizeof(at_cmd), "AT+QNETDEVCTL=%d,0,0\r\n", cid);
			send_at_cmd_sync3(commdev, at_cmd, NULL, 0, debug);

			sleep(1);
			snprintf(at_cmd, sizeof(at_cmd), "AT+QNETDEVCTL=%d,1,0\r\n", cid);
			break;
		case IH_FEATURE_MODEM_RM520N:
			//TODO: support dual apn
			modem_qmap_connect(commdev, cid, debug);
			return ERR_OK;
		default:
			return ERR_OK;
	}

	ret = send_at_cmd_sync_timeout(commdev, at_cmd, NULL, 0, debug, "OK", 5000);
	if(ret<0) {
		LOG_IN("AT DHCPC %s is not capable to connect",commdev);
		return ret;
	}

	return ret;
}

static int pvs8_throttling()
{
	int ret = 0;
	if(gl_ppp_redial_cnt < 3 ){
		if(pvs8_throttling_net == MODEM_NETWORK_2G){
			ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=2\r\n", NULL, 0, 1);
			if(ret<0) {
				ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=2\r\n", NULL, 0, 1);
			}
			set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);
			sleep(20);
		}
	} else if(gl_ppp_redial_cnt == 3 ){
		if(pvs8_throttling_net == MODEM_NETWORK_AUTO){
			ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=2\r\n", NULL, 0, 1);
			if(ret<0) {
				ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=2\r\n", NULL, 0, 1);
			}
			gl_ppp_redial_cnt = 1;
			ret = pvs8_throttling_net = MODEM_NETWORK_2G;
			set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);
			sleep(20);
			return ret;
		}else if(pvs8_throttling_net == MODEM_NETWORK_2G){
			ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=4\r\n", NULL, 0, 1);
			if(ret<0) {
				ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=4\r\n", NULL, 0, 1);
			}
			ret = pvs8_throttling_net = MODEM_NETWORK_3G;
			set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);
			sleep(20);
		}
	} else if(gl_ppp_redial_cnt>3){
		if(pvs8_throttling_net == MODEM_NETWORK_3G){
			ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=2\r\n", NULL, 0, 1);
			if(ret<0) {
				ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=2\r\n", NULL, 0, 1);
			}
			ret = pvs8_throttling_net = MODEM_NETWORK_2G;
			set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);
			sleep(20);
			return ret;
		}else {
			ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=4\r\n", NULL, 0, 1);
			if(ret<0) {
				ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=4\r\n", NULL, 0, 1);
			}
			set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);
			sleep(20);
			ret = pvs8_throttling_net = MODEM_NETWORK_3G;
		}	
	}

	if (gl_ppp_redial_cnt++>MAX_PPP_REDIAL) {
		LOG_ER("too many ppp redials, restart system...");
		system("reboot");
		while(1) sleep(1000);
	}
	return ret;
}


static int modem_force_ps_reattch(void)
{
	int ret = -1;

	if (get_modem_code(gl_modem_idx) != IH_FEATURE_MODEM_U8300C) {
		return 0;
	}

	LOG_IN(" Force to reattach from packet domain");
	ret = send_at_cmd_sync(gl_commport, "AT+CGATT=0\r\n", NULL, 0, 1);
	if(ret<0) {
		LOG_IN(" Force to deattach from packet domain failed! ret %d", ret);
	}

	sleep(2);	

	ret = send_at_cmd_sync(gl_commport, "AT+CGATT=1\r\n", NULL, 0, 1);
	if(ret<0) {
		LOG_IN(" Force to attach to packet domain failed! ret %d", ret);
	}

	return ret;
}

//liyb:OTA check and start update.
static int vzw_ota_update()
{
	int ret = -1;

	ret = send_at_cmd_sync(gl_commport, "AT#OTAUIDM=0\r\n", NULL, 0, 1);
	if(ret<0) {
		LOG_IN(" OTA check and start update failed! ret %d", ret);
	}
	
	return ret;
}

int check_ublox_mno_config(char *commdev, char *dst_value, int verbose)
{
	char tmp[128] = {0};
	char cmd[128] = {0};
	int ret = -1;

	if (!commdev) {
		return -1;
	}  

	if (!dst_value) {         
		return -1;
	}  

	ret = send_at_cmd_sync_timeout(commdev, "AT+UMNOCONF?\r\n", tmp, sizeof(tmp), 1, dst_value, 5000);
	if (ret < 0) {
		syslog(LOG_INFO, "need to init modem's MNO config");
		goto do_mno_config;
	}

	return 1;

do_mno_config:
	snprintf(cmd, sizeof(cmd), "AT+UMNOCONF=%s\r\n", dst_value);
	ret = send_at_cmd_sync_timeout(commdev, cmd, tmp, sizeof(tmp), 1, "OK", 5000);
	if (ret < 0) {
		syslog(LOG_ERR, "exec +UMNOCONF failed!");
		return -1;
	}  

	return 0;
}

/*liyb : check modem's current apn config.*/
static int check_modem_apn_config(char *commdev, int cid, char *apn, int apn_len, char *proto, int proto_len, BOOL debug)
{
	int ret = -1;
	int cid_t = 0;
	char buf[2048]={0};
	char *p = NULL;
	char *pos = NULL;
	char apn_t[64] = {0};
	char proto_t[8] = {0};
	char match_str[] = {"+CGDCONT:"};

	ret = send_at_cmd_sync_timeout(commdev, "AT+CGDCONT?\r\n", buf, sizeof(buf), debug, "OK", 5000);	
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+CGDCONT?,ret %d",commdev, ret);
		return ERR_FAILED;
	}

	pos = buf;
	while(pos && pos[0]){
		p = strstr(pos, match_str);
		if (!p){
			break;
		}
		
		p += strlen(match_str);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;

		if(sscanf(p, "%d,\"%[^\"]\",\"%[^\"]\",", &cid_t, proto_t, apn_t) < 3){
			break;
		}

		if(cid == cid_t){
			LOG_IN("cid%d proto: %s apn: %s", cid_t, proto_t, apn_t);

			if(proto){
				proto_t[sizeof(proto_t)-1] = '\0';
				strlcpy(proto, proto_t, proto_len);
			}

			if(apn){
				apn_t[sizeof(apn_t)-1] = '\0';
				strlcpy(apn, apn_t, apn_len);
			}

			return ERR_OK;
		}
		
		pos = p;
	};

	return ERR_FAILED;
}

static int setup_modem_pdp_parameters(char *commdev, int pdp_index, int pdp_type, char *apn, BOOL debug)
{
	char at_cmd[128]={0};
	int ret = -1;
	char buf[32]={0};

	if (apn)
		sprintf(at_cmd, "AT+CGDCONT=%d,\"%s\",\"%s\"\r\n", pdp_index, gl_pdp_type_str[pdp_type], apn);
	else 
		sprintf(at_cmd, "AT+CGDCONT=%d,\"%s\"\r\n", pdp_index, gl_pdp_type_str[pdp_type]);
	
	ret = send_at_cmd_sync_timeout(commdev, at_cmd, buf, sizeof(buf), debug, "OK", 5000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec %s ret %d,maybe pdp is already actived.",commdev, at_cmd, ret);
	}
	return ERR_OK;
}

static int setup_modem_auth_parameters(char *commdev, PPP_PROFILE *p_profile, int pdp_index, BOOL debug)
{
	char at_cmd[256]={0};
	int ret = -1;
	int auth_type = AUTH_TYPE_AUTO;
	char buf[32]={0};

	if (!p_profile->username[0] || !p_profile->password[0]) {
		if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300
			  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC20
			  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC25
			  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500
			  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM520N
			  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RG500
			  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EP06
			  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300C) {
			sprintf(at_cmd, "AT$QCPDPP=%d\r\n", pdp_index);
		}else if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_RM500U) {
			snprintf(at_cmd, sizeof(at_cmd),  "AT+QICSGP=%d,%d,\"%s\",\"\",\"\",0\r\n", pdp_index, get_pdp_type_setting(), p_profile->apn);
		}else if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_PLS8) {
			snprintf(at_cmd, sizeof(at_cmd),  "AT^SGAUTH=%d\r\n", pdp_index);
		}else if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ME909) {
			snprintf(at_cmd, sizeof(at_cmd), "AT^AUTHDATA=%d\r\n", ((ih_key_support("FH09") || ih_key_support("FH19")) ? 16:0));
		}else if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS31) {
			snprintf(at_cmd, sizeof(at_cmd),  "AT+CGAUTH=3\r\n");
		}else if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS61
				|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS81) {
			snprintf(at_cmd, sizeof(at_cmd),  "AT^SGAUTH=%d\r\n", pdp_index);
		}else if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_FM650
				|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_FG360
				|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_NL668) {
			snprintf(at_cmd, sizeof(at_cmd),  "AT+MGAUTH=%d,0\r\n", pdp_index);
		}else {
			return ERR_OK;
		}

		goto exec_cmd;
	}

	if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC20
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC25
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM520N
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RG500
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EP06
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300C) {
		if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300C) {//U9300C need config ppp auth parameters on EVDO/CDMA.
			snprintf(at_cmd, sizeof(at_cmd), "AT^PPPCFG=\"%s\",\"%s\"\r\n", p_profile->username, p_profile->password);
			ret = send_at_cmd_sync(commdev, at_cmd, NULL, 0, 0);
			if(ret<0) {
				LOG_IN("%s is not capable to setup authentication info.",commdev);
			}
		}
		sprintf(at_cmd, "AT$QCPDPP=%d,%d,\"%s\",\"%s\"\r\n", pdp_index, (p_profile->auth_type == AUTH_TYPE_PAP) ? 1:2, p_profile->password, p_profile->username);
	}else if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_RM500U) {
		if(p_profile->auth_type == AUTH_TYPE_AUTO){
			auth_type = 3; //PAP or CHAP
		}else{
			auth_type = p_profile->auth_type == AUTH_TYPE_PAP ? AUTH_TYPE_PAP : AUTH_TYPE_CHAP;
		}
		snprintf(at_cmd, sizeof(at_cmd),  "AT+QICSGP=%d,%d,\"%s\",\"%s\",\"%s\",%d\r\n", pdp_index, get_pdp_type_setting(), p_profile->apn, p_profile->username, p_profile->password, auth_type);
	}else if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_PLS8) {
		snprintf(at_cmd, sizeof(at_cmd),  "AT^SGAUTH=%d,%d,%s,%s\r\n", pdp_index, (p_profile->auth_type == AUTH_TYPE_PAP) ? 1:2, p_profile->password, p_profile->username);
	}else if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ME909) {
		snprintf(at_cmd, sizeof(at_cmd), "AT^AUTHDATA=%d,%d,,\"%s\",\"%s\"\r\n", ((ih_key_support("FH09") || ih_key_support("FH19")) ? 16:0), (p_profile->auth_type == AUTH_TYPE_PAP) ? 1:2, p_profile->password, p_profile->username);
	}else if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS31) {
		snprintf(at_cmd, sizeof(at_cmd),  "AT+CGAUTH=3,%d,%s,%s\r\n", p_profile->auth_type, p_profile->username, p_profile->password);
	}else if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS61
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS81) {
		snprintf(at_cmd, sizeof(at_cmd),  "AT^SGAUTH=%d,%d,\"%s\",\"%s\"\r\n", pdp_index, (p_profile->auth_type == AUTH_TYPE_PAP) ? 1:2, p_profile->username, p_profile->password);
	}else if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_LARAR2
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_TOBYL201) {
		if(p_profile->username[0] && p_profile->password[0]){
			snprintf(at_cmd, sizeof(at_cmd), "AT+UAUTHREQ=1,%d,\"%s\",\"%s\"\r\n", p_profile->auth_type, p_profile->username, p_profile->password);
		}else{
			snprintf(at_cmd, sizeof(at_cmd), "AT+UAUTHREQ=1\r\n");
		}
	}else if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_LP41) {
		snprintf(at_cmd, sizeof(at_cmd),  "AT%%PDNSET=1,\"%s\",\"IPv4v6\",\"%s\",\"%s\",\"%s\"\r\n",\
				p_profile->apn, (p_profile->auth_type == 1) ? "PAP":"CHAP",\
				p_profile->username[0] ? p_profile->username : "",\
				p_profile->password[0] ? p_profile->password : "");
	}else if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_FM650
			|| get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_NL668) {
		snprintf(at_cmd, sizeof(at_cmd),  "AT+MGAUTH=%d,%d,\"%s\",\"%s\"\r\n", pdp_index, 
				(p_profile->auth_type == 1 || p_profile->auth_type == 2) ? p_profile->auth_type : 3, 
				p_profile->username, p_profile->password);
	}else if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_FG360) {
		snprintf(at_cmd, sizeof(at_cmd),  "AT+MGAUTH=%d,%d,\"%s\",\"%s\"\r\n", pdp_index, 
				(p_profile->auth_type == 1 || p_profile->auth_type == 2) ? p_profile->auth_type : 2, 
				p_profile->username, p_profile->password);
	}else {
		return ERR_FAILED;
	}

exec_cmd:
	if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_FG360){
		snprintf(buf, sizeof(buf), "%d", pdp_index);
		eval("mipc_wwan", "-n", buf,
				  "-t", "ipv4v6", 
				  "-s", is_vzw_sim() ? "IMS" : p_profile->apn, p_profile->username, p_profile->password, gl_auth_type_str[p_profile->auth_type],
				  "--apn_ia_check");
		return ERR_OK; //skip
	}

	ret = send_at_cmd_sync(commdev, at_cmd, NULL, 0, 0);
	if(ret<0) {
		LOG_IN("%s is not capable to setup authentication info.",commdev);
		return ERR_FAILED;
	}

	return ERR_OK;
}

static int get_rogers_dial_interval(int redial_cnt)
{
	int ret = -1;
	int interval[] = {0, 0, 1, 2, 4, 8, 16, 30, 60, 60, 60}; 

	if (redial_cnt < 0) { 
		redial_cnt = 0; 
	}

	if (redial_cnt > 8) { 
		ret = 60*60;
	}else {
		ret = interval[redial_cnt]*60;
	}

	return ret; 
}


static int execute_init_command(char *commdev, char *init_str) 
{
	char at_cmd[128]={0};
	int ret = -1;
	char buf[32]={0};

	if (!commdev || !init_str || !init_str[0]) {
		return ERR_FAILED;
	}

	snprintf(at_cmd, sizeof(at_cmd),  "%s\r\n", init_str);
	ret = send_at_cmd_sync_timeout(commdev, at_cmd, buf, sizeof(buf), 1,"OK", 5000);
	if(ret<0) {
		LOG_ER("Execute init command %s failed! Please check the format of the init command", init_str);
		return ERR_FAILED;
	}

	LOG_IN("Succeed to execute init command %s", init_str);

	return ERR_OK;
}

static int get_profile_setting(PPP_PROFILE **p_profile, PPP_PROFILE **p_2nd_wwan_profile, PPP_CONFIG *info, MODEM_CONFIG *modem_config, MODEM_INFO *modem_info) 
{
	int profile_idx = 0;
	int profile_2nd_idx = 0;

	//check profile if valid
	if(modem_info->current_sim==SIM1) {
		profile_idx=info->profile_idx;
		if (modem_config->dual_wwan_enable[0]) {
			profile_2nd_idx = info->profile_wwan2_idx;
		}else {
			profile_2nd_idx = -1;
		} 
	} else if(modem_info->current_sim==SIM2) { 
		profile_idx=info->profile_2nd_idx;
		if (modem_config->dual_wwan_enable[1]) {
			profile_2nd_idx  = info->profile_wwan2_2nd_idx;
		}else {
			profile_2nd_idx  = -1;
		} 
	}
	
	if(profile_idx == 0) {
		*p_profile = &info->auto_profile;
	}else {
		if( profile_idx < 0 || info->profiles[profile_idx-1].type==PROFILE_TYPE_NONE) {
			*p_profile =  &info->default_profile;
		} else {
			*p_profile = info->profiles + profile_idx -1;
		}
	}

	if(profile_2nd_idx == 0) {
		*p_2nd_wwan_profile = &info->auto_profile;
	}else if (profile_2nd_idx < 0) {
		*p_2nd_wwan_profile = NULL;
	}else {
		if(info->profiles[profile_2nd_idx-1].type==PROFILE_TYPE_NONE) {
			*p_2nd_wwan_profile =  &info->default_profile;
		} else {
			*p_2nd_wwan_profile = info->profiles + profile_2nd_idx -1;
		}
	}

	return ERR_OK;
}

static int active_pdp_text(char *commdev, PPP_CONFIG *info, MODEM_CONFIG *modem_config, MODEM_INFO *modem_info, BOOL debug)
{

	int ret = -1;
	char buf[32]={0};
	char at_cmd[512]={0};
	PPP_PROFILE *p_profile = NULL;
	PPP_PROFILE *p_2nd_wwan_profile = NULL;
	int profile_idx = 0;
	uint64_t my_modem_code = -1;
	char apn[64] = {0};
	char proto[8] = {0};
	BOOL need_func_reset = FALSE;
	int pdp_type = PDP_TYPE_IPV4;
	int cid = DEFAULT_CID;

	my_modem_code = get_modem_code(gl_modem_idx);

	if (!(my_modem_code == IH_FEATURE_MODEM_U8300
		||my_modem_code == IH_FEATURE_MODEM_U8300C
		||my_modem_code == IH_FEATURE_MODEM_PLS8
		||my_modem_code == IH_FEATURE_MODEM_ELS61
		||my_modem_code == IH_FEATURE_MODEM_ELS81
		||my_modem_code == IH_FEATURE_MODEM_ELS31
		||my_modem_code == IH_FEATURE_MODEM_LE910
		||my_modem_code == IH_FEATURE_MODEM_ME209
		||my_modem_code == IH_FEATURE_MODEM_LP41
		||my_modem_code == IH_FEATURE_MODEM_EC20
		||my_modem_code == IH_FEATURE_MODEM_EC25
		||my_modem_code == IH_FEATURE_MODEM_RM500
		||my_modem_code == IH_FEATURE_MODEM_RM520N
		||my_modem_code == IH_FEATURE_MODEM_RM500U
		||my_modem_code == IH_FEATURE_MODEM_RG500
		||my_modem_code == IH_FEATURE_MODEM_FM650
		||my_modem_code == IH_FEATURE_MODEM_FG360
		||my_modem_code == IH_FEATURE_MODEM_EP06
		||my_modem_code == IH_FEATURE_MODEM_TOBYL201
		||my_modem_code == IH_FEATURE_MODEM_LARAR2
		||my_modem_code == IH_FEATURE_MODEM_ME909
		||my_modem_code == IH_FEATURE_MODEM_NL668)){
		 return ERR_OK;
	}

	get_profile_setting(&p_profile, &p_2nd_wwan_profile, info, modem_config, modem_info); 

	if(p_profile)
		LOG_IN("p_profile->pdp_type :%d.", p_profile->type);

	if(p_2nd_wwan_profile)
		LOG_IN("p_2nd_wwan_profile->pdp_type :%d.", p_2nd_wwan_profile->type);

	if(ih_key_support("FS39")
		|| ih_key_support("NAVA")
		|| ih_key_support("NATO")
		|| ih_key_support("NRQ1")
		|| ih_key_support("FQ39")
		|| ih_key_support("FQ38")){
		pdp_type = PDP_TYPE_IPV4V6;
		cid = get_pdp_context_id(WWAN_ID1);
	}else if(modem_support_ipv6()){
		pdp_type = get_pdp_type_setting();
	}

	if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ME909
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ELS31
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ELS61
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ELS81
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC20
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC25
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM520N
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500U
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RG500
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_FM650
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_FG360
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EP06
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PLS8
		  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300C) {
			if (my_modem_code == IH_FEATURE_MODEM_U8300
				  || my_modem_code == IH_FEATURE_MODEM_U8300C) {
				if (p_profile->dialno[0] && (strcmp(p_profile->dialno, "*99***2#") == 0)){//liyb:为默认承载设置鉴权
					setup_modem_auth_parameters(commdev, p_profile, 2, debug);
				}else {
					setup_modem_auth_parameters(commdev, p_profile, 1, debug);
				}
			}else if(my_modem_code == IH_FEATURE_MODEM_FG360){
				eval("mipc_wwan", "--apn_profile_get");
				setup_modem_auth_parameters(commdev, p_profile, cid, debug);
			}else {
				setup_modem_auth_parameters(commdev, p_profile, cid, debug);
			}

			if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500
				|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM520N){
				if(p_2nd_wwan_profile){
				    setup_modem_auth_parameters(commdev, p_2nd_wwan_profile, get_pdp_context_id(WWAN_ID2), 0);
				}
			}
	}

	ret = check_modem_apn_config(commdev, cid, apn, sizeof(apn), proto, sizeof(proto), debug);//liyb:check current apn first.
	if (ret == ERR_OK){
		if (!((my_modem_code == IH_FEATURE_MODEM_U8300
			  || my_modem_code == IH_FEATURE_MODEM_U8300C)&&p_profile->dialno[0] && (strcmp(p_profile->dialno, "*99***2#") == 0))) {
			if (apn[0] && p_profile->apn[0] && (!strcmp(p_profile->apn, apn))){
				need_func_reset = FALSE;
			}else {
				LOG_IN("Apn was changed, modem need to be function reset.");
				need_func_reset = TRUE;
			}
		}
	} 	

	if(!p_profile->apn[0]) {
		LOG_IN("Can't find apn parameter.");
		return ERR_OK;
	}
	
	switch (my_modem_code) {
		case IH_FEATURE_MODEM_LE910:
		case IH_FEATURE_MODEM_ELS31:
			if (profile_idx == 0/*MODEM_NETWORK_AUTO == modem_config->network_type*/) {
				bzero(info->auto_profile.apn, PPP_MAX_APN_LEN);
				if (my_modem_code==IH_FEATURE_MODEM_ELS31) {
					need_func_reset = TRUE;
					goto function_reset;
				}else {
					return ERR_OK;
				}
			}else {
				snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=3,\"IPV4V6\",\"%s\"\r\n", p_profile->apn);
			}
			break;
		case IH_FEATURE_MODEM_PLS8:
		case IH_FEATURE_MODEM_ELS61:
		case IH_FEATURE_MODEM_ELS81:
			if (my_modem_code==IH_FEATURE_MODEM_PLS8) {
				redial_modem_de_register(debug); 
				need_func_reset = TRUE;
			}
		

			if(!strcmp(USE_BLANK_APN, p_profile->apn)){ 
				snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1, \"IP\"\r\n");
			}else{
				snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", p_profile->apn);
			}

			send_at_cmd_sync_timeout(commdev, "AT+COPS=2\r\n", buf, sizeof(buf), 1, "OK", 5000);	
			send_at_cmd_sync_timeout(commdev, "AT+CGDCONT=2, \"IP\"\r\n", buf, sizeof(buf), 1, "OK", 5000);	
			ret = send_at_cmd_sync_timeout(commdev, at_cmd, buf, sizeof(buf), 1,"OK", 5000);
			if(ret<0) {
				LOG_IN(" %s is not capable to exec %s ret %d,maybe pdp is already actived.",commdev, at_cmd, ret);
			}
			send_at_cmd_sync_timeout(commdev, "AT+COPS=0\r\n", buf, sizeof(buf), 1, "OK", 5000);	
			
			if (my_modem_code==IH_FEATURE_MODEM_PLS8) {
				if (proto[0] && !strcasecmp(proto, "IPv4v6")) {
					LOG_IN("need to reset the modem");
					send_at_cmd_sync_timeout(commdev, "AT+CFUN=1,1\r\n", buf, sizeof(buf), 1, "OK", 5000);	
					sleep(15);
				}
			}

			return ERR_OK;
			break;
		case IH_FEATURE_MODEM_ME909:
			if (ih_key_support("FH09") || ih_key_support("FH19")) {//QUALCOMM platform.
				snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=16,\"IP\",\"%s\"\r\n", p_profile->apn);
			}else {// HISILICOM platform.
				snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=0,\"IP\",\"%s\"\r\n", p_profile->apn);
			}
			break;
		case IH_FEATURE_MODEM_U8300:
		case IH_FEATURE_MODEM_U8300C:
			if (p_profile->dialno[0] && (strcmp(p_profile->dialno, "*99***2#") == 0)){
				if (gl_myinfo.priv.ppp_config.auto_profile.apn[0]) {//liyb:设置默认承载
					snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1,\"IPV4V6\",\"%s\"\r\n", gl_myinfo.priv.ppp_config.auto_profile.apn);
				}
			}else {
				if(!strcmp(USE_BLANK_APN, p_profile->apn)){ //wucl: actually for blank apn which means profile1 is blank
					snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1, \"IP\"\r\n");
				}else{
					snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1,\"IPV4V6\",\"%s\"\r\n", p_profile->apn);
				}
			}
			break;
		case IH_FEATURE_MODEM_TOBYL201:
		case IH_FEATURE_MODEM_LARAR2:
			//redial_modem_set_cfun_mode(0, debug);
			if(!strcmp(USE_BLANK_APN, p_profile->apn)){ 
				if (ih_key_support("FB38")) {
					snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1,\"IP\"\r\n");
				} else {
					snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1,\"IP\"\r\n");
				}
			}else{
				if (my_modem_code == IH_FEATURE_MODEM_LARAR2) {
					snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", p_profile->apn);
				}else{
					snprintf(at_cmd, sizeof(at_cmd), "AT+UCGDFLT=0,\"IP\",\"%s\"\r\n", p_profile->apn);
				}
#if 0
				if (ih_key_support("FB38")) {
					snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", p_profile->apn);
				} else {
					snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", p_profile->apn);
				}
#endif
			}

			ret = send_at_cmd_sync_timeout(commdev, at_cmd, buf, sizeof(buf), debug,"OK", 5000);
			if(ret<0) {
				LOG_IN(" %s is not capable to exec %s ret %d,maybe pdp is already actived.",commdev, at_cmd, ret);
			}
			//redial_modem_set_cfun_mode(1, debug);
			return 0;
		case IH_FEATURE_MODEM_RM500:
		case IH_FEATURE_MODEM_RM520N:
		case IH_FEATURE_MODEM_RG500:
		case IH_FEATURE_MODEM_EP06:
			if(cid == VZW_CLASS3_CID){
				send_at_cmd_sync_timeout(commdev, "AT+CGDCONT=1,\"IPV4V6\",\"IMS\"\r\n", buf, sizeof(buf), 1, "OK", 5000);	
			}
			snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=%d,\"%s\",\"%s\"\r\n", cid, gl_pdp_type_str[pdp_type], p_profile->apn);
			
			if(modem_support_dual_apn()){
				if(p_2nd_wwan_profile && p_2nd_wwan_profile->apn[0]){
					setup_modem_pdp_parameters(commdev, get_pdp_context_id(WWAN_ID2), pdp_type, p_2nd_wwan_profile->apn, debug);
				}else{
					setup_modem_pdp_parameters(commdev, get_pdp_context_id(WWAN_ID2), pdp_type, NULL, debug);
				}
			}
			break;
		case IH_FEATURE_MODEM_FG360:
			return ERR_OK; //skip
		default:
			snprintf(at_cmd, sizeof(at_cmd), "AT+CGDCONT=1,\"%s\",\"%s\"\r\n", gl_pdp_type_str[pdp_type], p_profile->apn);
			break;
	}
	ret = send_at_cmd_sync_timeout(commdev, at_cmd, buf, sizeof(buf), debug,"OK", 5000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec %s ret %d,maybe pdp is already actived.",commdev, at_cmd, ret);
	}

function_reset:
#if 1
	if (my_modem_code == IH_FEATURE_MODEM_PLS8
		|| my_modem_code == IH_FEATURE_MODEM_ME909
		//|| my_modem_code == IH_FEATURE_MODEM_TOBYL201
	//	|| my_modem_code == IH_FEATURE_MODEM_ELS31
	//	|| my_modem_code == IH_FEATURE_MODEM_ELS61
		|| my_modem_code == IH_FEATURE_MODEM_ELS81
		|| my_modem_code == IH_FEATURE_MODEM_EC20
		|| my_modem_code == IH_FEATURE_MODEM_U8300
		|| my_modem_code == IH_FEATURE_MODEM_U8300C) {
		if (need_func_reset) {
			if (my_modem_code == IH_FEATURE_MODEM_PLS8
					|| my_modem_code == IH_FEATURE_MODEM_ELS81) {
				if (my_modem_code != IH_FEATURE_MODEM_ELS81) {
					redial_modem_force_register(debug);
				} else {
					redial_els_modem_function_reset(1);
				}
			} else {
				redial_modem_function_reset(debug); 
			}
#if 0
			ret = send_at_cmd_sync_timeout(commdev, "AT+CFUN=0\r\n", buf, sizeof(buf), debug,"OK", 10000);
			if(ret<0) {
				LOG_IN(" %s is not capable to exec AT+CFUN ret %d.",commdev, ret);
			}

			sleep(1);

			ret = send_at_cmd_sync_timeout(commdev, "AT+CFUN=1\r\n", buf, sizeof(buf), debug,"OK", 10000);
			if(ret<0) {
				LOG_IN(" %s is not capable to exec AT+CFUN ret %d, retry!.",commdev, ret);
				ret = send_at_cmd_sync_timeout(commdev, "AT+CFUN=1\r\n", buf, sizeof(buf), debug,"OK", 10000);
			}

#endif
			//sleep(10);
			gl_timeout = 15; 
		}
	}
#endif
	return ret;
}

int modem_enter_airplane_mode(char *commdev) 
{
	char buf[32]={0};
	int ret = -1;

	if (!commdev) {
		return 0;
	}

	ret = send_at_cmd_sync_timeout(commdev, "AT+CFUN=0\r\n", buf, sizeof(buf), 1,"OK", 10000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+CFUN ret %d.",commdev, ret);
	}

	return ret;
}

int redial_modem_set_cfun_mode(int mode, BOOL debug)
{
	int ret = -1;
	char buf[32] = {0};
	char cmd[64] = {0};

	snprintf(cmd, sizeof(cmd), "AT+CFUN=%d\r\n", mode);

	ret = send_at_cmd_sync_timeout(gl_commport, cmd, buf, sizeof(buf), debug, "OK", 10000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+CFUN ret %d.", gl_commport, ret);
		return -1;
	}

	return 0;
}

int redial_els_modem_function_reset(BOOL debug) 
{

	int ret = -1;
	char buf[32]={0};

	ret = send_at_cmd_sync_timeout(gl_commport, "AT+CFUN=4\r\n", buf, sizeof(buf), debug,"OK", 10000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+CFUN ret %d.", gl_commport, ret);
	}

	sleep(1);

	ret = send_at_cmd_sync_timeout(gl_commport, "AT+CFUN=1\r\n", buf, sizeof(buf), debug,"OK", 10000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+CFUN ret %d, retry!.", gl_commport, ret);
		ret = send_at_cmd_sync_timeout(gl_commport, "AT+CFUN=1\r\n", buf, sizeof(buf), debug,"OK", 10000);
	}


	sleep(3);
	ret = send_at_cmd_sync_timeout(gl_commport, "AT+COPS=0\r\n", buf, sizeof(buf), debug,"OK", 10000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+COPS=0 ret %d, retry!.", gl_commport, ret);
	}

	return ret;
}

int redial_modem_function_reset(BOOL debug) 
{

	int ret = -1;
	char buf[32]={0};

	ret = send_at_cmd_sync_timeout(gl_commport, "AT+CFUN=0\r\n", buf, sizeof(buf), debug,"OK", 10000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+CFUN ret %d.", gl_commport, ret);
	}

	sleep(1);

	ret = send_at_cmd_sync_timeout(gl_commport, "AT+CFUN=1\r\n", buf, sizeof(buf), debug,"OK", 10000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+CFUN ret %d, retry!.", gl_commport, ret);
		ret = send_at_cmd_sync_timeout(gl_commport, "AT+CFUN=1\r\n", buf, sizeof(buf), debug,"OK", 10000);
	}

	return ret;
}

int redial_modem_re_register(BOOL debug) 
{

	int ret = -1;
	char buf[32]={0};

	ret = send_at_cmd_sync_timeout(gl_commport, "AT+COPS=2\r\n", buf, sizeof(buf), debug,"OK", 10000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+COPS=2 ret %d.", gl_commport, ret);
	}

	sleep(1);

	ret = send_at_cmd_sync_timeout(gl_commport, "AT+COPS=0\r\n", buf, sizeof(buf), debug,"OK", 10000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+COPS=0 ret %d, retry!.", gl_commport, ret);
	}

	return ret;
}

int redial_modem_de_register(BOOL debug) 
{

	int ret = -1;
	char buf[32]={0};

	ret = send_at_cmd_sync_timeout(gl_commport, "AT+COPS=2\r\n", buf, sizeof(buf), debug,"OK", 10000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+COPS=2 ret %d.", gl_commport, ret);
	}

	return ret;
}

int redial_modem_force_register(BOOL debug) 
{

	int ret = -1;
	char buf[32]={0};

	ret = send_at_cmd_sync_timeout(gl_commport, "AT+COPS=0\r\n", buf, sizeof(buf), debug,"OK", 10000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT+COPS=2 ret %d.", gl_commport, ret);
	}

	return ret;
}

int auto_select_net_apn(char *imsi_code, BOOL debug)
{
	FILE  *conf_fd = NULL;
	char *pos = NULL;
	char buf[128] = {0};
	char tmp[128] = {0};
	char *usr = NULL;
	char *passwd = NULL;
	char *apn = NULL;
	char *p = NULL;
	char *auth = NULL;
	MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;
	char *submode = info->submode_name;
	//LOG_IN("submode_name:%s", info->submode_name);
	
	if(gl_modem_code==IH_FEATURE_MODEM_MC2716
		|| gl_modem_code==IH_FEATURE_MODEM_MC5635
		|| gl_modem_code==IH_FEATURE_MODEM_PVS8
		|| gl_modem_code==IH_FEATURE_MODEM_WPD200
		|| ((gl_modem_code==IH_FEATURE_MODEM_U8300C) 
			&& ((strcmp(submode, "EVDO") == 0) 
			|| (strcmp(submode, "HDR") == 0)
			|| (strcmp(submode, "CDMA") == 0)))
		) {
		snprintf(gl_myinfo.priv.ppp_config.auto_profile.dialno, PPP_MAX_DIALNO_LEN, "%s", DEFAULT_EVDO_DIAL_NUMBER);
	}else {
		if (gl_modem_code==IH_FEATURE_MODEM_LE910) {
			snprintf(gl_myinfo.priv.ppp_config.auto_profile.dialno, PPP_MAX_DIALNO_LEN, "%s", LE910_DIAL_NUMBER);
		}else {
			snprintf(gl_myinfo.priv.ppp_config.auto_profile.dialno, PPP_MAX_DIALNO_LEN, "%s", DEFAULT_DIAL_NUMBER);
		}
	} 

	if(gl_modem_code==IH_FEATURE_MODEM_MC2716
		|| gl_modem_code==IH_FEATURE_MODEM_MC5635
		|| gl_modem_code==IH_FEATURE_MODEM_PVS8
		|| gl_modem_code==IH_FEATURE_MODEM_WPD200) {
		goto use_default;
	}

	if(!imsi_code || !imsi_code[0]) {
		LOG_ER("Can't determine IMSI Code!");	
		goto use_default;
	}

	conf_fd = fopen(NETWORK_CONFIG_FILE_DIR, "r");
	if(!conf_fd) {
		LOG_ER("Open network config file failed!");	
		goto use_default;
	}
	
	//20404 live.vodafone.com vodafone vodafone 1;
	while(fgets(buf, sizeof(buf), conf_fd)) {
		strlcpy(tmp, buf, sizeof(tmp));
		p = tmp;
		pos = strsep(&p, " ");
		if(pos && !strncmp(imsi_code, pos, strlen(pos))){
			p = buf;
			pos = strsep(&p, "\n");
			break;
		}else{
			pos = NULL;
		}
	}

	fclose(conf_fd);
	
	if(pos) {
		LOG_IN("config_info:%s", pos);
	}else {
		LOG_IN("can't determine the apn parameter");
		goto use_default;
	}

	pos = strchr(pos, ' ');
	if(!pos) {
		goto use_default;
	}

	pos ++;

	if(!strchr(pos, ' ')) {
		apn = strsep(&pos, ";");
	}else {
		apn = strsep(&pos, " ");
	}

	if(apn == NULL) {
		LOG_ER("can't determine the apn to use.");	
		goto use_default;
	}
	
	snprintf(gl_myinfo.priv.ppp_config.auto_profile.apn, PPP_MAX_APN_LEN, "%s", apn);

	usr = strsep(&pos, " ");
	if(usr == NULL) {
		LOG_IN("seems we don't need to set username.");	
		return ERR_OK;
	}

	snprintf(gl_myinfo.priv.ppp_config.auto_profile.username, PPP_MAX_USR_LEN, "%s", usr);

	if(pos && !strchr(pos, ' ')){
		passwd = strsep(&pos, ";");
	}else{
		passwd = strsep(&pos, " ");
	}
	
	if(passwd == NULL) {
		LOG_IN("Seems no need to set password.");	
		return ERR_OK;
	}

	//wucl LOG_IN("passwd = %s", passwd);
	snprintf(gl_myinfo.priv.ppp_config.auto_profile.password, PPP_MAX_PWD_LEN, "%s", passwd);
	
	auth = strsep(&pos, ";");
	if(auth){
		gl_myinfo.priv.ppp_config.auto_profile.auth_type = atoi(auth);
	}

	return ERR_OK;

use_default:
	LOG_IN("Using default parameter");
	if(gl_modem_code==IH_FEATURE_MODEM_U8300
		||((gl_modem_code==IH_FEATURE_MODEM_ME909) && ih_key_support("TH09"))) {
		  snprintf(gl_myinfo.priv.ppp_config.auto_profile.apn, PPP_MAX_APN_LEN, "%s", DEFAULT_APN_CM);
	}else if(gl_modem_code==IH_FEATURE_MODEM_MC2716
		|| gl_modem_code==IH_FEATURE_MODEM_MC5635
		|| gl_modem_code==IH_FEATURE_MODEM_PVS8
		|| gl_modem_code==IH_FEATURE_MODEM_WPD200
		|| ((gl_modem_code==IH_FEATURE_MODEM_U8300C) 
			&& ((strcmp(submode, "EVDO") == 0) 
			|| (strcmp(submode, "HDR") == 0)
			|| (strcmp(submode, "CDMA") == 0)))
		) {
		  bzero(gl_myinfo.priv.ppp_config.auto_profile.apn, PPP_MAX_APN_LEN);
	}else {
		  snprintf(gl_myinfo.priv.ppp_config.auto_profile.apn, PPP_MAX_APN_LEN, "%s", DEFAULT_APN_CU);
	}
	
	if(gl_modem_code==IH_FEATURE_MODEM_MC2716
		|| gl_modem_code==IH_FEATURE_MODEM_MC5635
		|| gl_modem_code==IH_FEATURE_MODEM_PVS8
		|| gl_modem_code==IH_FEATURE_MODEM_WPD200
		|| ((gl_modem_code==IH_FEATURE_MODEM_U8300C) 
			&& ((strcmp(submode, "EVDO") == 0) 
			|| (strcmp(submode, "HDR") == 0)
			|| (strcmp(submode, "CDMA") == 0)))
		) {
		  snprintf(gl_myinfo.priv.ppp_config.auto_profile.username, PPP_MAX_USR_LEN, "%s", DEFAULT_EVDO_USERNAME);
		  snprintf(gl_myinfo.priv.ppp_config.auto_profile.password, PPP_MAX_PWD_LEN, "%s", DEFAULT_EVDO_PASSWORD);
	}else {
		  snprintf(gl_myinfo.priv.ppp_config.auto_profile.username, PPP_MAX_USR_LEN, "%s", DEFAULT_USERNAME);
		  snprintf(gl_myinfo.priv.ppp_config.auto_profile.password, PPP_MAX_PWD_LEN, "%s", DEFAULT_PASSWORD);
	}

	return ERR_OK;
}

/*
liyb:set nai user information for wpd200
*/
int wpd200_nai_setup(char *commdev, BOOL debug) 
{
	char buf[16] = {0};	
	char at_cmd[128] = {0};
	int ret = -1;
	
	if (!(gl_myinfo.priv.modem_config.nai_no[0] && gl_myinfo.priv.modem_config.nai_pwd[0]))
		return IH_ERR_INVALID;
	
	sprintf(at_cmd, "AT*NAI=%s\r\n", gl_myinfo.priv.modem_config.nai_no);
	ret = send_at_cmd_sync_timeout(commdev, at_cmd, buf, sizeof(buf), debug,"OK", 5000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT*NAI ret %d.",commdev, ret);
		return ret;
	}
	
	bzero(at_cmd, sizeof(at_cmd));
	sprintf(at_cmd, "AT*NAIPWD=%s\r\n", gl_myinfo.priv.modem_config.nai_pwd);
	ret = send_at_cmd_sync_timeout(commdev, at_cmd, buf, sizeof(buf), 0,"OK", 5000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT*NAIPWD ret %d.",commdev, ret);
		return ret;
	}
	
	return IH_ERR_OK;
}


/*
liyb:set nai user information for wpd200
*/
int wpd200_get_nai_username(char *commdev, BOOL debug)
{
	char buf[32] = {0};
	char *pos = NULL;
	char *nai = NULL;
	int ret = -1;

	ret = send_at_cmd_sync_timeout(commdev, "AT*NAI?\r\n", buf, sizeof(buf), debug,"OK", 5000);
	if(ret<0) {
		LOG_IN(" %s is not capable to exec AT*NAI? ret %d.",commdev, ret);
		return ret;
	}
	
	pos = buf;
	if (pos){
		pos = strstr(pos, "NAI:");
		if (!pos){
			return -1;
		}
		
		pos = strchr(pos, ':');
		
		pos ++;
		nai = strtok(pos, "\r");
		if (!nai){
			return -1;
		}
		bzero(gl_myinfo.priv.modem_config.nai_no, sizeof(gl_myinfo.priv.modem_config.nai_no));
		memcpy(gl_myinfo.priv.modem_config.nai_no, nai, strlen(nai)+1);
		return IH_ERR_OK;
	}

	return IH_ERR_OK;
}

int check_cur_apn_cfg_by_cid(char *commdev, int cid, char **apn, int verbose) 
{
 	char buf[512]={0};
	char *pos = NULL;
	int ret = 0;
	
	if (!commdev) {
		return -1;
	}

#if 0
	write_fd(fd, "AT+CGDCONT?\r\n", strlen("AT+CGDCONT?\r\n"), 1000, verbose);
	if(check_return2(fd, buf, sizeof(buf), "OK", 5000, verbose)<=0){
		syslog(LOG_DEBUG, "exec AT+CGDCONT? error");
		return -1;
	}
#endif
	ret = send_at_cmd_sync_timeout(commdev, "AT+CGDCONT?\r\n", buf, sizeof(buf), verbose, "OK", 5000);
	if(ret<0) {
		LOG_ER(" %s is not capable to exec at+cgdcont? ret %d.",commdev, ret);
		return ret;
	}

	pos = buf;
	while(pos){
		pos = strstr(pos, "CGDCONT:");
		if (!pos){
			break;
		}
        
        pos += strlen("CGDCONT:");

        while (pos && *pos == ' ') {
			pos++;	
		}

		if ((!pos) || atoi(pos) != cid) {
			continue;	
		}

        pos++;
		pos = strchr(pos,',' );	
		if (!pos){
			break;	
		}

		pos++;
		pos = strchr(pos,',' );	
		if (!pos){
			break;	
		}

		pos = strchr(pos, '\"');
		if (!pos){
			break;	
		}

		pos ++;
        if (*pos=='\"') {
            return -1;
        }

		*apn = strtok(pos, "\"");
		if (*apn){
            if (verbose) {
			    syslog(LOG_INFO, "cid:%d ,apn configration: %s", cid, *apn);  
            }

			return 0;
		}
	};

	return -1;   
}

#define VZWIMS      "VZWIMS"
#define VZWADMIN    "VZWADMIN"
#define VZWAPP      "VZWAPP"
int check_vzw_init_apn_by_cid(char *commdev, int cid, int verbose)
{
	char *apn = NULL;
    int ret = 0;

    ret = check_cur_apn_cfg_by_cid(commdev, cid, &apn, verbose); 
    if (ret < 0) {
        return 0;
    }

    if (!apn) {
        return 0;
    }

    if (!strncasecmp(apn, VZWIMS, strlen(VZWIMS))
        || !strncasecmp(apn, VZWADMIN, strlen(VZWADMIN))
        || !strncasecmp(apn, VZWAPP, strlen(VZWAPP))) {
        return -1;
    }

    return 0;
}

#define CGACT_RSP "+CGACT:"
#define UIPADDR_RSP "+UIPADDR:"
static int ublox_bridge_mode_get_net_paras(char *commdev, BOOL init, int verbose)
{
	char cmd_buf[64] = {0};
    char uip_buf[256] = {0};
    char ret_buf[256] = {0};
    char ip_buf[256] = {0};
    char dns_buf[1024] = {0};
	char gw_array[16] = {0};
    int cid = -1;
    int status = -1;
    char *pos = NULL;
    char *pos_cgact = NULL;
    char *pos1 = NULL;

	char *ip_str = NULL;
	char *gw_str = NULL;
	char *netmask_str = NULL;
	int ret = -1;
	
	BOOL have_apn = FALSE;

	PPP_PROFILE *p_profile;
	int profile_idx = 0;

    struct in_addr ipaddr;
    struct in_addr gateway;
    struct in_addr netmask;
	struct in_addr dns1;

    char *dns1_str = NULL;
    //char *dns2 = NULL;
    BOOL need_restart_wan1 = FALSE;
    BOOL got_valid_parameter = FALSE;
	BOOL point_2_point = FALSE;
	char buf[MAX_SYS_CMD_LEN] = {0};

	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	PPP_CONFIG *ppp_config = &(gl_myinfo.priv.ppp_config);

	//check profile if valid
	if(modem_info->current_sim==SIM1) profile_idx = ppp_config->profile_idx;
	else if(modem_info->current_sim==SIM2) profile_idx = ppp_config->profile_2nd_idx;
	if(profile_idx == 0) {
		p_profile =  &ppp_config->auto_profile;
	}else {
		if(ppp_config->profiles[profile_idx-1].type==PROFILE_TYPE_NONE) {
			p_profile =  &ppp_config->default_profile;
		} else {
			p_profile = ppp_config->profiles + profile_idx -1;
		}
	}
	
	if(p_profile->apn && p_profile->apn[0]) {
		if (verbose) {
			LOG_IN("User apn configure:%s.", p_profile->apn);
		}
		have_apn = TRUE;
	}

    //check gateway
    snprintf(cmd_buf, sizeof(cmd_buf), "%s", "AT+CGACT?\r\n");
	ret = send_at_cmd_sync_timeout(commdev, cmd_buf, ret_buf, sizeof(ret_buf), verbose,"OK", 5000);
	if (ret < 0) {
        syslog(LOG_ERR, "Failed to execute +CGACT?!");
		return -1;
    }
    
    pos_cgact = ret_buf;
    while ((pos_cgact = strstr(pos_cgact, CGACT_RSP)) != NULL) {
		point_2_point = FALSE;
        pos_cgact = pos_cgact + strlen(CGACT_RSP);
        while(*pos_cgact!=0 && *pos_cgact==' ') pos_cgact++;
        if (!pos_cgact) {
            break;
        }

			cid = atoi(pos_cgact);

			if ((gl_modem_code == IH_FEATURE_MODEM_TOBYL201) && ih_key_support("FB38")) {
				if (check_vzw_init_apn_by_cid(commdev, cid, verbose) < 0) {
					continue;
				}
			} else  {
				if (have_apn) {
					if ((cid != 1) && (cid != 4)) { 
						continue;
					}    
				}    
			}    


        pos_cgact = strchr(pos_cgact, ',');
        if (!pos_cgact) {
            break;
        }

        pos_cgact++;
        status = atoi(pos_cgact);
        if (1 != status) {
           continue; 
        }

        //get gateway
        snprintf(cmd_buf, sizeof(cmd_buf), "AT+UIPADDR=%d\r\n", cid);
		ret = send_at_cmd_sync_timeout(commdev, cmd_buf, uip_buf, sizeof(uip_buf), verbose,"OK", 5000);
        if(ret < 0){
            syslog(LOG_ERR, "Failed to execute %s!", cmd_buf);
			goto error;
        }
        
        pos = strstr(uip_buf, UIPADDR_RSP);
		if (!pos) {
			continue;
		}
        pos = strchr(pos, ',');
        if (!pos) {
            continue;
        }
        pos ++;

        pos = strchr(pos, ',');
        if (!pos) {
            continue;
        }
        pos ++;

        pos1 = strchr(pos, ',');
        if (!pos1) {
            continue;
        }

        pos1 = strchr(pos1, '\"');
        if (!pos1) {
            break;
        }
        pos1 ++;


        pos =strchr(pos, '\"');
        if (!pos) {
            break;
        }
        pos ++;
        
        gw_str = strsep(&pos, "\"");
        if (!gw_str || !gw_str[0]) {
            continue;
        }


        netmask_str = strsep(&pos1, "\"");
        if (!netmask_str || !netmask_str[0]) {
            continue;
        }

		snprintf(gw_array, sizeof(gw_array), "%s", gw_str);

		inet_aton(gw_str, &gateway);
        if(memcmp(&pinfo->remote_ip, &gateway, sizeof(struct in_addr))) {
            syslog(LOG_INFO, "gateway:%s", gw_str);
			memcpy(&pinfo->remote_ip, &gateway, sizeof(struct in_addr));
            need_restart_wan1 = TRUE;
        }

		inet_aton(netmask_str, &netmask);
        if(memcmp(&pinfo->netmask, &netmask, sizeof(struct in_addr))) {
            syslog(LOG_INFO, "netmask:%s", netmask_str);
			if (strncmp(netmask_str, "255.255.255.255", strlen(netmask_str))==0) {
				point_2_point = TRUE;
			}            
			//nvram_set(nv, gateway);
			memcpy(&pinfo->netmask, &netmask, sizeof(struct in_addr));
            need_restart_wan1 = TRUE;
        }

        //check local ip address and dns
        bzero(cmd_buf, sizeof(cmd_buf));
        snprintf(cmd_buf, sizeof(cmd_buf), "AT+CGPADDR=%d\r\n", cid);
		ret = send_at_cmd_sync_timeout(commdev, cmd_buf, ip_buf, sizeof(ip_buf), verbose,"OK", 5000);
        if(ret < 0){
            syslog(LOG_ERR, "Failed to set auth type for the modem!");
            continue;
        }

        pos = ip_buf;
        pos = strchr(pos, '\"');
        if (!pos) {
            break;
        }
        pos ++;

        ip_str = strsep(&pos, "\"");
        if (!ip_str) {
            continue;
        }

        got_valid_parameter = TRUE;

		inet_aton(ip_str, &ipaddr);
        if(memcmp(&pinfo->local_ip, &ipaddr, sizeof(struct in_addr))) {
            syslog(LOG_INFO, "local ip:%s", ip_str);
			memcpy(&pinfo->local_ip, &ipaddr, sizeof(struct in_addr));

			eval("ip", "addr", "flush", "dev", USB0_WWAN);
			if (point_2_point) {
				eval("ifconfig", USB0_WWAN, ip_str, "netmask", "255.255.255.255", "pointopoint", gw_array, "up");
			}else {
			
				if (!validate_ip_mask(ip_str, netmask_str)) {
					continue;
				}

				//eval("ip", "addr", "add", "dev", USB0_WWAN, "local", ip_str);
				eval("ifconfig", USB0_WWAN, ip_str, "netmask", netmask_str);

			}

            need_restart_wan1 = TRUE;
        }

        //get dns
        bzero(cmd_buf, sizeof(cmd_buf));
        snprintf(cmd_buf, sizeof(cmd_buf), "AT+CGCONTRDP=%d\r\n", cid);
		ret = send_at_cmd_sync_timeout(commdev, cmd_buf, dns_buf, sizeof(dns_buf), verbose,"OK", 5000);
        if(ret < 0){
            syslog(LOG_ERR, "Failed to execute %s!", cmd_buf);
            goto error;
        }

        pos = dns_buf;
        pos = strstr(pos, "CGCONTRDP:");
        if (!pos) {
            break;
        }

        pos = strchr(pos , ',');
        if (!pos) {
            break;
        }
        pos ++;

        pos = strchr(pos , ',');
        if (!pos) {
            break;
        }
        pos ++;

        pos = strchr(pos , ',');
        if (!pos) {
            break;
        }

        pos ++;

        pos = strchr(pos , ',');
        if (!pos) {
            break;
        }
        pos ++;

        pos = strchr(pos , ',');
        if (!pos) {
            break;
        }

        pos = strchr(pos , '\"');
        if (!pos) {
            break;
        }
        pos ++;

        dns1_str = strsep(&pos, "\"");
        if(!dns1_str) {
            break;
        }


		inet_aton(dns1_str, &dns1);
        if(memcmp(&pinfo->dns1, &dns1, sizeof(struct in_addr))) {
            //nvram_set(nv, dns1);
			memcpy(&pinfo->dns1, &dns1, sizeof(struct in_addr));
			snprintf(buf, sizeof(buf), "echo nameserver %s>>/etc/resolv.dnsmasq", dns1_str);
			system(buf);
            syslog(LOG_INFO, "dns:%s", dns1_str);
            need_restart_wan1 = TRUE;
        }

        break;
    }
    
    if (need_restart_wan1) {
        LOG_IN("network parameters were changed , restart wan port.");

        //set mtu
		if (gl_myinfo.priv.ppp_config.mtu){
			bzero(buf, sizeof(buf));
			if (ih_license_support(IH_FEATURE_MODEM_LP41)
				|| (ih_license_support(IH_FEATURE_MODEM_ME909) 
					&& (ih_key_support("FH09")
					|| ih_key_support("FH19")))) {
					if (ih_check_interface_exist(USB0_WWAN)) {
						sprintf(buf, "ifconfig %s mtu %d", USB0_WWAN, gl_myinfo.priv.ppp_config.mtu);
					} else {	
						sprintf(buf, "ifconfig %s mtu %d", ETH2_WWAN, gl_myinfo.priv.ppp_config.mtu);
					}
				}else {
					sprintf(buf, "ifconfig %s mtu %d", USB0_WWAN, gl_myinfo.priv.ppp_config.mtu);
			}
			system(buf);
		}

		return 1;
    }
    
    if (got_valid_parameter) {
        return 0;
    }else {
        return -1;
    }

error:
    return -1;
}

static int get_network_parameters(int idx, char *iface)
{   
    //char nv[64] = {0};
    int retval = -1;

	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;
	PPP_CONFIG *ppp_config = &(gl_myinfo.priv.ppp_config);

	if (!iface) {
		return -1;
	}

	clear_cellular_net_info();

	eval("ifconfig", iface, "up");
	switch (gl_modem_code) {
		case IH_FEATURE_MODEM_TOBYL201:
			retval = ublox_bridge_mode_get_net_paras(gl_commport, TRUE, 1);
			break;
		case IH_FEATURE_MODEM_ELS81:
			retval = modem_get_net_params_by_at(gl_commport, TRUE, ppp_config , info, pinfo, TRUE);
			break;
		default:
			return -1;
	}

    if (retval < 0) {
        gl_ppp_pid = -1;
        return -1;
    }
	
	pinfo->status = VIF_UP;
	gl_myinfo.svcinfo.uptime = get_uptime();
	snprintf(pinfo->iface, sizeof(pinfo->iface), iface);
#if 0
	ret = ih_ipc_send_no_ack(IH_SVC_REDIAL, IPC_MSG_UPDOWN_PPP_UP, (char *)pinfo, sizeof(VIF_INFO));
	if(ret) {
		LOG_ER("send msg to interface is not ok.");
	}

#endif
    gl_ppp_pid  = 65535;

    return 0;
}

static int check_cellular_interface(void)
{
	if (gl_modem_code == IH_FEATURE_MODEM_TOBYL201) {
		if (access(SYS_USB0_ADDR_LEN_PATH, F_OK) !=0) {
			LOG_ER("interface is not found!");
			return -1;
		}
	} else {
		return 0;
	}

	LOG_IN("interface is ready!");

	return 0;
}

void ledcon_dualsim_switch()
{
#if !(defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_WR3)
	ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
	ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
	ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#endif

#ifdef HAVE_LED_WWAN
	ledcon(LEDMAN_CMD_OFF, LED_WWAN);
#elif defined(INHAND_VG9) || defined(INHAND_IR8) || defined(INHAND_ER6) || defined(INHAND_WR3) || defined(INHAND_ER3)

#elif !(defined INHAND_IDTU9 || defined INHAND_IG5)
	ledcon(LEDMAN_CMD_ON, LED_WARN);
#endif
}

/*
 * handle redial state machine
 */
static void redial_handle(void)
{
	AT_CMD *pcmd;
	struct timeval tv;
	int timeout = 0;
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);
	VIF_INFO *secondary_vif_info = &(gl_myinfo.svcinfo.secondary_vif_info);
	VIF_INFO *sub_vif_info = gl_myinfo.svcinfo.sub_vif_info;
	MODEM_INFO *modem_info = &gl_myinfo.svcinfo.modem_info;
	DUALSIM_INFO *dualsim_info = &(gl_myinfo.priv.dualsim_info);
	MODEM_CONFIG *modem_config = &(gl_myinfo.priv.modem_config);
	SMS_CONFIG *sms_config = &(gl_myinfo.priv.sms_config);
	CELLULAR_SUB_IF *sub_ifs = gl_myinfo.priv.sub_ifs;
	SIM_STATUS *sim_status = gl_myinfo.svcinfo.sim_status;
	int ret;
	char vendor_id[16]={0}, product_id[16]={0};
	char data[2][MAX_EVENT_DATA_LEN];

	char buf1[16], buf2[16];

	redial_handle_enter(&gl_redial_state, &gl_myinfo.svcinfo.modem_info, &gl_myinfo.svcinfo.vif_info);
	
	//handle redial state machine
	switch(gl_redial_state) {
	case REDIAL_STATE_INIT:
		LOG_IN("redial init state");

		//wait sysinfo
		//FIXME: when redial reboot, maybe gl_got_sysinfo not set ready
		if(gl_got_sysinfo==TRUE) {
			LOG_DB("redial init modem type");
			gl_modem_idx = find_modem_type_idx(ih_license_get_modem());
			if(gl_modem_idx<0) {
				timeout = 20;
				break;
			}
			gl_modem_code = get_modem_code(gl_modem_idx);
			//custom at cmd according different module
			setup_custom_at_cmd(get_modem_code(gl_modem_idx));
			//init port, at command
			gl_commport = gl_modem_types[gl_modem_idx].commport;
			gl_dataport = gl_modem_types[gl_modem_idx].dataport;
			gl_at_lists = gl_modem_types[gl_modem_idx].at_lists;
			pinfo->enabled = gl_myinfo.priv.enable;
			if(gl_modem_code != IH_FEATURE_MODEM_NONE){
				pinfo->status = VIF_CREATE;
				ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char*)pinfo, sizeof(VIF_INFO));
				pinfo->status = VIF_DOWN;
				ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char*)pinfo, sizeof(VIF_INFO));
			}
			gl_redial_state = REDIAL_STATE_WAIT_CONFIG;
			//TODO: init default dial settings
			//gl_myinfo.priv.ppp_config.default_profile
#if (defined INHAND_VG9 || defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_WR3)
			set_modem(gl_modem_code, MODEM_START);
			if(gl_modem_code == IH_FEATURE_MODEM_FM650){
				set_modem(gl_modem_code, MODEM_RESET);
			}

			if(ih_license_support(IH_FEATURE_PCIE2ETH_RTL8125B)){
				timeout = 30;
			}
#if !(defined INHAND_IR8 || defined INHAND_ER6)
			cellular_ledcon(CELL_OFF);
#else
			ledcon_by_event(EV_DAIL_START);
#endif
#elif (defined INHAND_IG9)
			set_modem(gl_modem_code, MODEM_START);
			timeout = 20;
#else 
			if (gl_modem_code == IH_FEATURE_MODEM_LUA10
				||gl_modem_code == IH_FEATURE_MODEM_ELS61
				||gl_modem_code == IH_FEATURE_MODEM_ELS81
				||gl_modem_code == IH_FEATURE_MODEM_TOBYL201
				||((gl_modem_code == IH_FEATURE_MODEM_PLS8) && (!ih_key_support("FS59")
						&& !ih_key_support("FS39")))) {
			//||gl_modem_code == IH_FEATURE_MODEM_MC2716) {
				/*LUA10 is powered on userspace*/
				set_modem(gl_modem_code, MODEM_START);
			}
#endif
			gl_init_timestamp = time(NULL);
		} else {
			timeout = 2;
		}
		break;
	case REDIAL_STATE_WAIT_CONFIG:
		timeout = 2;
		//EN00: stay here forever
		//if(get_modem_code(gl_modem_idx)!=IH_FEATURE_MODEM_EXT
		if(get_modem_code(gl_modem_idx)!=IH_FEATURE_MODEM_NONE){
			if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500U){
				if(ih_check_interface_exist(PCIE0_WWAN)){
					gl_got_usbinfo = TRUE;
					timeout = ih_license_support(IH_FEATURE_ER6) ? 30 : 2;
				}
			}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_FG360){
				gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
				break;
			}

			if(!gl_got_usbinfo){
				gl_got_usbinfo = usb_get_device_info(NULL, vendor_id, product_id, NULL, NULL, 0) > 0 ? TRUE : FALSE;
			}

			if((gl_got_usbinfo || ((time(NULL) - gl_init_timestamp) > 50)) && gl_config_ready){
				if(modem_config->dualsim){
					dualsim_handle_backuptimer();
				}
				
				gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
			}
		}
		break;
	case REDIAL_STATE_MODEM_STARTUP:
#ifdef INHAND_WR3
		redial_set_ant(modem_config->ant);
#endif
		//clear modem info
		clear_modem_info(modem_info);
		//delete event handle
		if(gl_at_fd > 0){
			event_del(&gl_ev_at);
			CLOSE_FD(gl_at_fd);
		}
		//prevent receiving SIGHUP signal
		//update sim traffic status
		get_data_usage_info(SIM1, MONTHLY_DATA_USAGE, &sim_data_usage[0]);
		get_data_usage_info(SIM2, MONTHLY_DATA_USAGE, &sim_data_usage[1]);
		//check sim	
		if(!current_sim_is_right()) {
			ledcon_dualsim_switch();
		}

#if !(defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_WR3)
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#endif

#ifdef HAVE_LED_WWAN
		ledcon(LEDMAN_CMD_OFF, LED_WWAN);
#elif defined(INHAND_VG9) || defined(INHAND_IR8) || defined(INHAND_ER6) || defined(INHAND_WR3) || defined(INHAND_ER3)

#elif !(defined INHAND_IDTU9 || defined INHAND_IG5)
		ledcon(LEDMAN_CMD_ON, LED_WARN);
#endif

		//if reset many times, reboot system
		if(gl_modem_reset_cnt++ >= MAX_MODEM_RESET) {
			//FIXME: according redial & sms enable
			if(gl_myinfo.priv.enable==TRUE) {
				LOG_ER("modem is not ready!");
				schedule_router_or_modem_reboot(); 
			} else {
				LOG_IN("re-configure modem reset count.");
				gl_modem_reset_cnt = 0;
			}
		}
		//check enable option
		if(gl_myinfo.priv.enable==TRUE) {
			sim_status[modem_info->current_sim-1].at_errno = gl_at_errno;
			if(gl_at_errno == ERR_AT_OK){
				sim_status[modem_info->current_sim-1].retries = 0;
			}else{
				if(sim_status[modem_info->current_sim-1].retries < MAX_SIM_RETRY){
					sim_status[modem_info->current_sim-1].retries++;
				}
			}

			LOG_DB("sim1 status:%d retries:%d", sim_status[0].at_errno, sim_status[0].retries);
			LOG_DB("sim2 status:%d retries:%d", sim_status[1].at_errno, sim_status[1].retries);

#if (defined INHAND_IDTU9 || defined INHAND_IG5) 
			ledcon_by_event(EV_DAIL_START);
#elif defined(INHAND_VG9) || defined(INHAND_IR8) || defined(INHAND_ER6) || defined(INHAND_ER3)
		if (gl_at_errno==ERR_AT_NOMODEM
 			|| gl_at_errno==ERR_AT_NOSIM) {
#if !(defined INHAND_IR8 || defined INHAND_ER6)
			cellular_ledcon(CELL_CONNECTING_FAULT);
#else
			ledcon_by_event(EV_DAIL_MODEM_ERROR);
#endif
		} else {
#if !(defined INHAND_IR8 || defined INHAND_ER6)
			cellular_ledcon(CELL_CONNECTING);
#else
			ledcon_by_event(EV_DAIL_START);
#endif
		}
#endif
		} else {
#ifdef HAVE_LED_WWAN
			ledcon(LEDMAN_CMD_FLASH, LED_WWAN);
#elif (defined INHAND_IDTU9 || defined INHAND_IG5) || defined(INHAND_IR8) || defined(INHAND_ER6)
			ledcon_by_event(EV_DAIL_DISABLE);

			// modem signal led
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#elif defined(INHAND_VG9) || defined INHAND_ER3
			cellular_ledcon(CELL_OFF);
#else 
			ledcon(LEDMAN_CMD_FLASH, LED_WARN);
#endif
		}

		//do dual sim switch
		if(modem_config->dualsim || gl_sim_switch_flag){
			ret = check_dualsim_switch(modem_config, dualsim_info);
			if(ret==0) {
				do_dualsim_switch();
				if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PHS8
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ELS31
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PVS8) {
					timeout = 25;
				}else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LUA10						
					|| get_modem_code(gl_modem_idx) ==IH_FEATURE_MODEM_ME909
					|| get_modem_code(gl_modem_idx) ==IH_FEATURE_MODEM_PLS8
					//|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_MC2716
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_MC7350) { 
					timeout = 30; /*LUA10 needs about 15s to be enumed*/
				} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300
					|| ih_license_support(IH_FEATURE_MODEM_U8300C)) {
					timeout = 40;
				} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LP41
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM520N
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500U
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RG500
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_FM650) {
					timeout = 15;
				} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC25
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EP06
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC20) {
					timeout = 20;
				} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_NL668) {
					timeout = 34;
				} else {
					timeout = 8;
				}
			}
		}
	
		if(gl_myinfo.priv.ppp_config.infinitely_dial_retry){
			LOG_IN("scanning modem (%d)...", gl_modem_reset_cnt);
		}else{
			LOG_IN("scanning modem (%d/%d)...", gl_modem_reset_cnt, MAX_MODEM_RESET);
		}
		//TODO: try to start modem
		//TODO: according errno to start modem
		fixup_at_list();
		if(gl_at_errno || gl_modem_reset_flag) {
			//if no modem, MODEM_FORCE_RESET
			if(gl_at_errno==ERR_AT_NOMODEM
					|| gl_at_errno==ERR_AT_NEED_RESET
					|| gl_modem_reset_flag==TRUE) {

#if (defined INHAND_IDTU9 || defined INHAND_IG5 || defined INHAND_IR8 || defined INHAND_ER6)
				ledcon_by_event(EV_DAIL_MODEM_ERROR);
#endif

				if(((gl_modem_code == IH_FEATURE_MODEM_MC2716) || gl_modem_code==IH_FEATURE_MODEM_MC5635 || ((gl_modem_code==IH_FEATURE_MODEM_ME909) && ih_key_support("TH09"))) 
				  && (ih_license_support(IH_FEATURE_WLAN_MTK) && ih_license_support(IH_FEATURE_IR9))) {
					  if(++gl_check_modem_err_cnt >= MAX_CHECK_MODEM_RETRY) {
						  LOG_ER("Modem is not found for %d times, restart system...", MAX_CHECK_MODEM_RETRY);
						  gl_check_modem_err_cnt = 0;
						  schedule_router_or_modem_reboot(); 
					  }			  
				}else if (gl_modem_code == IH_FEATURE_MODEM_U8300C
						|| gl_modem_code == IH_FEATURE_MODEM_FG360) {
					  if(++gl_check_modem_err_cnt >= MAX_CHECK_MODEM_RETRY) {
						  LOG_ER("Modem is not found for %d times, restart system...", MAX_CHECK_MODEM_RETRY);
						  gl_check_modem_err_cnt = 0;
						  schedule_router_or_modem_reboot(); 
					  }
				}
				gl_modem_reset_flag = FALSE;
				set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);
				if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PHS8 
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PLS8
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ELS31
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PVS8) {
					timeout = 25;
				}else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LUA10
					|| get_modem_code(gl_modem_idx) ==IH_FEATURE_MODEM_ME909
					//|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_MC2716						
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_MC7350) { 
					timeout = 30; /*LUA10 needs about 15s to be enumed*/
				} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300C){
					timeout = 40;
				} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_MC2716
						|| gl_modem_code==IH_FEATURE_MODEM_MC5635
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LARAR2){
					timeout = 25;
				} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LP41
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_TOBYL201) {
					timeout = 20;
				} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ME209
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LE910){
					timeout = 5;
					sleep(20);	
				} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC25
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EP06
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC20
					|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_NL668) {
					timeout = 20;
				} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM520N
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_FM650
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500U
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RG500) {
					timeout = 30;
				} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ELS81) {
					timeout = 12;
				} else {
					timeout = 8;
				}
			} else if(gl_at_errno==ERR_AT_NOSIGNAL
				    || gl_at_errno==ERR_AT_NOSIM
				    || gl_at_errno==ERR_AT_NOREG) {
				//single sim, soft reset per 3 time
				
#if (defined INHAND_IDTU9 || defined INHAND_IG5 || defined INHAND_IR8 || defined INHAND_ER6)
				if (gl_at_errno==ERR_AT_NOSIM) {
					ledcon_by_event(EV_DAIL_NOSIM);
				}else {
					ledcon_by_event(EV_DAIL_OTHER_ERROR);
				}
#endif
				if(!modem_config->dualsim && gl_modem_reset_cnt%MAX_REG_RETRY==0){

					if ((gl_at_errno==ERR_AT_NOSIM) && gl_modem_reset_cnt < (3*MAX_REG_RETRY)) {
						if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500
							|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM520N
							|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_FM650
							|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500U
							|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RG500
							|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_NL668){
							gl_modem_cfun_reset_flag = TRUE;
						}else{
							set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);
						}
					}else {
						set_modem(get_modem_code(gl_modem_idx), MODEM_CFUN_RESET);
					}
					if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PHS8
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PLS8
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LE910
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ME209
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC20
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC25
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM520N
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500U
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RG500
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_FM650
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EP06
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PVS8
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_NL668) {
						timeout = 20;
					}else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LUA10
						|| get_modem_code(gl_modem_idx) ==IH_FEATURE_MODEM_ME909		    				
						//|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_MC2716
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_MC7350) { 
						timeout = 30; /*LUA10 needs about 15s to be enumed*/
					} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300
						|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300C) {
						timeout = 40;
					} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LP41
							|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_TOBYL201
							|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LARAR2) {
						timeout = 15;
					} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ELS31) {
						timeout = 25;
					} else if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ELS81) {
						timeout = 12;
					} else {
						timeout = 8;
					}
				}else if ((get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300) && (gl_at_errno != ERR_AT_NOREG)){
						//timeout = 35;
				}
				gl_check_modem_err_cnt = 0;
			} else if (gl_at_errno==ERR_AT_NOVALIDIP) {
				if(!modem_config->dualsim && gl_modem_reset_cnt%MAX_UPDATE_APN_RETRY==0){
					set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);
					if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ELS31) {
						timeout = 25;
					} else {
						timeout = 8;
					}
				}
			}
		}else {
			if (0==(gl_ppp_redial_cnt%MAX_DHCP_RETRY)) {
				//if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PLS8) 
				if (get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_DHCP
					|| get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_MIPC
					|| get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_QMI) {
					if (gl_ppp_redial_cnt != 0) {
						if(ih_license_support(IH_FEATURE_MODEM_FM650)
							|| ih_license_support(IH_FEATURE_MODEM_FG360)
							|| ih_license_support(IH_FEATURE_MODEM_RM500U)
							|| ih_license_support(IH_FEATURE_MODEM_RM520N)
							|| ih_license_support(IH_FEATURE_MODEM_RG500)
							|| ih_license_support(IH_FEATURE_MODEM_NL668)){
							gl_modem_cfun_reset_flag = TRUE;
							timeout=10;
						}else if(ih_license_support(IH_FEATURE_MODEM_RM500)){
							set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);
							timeout=30;
						}else{
							set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);
							timeout=20;
						}
					}
				}
			}else if((gl_ppp_redial_cnt > 0) && !(gl_ppp_redial_cnt % 5)){
				if(ih_license_support(IH_FEATURE_PCIE2ETH_RTL8125B)){
					set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);
					timeout = 30;
				}
			}
						
			gl_check_modem_err_cnt = 0;
		}
		gl_redial_state = REDIAL_STATE_MODEM_LOAD_DRV;
		break;
	case REDIAL_STATE_MODEM_LOAD_DRV:
		//if need switch sim, change it at once
		if(!current_sim_is_right()) {
			ledcon_dualsim_switch();
			gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
			timeout = 1;
			break;
		}
		
		gl_modem_dev_state = MODEM_DEV_NONE;

		//find usb id, and load driver	
		if (usb_get_device_info(NULL, vendor_id, product_id, NULL, NULL, 1)>0) {
#if (!(defined INHAND_IP812))
			if (ih_license_support(IH_FEATURE_MODEM_PLS8)
				|| ih_license_support(IH_FEATURE_MODEM_ELS31)
				|| ih_license_support(IH_FEATURE_MODEM_ELS61)
				|| ih_license_support(IH_FEATURE_MODEM_LP41)){
				eval("insmod", "/modules/cdc-acm.ko");
				//eval("insmod", "/modules/cdc_ether.ko");
			} else if (ih_license_support(IH_FEATURE_MODEM_ELS81)) {
				eval("insmod", "/modules/cdc-wdm.ko");
				eval("insmod", "/modules/cdc-acm.ko");
				//eval("insmod", "/modules/cdc_mbim.ko");
			} else if (ih_license_support(IH_FEATURE_MODEM_TOBYL201)) {
				eval("insmod", "/modules/cdc-acm.ko");
				eval("insmod", "/modules/cdc_ether.ko");
				eval("insmod", "/modules/rndis_host.ko");
			} else {
				if ((get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LE910) && ih_license_support(IH_FEATURE_GPS)) { 
					eval("rmmod", "/modules/option.ko");
					eval("rmmod", "/modules/usb_wwan.ko");
				}
 
#if (defined INHAND_VG9)
				eval("rmmod", "/modules/usbserial.ko");
				sprintf(buf1, "vendor=0x%s", vendor_id);
				sprintf(buf2, "product=0x%s", product_id);
				eval("insmod", "/modules/usbserial.ko", "maxSize=4096", buf1, buf2);
#else 
				my_insmod("usbserial.ko", "maxSize=4096", NULL);
				my_insmod("usb_wwan.ko", NULL);
				sprintf(buf1, "vendor=0x%s", vendor_id);
				sprintf(buf2, "product=0x%s", product_id);
				my_insmod("option.ko", buf1, buf2, NULL);
				if(gl_modem_code == IH_FEATURE_MODEM_EP06
					|| gl_modem_code == IH_FEATURE_MODEM_EC20
					|| gl_modem_code == IH_FEATURE_MODEM_EC25
					|| gl_modem_code == IH_FEATURE_MODEM_RG500){
					my_insmod("cdc-wdm.ko", NULL);
					if(modem_support_dual_apn()){
						my_insmod("qmi_wwan_q.ko", "qmap_mode=6", NULL);
					}else{
						my_insmod("qmi_wwan_q.ko", NULL);
					}
				}else if(gl_modem_code == IH_FEATURE_MODEM_RM500U
					|| gl_modem_code == IH_FEATURE_MODEM_FM650){
					my_insmod("cdc_ncm.ko", NULL);
				}else if(gl_modem_code == IH_FEATURE_MODEM_NL668){
					my_insmod("cdc-wdm.ko", NULL);
					my_insmod("qmi_wwan.ko", NULL);
				}
#endif
			}
#else
			if (ih_license_support(IH_FEATURE_MODEM_MU609)
				|| ih_license_support(IH_FEATURE_MODEM_ME909)) {
				sprintf(buf1, "vendor=0x%s", vendor_id);
				sprintf(buf2, "product=0x%s", product_id);
				eval("insmod", "/modules/usbserial.ko", "maxSize=4096", buf1, buf2);
			} else if (ih_license_support(ih_feature_modem_mc7350)){
				eval("insmod", "/modules/gobiserial.ko");
			} else { //LUA10, U8300
				eval("insmod", "/modules/usbserial.ko", "maxSize=4096");
				eval("insmod", "/modules/usb_wwan.ko");
				sprintf(buf1, "vendor=0x%s", vendor_id);
				sprintf(buf2, "product=0x%s", product_id);
				eval("insmod", "/modules/option.ko", buf1, buf2);
			}
			
#endif
			gl_modem_dev_state |= MODEM_DEV_USB_ALIVE;
		}
		
		//find pci id, and load driver	
		if(find_pci_device(gl_modem_types[gl_modem_idx].pci_vid) > 0){
			if(gl_modem_code == IH_FEATURE_MODEM_RM500){
				if(modem_support_dual_apn()){
					my_insmod("pcie_mhi.ko", "qmap_mode=6", NULL);
				}else{
					my_insmod("pcie_mhi.ko", NULL);
				}
			}

			if(device_test(gl_modem_types[gl_modem_idx].pcieport, F_OK, 5)){
				gl_modem_dev_state |= MODEM_DEV_PCIE_ALIVE;
			}else{
				LOG_ER("PCIe interface(%s) is unusable!!", gl_modem_types[gl_modem_idx].pcieport);
				if(gl_modem_code == IH_FEATURE_MODEM_RM500){
					gl_modem_dev_state = MODEM_DEV_NONE;
				}
			}
		}
		
		//fg360 send at via socket.
		if(check_socket_device(gl_modem_types[gl_modem_idx].commport) == ERR_OK){
			gl_modem_dev_state |= MODEM_DEV_SOCK_ALIVE;
		}

		if(gl_modem_dev_state == MODEM_DEV_NONE){
			gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
			timeout = LIMIT(gl_modem_reset_cnt*20, 20, 120);
			gl_at_errno = ERR_AT_NOMODEM;
			LOG_ER("modem not found");
		}else{
			if(gl_modem_dev_state == MODEM_DEV_PCIE_ALIVE){
				gl_commport = gl_dataport = gl_modem_types[gl_modem_idx].pcieport;
			}else if(gl_modem_code == IH_FEATURE_MODEM_RM500U){
				if(gl_modem_dev_state & MODEM_DEV_PCIE_ALIVE){
					gl_commport = gl_dataport = gl_modem_types[gl_modem_idx].pcieport;
				}
			}
			
			/*clear global variable*/
			clear_at_state();

			//init comm com
			LOG_DB("commport:%s", gl_commport);
			gl_at_fd = open_dev(gl_commport);		
			modem_init_serial(gl_at_fd, 230400);

			//set read fd handle
			event_set(&gl_ev_at, gl_at_fd,
					EV_READ, redial_wait_at_callback, &gl_ev_at);
			event_base_set(g_my_event_base, &gl_ev_at);

			gl_at_ready = FALSE;
			gl_redial_state = REDIAL_STATE_MODEM_WAIT_AT;
		}
		break;
	case REDIAL_STATE_MODEM_WAIT_AT:
		//if need switch sim, change it at once
		if(modem_config->dualsim && gl_sim_switch_flag==TRUE) {
			ledcon_dualsim_switch();
			gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
			timeout = 1;
			break;
		}

		if(!gl_at_ready && (gl_at_try_n++ < 10)){
			bzero(gl_at_buf, sizeof(gl_at_buf));
			write(gl_at_fd, AT_INIT_CMD, strlen(AT_INIT_CMD));
			event_add(&gl_ev_at, NULL);
			timeout = AT_DEFAULT_TIMEOUT;
		}else{
			event_del(&gl_ev_at);
			CLOSE_FD(gl_at_fd);

			gl_redial_state = REDIAL_STATE_MODEM_INIT;
			if(gl_myinfo.priv.enable==TRUE){
				if(!gl_at_ready){
					gl_at_errno = ERR_AT_NOMODEM;
					gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
				}
			}

			LOG_IN("device %s is %sready", gl_commport, gl_at_ready ? "" : "not ");
			timeout = 2;
		}
		break;
	case REDIAL_STATE_MODEM_INIT:
		//if need switch sim, change it at once
		if(!current_sim_is_right()) {
			ledcon_dualsim_switch();
			gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
			timeout = 1;
			break;
		}

		save_raw_name_to_ifalias();

		/*clear global variable*/
		clear_at_state();
		
		check_modem_basic_info(&gl_myinfo.svcinfo.modem_info);
		
		if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_TOBYL201) {
			if (check_cellular_interface() < 0) {
				gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
				timeout = 20;
				gl_at_errno = ERR_AT_NOMODEM;
				break;
			}
		}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500){
			if((gl_modem_dev_state & MODEM_DEV_USB_ALIVE)
				&& (check_rm500q_data_iface(gl_commport, 1, gl_myinfo.priv.ppp_debug) == 0)){
				gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
				gl_modem_reset_flag = TRUE;
				timeout = 1;
				break;
			}
		}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM500U){
			if(check_rm500u_pcie_mode(gl_commport, 0, gl_myinfo.priv.ppp_debug) == 0){
				gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
				gl_modem_reset_flag = TRUE;
				timeout = 1;
				break;
			}
		}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_FM650){
			if(check_fm650_ippass(gl_commport, gl_myinfo.priv.ppp_debug) == 0){
				gl_modem_reset_flag = TRUE;
			}

			if((gl_modem_dev_state & MODEM_DEV_USB_ALIVE)
				&& (check_fm650_data_iface(gl_commport, "pcie", gl_myinfo.priv.ppp_debug) == 0)){
				gl_modem_reset_flag = TRUE;
			}

			if(gl_modem_reset_flag){
				gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
				timeout = 1;
				break;
			}
		}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_RM520N){
			if(check_rm520n_eth_setting(gl_commport, gl_myinfo.priv.ppp_debug) > 0){
				gl_modem_reset_flag = TRUE;
				gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
				timeout = 1;
				break;
			}
		}

#ifdef INHAND_WR3
	char buf[32];
	int current_sim = 0;
	char *p = NULL;

	send_at_cmd_sync(gl_commport, "AT+QUIMSLOT?\r\n", buf, sizeof(buf), 1);
	p = strstr(buf, "+QUIMSLOT:");
	if(p){
		p += strlen("+QUIMSLOT:");
		current_sim = atoi(p);
	}

	if(modem_info->current_sim != current_sim){
		if(modem_info->current_sim == SIM1){
			send_at_cmd_sync(gl_commport, "AT+QUIMSLOT=1\r\n", NULL, 0, 1);
		}else{
			send_at_cmd_sync(gl_commport, "AT+QUIMSLOT=2\r\n", NULL, 0, 1);
		}
	}
#endif

		check_nr5g_mode(gl_commport, gl_myinfo.priv.ppp_debug);

		enable_modem_auto_time_zone_update(gl_commport, gl_myinfo.priv.ppp_debug);

		enable_ipv4v6_for_tmobile_profile(gl_commport, gl_myinfo.priv.ppp_debug);

		if(should_enter_sleep_state()){
			LOG_IN("Modem sleep ...");
			gl_redial_state = REDIAL_STATE_MODEM_SLEEP;
		}else{
			gl_redial_state = REDIAL_STATE_MODEM_READY;
		}

		if (get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_WPD200) {
			wpd200_nai_setup(gl_commport, gl_myinfo.priv.ppp_debug);
			wpd200_get_nai_username(gl_commport, gl_myinfo.priv.ppp_debug);
		}

		if(gl_modem_cfun_reset_flag == TRUE){
        		set_modem(gl_modem_code, MODEM_CFUN_RESET);
			gl_modem_cfun_reset_flag = FALSE;
			timeout = 8;
		}
		break;
	case REDIAL_STATE_MODEM_SLEEP:
#ifdef INHAND_ER3
		event_del(&gl_ev_at);
		CLOSE_FD(gl_at_fd);
	
		gl_at_fd = open_dev(gl_commport);		
		if(gl_at_fd > 0){
			modem_init_serial(gl_at_fd, 230400);
			//set read fd handle
			event_set(&gl_ev_at, gl_at_fd, EV_READ, redial_wait_sim_callback, &gl_ev_at);
			event_base_set(g_my_event_base, &gl_ev_at);
			event_add(&gl_ev_at, NULL);
		}
		timeout = 2 * 60;
#else
		timeout = 365 * 24 * 3600;
#endif
		break;
	case REDIAL_STATE_MODEM_READY:
		//if need switch sim, change it at once
		if(!current_sim_is_right()) {
			ledcon_dualsim_switch();
			gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
			timeout = 1;
			break;
		}

		//check enable option
		if(gl_myinfo.priv.enable==TRUE) {
#ifdef HAVE_LED_WWAN
			ledcon(LEDMAN_CMD_OFF, LED_WWAN);
#elif (defined INHAND_IDTU9 || defined INHAND_IG5)
			ledcon_by_event(EV_DAIL_START);
#elif defined(INHAND_VG9) || defined(INHAND_IR8) || defined(INHAND_ER6) || defined(INHAND_ER3)

#else
			ledcon(LEDMAN_CMD_ON, LED_WARN);
#endif
			//maybe network type changed, fixup at list again.
			fixup_at_list();

			clear_at_state();
			LOG_DB("redial modem ready state");
			//open at com, set event
			//some module, dataport cannot set AT, open comport
			if(ih_license_support(IH_FEATURE_MODEM_MC7350)			
						|| ih_license_support(IH_FEATURE_MODEM_ELS81)){								
				gl_at_fd = open_dev(gl_commport);	
			}else {
				gl_at_fd = open_dev(gl_dataport);		
			}
			modem_init_serial(gl_at_fd, 230400);

			//check and run at task
			run_redial_pending_task(gl_redial_pending_task, gl_myinfo.priv.ppp_debug);
		
			if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_EC20
				|| get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_U8300C){
				if(check_modem_nv_parameter(gl_myinfo.priv.ppp_debug)){
					gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
					gl_modem_reset_flag = TRUE;
					//we should close gl_at_fd before rebooting modem, otherwise we will receive SIGHUP signal!!
					CLOSE_FD(gl_at_fd);
					timeout = 1;
					break;
				}
			}
			//set read fd handle
			event_set(&gl_ev_at, gl_at_fd,
					EV_READ, redial_at_callback, &gl_ev_at);
			event_base_set(g_my_event_base, &gl_ev_at);
			gl_redial_state = REDIAL_STATE_AT;
		} else {
#if (defined INHAND_IDTU9 || defined INHAND_IG5 || defined INHAND_IR8 || defined INHAND_ER6)
			ledcon_by_event(EV_DAIL_DOWN);
#endif
			if(sms_config->enable) {
				LOG_DB("redial goto sms only state");
				sms_init(gl_commport, sms_config->mode);
				if(sms_config->interval) {
					evutil_timerclear(&tv);
					tv.tv_sec = sms_config->interval;
					event_add(&gl_ev_sms_tmr, &tv);
				}
				gl_redial_state = REDIAL_STATE_SMS_ONLY;
			} else {
				timeout = 5;
			}
		}

		break;
	case REDIAL_STATE_SMS_ONLY:
		//check enable option
		if(gl_myinfo.priv.enable==TRUE) {
			event_del(&gl_ev_sms_tmr);

			gl_redial_state = REDIAL_STATE_MODEM_READY;
		} else {
			timeout = 5;
		}
		break;
	case REDIAL_STATE_AT:
		//if need switch sim, change it at once
		if(!current_sim_is_right()) {
			ledcon_dualsim_switch();
			gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
			timeout = 1;
			break;
		}

		memcpy(&myinfo_tmp, &gl_myinfo, sizeof(MY_REDIAL_INFO));
		//before reach max, send at cmd
		if(gl_at_lists[gl_at_index]!=-1) {
			if ((gl_modem_code==IH_FEATURE_MODEM_U8300C) && (gl_at_index >= U8300C_MORE_AT_OFFSET)) {
				check_current_net_type(gl_commport); 
			}

			pcmd = find_at_cmd2(gl_at_lists[gl_at_index], gl_at_try_n);
			if(gl_at_try_n++ < pcmd->max_retry) {
				if(pcmd->desc) LOG_IN("detecting modem %s (%d/%d)...", pcmd->desc, gl_at_try_n, pcmd->max_retry);
				gl_at_errno = pcmd->err_code;
				gl_at_buf[0] = '\0';
				if(gl_modem_code == IH_FEATURE_MODEM_FG360){//reconnect
					event_del(&gl_ev_at);
					CLOSE_FD(gl_at_fd);
					gl_at_fd = open_dev(gl_dataport);		
					event_set(&gl_ev_at, gl_at_fd,
							EV_READ, redial_at_callback, &gl_ev_at);
					event_base_set(g_my_event_base, &gl_ev_at);
				}
				modem_write(gl_at_fd, pcmd->atc, strlen(pcmd->atc), 0);
				//add read event
				event_add(&gl_ev_at, NULL);
				timeout = pcmd->timeout;
			} else {
				event_del(&gl_ev_at);
				/*skip no important command*/
				if(gl_at_errno==ERR_AT_OK || gl_at_errno==ERR_AT_NAK) {
					LOG_IN("retry %s reach max %d, skip it", pcmd->name, pcmd->max_retry);
					//reschedule next command
					gl_at_index++;
					gl_at_try_n = 0;
				} else if (gl_at_errno==ERR_AT_NEED_RESET){
					timeout = 1;
					CLOSE_FD(gl_at_fd);
					gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
				} else if (ih_license_support(IH_FEATURE_MODEM_LUA10)
					&& (gl_at_lists[gl_at_index] == AT_CMD_CEREG)
					&& (gl_myinfo.priv.modem_config.network_type == MODEM_NETWORK_AUTO)) {
					/*Workaround for LUA10: +CEREG? would be failed in AUTO mode*/
					LOG_IN("retry %s reach max %d, skip it and try 'AT+CREG?'", pcmd->name, pcmd->max_retry);
					//reschedule next command
					gl_at_index++;
					gl_at_try_n = 0;
				} else if ((ih_license_support(IH_FEATURE_MODEM_U8300)
						|| ih_license_support(IH_FEATURE_MODEM_EP06)
						|| ih_license_support(IH_FEATURE_MODEM_EC25))
					&& (gl_at_lists[gl_at_index] == AT_CMD_CEREG)
					&& (gl_myinfo.priv.modem_config.network_type == MODEM_NETWORK_AUTO 
						|| gl_myinfo.priv.modem_config.network_type == MODEM_NETWORK_3G
						|| gl_myinfo.priv.modem_config.network_type == MODEM_NETWORK_2G)) {
					/*Workaround for u8300: +CEREG? would be failed in AUTO mode*/
					LOG_IN("retry %s reach max %d, skip it and try 'AT+CREG?'", pcmd->name, pcmd->max_retry);
					//reschedule next command
					gl_at_lists[11] = AT_CMD_CGREG;
					gl_at_index++;
					gl_at_try_n = 0;
				}else if (ih_license_support(IH_FEATURE_MODEM_U8300C) && (gl_at_lists[gl_at_index] == AT_CMD_CIMI)){
					LOG_IN("retry %s reach max %d, skip it and try 'AT+QCIMI'", pcmd->name, pcmd->max_retry);
					gl_at_lists[++gl_at_index] = AT_CMD_QCIMI;
					gl_at_try_n = 0;
				}else if (ih_license_support(IH_FEATURE_MODEM_U8300C) && (gl_at_lists[gl_at_index] == AT_CMD_CPIN)){
					LOG_IN("retry %s reach max %d, skip it and try 'AT+QCPIN'", pcmd->name, pcmd->max_retry);
					gl_at_lists[++gl_at_index] = AT_CMD_QCPIN;
					gl_at_try_n = 0;
				}else if ((ih_license_support(IH_FEATURE_MODEM_PLS8)
							|| ih_license_support(IH_FEATURE_MODEM_LARAR2)
							|| ih_license_support(IH_FEATURE_MODEM_ELS81)
							|| ih_license_support(IH_FEATURE_MODEM_TOBYL201)
							|| ih_license_support(IH_FEATURE_MODEM_ELS61)) && (gl_myinfo.priv.modem_config.network_type == MODEM_NETWORK_AUTO 
						|| gl_myinfo.priv.modem_config.network_type == MODEM_NETWORK_4G) && (gl_at_lists[gl_at_index] == AT_CMD_CEREG)) {
					LOG_IN("retry %s reach max %d, skip it and try 'AT+CREG?'", pcmd->name, pcmd->max_retry);
					if (ih_license_support(IH_FEATURE_MODEM_TOBYL201)) {
						gl_at_lists[15] = AT_CMD_CREG;
					} else {
						gl_at_lists[12] = AT_CMD_CREG;
					}
					//reschedule next command
					gl_at_index++;
					gl_at_try_n = 0;
				}else if (ih_license_support(IH_FEATURE_MODEM_ELS31)
						&& (gl_at_lists[gl_at_index] == AT_CMD_CEREG)
						&& (gl_at_lists[gl_at_index+1] == AT_CMD_AT)) {
					gl_at_lists[gl_at_index++] = AT_CMD_CEREG;
					gl_at_try_n = 0;
				}else {
					LOG_IN("retry %s reach max %d, re-scan modem", pcmd->name, pcmd->max_retry);
					CLOSE_FD(gl_at_fd);
#if 0
					if((gl_modem_code == IH_FEATURE_MODEM_U8300) && (gl_at_lists[gl_at_index] == AT_CMD_CSQ)){
						LOG_IN("The modem has no signal, reset modem!");
						set_modem(gl_modem_code, MODEM_RESET);
					}
#endif
					gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
					timeout = gl_myinfo.priv.dial_interval;
				}
			}

			if (memcmp(&gl_myinfo, &myinfo_tmp, sizeof(MY_REDIAL_INFO)) != 0) {
				//broadcast svcinfo
				ret = ih_ipc_publish(IPC_MSG_REDIAL_SVCINFO, (char *)&gl_myinfo, sizeof(MY_REDIAL_INFO));
				if(ret) LOG_ER("signal refresh broadcast msg is not ok.");
			}

			//if((pcmd == NULL) || ((pcmd->atc)[0] == '\0'))			
			//gl_at_index++;
		} else {
			clear_at_state();
			gl_modem_reset_cnt = 0;
			//FIXME:
			event_del(&gl_ev_at);
			CLOSE_FD(gl_at_fd);

			//try to update pol file
			start_redial_pending_task(REDIAL_PENDING_TASK_UPDATE_POL);

#if defined(INHAND_VG9) || defined INHAND_ER3
			cellular_ledcon(CELL_CONNECTING);
#elif defined(INHAND_IR8) || defined(INHAND_ER6)
			ledcon_by_event(EV_DAIL_START);
#endif
			//set timeout
			if(get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP
				|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_MIPC
				|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI){
				gl_redial_state = REDIAL_STATE_DHCP;
				if(gl_redial_dial_state == REDIAL_DIAL_STATE_KILLED){
					LOG_IN("waiting activate the link..."); 
				}

				if(gl_myinfo.priv.ppp_config.ppp_mode==PPP_DEMAND) {
					//add sms timer
					if(sms_config->enable) {
						sms_init(gl_commport, sms_config->mode);
						if(sms_config->interval) {
							evutil_timerclear(&tv);
							tv.tv_sec = sms_config->interval;
							event_add(&gl_ev_sms_tmr, &tv);
						}
					}
				}
			}else if (get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_NONE) {
				if(gl_myinfo.priv.ppp_config.ppp_mode==PPP_DEMAND) {
					//add sms timer
					if(sms_config->enable) {
						sms_init(gl_commport, sms_config->mode);
						if(sms_config->interval) {
							evutil_timerclear(&tv);
							tv.tv_sec = sms_config->interval;
							event_add(&gl_ev_sms_tmr, &tv);
						}
					}
				}
				gl_redial_state = REDIAL_STATE_GET_STATIC_PARAMS;
			}else{
				gl_redial_state = REDIAL_STATE_PPP;
			}
		}
		//TODO: check data port because AT fail when kill -9 pppd
		break;
	case REDIAL_STATE_PPP:
		/*check enable & backup option before dialup*/
		if(gl_myinfo.priv.enable==FALSE ||
		    (gl_myinfo.priv.backup && gl_myinfo.priv.backup_action!=BACKUP_UP)) {
#if !(defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_WR3)
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#endif

#ifdef HAVE_LED_WWAN
			ledcon(LEDMAN_CMD_OFF, LED_WWAN);
#elif defined(INHAND_VG9) || defined(INHAND_IR8) || defined(INHAND_ER6) || defined(INHAND_WR3) || defined(INHAND_ER3)

#elif !(defined INHAND_IDTU9 || defined INHAND_IG5)
			ledcon(LEDMAN_CMD_ON, LED_WARN);
#endif
			
			gl_redial_state = REDIAL_STATE_PPP;
		} else {
			//if need switch sim, change it at once
			if(!current_sim_is_right()) {
				ledcon_dualsim_switch();
				gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
			} else {
#ifdef HAVE_LED_WWAN
				ledcon(LEDMAN_CMD_FLASH, LED_WWAN);
#elif defined(INHAND_VG9) || defined(INHAND_IR8) || defined(INHAND_ER6) || defined(INHAND_WR3) || defined(INHAND_ER3)

#elif !(defined INHAND_IDTU9 || defined INHAND_IG5)
				ledcon(LEDMAN_CMD_FLASH, LED_WARN);
#endif
				if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_PVS8){
					char buf[1024]={0};
					pcmd = find_at_cmd(AT_CMD_VNUM);
					if(pcmd) {
						ret = send_at_cmd_sync_timeout(gl_commport, pcmd->atc, buf, sizeof(buf), 0,"OK",3000);
						if(ret==0) {
							pvs8_activation_action(pcmd, buf, modem_info, modem_config);
						}
					}

					ret = disable_pvs8_tetheredNai();
					if(ret < 0){
						LOG_WA("CDMA/TetheredNai should be disabled.");	
					}
				}
				if((get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PVS8) && !pvs8_activation){
					timeout = 60;
					LOG_DB("PVS8 module is inactive, cannot execute ppp, cnt %d, waiting for %d s", gl_ppp_redial_cnt, timeout);
					gl_redial_state = REDIAL_STATE_PPP;
					break;
				}else {
					if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PVS8 &&
						modem_config->network_type == MODEM_NETWORK_AUTO){
						if(pvs8_throttling()){
							LOG_IN("PVS8 module change network type reset");
						}		
					}else{
						if (gl_ppp_redial_cnt++>MAX_PPP_REDIAL) {
							LOG_ER("too many ppp redials, restart system...");
							schedule_router_or_modem_reboot(); 
						}else{
							LOG_IN("ppp redial (%d/%d)...", gl_ppp_redial_cnt, MAX_PPP_REDIAL);
						}
					}
					if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_MC7350){	
						ret =at_ppp(gl_commport, gl_dataport, &gl_myinfo.priv.ppp_config,modem_config, modem_info, gl_myinfo.priv.ppp_debug);
						if(ret<0){
							LOG_IN("redial AT PPP err, ret %d",ret);
							gl_redial_state = REDIAL_STATE_PPP;
						}
					}else {
						ppp_build_config(gl_dataport, &gl_myinfo.priv.ppp_config,modem_config, modem_info, gl_modem_code, gl_myinfo.priv.ppp_debug);
					}
					//start ppp
					if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PVS8){
						LOG_IN("PVS8 start ppp (%d/%d)... network type %d", gl_ppp_redial_cnt, MAX_PPP_REDIAL,pvs8_throttling_net);
					}
					start_ppp();
					//TODO: if ppp demand
					//pinfo->status = IF_UP;
					//add ppp monitor timer
					if(gl_myinfo.priv.ppp_config.ppp_mode==PPP_ONLINE) {
						evutil_timerclear(&tv);
						tv.tv_sec = REDIAL_PPP_TIMEOUT;//10 mins
						event_add(&gl_ev_ppp_tmr, &tv);
					} else if(gl_myinfo.priv.ppp_config.ppp_mode==PPP_DEMAND) {
						//add sms timer
						if(sms_config->enable) {
							sms_init(gl_commport, sms_config->mode);
							if(sms_config->interval) {
								evutil_timerclear(&tv);
								tv.tv_sec = sms_config->interval;
								event_add(&gl_ev_sms_tmr, &tv);
							}
						}
					}

					gl_redial_state = REDIAL_STATE_CONNECTING;
				}
			}
		}

		if(gl_redial_state == REDIAL_STATE_PPP){
			timeout = 5;
		}
		break;
	case REDIAL_STATE_DHCP:
		/*check enable & backup option before dialup*/
		if(gl_myinfo.priv.enable==FALSE ||
		    (gl_myinfo.priv.backup && gl_myinfo.priv.backup_action!=BACKUP_UP)) {
#if !(defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_WR3)
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#endif

#ifdef HAVE_LED_WWAN
			ledcon(LEDMAN_CMD_OFF, LED_WWAN);
#elif defined(INHAND_VG9) || defined(INHAND_IR8) || defined(INHAND_ER6) || defined(INHAND_WR3) || defined(INHAND_ER3)

#elif !(defined INHAND_IDTU9 || defined INHAND_IG5)
			ledcon(LEDMAN_CMD_ON, LED_WARN);
#endif
			gl_redial_state = REDIAL_STATE_DHCP;
		} else {
			//if need switch sim, change it at once
			if(!current_sim_is_right()) {
				ledcon_dualsim_switch();
				gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
			}else if(gl_redial_dial_state == REDIAL_DIAL_STATE_KILLED){
				gl_redial_state = REDIAL_STATE_DHCP;
			} else{
#ifdef HAVE_LED_WWAN
				ledcon(LEDMAN_CMD_FLASH, LED_WWAN);
#elif defined(INHAND_VG9) || defined(INHAND_IR8) || defined(INHAND_ER6) || defined(INHAND_WR3) || defined(INHAND_ER3)

#elif !(defined INHAND_IDTU9 || defined INHAND_IG5)
				ledcon(LEDMAN_CMD_FLASH, LED_WARN);
#endif

				LOG_IN("start %s (%d/%d)...", get_sub_deamon_name(), gl_ppp_redial_cnt, MAX_PPP_REDIAL);

				if (gl_ppp_redial_cnt++>MAX_PPP_REDIAL) {
					LOG_ER("too many %s retries...", get_sub_deamon_name());
					schedule_router_or_modem_reboot(); 
					//system("reboot");
					//while(1) sleep(1000);
					//timeout = 20;
					//break;
					gl_ppp_redial_cnt = 0;
				}else{
					//LOG_IN("start dhcp client (%d/%d)...", gl_ppp_redial_cnt, MAX_PPP_REDIAL);
				}

				ret =active_modem_data_connection(gl_commport, &gl_myinfo.priv.ppp_config,modem_config, modem_info, gl_myinfo.priv.ppp_debug);
				if(ret<0){
					LOG_IN("active data connection err, ret %d",ret);
					//gl_redial_state = REDIAL_STATE_DHCP;
				}

				if (ih_license_support(IH_FEATURE_MODEM_LP41)
					|| (ih_license_support(IH_FEATURE_MODEM_ME909) 
						&& (ih_key_support("FH09")
						|| ih_key_support("FH19")))) {
					if (ih_check_interface_exist(USB0_WWAN)) {
						start_dhcpc(USB0_WWAN, WWAN_ID1, gl_myinfo.priv.ppp_debug);
					} else {	
						start_dhcpc(ETH2_WWAN, WWAN_ID1, gl_myinfo.priv.ppp_debug);
					}
				}else if (ih_license_support(IH_FEATURE_MODEM_RM500)
					|| ih_license_support(IH_FEATURE_MODEM_RG500)
					|| ih_license_support(IH_FEATURE_MODEM_EC20)
					|| ih_license_support(IH_FEATURE_MODEM_EC25)
					|| ih_license_support(IH_FEATURE_MODEM_EP06)
					|| ih_license_support(IH_FEATURE_MODEM_NL668)){
					start_ih_qmi(WWAN_ID1, gl_myinfo.priv.ppp_debug);
					if(dual_apn_is_enabled()){
						start_ih_qmi(WWAN_ID2, gl_myinfo.priv.ppp_debug);
					}
				}else if (ih_license_support(IH_FEATURE_MODEM_ELS61)) {
						start_dhcpc(ETH2_WWAN, WWAN_ID1, gl_myinfo.priv.ppp_debug);
				}else if (ih_license_support(IH_FEATURE_MODEM_FM650)
					|| ih_license_support(IH_FEATURE_MODEM_RM500U)) {
					start_dhcpc(PCIE0_WWAN, WWAN_ID1, gl_myinfo.priv.ppp_debug);
				}else if (ih_license_support(IH_FEATURE_MODEM_FG360)) {
					start_mipc_wwan(WWAN_ID1, gl_myinfo.priv.ppp_debug);
				}else if (ih_license_support(IH_FEATURE_MODEM_RM520N)) {
					start_dhcpc(APN1_WWAN, WWAN_ID1, gl_myinfo.priv.ppp_debug);
				}else {
					start_dhcpc(USB0_WWAN, WWAN_ID1, gl_myinfo.priv.ppp_debug);
				}

				if(gl_myinfo.priv.ppp_config.ppp_mode==PPP_ONLINE) {
					evutil_timerclear(&tv);
					tv.tv_sec = REDIAL_DHCP_TIMEOUT;
					event_add(&gl_ev_ppp_tmr, &tv);
				}
				
				gl_dual_wwan_enable = dual_apn_is_enabled();
				gl_redial_dial_state = REDIAL_DIAL_STATE_STARTED;
				gl_redial_state = REDIAL_STATE_CONNECTING;
			}
		}
		
		if(gl_redial_state == REDIAL_STATE_DHCP){
			timeout = 5;
		}
		break;

	case REDIAL_STATE_GET_STATIC_PARAMS:
		if(gl_myinfo.priv.enable==FALSE ||
		    (gl_myinfo.priv.backup && gl_myinfo.priv.backup_action!=BACKUP_UP)) {
#if !(defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_WR3)
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#endif

#ifdef HAVE_LED_WWAN
			ledcon(LEDMAN_CMD_OFF, LED_WWAN);
#elif defined(INHAND_VG9) || defined(INHAND_IR8) || defined(INHAND_ER6) || defined(INHAND_WR3) || defined(INHAND_ER3)

#elif !(defined INHAND_IDTU9 || defined INHAND_IG5)
			ledcon(LEDMAN_CMD_ON, LED_WARN);
#endif
			gl_redial_state = REDIAL_STATE_GET_STATIC_PARAMS;
		}else if (gl_redial_dial_state == REDIAL_DIAL_STATE_KILLED) {
				gl_redial_state = REDIAL_STATE_GET_STATIC_PARAMS;
		} else {
			//if need switch sim, change it at once
			if(!current_sim_is_right()) {
				ledcon_dualsim_switch();
				gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
				timeout = 5;	
				break;
			}

			LOG_IN("start building data connection (%d/%d)...", gl_ppp_redial_cnt, MAX_PPP_REDIAL);

			if (gl_ppp_redial_cnt++>MAX_PPP_REDIAL) {
				LOG_ER("too many retries...");
				schedule_router_or_modem_reboot(); 
				//system("reboot");
				//while(1) sleep(1000);
				//timeout = 20;
				//break;
				gl_ppp_redial_cnt = 0;
			}else{
				//LOG_IN("start dhcp client (%d/%d)...", gl_ppp_redial_cnt, MAX_PPP_REDIAL);
			}

			if (ih_license_support(IH_FEATURE_MODEM_ELS81)) {
				//build_mbim_config_file(&gl_myinfo.priv.ppp_config, &gl_myinfo.svcinfo.modem_info);
				//clear_mbim_child_processes();
#if 0
				ret = mbim_start_data_connection_by_cli(gl_dataport);
				if (ret < 0) {
					LOG_ER("failed to build data connection!");
					clear_mbim_child_processes();
					//set_modem(get_modem_code(gl_modem_idx), MODEM_FORCE_RESET);

					gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
					//timeout = 15;
					//gl_at_errno = ERR_AT_NOMODEM;
					break;
				}
#endif
				ret =active_modem_data_connection(gl_commport, &gl_myinfo.priv.ppp_config,modem_config, modem_info, gl_myinfo.priv.ppp_debug);
				if(ret<0){
					LOG_IN("active data connection err, ret %d",ret);
					//gl_redial_state = REDIAL_STATE_DHCP;
				}
				sleep(1);
			} 

			ret = get_network_parameters(1, USB0_WWAN);
			if (ret < 0) {
				gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
				//timeout = 3;
				break;
			}

			evutil_timerclear(&tv);
			if(modem_config->dualsim)
				tv.tv_sec = dualsim_info->uptime;
			else
				tv.tv_sec = REDIAL_PPP_UP_TIME;
			event_add(&gl_ev_ppp_up_tmr, &tv);


			gl_redial_state = REDIAL_STATE_CONNECTING;
		}
		break;

	case REDIAL_STATE_CONNECTING:
		timeout = 1;
		if(get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_NONE) {
			//goto check_vif_status;
		}

		if(((gl_ppp_pid<0) && (gl_odhcp6c_pid <0))
			|| (gl_dual_wwan_enable && (gl_2nd_ppp_pid < 0) && (gl_2nd_odhcp6c_pid < 0))) {
			pinfo->status = VIF_DOWN;
			secondary_vif_info->status = VIF_DOWN;

			gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
			//redial interval
			if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PVS8){
				if( modem_config->network_type == MODEM_NETWORK_AUTO && 
				pvs8_throttling_net != MODEM_NETWORK_2G){
					timeout = 10;
				}else {
					if(gl_ppp_redial_cnt >5){
						timeout = PVS8_MAX_REDIAL_INTERVAL;
					}else {
						timeout = redial_interval[gl_ppp_redial_cnt];
					}
					LOG_IN("PVS8 ppp connecting redial (%d/%d)...waiting for %d minutes", 
						gl_ppp_redial_cnt, MAX_PPP_REDIAL,timeout);		
					timeout *= 60;
				}
			}else if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_LE910){
				if (0==gl_myinfo.priv.ppp_config.profile_idx){
					if (gl_ppp_redial_cnt==1 || gl_ppp_redial_cnt%5==0) {
						vzw_ota_update();
					}
					LOG_IN("FT10 auto mode: waiting for 20 second"); 
					timeout = FT10_AUTO_REDIAL_INTERVAL;
				}else {
					timeout = gl_myinfo.priv.dial_interval;
				}
			}else if(get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_LARAR2){
				timeout = get_rogers_dial_interval(gl_ppp_redial_cnt);
			}else {
				timeout = gl_myinfo.priv.dial_interval;
			}
#if !(defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_WR3)
			//clear led
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#endif
#ifdef HAVE_LED_WWAN
			ledcon(LEDMAN_CMD_OFF, LED_WWAN);
#elif defined(INHAND_VG9) || defined(INHAND_IR8) || defined(INHAND_ER6) || defined(INHAND_WR3) || defined(INHAND_ER3)

#elif !(defined INHAND_IDTU9 || defined INHAND_IG5)
			ledcon(LEDMAN_CMD_ON, LED_WARN);
#endif
			if(gl_myinfo.priv.ppp_config.ppp_mode==PPP_ONLINE) {
				//clear ppp monitor timer
				event_del(&gl_ev_ppp_tmr);
			} else if(gl_myinfo.priv.ppp_config.ppp_mode==PPP_DEMAND) {
				//clear sms timer
				event_del(&gl_ev_sms_tmr);
			}
		}

//check_vif_status:
		//check ppp state, dispatch it
		if((pinfo->status==VIF_UP && !gl_dual_wwan_enable)
			|| (gl_dual_wwan_enable && (pinfo->status==VIF_UP) && (secondary_vif_info->status==VIF_UP))) {

#ifdef HAVE_LED_WWAN
			ledcon(LEDMAN_CMD_ON, LED_WWAN);
#elif (defined INHAND_IDTU9 || defined INHAND_IG5)
			ledcon_by_event(EV_DAIL_UP);
#elif defined(INHAND_VG9)
			cellular_ledcon(CELL_CONNECTED);
#elif defined(INHAND_ER3)
			if(modem_info->network == CELL_NET_STATUS_5G){
				cellular_ledcon(CELL_CONNECTED_5G);
			}else{
				cellular_ledcon(CELL_CONNECTED);
			}
#elif defined(INHAND_IR8) || defined(INHAND_ER6)
			ledcon_by_event(EV_DAIL_UP);
#else
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
#endif
			LOG_IN("Interface Cellular1, changed state to up");

			//broadcast svcinfo
			ret = ih_ipc_publish(IPC_MSG_REDIAL_SVCINFO, (char *)&gl_myinfo, sizeof(MY_REDIAL_INFO));
			if(ret) LOG_ER("broadcast msg is not ok.");

			//broadcast ppp up
			ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)pinfo, sizeof(VIF_INFO));
			if(gl_dual_wwan_enable){
				ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)secondary_vif_info, sizeof(VIF_INFO));
			}
			snprintf(data[0], MAX_EVENT_DATA_LEN, "sim%d", modem_info->current_sim);
			EV_RECORD(EV_CODE_UPLINK_STATE_C, (modem_info->current_sim == SIM2) ? UPLINK_SIM2 : UPLINK_SIM1);
			if(ret) LOG_ER("broadcast msg is not ok.");
			/*publish sub interface*/
			if(sub_ifs[modem_info->current_sim-1].if_info.type) {
				LOG_IN("Interface Cellular1.%d, changed state to up", modem_info->current_sim);
				sub_vif_info[modem_info->current_sim-1] = *pinfo;
				sub_vif_info[modem_info->current_sim-1].if_info = sub_ifs[modem_info->current_sim-1].if_info;
				ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)&sub_vif_info[modem_info->current_sim-1], sizeof(VIF_INFO));
				if(ret) LOG_ER("broadcast msg is not ok.");
			}
			//add signal, sms timer
			evutil_timerclear(&tv);
			if(gl_myinfo.priv.ppp_config.ppp_mode==PPP_ONLINE && sms_config->enable) {
				sms_init(gl_commport, sms_config->mode);
				if(sms_config->interval) {
					tv.tv_sec = sms_config->interval;
					event_add(&gl_ev_sms_tmr, &tv);
				}
			}
			if(gl_myinfo.priv.signal_interval>0) {
				//tv.tv_sec = gl_myinfo.priv.signal_interval;
				tv.tv_sec = gl_myinfo.priv.signal_interval < 20 ? gl_myinfo.priv.signal_interval : 20;
				event_add(&gl_ev_signal_tmr, &tv);
			}
			if(gl_myinfo.priv.ppp_config.ppp_mode==PPP_ONLINE) {
				//clear ppp monitor timer
				event_del(&gl_ev_ppp_tmr);
			}

			if(get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP
				|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_NONE) {
				tv.tv_sec = REDIAL_CHECK_CONN_STATE_INTERVAL ;
				event_add(&gl_ev_wwan_connect_tmr, &tv);
			}

			if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PVS8 &&
				modem_config->network_type == MODEM_NETWORK_AUTO){
				ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=8\r\n", NULL, 0, 0);
				if(ret<0) {
					ret = send_at_cmd_sync(gl_commport, "AT^PREFMODE=8\r\n", NULL, 0, 0);
				}
				pvs8_throttling_net = MODEM_NETWORK_AUTO;
			}
			gl_wait_reconnect_cnt = 0;
			gl_redial_state = REDIAL_STATE_CONNECTED;
		}
		break;
	case REDIAL_STATE_CONNECTED:
		timeout = 5;
		
		//TODO:if dualsim and flow overflow do switch
		if(((gl_ppp_pid<0) && (gl_odhcp6c_pid <0)) 
			|| (gl_dual_wwan_enable && (gl_2nd_ppp_pid < 0) && (gl_2nd_odhcp6c_pid < 0))) {
			LOG_IN("Interface Cellular1, changed state to down");
			//broadcast ppp down
			pinfo->status = VIF_DOWN;
			ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)pinfo, sizeof(VIF_INFO));
			if(gl_dual_wwan_enable){
				secondary_vif_info->status = VIF_DOWN;
				ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)secondary_vif_info, sizeof(VIF_INFO));
			}
			snprintf(data[0], MAX_EVENT_DATA_LEN, "sim%d", modem_info->current_sim);
			EV_RECORD(EV_CODE_UPLINK_STATE_D, (modem_info->current_sim == SIM2) ? UPLINK_SIM2 : UPLINK_SIM1);
			if(ret) LOG_ER("broadcast msg is not ok.");
			/*publish sub interface*/
			if(sub_ifs[modem_info->current_sim-1].if_info.type) {
				LOG_IN("Interface Cellular1.%d, changed state to down", modem_info->current_sim);
				sub_vif_info[modem_info->current_sim-1] = *pinfo;
				sub_vif_info[modem_info->current_sim-1].if_info = sub_ifs[modem_info->current_sim-1].if_info;
				ret = ih_ipc_publish(IPC_MSG_VIF_LINK_CHANGE, (char *)&sub_vif_info[modem_info->current_sim-1], sizeof(VIF_INFO));
				if(ret) LOG_ER("broadcast msg is not ok.");
			}
			//broadcast svcinfo
			//ret = ih_ipc_publish(IPC_MSG_REDIAL_SVCINFO, (char *)&gl_myinfo.svcinfo, sizeof(REDIAL_SVCINFO));
			ret = ih_ipc_publish(IPC_MSG_REDIAL_SVCINFO, (char *)&gl_myinfo, sizeof(MY_REDIAL_INFO));
			if(ret) LOG_ER("broadcast msg is not ok.");
			//clear sms, signal timer
			event_del(&gl_ev_sms_tmr);
			event_del(&gl_ev_signal_tmr);
			event_del(&gl_ev_ppp_up_tmr);
			event_del(&gl_ev_wwan_connect_tmr);
/*
			if ((get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PLS8) && (gl_redial_dial_state == REDIAL_DIAL_STATE_KILLED)){
				gl_redial_state = REDIAL_STATE_DHCP;
			}else 
*/
			gl_redial_state = REDIAL_STATE_MODEM_STARTUP;
			//redial interval
			if(gl_myinfo.priv.enable){
				timeout = gl_myinfo.priv.dial_interval;
			}else{
				timeout = 1;
			}
#if !(defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_WR3)
			//clear led
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#endif

#ifdef HAVE_LED_WWAN
			ledcon(LEDMAN_CMD_OFF, LED_WWAN);
#elif (defined INHAND_IDTU9 || defined INHAND_IG5)
			ledcon_by_event(EV_DAIL_DOWN);
#elif defined(INHAND_VG9) || defined INHAND_ER3
			cellular_ledcon(CELL_CONNECTING);
#elif defined(INHAND_IR8) || defined(INHAND_ER6)
			ledcon_by_event(EV_DAIL_START);
#else
			ledcon(LEDMAN_CMD_ON, LED_WARN);
#endif
			if(get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_DHCP
				|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_MIPC
				|| get_sub_deamon_type(gl_modem_idx)==SUB_DEAMON_QMI){
				gl_check_signal_retry_cnt = 0;
				gl_open_usbserial_retry_cnt = 0;
			}

			if (gl_myinfo.priv.enable == FALSE) {
				if (get_modem_code(gl_modem_idx)==IH_FEATURE_MODEM_ELS31) {
					sleep(3);
					modem_enter_airplane_mode(gl_commport); 
				}
			}

			if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_PVS8
				  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_ME209
				  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LARAR2
				  || get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LE910){
				if(gl_ppp_redial_cnt >5){
					timeout = PVS8_MAX_REDIAL_INTERVAL;
				}else if(get_modem_code(gl_modem_idx) == IH_FEATURE_MODEM_LARAR2){
					timeout = get_rogers_dial_interval(gl_ppp_redial_cnt)/60;
				}else {
					timeout = redial_interval[gl_ppp_redial_cnt];
				}
				LOG_IN("ppp connected redial (%d/%d)...waiting for %d minutes", gl_ppp_redial_cnt, MAX_PPP_REDIAL,timeout);		
				timeout *= 60;
			}else {
				if(gl_myinfo.priv.enable){
					LOG_IN("waiting %d seconds for redial", timeout);
				}
			}
		}else{
			if(get_sub_deamon_type(gl_modem_idx) == SUB_DEAMON_QMI){
				if(pinfo->status == VIF_DOWN || (gl_dual_wwan_enable && secondary_vif_info->status == VIF_DOWN)){
					timeout = 1;
					if(gl_wait_reconnect_cnt++ >= 3){
						stop_child_by_sub_daemon();
					}
				}else if((pinfo->status == VIF_UP && !gl_dual_wwan_enable)
						|| (gl_dual_wwan_enable && pinfo->status == VIF_UP && secondary_vif_info->status == VIF_UP)){
					gl_wait_reconnect_cnt = 0;
				}
			}
		}
		break;
	case REDIAL_STATE_SUSPEND:
		timeout = 10;
		break;
	default:
		break;
	}
	my_restart_timer(timeout);
	redial_handle_leave(&gl_redial_state, &gl_myinfo.svcinfo.modem_info);
}

/*
 * my timer for doing test
 *
 * @param fd		not used
 * @param event		not used
 * @param args		timer callback data
 */
static void my_cb_timer(int fd, short event, void *arg)
{
	redial_handle();
	//my_restart_timer();
}

#ifdef INHAND_WR3
static void redial_set_ant(bool ant)
{
	int value = 1;

	if(ant){ // external ant
		value = 0;
	}else{ 	// internal ant
		value = 1;
	}

	gpio(GPIO_WRITE, GPIO_MANTSEL, &value);
}
#endif

int main(int argc, char* argv[])
{
	const char *pidfilename = "/var/run/"IDENT".pid";
	int c;
	BOOL standalone = FALSE;

#ifdef MEMWATCH
	memwatch_init(gl_my_svc_id);
#endif /* MEMWATCH */

	gl_xargv = argv;
	gl_xargc = argc;

	while ((c = getopt(argc, argv, "hdi:s")) != -1) {
		switch (c) {
		case 'h':
			printf(
				"Usage: %s [options]\n"
				"  -d        	enable debug\n"
				"  -s        	standalone mode\n"
				"  -i <svc-id> 	service id\n"
				, argv[0]);
			return 1;
		case 's':
			standalone = TRUE;
			break;
		case 'd':
			gl_debug = TRUE;
			break;
		case 'i':
			gl_my_svc_id = atoi(optarg);
			break;
		default:
			printf("ignore unknown arg: -%c %s", c, optarg ? optarg : "");
			break;
		}
	}

	write_pidfile(pidfilename, getpid());

	openlog(IDENT, LOG_PID, LOG_DAEMON);
	snprintf(gl_trace_file, sizeof(gl_trace_file), "/var/log/" IDENT "%d.log", gl_my_svc_id);
	logtrace_init(IDENT, gl_trace_file, gl_debug ? DEFAULT_TRACE_CAT : 0);

	ih_srand(time(NULL) + gl_my_svc_id);
	//load running config if possible
	snprintf(gl_running_state_file, sizeof(gl_running_state_file), "/var/run/" IDENT "%d.state", gl_my_svc_id);
	if (my_load_running_state() != ERR_OK) {
		gl_myinfo = gl_default_myinfo;
		gl_myinfo.priv.debug = gl_debug;
	} else {
		//Try to stop smsd service
		gl_smsd_pid = read_pidfile("/var/run/smsd.pid");
		stop_smsd();
		//Try to start smsd service
		if(gl_myinfo.priv.sms_config.enable)
			start_smsd();
		//Try to stop ppp service
		gl_ppp_pid = read_pidfile(PPP_PID_FILE);
		gl_2nd_ppp_pid = read_pidfile(PPP_PID_FILE2);
		gl_qmi_proxy_pid = read_pidfile(QMI_PROXY_PID_FILE);

		//restore sim state
		if(gl_myinfo.priv.modem_config.dualsim) {
			//ih_license_init(g_sysinfo.model_name, g_sysinfo.product_number);	
			ih_license_init(g_sysinfo.family_name, g_sysinfo.model_name, g_sysinfo.product_number);	
			gl_modem_idx = find_modem_type_idx(ih_license_get_modem());
			if(gl_modem_idx>=0) {
				modem_dualsim_switch(get_modem_code(gl_modem_idx), SIM1);
			}
		}
		
		stop_all_child_by_pidfile();

		gl_config_ready = TRUE;
	}

	trace_category_set(gl_myinfo.priv.debug ? DEFAULT_TRACE_CAT : 0);
	gl_myinfo.priv.ppp_debug = TRUE;

	ih_ipc_open(gl_my_svc_id);

	signal_setup();
	
	ih_ipc_subscribe(IPC_MSG_CONFIG_RESET);
	ih_ipc_subscribe(IPC_MSG_CONFIG_FINISH);
	ih_ipc_subscribe(IPC_MSG_SW_SYSINFO);
	ih_ipc_subscribe(IPC_MSG_BACKUPD_SVCINFO);
	ih_ipc_subscribe(IPC_MSG_SMS_SEND);
	ih_ipc_subscribe(IPC_MSG_PYSMS_SEND);
	ih_ipc_subscribe(IPC_MSG_SDWAN_STATUS_INFO);
	ih_ipc_subscribe(IPC_MSG_DATA_USAGE);
	ih_ipc_subscribe(IPC_MSG_IP_PASSTHROUGH_CONFIG);
	ih_ipc_subscribe(IPC_MSG_UPDOWN_ODHCP6C_EVENT);
	ih_ipc_subscribe(IPC_MSG_AGENT_PREFIX_USER_NOTIFY);

	vif_link_change_register(NULL);
	ih_ipc_register_msg_handle(IPC_MSG_SW_SYSINFO, my_sysinfo_handle);
	ih_ipc_register_msg_handle(IPC_MSG_CONFIG_RESET, config_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_CONFIG_FINISH, config_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_CMD, my_cmd_handle);	
	ih_ipc_register_msg_handle(IPC_MSG_UPDOWN_PPP_UP, my_msg_handle);	
	ih_ipc_register_msg_handle(IPC_MSG_UPDOWN_PPP_DOWN, my_msg_handle);	
	ih_ipc_register_msg_handle(IPC_MSG_VIF_LINK_CHANGE_REQ, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_NETWATCHER_LINKDOWN, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_REDIAL_SVCINFO_REQ, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_REDIAL_TIME_SYNC_REQ, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_REDIAL_SUSPEND_REQ, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_BACKUPD_SVCINFO, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_SMS_SEND, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_PYSMS_SEND, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_UPDOWN_DHCPC_EVENT, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_SDWAN_STATUS_INFO, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_DATA_USAGE, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_IP_PASSTHROUGH_CONFIG,ippt_msg_handle);	
	ih_ipc_register_msg_handle(IPC_MSG_UPDOWN_ODHCP6C_EVENT, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_AGENT_PREFIX_USER_NOTIFY, my_msg_handle);
	ih_ipc_register_msg_handle(IPC_MSG_MIPC_WWAN_EVENT, my_msg_handle);	

	//ih_ipc_register_msg_handle(IPC_MSG_NOTIFY, notify_msg_handle);

	evtimer_set(&gl_ev_tmr,
			my_cb_timer,
			&gl_ev_tmr);
	event_base_set(g_my_event_base, &gl_ev_tmr);
	my_restart_timer(1);
	//set sms, signal timer, but no add these
	evtimer_set(&gl_ev_sms_tmr,
			my_sms_timer,
			&gl_ev_sms_tmr);
	event_base_set(g_my_event_base, &gl_ev_sms_tmr);

	evtimer_set(&gl_ev_signal_tmr,
			my_signal_timer,
			&gl_ev_signal_tmr);
	event_base_set(g_my_event_base, &gl_ev_signal_tmr);

	evtimer_set(&gl_ev_ppp_tmr,
			my_ppp_timer,
			&gl_ev_ppp_tmr);
	event_base_set(g_my_event_base, &gl_ev_ppp_tmr);
	
	evtimer_set(&gl_ev_ppp_up_tmr,
			my_ppp_up_timer,
			&gl_ev_ppp_up_tmr);
	event_base_set(g_my_event_base, &gl_ev_ppp_up_tmr);
	
	evtimer_set(&gl_ev_dualsim_tmr,
			my_dualsim_timer,
			&gl_ev_dualsim_tmr);
	event_base_set(g_my_event_base, &gl_ev_dualsim_tmr);

	evtimer_set(&gl_ev_wwan_connect_tmr,
			my_wwan_connect_timer,
			&gl_ev_wwan_connect_tmr);
	event_base_set(g_my_event_base, &gl_ev_wwan_connect_tmr);

	evtimer_set(&gl_ev_redial_pending_task_tmr,
			my_redial_pending_task_timer,
			&gl_ev_redial_pending_task_tmr);
	event_base_set(g_my_event_base, &gl_ev_redial_pending_task_tmr);

	evtimer_set(&g_ev_factory_modem_tmr,
			my_factory_modem_timer,
			&g_ev_factory_modem_tmr);
	event_base_set(g_my_event_base, &g_ev_factory_modem_tmr);

#if defined NEW_WEBUI
	mosq_client_init();
#endif

	event_base_dispatch(g_my_event_base);

	/* save running state */
	my_dump_running_state();

#if defined NEW_WEBUI
	in_mosq_clean();
	mosq_list_clean();
#endif

	stop_smsd();

	ih_ipc_close();
	unlink(pidfilename);

#ifdef NETCONF
	ih_sysrepo_cleanup();
#endif
	
	stop_child_by_sub_daemon();

	if (gl_restart) {
		execv(gl_xargv[0], gl_xargv);
		LOG_ER("Restart FAILED");
	}
	
	MYTRACE_LEAVE();
	logtrace_cleanup();
	closelog();
	
	return 0;
}
