#ifndef _REDIAL_H_
#define _REDIAL_H_

typedef enum {
	REDIAL_STATE_INIT = 0,
	REDIAL_STATE_WAIT_CONFIG,
	REDIAL_STATE_MODEM_STARTUP,
	REDIAL_STATE_MODEM_LOAD_DRV,
	REDIAL_STATE_MODEM_WAIT_AT,
	REDIAL_STATE_MODEM_INIT,
	REDIAL_STATE_MODEM_SLEEP,
	REDIAL_STATE_MODEM_READY,
	REDIAL_STATE_SUSPEND,
	REDIAL_STATE_SMS_ONLY,
	REDIAL_STATE_AT,
	REDIAL_STATE_PPP,
	REDIAL_STATE_DHCP,
	REDIAL_STATE_GET_STATIC_PARAMS,
	REDIAL_STATE_CONNECTING,
	REDIAL_STATE_CONNECTED,
}REDIAL_STATE;

typedef enum {
	REDIAL_DIAL_STATE_NONE = 0,
	REDIAL_DIAL_STATE_STARTED ,
	REDIAL_DIAL_STATE_KILLED ,
}REDIAL_DIAL_STATE;

typedef enum {
	SUB_DEAMON_PPP =0,
	SUB_DEAMON_DHCP,
	SUB_DEAMON_QMI,
	SUB_DEAMON_MIPC,
	SUB_DEAMON_NONE,
} REDIAL_SUB_DEAMON;

typedef enum {
	REDIAL_PENDING_TASK_NONE 		= 0,
	REDIAL_PENDING_TASK_UPDATE_POL 		= (1 << 0),
	REDIAL_PENDING_TASK_UPDATE_TIME 	= (1 << 1),
} REDIAL_PENDING_TASK;

typedef struct {
	int integer[2];
	const char *str;
}INTEGER_MAP_STRING;

#define START_REDIAL_PENDING_TASK		1

#define REDIAL_DEFAULT_IDLE		60
#define REDIAL_DEFAULT_TIMEOUT		120
#define REDIAL_DEFAULT_DIAL_INTERVAL	10
#define REDIAL_DEFAULT_CSQ_INTERVAL	15
#define REDIAL_CHECK_CONN_STATE_INTERVAL	20

#define REDIAL_CONFIG_NONE	0
#define REDIAL_CONFIG_GENERAL	1
#define REDIAL_CONFIG_PPP	2
#define REDIAL_CONFIG_DHCP	3
#define REDIAL_CONFIG_MODEM	4

#define REDIAL_PPP_UP_TIME	600
#define REDIAL_PPP_TIMEOUT	600
#define REDIAL_DHCP_TIMEOUT	120

#define DEFAULT_CID	1
#define VZW_CLASS3_CID	3
#define VZW_CLASS6_CID	6

#if 0
#define REDIAL_SUBIF_MAX	2

typedef struct {
	IF_INFO if_info;
	//profile
}CELLULAR_SUB_IF;
#endif

#define SEQ_SIM_FILE	"/var/backups/sim"
#define USB0_WWAN	"usb0"
#define ETH2_WWAN	"eth2"
#define WWAN0_WWAN	"wwan0"
#define PCIE0_WWAN	"pcie0"
#define WWAN0_BASE	WWAN0_WWAN
#define RMNET_MHI0_BASE	"rmnet_mhi0"
#define APN1_WWAN	"apn1"
#define CELL_SWPORT	"lan0"

#define WWAN_PORT_CELLULAR1     1
#define WWAN_PORT_CELLULAR2     2
#define CELLULAR_DATA_CONNECT   1
#define CELLULAR_DATA_DISCONNECT   0

#define WWAN_ID1 1
#define WWAN_ID2 2

#define IDENT		"redial"

typedef struct {
	int default_cid;
}MY_GLOBAL;

extern MY_GLOBAL gl;

#endif//_REDIAL_H_