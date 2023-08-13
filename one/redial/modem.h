/*
 *define header file about modem
 */

#ifndef _MODEM_H_
#define _MODEM_H_

#include "redial_ipc.h"
#include "build_info.h"

#if (defined INHAND_IG9 || defined INHAND_VG9 || defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_WR3)
#define PROC_USB_DEVICES "/sys/kernel/debug/usb/devices"
#else
#define PROC_USB_DEVICES "/proc/bus/usb/devices"
#endif

#define LIMIT(cur, min, max) (((cur) < (min)) ? (min) : (((cur) > (max)) ? (max) : (cur)))

#define RANGE_CHK(cur, min, max) ((LIMIT(cur, min, max) == cur) ? (TRUE) : (FALSE))

#define VALID_SRVCELL_INFO(val) ((val[0]) && !((strlen(val) == 1) && ((val[0]) == '-')))

#define STR_SKIP(pos, ptr, str, count, error) \
	for(int _i = 0; ptr && _i < count; _i++){ \
		pos = strsep(&ptr, str); \
		if(!pos || !ptr) goto error; \
		while(*ptr!=0 && *ptr==' ') ptr++; \
	}

#define STR_FORMAT(buf, format, args...) \
	snprintf(buf, sizeof(buf), format, ##args);

#define STRCAT_FORMAT(buf, format, args...) \
	char _append[128] = {0}; \
	snprintf(_append, sizeof(_append), format, ##args); \
	if((strlen(buf) + strlen(_append)) < (sizeof(buf) - 1)){ \
		strcat(buf, _append); \
	}

//see 3GPP TS 45.008
#define RSSI2RXLEV_SCALE 3

#define BIT_RAT 	(1 << 0)
#define BIT_MCC 	(1 << 1)
#define BIT_MNC 	(1 << 2)
#define BIT_LAC 	(1 << 3)
#define BIT_CELLID 	(1 << 4)
#define BIT_RSSI 	(1 << 5)
#define BIT_BER 	(1 << 6)
#define BIT_RSCP 	(1 << 7)
#define BIT_ECIO 	(1 << 8)
#define BIT_RSRP 	(1 << 9)
#define BIT_RSRQ 	(1 << 10)
#define BIT_SINR 	(1 << 11)
#define BIT_PCI 	(1 << 12)
#define BIT_BAND 	(1 << 13)
#define BIT_SS_RSRP 	(1 << 14)
#define BIT_SS_RSRQ 	(1 << 15)
#define BIT_SS_SINR 	(1 << 16)
#define BIT_ARFCN 	(1 << 17)
#define MASK_ALL	(0xEFFFFFFF)

#define BITS_UNSET(mask, bits) ((mask) & (~(bits)))

#define SIGNAL_BAR_NONE 		0
#define SIGNAL_BAR_POOR 		1
#define SIGNAL_BAR_MODERATE 		2
#define SIGNAL_BAR_GOOD 		3
#define SIGNAL_BAR_GREAT 		4

/*Use to suit the different at command set.*/
enum{
	AT_CMD_AT	=	0,
	AT_CMD_ATH	=	1,
	AT_CMD_ATE0	=	2,
	AT_CMD_INIT	= 	3,
	AT_CMD_GSN	= 	4,
	AT_CMD_CPIN	=	5,
	AT_CMD_CIMI	=	6,
	AT_CMD_CSQ	=	7,
	AT_CMD_CREG	=	8,
	AT_CMD_CCLK	=	9,
	AT_CMD_CHUP	= 	10,
	AT_CMD_COPS	= 	11,
	AT_CMD_COPS2	= 	12,
	AT_CMD_PREFMODE	= 	13,
	AT_CMD_AUTO	= 	14,
	AT_CMD_2G	= 	15,
	AT_CMD_3G	= 	16,
	AT_CMD_SPIC	= 	17,
	AT_CMD_CMGF	= 	18,
	AT_CMD_CMGR	= 	19,
	AT_CMD_CMGD	= 	20,
	AT_CMD_CMGS	= 	21,
	AT_CMD_3GCSQ	= 	22,
	AT_CMD_SCPIN	= 	23,
	AT_CMD_SYSCFG	= 	24,
	AT_CMD_CMGL	= 	25,
	AT_CMD_CELLID	= 	26,
	AT_CMD_CURC	= 	27,
	AT_CMD_CGREG	=	28,
	AT_CMD_CGREG2	=	29,
	AT_CMD_SYSINFO	=	30,
	AT_CMD_SYSINFOEX	=	31,
	AT_CMD_CNUM	=	32,
	AT_CMD_SMONI	=	33,
	AT_CMD_CURC2	= 	34,
	AT_CMD_SLED	= 	35,
	AT_CMD_VCSQ	= 	36, //pvs8 csq
	AT_CMD_4G	= 	37,
	AT_CMD_CEREG	= 	38,
	AT_CMD_CEREG2	= 	39,
	AT_CMD_VNUM	= 	40,
	AT_CMD_CREG2	= 	41,
	AT_CMD_PSRAT	= 	42,
	AT_CMD_VSYSINFO = 	43,
	AT_CMD_CURC1	= 	44,
	AT_CMD_SWWAN	= 	45,
	AT_CMD_CEUS	= 	46,
	AT_CMD_CGATT	= 	47,
	AT_CMD_CGATT1	= 	48,
	AT_CMD_3G2G	= 	49,
	AT_CMD_COPS0	= 	50,
	AT_CMD_NAIPWD	= 	51,
	AT_CMD_WSYSINFO	= 	52,
	AT_CMD_MCC	= 	53,
	AT_CMD_MNC	= 	54,
	AT_CMD_WCPIN	= 	55,
	AT_CMD_STATE	= 	56,
	AT_CMD_HSTATE	= 	57,
	AT_CMD_CAD	= 	58,
	AT_CMD_WCSQ	= 	59,
	AT_CMD_HLED	= 	60,

	AT_CMD_QCPIN,
	AT_CMD_QPINC,
	AT_CMD_CCSQ,
	AT_CMD_CSQ1,
	AT_CMD_ICCID,
	AT_CMD_CGDCONT_CMNET,
	AT_CMD_CGDCONT_3GNET,
	AT_CMD_CGDCONT_CTLTE,
	AT_CMD_QCIMI,
	AT_CMD_PSRAT_C,
	AT_CMD_EVDO,
	AT_CMD_EHRPD0,
	AT_CMD_COPS3,
	AT_CMD_AINIT,
	AT_CMD_PDNACT,
	AT_CMD_CGPADDR,
	AT_CMD_CGACT1,
	AT_CMD_CGACT2,
	AT_CMD_MEAS8,
	AT_CMD_NDISSTATQRY,
	AT_CMD_NDISDUP0,
	AT_CMD_CGSN,
	AT_CMD_SQNBYPASS,
	AT_CMD_LICCID,
	AT_CMD_HICCID,
	AT_CMD_SICCID,
	AT_CMD_CCID,
	AT_CMD_CFUN1,
	AT_CMD_CFUN0,
	AT_CMD_CFUN11,
	AT_CMD_SXRAT,
	AT_CMD_CGDCONT,
	AT_CMD_SICA,
	AT_CMD_SICA13,
	AT_CMD_CGPADDR3,
	AT_CMD_CGPADDR2,
	AT_CMD_UMNOCONF_3_15,
	AT_CMD_UMNOCONF0_3_15,
	AT_CMD_UMNOCONF_1_20,
	AT_CMD_UMNOCONF0_1_20,
	AT_CMD_UMNOCONF0_1_7,
	AT_CMD_UBMCONF2,
	AT_CMD_CESQ,
	AT_CMD_MONSC,
	AT_CMD_SSRVSET1,
	AT_CMD_QCFG1,
	AT_CMD_CLCK,
	AT_CMD_ATI,
	AT_CMD_UCGED,
	AT_CMD_UCGED2,
	AT_CMD_CTZU,
	AT_CMD_5G,
	AT_CMD_5G4G,
	AT_CMD_C5GREG,
	AT_CMD_C5GREG2,
	AT_CMD_MODE_PREF,
	AT_CMD_MODE_PREF2,
	AT_CMD_QENG_SRVCELL,
	AT_CMD_QENG_SRVCELL2,
	AT_CMD_CREG_AUTO,
	AT_CMD_CGMI,
	AT_CMD_CGMM,
	AT_CMD_CGMR,
	AT_CMD_CGMR2,
	AT_CMD_GMR,
	AT_CMD_QGMR,
	AT_CMD_HWVER,
	AT_CMD_QNR5G_MODE,
	AT_CMD_GTCCINFO,
	AT_CMD_GTRNDIS,
	AT_CMD_CAVIMS,
	AT_CMD_CAVIMS0,
	AT_CMD_QCPRFMOD,
	AT_CMD_MPDN_STATUS,
	AT_CMD_QLTS,
	AT_CMD_QCFG_IMS,
	AT_CMD_QCFG_IMS2,
	AT_CMD_QCFG
};

/*define error code*/
typedef enum{
	ERR_AT_OK=0,
	ERR_AT_NAK,
	ERR_AT_NOMODEM,
	ERR_AT_NEED_RESET,
	ERR_AT_NOSIM,
	ERR_AT_NOSIGNAL,
	ERR_AT_LOWSIGNAL,
	ERR_AT_NOREG,
	ERR_AT_ROAM,
	ERR_AT_NOVALIDIP,
	ERR_AT_BACKTOLAST,
}AT_ERR_CODE;

#if 0
typedef struct {
	int network_type;
	int dualsim;
	int policy_main;
	int main_sim;
	int backup_sim;
	int csq[2];//baodn change it for 2 SIM cards
	int csq_interval[2];
	int csq_retries[2];
	char pincode[2][10];
	int roam_enable[2];
	int activate_enable;
	char activate_no[16];
	char phone_no[20];
	char nai_no[64];
	char nai_pwd[64];
}MODEM_CONFIG;

typedef struct {
	int retries;
	int backtime;
	int uptime;
}DUALSIM_INFO;
#endif

typedef struct AT_CMD{
	int	index;
	char	*name;
	char	atc[64];
	char	resp[16];
	int	timeout;
	int	max_retry;
	int (*at_cmd_handle)(struct AT_CMD* pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
	char	*desc;
	AT_ERR_CODE err_code;
}AT_CMD;

#define MAX_DUALSIM_RULE_NUM	3

#define AT_DEFAULT_TIMEOUT	3
#define AT_DEFAULT_TIMES	3

#define AT_INIT_CMD 		"AT\r\n"

#define ICCID_OPSSTR_SIZ	6
#define MAX_NUMBER_PROFILES 16

#define USE_BLANK_APN			"use-blank-apn" /* wucl: this is to represent blank apn for U8300W/U8300C */

/**Radio Access Aechnology**/
#define CDMA_BAND_800 800
#define GSM_BAND_850 850
#define GSM_BAND_900 900
#define GSM_BAND_1800 1800
#define GSM_BAND_1900 1900
#define WCDMA_BAND_2100 2100
#define WCDMA_BAND_900 900
#define WCDMA_BAND_850 850
#define WCDMA_BAND_1900 1900
#define UMTS_BAND_1 1
#define UMTS_BAND_8 8
#define TD_SCDMA_BAND_34 34
#define TD_SCDMA_BAND_39 39
#define FDD_LTE_BAND_1 1
#define FDD_LTE_BAND_2 2
#define FDD_LTE_BAND_3 3
#define FDD_LTE_BAND_4 4
#define FDD_LTE_BAND_5 5
#define FDD_LTE_BAND_6 6
#define FDD_LTE_BAND_7 7
#define FDD_LTE_BAND_8 8
#define FDD_LTE_BAND_20 20
#define TDD_LTE_BAND_38 38
#define TDD_LTE_BAND_39 39
#define TDD_LTE_BAND_40 40
#define TDD_LTE_BAND_41 41
#define UNKNOWN_BAND 0xFFFF

AT_CMD* find_at_cmd(int index);
int setup_custom_at_cmd(uint64_t modem_code);
int modem_init_serial(int fd, int baud);
int modem_read(int fd, char* buf, int len);
int modem_write(int fd, char* buf, int len, int timeout);

int my_insmod(const char *mod_name, ...);
int usb_get_device_info(int *iface_num, char *vendor_id, char *product_id, char *manufacturer, char *product, int verbose);
int device_test(const char *dev, int mode, int sec);
int find_pci_device(int target_vid);
int check_socket_device(char *dev);
int open_dev(char *dev);
int read_fd(int fd, char* buf, size_t size, int timeout, int verbose);
int write_fd(int fd, char* buf, size_t size, int timeout, int verbose);
int check_return2(int fd, char* buf, int len, char* want, int timeout, int verbose);
int flush_fd(int fd);
int send_at_cmd_sync(char *dev, char* cmd, char* rsp, int len, int verbose);
int send_at_cmd_sync3(const char *dev, char* cmd, char* rsp, int len, int verbose);
int send_at_cmd_sync_timeout(char *dev, char* cmd, char* rsp, int len, int verbose, char *check_value, int timeout);
void modem_dualsim_switch(uint64_t modem_code, int idx);

enum {
	MODEM1 = 0x01,	
	MODEM_MAX,
};

enum {
	MODEM_START = 0,
	MODEM_STOP,
	MODEM_RESET,
	MODEM_FORCE_STOP,
	MODEM_FORCE_RESET,
	MODEM_RE_REGISTER,
	MODEM_CFUN_RESET
};

typedef enum {
	CUR_NETWORK_4G=3,
	CUR_NETWORK_3G=2, 
	CUR_NETWORK_2G=1,
	CUR_NETWORK_NONE=0,
}CUR_NETWORK_TYPE;

enum {
	MODEM_DEV_NONE = 0,
	MODEM_DEV_USB_ALIVE = (1 << 0),
	MODEM_DEV_PCIE_ALIVE = (1 << 1),
	MODEM_DEV_SOCK_ALIVE = (1 << 2),
};

int set_modem(uint64_t modem_code, int action);
extern AT_CMD *g_modem_custom_cmds; 

#endif//_MODEM_H_
