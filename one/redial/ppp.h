#ifndef _PPP_H_
#define _PPP_H_

#include "modem.h"

#define PPP_PID_FILE	"/var/run/wan1.pid"
#define PPP_PID_FILE2	"/var/run/wan1_1.pid"
#define QMI_PROXY_PID_FILE "/var/run/qmi-proxy.pid"

#if 0
#define PPP_MAX_PROFILE		10

#define PROFILE_TYPE_NONE	0
#define PROFILE_TYPE_GSM	1
#define PROFILE_TYPE_CDMA	2
#define PPP_MAX_USR_LEN		129
#define PPP_MAX_PWD_LEN		129
#define PPP_MAX_APN_LEN		32
#define PPP_MAX_DIALNO_LEN	16

enum {
	AUTH_TYPE_AUTO = 0,
	AUTH_TYPE_PAP,
	AUTH_TYPE_CHAP,
	AUTH_TYPE_MSCHAP,
	AUTH_TYPE_MSCHAPV2,
};

enum{
	PPP_ONLINE = 0,
	PPP_DEMAND = 1,
	PPP_MANUAL = 2
};

typedef struct {
	int type;
	char apn[PPP_MAX_APN_LEN];
	char dialno[PPP_MAX_DIALNO_LEN];
	int auth_type;
	char username[PPP_MAX_USR_LEN];
	char password[PPP_MAX_PWD_LEN];
}PPP_PROFILE;

typedef enum {
	PPP_COMP_NONE = 0,
	PPP_COMP_BSD,
	PPP_COMP_DEFLATE,
	PPP_COMP_MPPC,
	PPP_COMP_LZS,
	PPP_COMP_PREDICTOR,
}PPP_COMP;

typedef struct{
	int profile_idx;
	int profile_2nd_idx;
	int profile_wwan2_idx;
	int profile_wwan2_2nd_idx;
	PPP_PROFILE profiles[PPP_MAX_PROFILE];
	PPP_PROFILE default_profile;
	PPP_PROFILE auto_profile;
	int mtu;
	int mru;
	int timeout;
	int peerdns;
	int ppp_mode;
	int trig_data;
	int trig_sms;
	int idle;
	int ppp_static;
	char ppp_ip[16];
	char ppp_peer[16];
	int lcp_echo_interval;
	int lcp_echo_retries;
	int default_am; 
	short nopcomp;
	short noaccomp;
	short vj;
	short novjccomp;
	PPP_COMP ppp_comp;
	char init_string[64];
	char options[128];
	BOOL prefix_share;
	BOOL ndp_enable;
	BOOL static_if_id_enable;
	char static_interface_id[MAX_STATIC_IF_ID_LEN];
	char pd_label[PD_LABEL_LEN];
	char shared_label[PD_LABEL_LEN];
}PPP_CONFIG;
#endif

int ppp_build_config(char *dev, PPP_CONFIG *info, MODEM_CONFIG *config, MODEM_INFO *modem_info, int modem_code, BOOL debug);

int at_ppp_build_config(char *dev, PPP_CONFIG *info, MODEM_CONFIG *config, MODEM_INFO *modem_info, BOOL debug);
#endif//_PPP_H_
