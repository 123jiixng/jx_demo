/*
 * $Id:
 *
 *   Modem handle
 *
 * Copyright (c) 2001-2012 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 01/13/2012
 * Author: Zhangly
 * Description:
 * 		handle at command
 *
 */

#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <arpa/inet.h>

#include "ih_ipc.h"
#include "ih_updown_ipc.h"
#include "shared.h"
#include "vif_shared.h"

#include "modem.h"
#include "redial.h"
#include "operator.h"

extern int gl_at_fd;
extern char *gl_commport;
extern uint64_t gl_modem_code;
extern BOOL gl_modem_cfun_reset_flag;
extern int *gl_at_lists;
extern MY_REDIAL_INFO gl_myinfo;

static void update_cellinfo(MODEM_INFO *info);
void update_sig_leds(int sig);
void update_sig_leds2(int sig);
int update_srvcell_info(SRVCELL_INFO *cell, int mask);
/*************************AT***************************************/
int check_at(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_ate0(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_cgmi(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_cgmm(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_cgmr(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_cgmr2(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_hwver(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_imei(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_imei_or_gsn(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_sim(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_imsi(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_phonenum(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_vphonenum(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_signal(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_reg(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_network(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_sysinfo(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_cellid(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_smoni(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_monsc(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_network_sel(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_pls8_connection_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_wpd200_rssi_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_wpd200_net_code(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_ps_attachment_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_network_sel_u8300c(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_sim_operator(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_fb78_signal_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_lp41_connection_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_lp41_ipaddr_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_lp41_signal_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_ecm_connection_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_check_sim_iccid(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_els_sica_connection_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info); 
int check_modem_apn_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info); 
void fixup_u8300c_at_list1(void);
int check_modem_cgpaddr_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info); 
int check_modem_cgpaddr(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_ublox_umnoconf(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info); 
int check_modem_ssrvset_actsrvset_10(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info); 
int get_band_by_arfcn(int network, int arfcn);
int check_modem_ctzu_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_cclk_update_time(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_qeng_servingcell(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_qeng_servingcell2(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_nwscanmode(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_mode_pref(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_mode_pref2(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_nr5g_disable_mode(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_gtccinfo(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_gtrndis(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_qcfg_ims(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_qcfg_ims2(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_cavims(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);
int check_modem_mpdn_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info);

extern int auto_select_net_apn(char *imsi_code, BOOL debug);
extern int redial_modem_function_reset(BOOL debug); 
extern int redial_modem_re_register(BOOL debug); 
extern VIF_INFO *my_get_reidal_vif_info(void);
extern BOOL ppp_debug(void);
extern BOOL allow_reboot(void);
extern int get_pdp_type_setting(void);

AT_CMD *g_modem_custom_cmds = NULL; 

AT_CMD gl_at_cmds[] = {
	{AT_CMD_AT,	"AT_CMD_AT",	"AT\r\n",	"",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES,	check_at, NULL, ERR_AT_NOMODEM},
	{AT_CMD_ATH,	"AT_CMD_ATH",	"ATH\r\n",	"",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES,	check_at, NULL, ERR_AT_OK},
	{AT_CMD_ATE0,	"AT_CMD_ATE0",	"ATE0\r\n",	"",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES,	check_ate0, NULL, ERR_AT_NAK},
	{AT_CMD_CGMI,	"AT_CMD_CGMI",	"AT+CGMI\r\n",	"CGMI", AT_DEFAULT_TIMEOUT,	5,	check_modem_cgmi, "manufacturer", ERR_AT_NAK},
	{AT_CMD_CGMM,	"AT_CMD_CGMM",	"AT+CGMM\r\n",	"CGMM",	AT_DEFAULT_TIMEOUT,	5,	check_modem_cgmm, "product model", ERR_AT_NAK},
	{AT_CMD_CGMR,	"AT_CMD_CGMR",	"AT+CGMR\r\n",	"CGMR",	AT_DEFAULT_TIMEOUT,	5,	check_modem_cgmr, "software version", ERR_AT_NAK},
	{AT_CMD_CGMR2,	"AT_CMD_CGMR2",	"AT+CGMR\r\n",	"HW Version:",	AT_DEFAULT_TIMEOUT,5,	check_modem_cgmr2,"hardware version", ERR_AT_NAK},
	{AT_CMD_GMR,	"AT_CMD_GMR",	"AT+GMR\r\n",	"GMR",	AT_DEFAULT_TIMEOUT,	5,	check_modem_cgmr, "software version", ERR_AT_NAK},
	{AT_CMD_QGMR,	"AT_CMD_QGMR",	"AT+QGMR\r\n",	"QGMR",	AT_DEFAULT_TIMEOUT,	5,	check_modem_cgmr, "software version", ERR_AT_NAK},
	{AT_CMD_HWVER,	"AT_CMD_HWVER",	"AT^HWVER\r\n",	"HWVER",AT_DEFAULT_TIMEOUT,	5,	check_modem_hwver,"hardware version", ERR_AT_NAK},
	{AT_CMD_GSN,	"AT_CMD_GSN",	"AT+GSN\r\n",	"GSN:",	AT_DEFAULT_TIMEOUT,	5,	check_modem_imei, "imei", ERR_AT_NAK},
	{AT_CMD_CGSN,	"AT_CMD_CGSN",	"AT+CGSN\r\n",	"",	AT_DEFAULT_TIMEOUT,	5,	check_modem_imei, "imei", ERR_AT_NAK},
	{AT_CMD_CPIN,	"AT_CMD_CPIN",	"AT+CPIN?\r\n",	"",	10,	5,	check_modem_sim, "sim card", ERR_AT_NOSIM},
	{AT_CMD_QCPIN,	"AT_CMD_QCPIN",	"AT+QCPIN?\r\n","",	10,	5,	check_modem_sim, "sim card", ERR_AT_NOSIM},
	{AT_CMD_QPINC,	"AT_CMD_QPINC",	"AT+QPINC?\r\n","",	10,	5,	check_modem_sim, "sim card", ERR_AT_NOSIM},
	{AT_CMD_SCPIN,	"AT_CMD_SCPIN",	"AT^CPIN?\r\n",	"",	10,	5,	check_modem_sim, "sim card", ERR_AT_NOSIM},
	{AT_CMD_WCPIN,	"AT_CMD_WCPIN",	"AT*RUIMS?\r\n",	"",	10,	5,	NULL/*check_modem_sim*/, "sim card", ERR_AT_NOSIM},
	{AT_CMD_CIMI,	"AT_CMD_CIMI",	"AT+CIMI\r\n",	"",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES,	check_modem_imsi, "imsi", ERR_AT_NOSIM},
	{AT_CMD_CGATT,	"AT_CMD_CGATT",	"AT+CGATT?\r\n",	"CGATT:",	AT_DEFAULT_TIMEOUT, 30,	check_ps_attachment_status, NULL, ERR_AT_NAK},
	{AT_CMD_CGACT1,	"AT_CMD_CGACT1","AT+CGACT=1,1\r\n",	"",	10, 1,	NULL, NULL, ERR_AT_NAK},
	{AT_CMD_CGACT2,	"AT_CMD_CGACT2","AT+CGACT=1,2\r\n",	"",	10, 1,	NULL, NULL, ERR_AT_NAK},
	{AT_CMD_CNUM,	"AT_CMD_CNUM",	"AT+CNUM\r\n",	"CNUM:", AT_DEFAULT_TIMEOUT,	1,	check_modem_phonenum, "phone number", ERR_AT_NAK},
	{AT_CMD_CSQ,	"AT_CMD_CSQ",	"AT+CSQ\r\n",	"CSQ:",	AT_DEFAULT_TIMEOUT,	30,	check_modem_signal, "signal", ERR_AT_NOSIGNAL},
	{AT_CMD_CSQ1,	"AT_CMD_CSQ1",	"AT+CSQ\r\n",	"CSQ:",	AT_DEFAULT_TIMEOUT,	60,	check_modem_signal, "signal", ERR_AT_NOSIGNAL},
	{AT_CMD_CCSQ,	"AT_CMD_CCSQ",	"AT+CCSQ\r\n",	"CCSQ:",	AT_DEFAULT_TIMEOUT,	30,	check_modem_signal, "signal", ERR_AT_NOSIGNAL},
	{AT_CMD_CREG,	"AT_CMD_CREG",	"AT+CREG?\r\n",	"REG:",	5,	5,	check_modem_reg, "register status", ERR_AT_NOREG},
	{AT_CMD_CREG2,	"AT_CMD_CREG2",	"AT+CREG=2\r\n","",	AT_DEFAULT_TIMEOUT,	1, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_CGREG,	"AT_CMD_CGREG",	"AT+CGREG?\r\n",	"REG:",	5,	5,	check_modem_reg, "register status", ERR_AT_NOREG},
	{AT_CMD_CGREG2,	"AT_CMD_CGREG2","AT+CGREG=2\r\n","",	AT_DEFAULT_TIMEOUT,	1, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_CEREG,	"AT_CMD_CEREG",	"AT+CEREG?\r\n",	"REG:",	5,	5,	check_modem_reg, "register status", ERR_AT_NOREG},
	{AT_CMD_CEREG2,	"AT_CMD_CEREG2","AT+CEREG=2\r\n","",	AT_DEFAULT_TIMEOUT,	1, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_C5GREG,	"AT_CMD_C5GREG","AT+C5GREG?\r\n",	"+C5GREG:",	5,	5,	check_modem_reg, "register status", ERR_AT_NOREG},
	{AT_CMD_C5GREG2,"AT_CMD_C5GREG2","AT+C5GREG=2\r\n","",	AT_DEFAULT_TIMEOUT,	1, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_CURC,	"AT_CMD_CURC",	"AT^CURC=0\r\n",	"",	AT_DEFAULT_TIMEOUT,	1,	NULL, NULL, ERR_AT_NAK},
	{AT_CMD_CURC1,	"AT_CMD_CURC1",	"AT^CURC=1\r\n",	"",	AT_DEFAULT_TIMEOUT,	1,	NULL, NULL, ERR_AT_NAK},
	{AT_CMD_COPS,	"AT_CMD_COPS",	"AT+COPS?\r\n",	"+COPS:",	AT_DEFAULT_TIMEOUT,	3,	check_modem_network, NULL, ERR_AT_NAK},
	{AT_CMD_COPS0,	"AT_CMD_COPS0",	"AT+COPS=0\r\n",	"",	AT_DEFAULT_TIMEOUT,	3,	NULL, NULL, ERR_AT_NAK},
	{AT_CMD_COPS2,	"AT_CMD_COPS2","AT+COPS=3,2\r\n","",	AT_DEFAULT_TIMEOUT,	1, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_COPS3,	"AT_CMD_COPS3","AT+COPS=0,2\r\n","",	AT_DEFAULT_TIMEOUT,	1, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_SYSINFO,"AT_CMD_SYSINFO",	"AT^SYSINFO\r\n",	"SYSINFO:",	AT_DEFAULT_TIMEOUT,	1,	check_modem_sysinfo, NULL, ERR_AT_NAK},
	{AT_CMD_VSYSINFO,"AT_CMD_VSYSINFO",	"AT^SYSINFO\r\n",	"SYSINFO:",	AT_DEFAULT_TIMEOUT,	1,	check_modem_sysinfo, NULL, ERR_AT_NAK},
	{AT_CMD_WSYSINFO,"AT_CMD_VSYSINFO",	"AT^SYSINFO\r\n",	"SYSINFO: ",	AT_DEFAULT_TIMEOUT,	10,	check_modem_sysinfo, NULL, ERR_AT_NAK},
	{AT_CMD_SYSINFOEX,"AT_CMD_SYSINFOEX",	"AT^SYSINFOEX\r\n",	"SYSINFOEX:",	AT_DEFAULT_TIMEOUT,	10,	check_modem_sysinfo, NULL, ERR_AT_NAK},
	{AT_CMD_SMONI,"AT_CMD_SMONI",	"AT^SMONI\r\n",	"SMONI:",	AT_DEFAULT_TIMEOUT,	5,	check_modem_smoni, NULL, ERR_AT_NAK},
	{AT_CMD_MONSC,"AT_CMD_MONSC",	"AT^MONSC\r\n",	"MONSC:",	AT_DEFAULT_TIMEOUT,	1,	check_modem_monsc, NULL, ERR_AT_NAK},
	{AT_CMD_CURC2,	"AT_CMD_CURC2",	"AT^CURC=2,0,0\r\n",	"",	AT_DEFAULT_TIMEOUT,	1,	NULL, NULL, ERR_AT_NAK},
	{AT_CMD_CEUS,	"AT_CMD_CEUS",	"AT+CEUS=1\r\n",	"",	AT_DEFAULT_TIMEOUT,	1,	NULL, NULL, ERR_AT_NAK},
	{AT_CMD_NDISSTATQRY,	"AT_CMD_NDISSTATQRY",	"AT^NDISSTATQRY?\r\n",	"NDISSTATQRY",	AT_DEFAULT_TIMEOUT,	1, check_ecm_connection_status, NULL, ERR_AT_NAK},
	{AT_CMD_NDISDUP0,	"AT_CMD_NDISDUP0",	"AT^NDISDUP=1,0\r\n", "",	AT_DEFAULT_TIMEOUT,	1, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_LICCID,	"AT_CMD_LICCID", "AT+ICCID\r\n", "CID:",	AT_DEFAULT_TIMEOUT,	5, check_check_sim_iccid, NULL, ERR_AT_NAK},
	{AT_CMD_HICCID,	"AT_CMD_HICCID", "AT^ICCID?\r\n", "ICCID:",	AT_DEFAULT_TIMEOUT,	5, check_check_sim_iccid, NULL, ERR_AT_NAK},
	{AT_CMD_SICCID,	"AT_CMD_SICCID", "AT^SCID\r\n", "SCID:",	AT_DEFAULT_TIMEOUT, 5, check_check_sim_iccid, NULL, ERR_AT_NAK},
	{AT_CMD_CCID,	"AT_CMD_CCID", "AT+CCID\r\n", "CCID:",	AT_DEFAULT_TIMEOUT,	5, check_check_sim_iccid, NULL, ERR_AT_NAK},
	{AT_CMD_CFUN1,	"AT_CMD_CFUN1", "AT+CFUN=1\r\n", "",	AT_DEFAULT_TIMEOUT,	3, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_CFUN0,	"AT_CMD_CFUN0", "AT+CFUN=0\r\n", "",	AT_DEFAULT_TIMEOUT,	3, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_CFUN11,	"AT_CMD_CFUN11", "AT+CFUN=1,1\r\n", "",	AT_DEFAULT_TIMEOUT,	3, NULL, "OK", ERR_AT_NAK},
	{AT_CMD_CGDCONT,	"AT_CMD_CGDCONT", "AT+CGDCONT?\r\n", "",	AT_DEFAULT_TIMEOUT,	5, check_modem_apn_status, NULL, ERR_AT_NAK},
	{AT_CMD_CGPADDR3,	"AT_CMD_CGPADDR3", "AT+CGPADDR=3\r\n", "+CGPADDR", AT_DEFAULT_TIMEOUT,	15, check_modem_cgpaddr_status, NULL, ERR_AT_NAK},
	{AT_CMD_CGPADDR2,	"AT_CMD_CGPADDR2", "AT+CGPADDR=2\r\n", "+CGPADDR", AT_DEFAULT_TIMEOUT,	3, check_modem_cgpaddr_status, NULL, ERR_AT_NOVALIDIP},
	{AT_CMD_CTZU, "AT_CMD_CTZU", "AT+CTZU?\r\n", "+CTZU:", AT_DEFAULT_TIMEOUT, 1, check_modem_ctzu_status, NULL, ERR_AT_NAK},
	{AT_CMD_CCLK, "AT_CMD_CCLK", "AT+CCLK?\r\n", "+CCLK:", AT_DEFAULT_TIMEOUT, 1, check_modem_cclk_update_time, NULL, ERR_AT_NAK},
	{AT_CMD_QLTS, "AT_CMD_QLTS", "AT+QLTS\r\n", "+QLTS:", AT_DEFAULT_TIMEOUT, 1, check_modem_cclk_update_time, NULL, ERR_AT_NAK},
	//custom according module
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G2G,	"AT_CMD_3G2G",	"","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_5G,	"AT_CMD_5G",	"","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_5G4G,	"AT_CMD_5G4G",	"","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_EVDO,	"AT_CMD_EVDO",	"AT+MODODREX=10\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_SLED,	"AT_CMD_SLED",	"AT^SLED=1\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_HLED,	"AT_CMD_HLED",	"AT^LEDCTRL=1\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	//PVS08 AT
	{AT_CMD_VCSQ,	"AT_CMD_VCSQ",	"AT+CSQ?\r\n",	"CSQ:",	AT_DEFAULT_TIMEOUT,	30,	check_modem_signal, "signal", ERR_AT_NOSIGNAL},	  
	{AT_CMD_VNUM,	"AT_CMD_VNUM",	"AT$QCMIPGETP=0\r\n", "NAI:", AT_DEFAULT_TIMEOUT,	1,	check_modem_vphonenum, "pvs8 phone number", ERR_AT_NAK},
	//U8300
	{AT_CMD_PSRAT,	"AT_CMD_PSRAT",	"AT+PSRAT\r\n", "+PSRAT:", AT_DEFAULT_TIMEOUT,	20,	check_modem_network_sel, NULL, ERR_AT_NAK},
	//PLS8
	{AT_CMD_SWWAN,	"AT_CMD_SWWAN",	"AT^SWWAN?\r\n", "SWWAN:", AT_DEFAULT_TIMEOUT,	3,	check_pls8_connection_status, NULL, ERR_AT_NAK},
	//WPD200
	{AT_CMD_STATE,	"AT_CMD_STATE",	"AT*STATE?\r\n", "STATE:", AT_DEFAULT_TIMEOUT,	3,	NULL/*check_wpd200_reg_status*/, NULL, ERR_AT_NAK},
	{AT_CMD_HSTATE,	"AT_CMD_HSTATE","AT*HSTATE?\r\n", "HSTATE:", AT_DEFAULT_TIMEOUT, 30,	check_wpd200_rssi_status, NULL, ERR_AT_NAK},
	{AT_CMD_WCSQ,	"AT_CMD_WCSQ","AT+CSQ?\r\n", "\r", AT_DEFAULT_TIMEOUT, 6,	check_modem_signal, NULL, ERR_AT_NAK},
	{AT_CMD_MCC,	"AT_CMD_MCC","AT*MCC?\r\n", "MCC:", AT_DEFAULT_TIMEOUT, 3,	check_wpd200_net_code, NULL, ERR_AT_NAK},
	{AT_CMD_MNC,	"AT_CMD_MNC","AT*MNC?\r\n", "MNC:", AT_DEFAULT_TIMEOUT, 3,	check_wpd200_net_code, NULL, ERR_AT_NAK},
	{AT_CMD_PSRAT_C,	"AT_CMD_PSRAT",	"AT+PSRAT\r\n", "+PSRAT:", AT_DEFAULT_TIMEOUT,	30,	check_modem_network_sel_u8300c, NULL, ERR_AT_NOREG},
	//U8300C
	{AT_CMD_ICCID,	"AT_CMD_ICCID","AT+ICCID\r\n", "^SCID:", AT_DEFAULT_TIMEOUT,	5,	check_sim_operator,  "sim card(iccid)", ERR_AT_NOSIM},
	{AT_CMD_CGDCONT_CMNET,	"AT_CMD_CGDCONT","AT+CGDCONT=1,\"IP\",\"cmnet\"\r\n", "+CGDCONT:", AT_DEFAULT_TIMEOUT,	5,	NULL, NULL, ERR_AT_NAK},
	{AT_CMD_CGDCONT_3GNET,	"AT_CMD_CGDCONT","AT+CGDCONT=1,\"IP\",\"3gnet\"\r\n", "+CGDCONT:", AT_DEFAULT_TIMEOUT,	5,	NULL, NULL, ERR_AT_NAK},
	{AT_CMD_CGDCONT_CTLTE,	"AT_CMD_CGDCONT","AT+CGDCONT=1,\"IP\",\"ctlte\"\r\n", "+CGDCONT:", AT_DEFAULT_TIMEOUT,	5,	NULL, NULL, ERR_AT_NAK},
	{AT_CMD_QCIMI,	"AT_CMD_QCIMI",	"AT+QCIMI\r\n",	"",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES,	check_modem_imsi, "imsi", ERR_AT_NOSIM},
	{AT_CMD_EHRPD0,	"AT_CMD_EHRPD0",	"AT+EHRPDENABLE=0\r\n",	"",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES,	NULL, NULL, ERR_AT_NAK},
	{AT_CMD_ATI,	"AT_CMD_ATI",	"ATI\r\n",	"IMEI:",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, check_modem_imei_or_gsn, "imei", ERR_AT_NAK},
	//ATEL:LP41
	{AT_CMD_AINIT,	"AT_CMD_AINIT",	"AT%STATUS=\"init\"\r\n",	"",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES,NULL, "INIT", ERR_AT_NAK},
	{AT_CMD_PDNACT,	"AT_CMD_PDNACT", "AT%PDNACT?\r\n",	"",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, check_lp41_connection_status, "%PDNACT", ERR_AT_NAK},
	{AT_CMD_CGPADDR, "AT_CMD_CGPADDR", "AT+CGPADDR\r\n", "+CGPADDR:", AT_DEFAULT_TIMEOUT, AT_DEFAULT_TIMES, check_modem_cgpaddr, NULL, ERR_AT_NAK},
	{AT_CMD_MEAS8,	"AT_CMD_MEAS8", "AT%MEAS=\"8\"\r\n",	"RSRP",  AT_DEFAULT_TIMEOUT, 30, check_lp41_signal_status, "RSSI", ERR_AT_NOSIGNAL},
	
	//ELSXX
	{AT_CMD_SQNBYPASS,	"AT_CMD_SQNBYPASS", "AT+SQNBYPASS=\"enable\"\r\n", "",  AT_DEFAULT_TIMEOUT, 2, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_SICA, "AT_CMD_SICA", "AT^SICA?\r\n", "", AT_DEFAULT_TIMEOUT, 1, check_els_sica_connection_status, "^SICA", ERR_AT_NAK },
	{AT_CMD_SICA13, "AT_CMD_SICA13", "AT^SICA=1,3\r\n", "", AT_DEFAULT_TIMEOUT, 3, NULL, NULL, ERR_AT_NOREG },
	{AT_CMD_SSRVSET1, "AT_CMD_SSRVSET", "AT^SSRVSET=\"actSrvSet\"\r\n", "^SSRVSET:", AT_DEFAULT_TIMEOUT, 1, check_modem_ssrvset_actsrvset_10, NULL, ERR_AT_NEED_RESET},
	//UBLOX
	{AT_CMD_UMNOCONF_3_15, "AT_CMD_UMNOCONF", "AT+UMNOCONF?\r\n", "3,15", AT_DEFAULT_TIMEOUT, 1, check_ublox_umnoconf, NULL, ERR_AT_NAK },
	{AT_CMD_UMNOCONF0_3_15, "AT_CMD_UMNOCONF", "AT+UMNOCONF=3,15\r\n", "", AT_DEFAULT_TIMEOUT, 3, NULL, "OK", ERR_AT_NAK },
	{AT_CMD_UMNOCONF_1_20, "AT_CMD_UMNOCONF", "AT+UMNOCONF?\r\n", "1,20", AT_DEFAULT_TIMEOUT, 1, check_ublox_umnoconf, NULL, ERR_AT_NAK },
	{AT_CMD_UMNOCONF0_1_20, "AT_CMD_UMNOCONF", "AT+UMNOCONF=1,20\r\n", "", AT_DEFAULT_TIMEOUT, 3, NULL, "OK", ERR_AT_NAK },
	{AT_CMD_UMNOCONF0_1_7, "AT_CMD_UMNOCONF", "AT+UMNOCONF=1,7\r\n", "", AT_DEFAULT_TIMEOUT, 3, NULL, "OK", ERR_AT_NAK },
	{AT_CMD_UBMCONF2, "AT_CMD_UBMCONF2", "AT+UBMCONF=2\r\n", "", AT_DEFAULT_TIMEOUT, 3, NULL, "OK", ERR_AT_NAK },
	{AT_CMD_CESQ, "AT_CMD_CESQ", "AT+CESQ\r\n", "CESQ:", AT_DEFAULT_TIMEOUT, 45, check_modem_signal, "OK", ERR_AT_NAK },
	{AT_CMD_UCGED, "AT_CMD_UCGED", "AT+UCGED?\r\n", "UCGED:", AT_DEFAULT_TIMEOUT, 1, check_fb78_signal_status, "OK", ERR_AT_NAK },
	{AT_CMD_UCGED2, "AT_CMD_UCGED2", "AT+UCGED=2\r\n", "UCGED:", AT_DEFAULT_TIMEOUT, 1, NULL, "OK", ERR_AT_NAK },
	//EC2X
	{AT_CMD_QCFG1, "AT_CMD_QCFG", "AT+QCFG=\"PDP/duplicatechk\",0\r\n", "", AT_DEFAULT_TIMEOUT, 1, NULL, "OK", ERR_AT_NAK },
	{AT_CMD_CLCK, "AT_CMD_CLCK", "AT+CLCK=\"pu\",0,\"12341234\"\r\n", "", AT_DEFAULT_TIMEOUT, 1, NULL, "OK", ERR_AT_NAK },
	{AT_CMD_QCFG, "AT_CMD_QCFG", "AT+QCFG=\"nwscanmode\"\r\n", "\"nwscanmode\",", AT_DEFAULT_TIMEOUT, 1, check_modem_nwscanmode, "nwscanmode", ERR_AT_NAK },
	{AT_CMD_QENG_SRVCELL2, "AT_CMD_QENG_SRVCELL2", "AT+QENG=\"servingcell\"\r\n", "+QENG:", AT_DEFAULT_TIMEOUT, 1, check_modem_qeng_servingcell2, NULL, ERR_AT_NOSIGNAL},
	{AT_CMD_QCFG_IMS, "AT_CMD_QCFG_IMS", "AT+QCFG=\"ims\"\r\n", "+QCFG:", AT_DEFAULT_TIMEOUT, 1, check_modem_qcfg_ims, "ims config", ERR_AT_NAK},
	{AT_CMD_QCFG_IMS2, "AT_CMD_QCFG_IMS2", "AT+QCFG=\"ims\"\r\n", "+QCFG:", AT_DEFAULT_TIMEOUT, 1, check_modem_qcfg_ims2, "ims config", ERR_AT_NAK},
	//QEUCTEL 5G
	{AT_CMD_MODE_PREF, "AT_CMD_MODE_PREF", "AT+QNWPREFCFG=\"mode_pref\"\r\n", "\"mode_pref\",", AT_DEFAULT_TIMEOUT, 1, check_modem_mode_pref, "mode_pref", ERR_AT_NAK },
	{AT_CMD_MODE_PREF2, "AT_CMD_MODE_PREF2", "AT+QNWPREFCFG=\"mode_pref\"\r\n", "\"mode_pref\",", AT_DEFAULT_TIMEOUT, 1, check_modem_mode_pref2, "mode_pref", ERR_AT_NAK },
	{AT_CMD_QENG_SRVCELL, "AT_CMD_QENG_SRVCELL", "AT+QENG=\"servingcell\"\r\n", "+QENG:", AT_DEFAULT_TIMEOUT, 30, check_modem_qeng_servingcell, "signal", ERR_AT_NOSIGNAL},
	{AT_CMD_QNR5G_MODE, "AT_CMD_QNR5G_MODE", "AT+QNWPREFCFG=\"nr5g_disable_mode\"\r\n", "nr5g_disable", AT_DEFAULT_TIMEOUT, 1, check_modem_nr5g_disable_mode, NULL, ERR_AT_NAK},
	{AT_CMD_QCPRFMOD, "AT_CMD_QCPRFMODE","AT$QCPRFMOD=PID:1,OVRRIDEHOPDP:\"IPV4V6\"\r\n","", AT_DEFAULT_TIMEOUT,	1, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_MPDN_STATUS, "AT_CMD_MPDN_STATUS","AT+QMAP=\"MPDN_status\"\r\n", "+QMAP:", AT_DEFAULT_TIMEOUT,	1, check_modem_mpdn_status, NULL, ERR_AT_NAK},
	//FIBOCOM 5G
	{AT_CMD_GTCCINFO, "AT_CMD_GTCCINFO", "AT+GTCCINFO?\r\n", "+GTCCINFO:", AT_DEFAULT_TIMEOUT,	30, check_modem_gtccinfo, "signal", ERR_AT_NOSIGNAL},
	{AT_CMD_GTRNDIS, "AT_CMD_GTRNDIS", "AT+GTRNDIS?\r\n", "GTRNDIS:", AT_DEFAULT_TIMEOUT,	1, check_modem_gtrndis, NULL, ERR_AT_NAK},
	{AT_CMD_CAVIMS, "AT_CMD_CAVIMS", "AT+CAVIMS?\r\n", "+CAVIMS:", AT_DEFAULT_TIMEOUT, 1, check_modem_cavims, "ims config", ERR_AT_NAK },
	{AT_CMD_CAVIMS0, "AT_CMD_CAVIMS0", "AT+CAVIMS=0\r\n", "", AT_DEFAULT_TIMEOUT, 1, NULL, NULL, ERR_AT_NAK },
	//especial at
	{AT_CMD_CREG_AUTO, "AT_CMD_CREG_AUTO", "AT\r\n", "", AT_DEFAULT_TIMEOUT, 30, NULL, "register status", ERR_AT_NAK },

	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_huawei_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT^SYSCFG=2,2,3FFFFFFF,1,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT^SYSCFG=13,1,3FFFFFFF,1,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT^SYSCFG=14,2,3FFFFFFF,1,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};
AT_CMD gl_zte_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT^PREFMODE=8\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT^PREFMODE=2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT^PREFMODE=4\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};
AT_CMD gl_sierra_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT^SYSCFG=2,2,3FFFFFFF,1,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT^SYSCFG=13,1,3FFFFFFF,1,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT^SYSCFG=14,2,3FFFFFFF,1,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};
AT_CMD gl_cinterion_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT+COPS=0,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT+COPS=0,2,,0\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+COPS=0,2,,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+COPS=0,2,,7\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_cinterion_plas_xx_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT+COPS=0\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT+COPS=0,,,0\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+COPS=0,,,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+COPS=0,,,7\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_cinterion_cat1_e_at_cmds[] = {
	{AT_CMD_AUTO, "AT_CMD_AUTO",	"AT^SXRAT=5,3\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT^SXRAT=0\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT^SXRAT=2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},//No use, ELS61-E doesn't support HSPA/WCDMA.
	{AT_CMD_4G,	"AT_CMD_4G",	"AT^SXRAT=3\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_cinterion_cat1_us_at_cmds[] = {
	{AT_CMD_AUTO, "AT_CMD_AUTO",	"AT^SXRAT=4,3\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT^SXRAT=0\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},//No use, ELS61-US doesn't support GSM/EDGE.
	{AT_CMD_3G,	"AT_CMD_3G",	"AT^SXRAT=2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT^SXRAT=3\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_quectel_ec25_at_cmds[] = {
	{AT_CMD_AUTO, "AT_CMD_AUTO",	"AT+QCFG=\"nwscanmode\",0,1\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT+QCFG=\"nwscanmode\",1,1\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+QCFG=\"nwscanmode\",5,1\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+QCFG=\"nwscanmode\",3,1\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_EVDO,	"AT_CMD_EVDO",	"AT+QCFG=\"nwscanmode\",8,1\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_quectel_rm500_at_cmds[] = {
	{AT_CMD_AUTO, "AT_CMD_AUTO",	"AT+QNWPREFCFG=\"mode_pref\",AUTO\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+QNWPREFCFG=\"mode_pref\",WCDMA\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+QNWPREFCFG=\"mode_pref\",LTE\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_5G,	"AT_CMD_5G",	"AT+QNWPREFCFG=\"mode_pref\",NR5G\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_5G4G,	"AT_CMD_5G4G",	"AT+QNWPREFCFG=\"mode_pref\",NR5G:LTE\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_quectel_ep06_at_cmds[] = {
	{AT_CMD_AUTO, "AT_CMD_AUTO",	"AT+QCFG=\"nwscanmode\",0,1\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+QCFG=\"nwscanmode\",2,1\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+QCFG=\"nwscanmode\",3,1\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_fibocom_fm650_at_cmds[] = {
	{AT_CMD_AUTO, "AT_CMD_AUTO",	"AT+GTRAT=10\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+GTRAT=2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+GTRAT=3\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_5G,	"AT_CMD_5G",	"AT+GTRAT=14\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_5G4G,	"AT_CMD_5G4G",	"AT+GTRAT=16\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_fibocom_fg360_at_cmds[] = {
	{AT_CMD_AUTO, "AT_CMD_AUTO",	"AT+GTRAT=10\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+GTRAT=2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+GTRAT=3\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_5G,	"AT_CMD_5G",	"AT+GTRAT=14\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_5G4G,	"AT_CMD_5G4G",	"AT+GTRAT=17\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_fibocom_nl668_at_cmds[] = {
	{AT_CMD_AUTO, "AT_CMD_AUTO",	"AT+GTRAT=10\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+GTRAT=2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+GTRAT=3\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_cinterion_band_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT^SCFG=\"Radio/Band\",\"511\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT^SCFG=\"Radio/Band\",\"15\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT^SCFG=\"Radio/Band\",\"496\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_cinterion_pls8_e_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT^SCFG=\"Radio/Band\",\"2928787\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT^SCFG=\"Radio/Band\",\"3\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT^SCFG=\"Radio/Band\",\"4240\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT^SCFG=\"Radio/Band\",\"2924544\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G2G,	"AT_CMD_3G2G",	"AT^SCFG=\"Radio/Band\",\"4243\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};
AT_CMD gl_cinterion_pls8_us_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT^SCFG=\"Radio/Band\",\"1262191\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT^SCFG=\"Radio/Band\",\"15\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT^SCFG=\"Radio/Band\",\"608\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT^SCFG=\"Radio/Band\",\"1261568\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G2G,	"AT_CMD_3G2G",	"AT^SCFG=\"Radio/Band\",\"623\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_cinterion_pls8_x_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT^SCFG=\"Radio/Band\",\"5456495\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT^SCFG=\"Radio/Band\",\"15\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT^SCFG=\"Radio/Band\",\"608\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT^SCFG=\"Radio/Band\",\"5455872\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G2G,	"AT_CMD_3G2G",	"AT^SCFG=\"Radio/Band\",\"623\",\"1\"\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_tw_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT^SYSCONFIG=2,2,1,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT^SYSCONFIG=13,1,1,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT^SYSCONFIG=15,2,1,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT^SYSCONFIG=38,0,1,2\r\n","",	AT_DEFAULT_TIMEOUT,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_hw_lte_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT^SYSCFGEX=\"030201\",3FFFFFFF,1,2,7FFFFFFFFFFFFFFF,,\r\n", "",	10,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT^SYSCFGEX=\"01\",3FFFFFFF,1,2,7FFFFFFFFFFFFFFF,,\r\n", "",	10,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT^SYSCFGEX=\"02\",3FFFFFFF,1,2,7FFFFFFFFFFFFFFF,,\r\n", "",	10,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT^SYSCFGEX=\"03\",3FFFFFFF,1,2,7FFFFFFFFFFFFFFF,,\r\n", "",	10,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G2G,	"AT_CMD_3G2G",	"AT^SYSCFGEX=\"0201\",3FFFFFFF,1,2,7FFFFFFFFFFFFFFF,,\r\n", "",	10,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_lonsung_lte_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT+MODODREX=2\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT+MODODREX=3\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+MODODREX=7\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+MODODREX=5\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_wt_evdo_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT*MODE=3\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT*MODE=5\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT*MODE=6\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_u8300c_lte_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT+MODODREX=2\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT+MODODREX=3\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+MODODREX=7\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+MODODREX=5\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_ublox_lte_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT+URAT=6\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT+URAT=0\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+URAT=2\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+URAT=3\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_ublox_lara_r2_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT+URAT=5,3\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT+URAT=0\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+URAT=2\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+URAT=3\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};

AT_CMD gl_ublox_lara_r202_at_cmds[] = {
	{AT_CMD_AUTO,	"AT_CMD_AUTO",	"AT+URAT=6,3\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_2G,	"AT_CMD_2G",	"AT+URAT=0\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_3G,	"AT_CMD_3G",	"AT+URAT=2\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{AT_CMD_4G,	"AT_CMD_4G",	"AT+URAT=3\r\n", "",	AT_DEFAULT_TIMES,	AT_DEFAULT_TIMES, NULL, NULL, ERR_AT_NAK},
	{-1,NULL,"","",AT_DEFAULT_TIMEOUT,AT_DEFAULT_TIMES,NULL,NULL,ERR_AT_OK},
};


/*TODO*/
/* FixMe: 6th command must be NETWORK_TYPE_CMD: AT_CMD_AUTO or ATCMD_2G or ... */
int gl_wcdma_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_CURC, AT_CMD_GSN, AT_CMD_SCPIN, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_CSQ, AT_CMD_CGREG, AT_CMD_COPS, AT_CMD_SYSINFO, AT_CMD_HICCID, -1};
int gl_evdo_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_AUTO, AT_CMD_CSQ, AT_CMD_VSYSINFO,-1};
int gl_evdo_nosim_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_GSN, AT_CMD_AT, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_AUTO, AT_CMD_CSQ, AT_CMD_VSYSINFO,-1};
int gl_hspaplus_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_CURC, AT_CMD_GSN, AT_CMD_SCPIN, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_CSQ, AT_CMD_CGREG2, AT_CMD_CGREG, AT_CMD_COPS, AT_CMD_SYSINFOEX, -1};
int gl_hspaplus_mu609_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_HLED, AT_CMD_GSN, AT_CMD_SCPIN, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_CSQ, AT_CMD_CGREG2, AT_CMD_CGREG, AT_CMD_COPS2, AT_CMD_COPS, AT_CMD_SYSINFOEX, AT_CMD_HICCID, -1};
int gl_hspaplus_phs8_at_lists[] = {AT_CMD_AT, AT_CMD_SLED, AT_CMD_ATE0, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI,AT_CMD_AT, AT_CMD_SMONI, AT_CMD_AUTO, AT_CMD_COPS3, AT_CMD_SMONI, AT_CMD_CSQ, AT_CMD_CREG, AT_CMD_COPS, AT_CMD_SMONI, AT_CMD_SICCID, -1};
int gl_evdo_pvs8_at_lists[] = {AT_CMD_AT, AT_CMD_SLED, AT_CMD_ATE0, AT_CMD_GSN, AT_CMD_CIMI, AT_CMD_SMONI, AT_CMD_AUTO, AT_CMD_VCSQ, AT_CMD_SMONI,-1};
int gl_lte_pls8_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_SLED, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_AT, AT_CMD_CSQ, AT_CMD_SMONI, AT_CMD_CEREG, AT_CMD_CREG2, AT_CMD_CREG, AT_CMD_COPS,AT_CMD_CGATT, AT_CMD_SICCID, -1};
int gl_lte_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_CSQ, AT_CMD_CREG, AT_CMD_COPS2, AT_CMD_COPS, -1};
int gl_telit_lte_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_CSQ, AT_CMD_CEREG2, AT_CMD_CEREG, AT_CMD_COPS2, AT_CMD_COPS, AT_CMD_CGREG2, AT_CMD_LICCID, -1};
int gl_lte_me209_at_lists[] = {AT_CMD_AT, AT_CMD_HLED, AT_CMD_ATE0, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_CSQ, AT_CMD_CGREG2, AT_CMD_CGREG, AT_CMD_COPS2, AT_CMD_COPS, AT_CMD_HICCID, -1};
int gl_lte_me909_at_lists[] = {AT_CMD_AT, AT_CMD_HLED, AT_CMD_ATE0, AT_CMD_CEUS, AT_CMD_NDISDUP0, AT_CMD_GSN, AT_CMD_SCPIN, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_AT, AT_CMD_CSQ, AT_CMD_CGREG2, AT_CMD_CGREG, AT_CMD_AT, AT_CMD_COPS, AT_CMD_SYSINFOEX, AT_CMD_CGATT, AT_CMD_HICCID, AT_CMD_MONSC, -1};
int gl_lte_lua10_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_CSQ, AT_CMD_CEREG2, AT_CMD_CEREG, AT_CMD_CREG, AT_CMD_COPS2, AT_CMD_COPS, AT_CMD_HICCID, -1};
int gl_lte_u8300_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_AT,  AT_CMD_AUTO, AT_CMD_CSQ, AT_CMD_CEREG2,AT_CMD_CEREG, AT_CMD_CREG2,AT_CMD_CREG,AT_CMD_COPS2,AT_CMD_COPS,AT_CMD_PSRAT, AT_CMD_LICCID, -1};
int gl_lte_ec25_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_QCFG_IMS, AT_CMD_GSN, AT_CMD_QCFG1, AT_CMD_CPIN, AT_CMD_CIMI,  AT_CMD_QCFG, AT_CMD_CSQ1, AT_CMD_CEREG2, AT_CMD_CGREG2,AT_CMD_CREG2,AT_CMD_CREG_AUTO,AT_CMD_COPS2,AT_CMD_COPS,AT_CMD_AT, AT_CMD_LICCID, AT_CMD_QENG_SRVCELL2, -1};
int gl_nr_rm500_at_lists[] = {AT_CMD_AT, AT_CMD_CFUN1, AT_CMD_ATE0, AT_CMD_QCFG_IMS, AT_CMD_GSN, AT_CMD_QCFG1, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_MODE_PREF, AT_CMD_QENG_SRVCELL, AT_CMD_C5GREG2, AT_CMD_CEREG2, AT_CMD_CGREG2, AT_CMD_CREG_AUTO,AT_CMD_AT, AT_CMD_LICCID, -1};
int gl_nr_rm520n_at_lists[] = {AT_CMD_AT, AT_CMD_CFUN1, AT_CMD_ATE0, AT_CMD_QCFG_IMS, AT_CMD_GSN, AT_CMD_QCFG1, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_MODE_PREF, AT_CMD_QENG_SRVCELL, AT_CMD_C5GREG2, AT_CMD_CEREG2, AT_CMD_CGREG2, AT_CMD_CREG_AUTO,AT_CMD_AT, AT_CMD_LICCID, -1};
int gl_nr_rm500u_at_lists[] = {AT_CMD_AT, AT_CMD_CFUN1, AT_CMD_ATE0, AT_CMD_QCFG_IMS2, AT_CMD_GSN, AT_CMD_AT, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_MODE_PREF2, AT_CMD_QENG_SRVCELL, AT_CMD_C5GREG2, AT_CMD_CEREG2, AT_CMD_CGREG2, AT_CMD_CREG_AUTO,AT_CMD_AT, AT_CMD_CCID, -1};
int gl_lte_ep06_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_QCFG_IMS, AT_CMD_GSN, AT_CMD_QCFG1, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_QCFG, AT_CMD_CSQ, AT_CMD_CEREG2, AT_CMD_CGREG2, AT_CMD_CREG_AUTO,AT_CMD_COPS2,AT_CMD_COPS,AT_CMD_AT, AT_CMD_LICCID, -1};
int gl_lte_ec20_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_QCFG_IMS, AT_CMD_QCFG1, AT_CMD_CGSN, AT_CMD_CPIN, AT_CMD_CIMI,  AT_CMD_QCFG, AT_CMD_CSQ1, AT_CMD_CEREG2,AT_CMD_CGREG2,AT_CMD_CREG2,AT_CMD_CREG_AUTO,AT_CMD_COPS2,AT_CMD_COPS, AT_CMD_AT, AT_CMD_LICCID, AT_CMD_QENG_SRVCELL2, -1};
int gl_lte_mc7350_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_CSQ, AT_CMD_CREG, AT_CMD_COPS2, AT_CMD_COPS, -1};
int gl_wpd200_at_lists[] = {AT_CMD_ATE0, AT_CMD_AT, AT_CMD_GSN, AT_CMD_AT, AT_CMD_AT,AT_CMD_AT, AT_CMD_AUTO, AT_CMD_HSTATE,AT_CMD_AT, AT_CMD_WSYSINFO, AT_CMD_MCC, AT_CMD_MNC, -1};
int gl_lp41_lte_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0,AT_CMD_AINIT, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_MEAS8, AT_CMD_CGACT1, AT_CMD_CEREG, AT_CMD_COPS2, AT_CMD_COPS, -1};

int gl_lte_els31_at_lists[] = {AT_CMD_AT, AT_CMD_CFUN1, AT_CMD_ATE0, AT_CMD_CGSN, AT_CMD_CPIN, AT_CMD_SQNBYPASS, AT_CMD_CIMI, AT_CMD_CSQ, AT_CMD_CEREG2, AT_CMD_CEREG, AT_CMD_AT, AT_CMD_COPS2, AT_CMD_COPS, AT_CMD_CGATT, AT_CMD_CGDCONT, AT_CMD_CCID, AT_CMD_SICA13, AT_CMD_CGPADDR3, AT_CMD_CGPADDR2, -1};

int gl_lte_els61_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_AT, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_CSQ, AT_CMD_COPS,  AT_CMD_SMONI, AT_CMD_CEREG, AT_CMD_CREG2, AT_CMD_CEREG, AT_CMD_CGATT, AT_CMD_CCID, -1};
int gl_lte_els81_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_SSRVSET1, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_CSQ, AT_CMD_COPS,  AT_CMD_SMONI, AT_CMD_CEREG, AT_CMD_CREG2, AT_CMD_CEREG, AT_CMD_CGATT, AT_CMD_CCID, -1};

int gl_ublox_lara_cat1_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_CFUN0, AT_CMD_AUTO, AT_CMD_CFUN1, AT_CMD_CESQ, AT_CMD_CEREG2, AT_CMD_CEREG, AT_CMD_CREG2, AT_CMD_AT, AT_CMD_COPS, AT_CMD_CGATT, AT_CMD_CCID, -1};

int gl_ublox_toby_lte_at_lists[] = {AT_CMD_AT, AT_CMD_UBMCONF2, AT_CMD_UMNOCONF0_1_7, AT_CMD_ATE0,  AT_CMD_AT, AT_CMD_CFUN0, AT_CMD_AUTO, AT_CMD_CFUN1 ,AT_CMD_GSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_CESQ, AT_CMD_CEREG2, AT_CMD_CEREG, AT_CMD_CREG2, AT_CMD_AT, AT_CMD_COPS2, AT_CMD_COPS, AT_CMD_CGACT2, AT_CMD_CGATT, AT_CMD_CCID, -1};

int gl_nr_fm650_at_lists[] = {AT_CMD_AT, AT_CMD_CFUN1, AT_CMD_ATE0, AT_CMD_CGSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_GTCCINFO, AT_CMD_C5GREG2, AT_CMD_CEREG2, AT_CMD_CGREG2, AT_CMD_CREG_AUTO, AT_CMD_COPS2, AT_CMD_COPS, AT_CMD_LICCID, AT_CMD_CAVIMS0, -1};

int gl_nr_fg360_at_lists[] = {AT_CMD_AT, AT_CMD_CFUN1, AT_CMD_ATE0, AT_CMD_CAVIMS, AT_CMD_CGSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_GTCCINFO, AT_CMD_C5GREG2, AT_CMD_CEREG2, AT_CMD_CGREG2, AT_CMD_CREG_AUTO, AT_CMD_COPS2, AT_CMD_COPS, AT_CMD_LICCID, -1};

int gl_lte_nl668_at_lists[] = {AT_CMD_AT, AT_CMD_CFUN1, AT_CMD_ATE0, AT_CMD_CGSN, AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_AUTO, AT_CMD_GTCCINFO, AT_CMD_CEREG2, AT_CMD_CGREG2, AT_CMD_CREG_AUTO, AT_CMD_COPS2, AT_CMD_COPS, AT_CMD_CCID, -1};

/*CMD_AUTO reset modem config every time*/
int gl_lte_u8300c_at_lists_const[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_ATI, AT_CMD_EHRPD0, AT_CMD_CPIN, AT_CMD_AT, AT_CMD_AUTO, AT_CMD_PSRAT_C, 
				-1, -1, -1,	-1,	-1,	-1,	-1,	-1,	-1,	-1, -1, -1, -1};
int gl_lte_u8300c_at_lists[] = {AT_CMD_AT, AT_CMD_ATE0, AT_CMD_ATI, AT_CMD_EHRPD0, AT_CMD_CPIN, AT_CMD_AT, AT_CMD_AUTO, AT_CMD_PSRAT_C, 
				-1, -1, -1,	-1,	-1,	-1,	-1,	-1,	-1,	-1, -1, -1, -1};
int u8300c_lte_at_lists[] = {AT_CMD_CPIN, AT_CMD_CIMI, AT_CMD_AT, AT_CMD_CSQ, AT_CMD_CREG2, AT_CMD_CREG, AT_CMD_COPS2,AT_CMD_COPS,AT_CMD_PSRAT , AT_CMD_LICCID, AT_CMD_ATI, -1};
int u8300c_cdma_at_lists[] = {AT_CMD_QCPIN, AT_CMD_QCIMI, AT_CMD_AT, AT_CMD_CCSQ, AT_CMD_AT, AT_CMD_AT, AT_CMD_COPS2,AT_CMD_COPS,AT_CMD_PSRAT, AT_CMD_LICCID, AT_CMD_ATI, -1};

AT_CMD* find_at_cmd(int index)
{
	AT_CMD *cmd;

	for(cmd=gl_at_cmds; cmd->index!=-1; cmd++){
		if(cmd->index==index) return cmd;
	}

	return NULL;
}

AT_CMD* find_at_cmd_by_name(char *name)
{
	AT_CMD *cmd;

	if(name==NULL) return NULL;

	for(cmd=gl_at_cmds; cmd->name!=NULL; cmd++){
		if(strcmp(cmd->name, name)==0) return cmd;
	}

	return NULL;
}

int setup_custom_at_cmd(uint64_t modem_code)
{
	AT_CMD *cmd, *custom, *custom_cmds;

	switch(modem_code) {
	case IH_FEATURE_MODEM_MC2716:
	case IH_FEATURE_MODEM_MC5635:
	case IH_FEATURE_MODEM_PVS8:
		custom_cmds = gl_zte_at_cmds;
		break;
	case IH_FEATURE_MODEM_MC7710:
		custom_cmds = gl_sierra_at_cmds;
		break;
	case IH_FEATURE_MODEM_PHS8:
#if 1
		custom_cmds = gl_cinterion_band_at_cmds;
		break;
#endif
	case IH_FEATURE_MODEM_MC7350:
		custom_cmds = gl_cinterion_at_cmds;
		break;
	case IH_FEATURE_MODEM_PLS8:
		if (ih_key_support("FS18")){
			custom_cmds = gl_cinterion_pls8_us_at_cmds;
		}else if (ih_key_support("FS08")) {
			custom_cmds = gl_cinterion_pls8_e_at_cmds;
		}else if (ih_key_support("FS59")
				|| ih_key_support("FS39")) {
			custom_cmds = gl_cinterion_plas_xx_at_cmds;
		}else {
			custom_cmds = gl_cinterion_pls8_x_at_cmds;
		}
		break;
	case IH_FEATURE_MODEM_ELS61:
	case IH_FEATURE_MODEM_ELS81:
		if (ih_key_support("FS61")){
			custom_cmds = gl_cinterion_cat1_e_at_cmds;
		}else {
			custom_cmds = gl_cinterion_cat1_us_at_cmds;
		}
		break;
	case IH_FEATURE_MODEM_EC20:
	case IH_FEATURE_MODEM_EC25:
		custom_cmds = gl_quectel_ec25_at_cmds;
		break;
	case IH_FEATURE_MODEM_RM500:
	case IH_FEATURE_MODEM_RM520N:
	case IH_FEATURE_MODEM_RM500U:
	case IH_FEATURE_MODEM_RG500:
		custom_cmds = gl_quectel_rm500_at_cmds;
		break;
	case IH_FEATURE_MODEM_EP06:
		custom_cmds = gl_quectel_ep06_at_cmds;
		break;
	case IH_FEATURE_MODEM_FM650:
		custom_cmds = gl_fibocom_fm650_at_cmds;
		break;
	case IH_FEATURE_MODEM_FG360:
		custom_cmds = gl_fibocom_fg360_at_cmds;
		break;
	case IH_FEATURE_MODEM_NL668:
		custom_cmds = gl_fibocom_nl668_at_cmds;
		break;
	case IH_FEATURE_MODEM_LUA10:
		custom_cmds = gl_tw_at_cmds;
		break;	
	case IH_FEATURE_MODEM_ME909:
		custom_cmds = gl_hw_lte_at_cmds;
		break;	
	case IH_FEATURE_MODEM_U8300:
		custom_cmds = gl_lonsung_lte_at_cmds;
		break;	
	case IH_FEATURE_MODEM_WPD200:
		custom_cmds = gl_wt_evdo_at_cmds;
		break;
	case IH_FEATURE_MODEM_U8300C:
		custom_cmds = gl_u8300c_lte_at_cmds;
		break;
	case IH_FEATURE_MODEM_LARAR2:
		if (ih_key_support("FB13")){
			custom_cmds = gl_ublox_lara_r202_at_cmds;
		}else{
			custom_cmds = gl_ublox_lara_r2_at_cmds;
		}

		break;
	case IH_FEATURE_MODEM_TOBYL201:
		custom_cmds = gl_ublox_lte_at_cmds;
		break;
	case IH_FEATURE_MODEM_EM770W:
	case IH_FEATURE_MODEM_EM820W:
	case IH_FEATURE_MODEM_MU609:
	default:
		custom_cmds = gl_huawei_at_cmds;
		break;
	}
	
	g_modem_custom_cmds = custom_cmds;

	/*custom_cmds --> gl_at_cmds*/
	for(custom=custom_cmds; custom->index!=-1; custom++){
		cmd = find_at_cmd(custom->index);
		if(cmd) {
			*cmd = *custom;
		}
	}
	return 0;
}

int modem_init_serial(int fd, int baud)
{
	if(gl_modem_code == IH_FEATURE_MODEM_FG360){
		return 0;
	}

	serial_set_speed(fd, baud);
	serial_set_parity(fd, 8, 1, 'N', 0, 0);

	return 0;
}

int modem_read(int fd, char* buf, int len)
{
	int ret;

	ret = read(fd, buf, len-1);
	if(ret>0) {
		buf[ret]='\0';
		LOG_IN("modem response (%d): %s", ret, buf);
	}

	return ret;
}

int modem_write(int fd, char* buf, int len, int timeout)
{
	char tmp[64];
	int i=0;
	//try to flush fd
	while(i < 10 && (read(fd, tmp, sizeof(tmp)) > 0))i++;

	LOG_IN("send to modem (%d): %s", len, buf);

	//FIXME: timeout write
	write(fd, buf, len);
	return 0;
}

int my_insmod(const char *module, ...)
{
	va_list ap;
	int argc = 0;
	char *argv[MAX_ARGS+1] = {NULL};
	char mod_name[32] = {0};
	char buff[64] = {0};
	char *p = NULL;

	if(!module || !module[0]){
		return -1;
	}

	strlcpy(mod_name, module, sizeof(mod_name));

	p = strstr(mod_name, ".");
	if(p) *p = '\0';
	
	snprintf(buff, sizeof(buff), "/sys/module/%s/initstate", mod_name);
	if(!access(buff, R_OK)){
		LOG_DB("module is already loaded - %s", mod_name);
		return 0;
	}

	snprintf(buff, sizeof(buff), "/modules/%s.ko", mod_name);

	argv[argc++] = "insmod";
	argv[argc++] = buff;

	va_start(ap, module);
	do{
		p = va_arg(ap, char *);
		argv[argc++] = p;
		if(!p) break;
	}while(argc<MAX_ARGS);

	va_end(ap);

	return _eval(argv, NULL, 0, NULL);
}

typedef struct USB_BLACKLIST{
	char vendor[8];
	char product[8];
}USB_BLACKLIST;

static int usb_match_blacklist(char *vendor, char *product)
{
	//TODO: add new blacklist
	USB_BLACKLIST blacklist[] = {	{"040d","3801"},/*VIA WLAN*/
					{"0d6b","0003"},/*xHCI Host Controller*/
					{"0d6b","0002"},/*xHCI Host Controller*/
					{"0424","5744"},/*Microchip Tech USB5744*/
					{"0424","2744"},/*Microchip Tech USB2744*/
					{"0424","2740"},/*Microchip Tech Hub Controller*/
					};
	int i;

	for(i=0; i<sizeof(blacklist)/sizeof(USB_BLACKLIST); i++) {
		if(strcmp(blacklist[i].vendor, vendor)==0
			&& strcmp(blacklist[i].product, product)==0) {
			return 1;
		}
	}

	return 0;
}

static int usb_match_whitelist(char *vendor, char *product)
{
	//TODO: add new whitelist
	USB_BLACKLIST whitelist[] = {	{"2c7c","0800"},/*NRQ0*/
					{"2c7c","0801"},/*RM520N*/
					{"2c7c","0900"},/*NRQ2*/
					{"2c7c","0125"},/*LQ20,FQ58,FQ38,FQ78*/
					{"2c7c","0306"},/*FQ39*/
					{"1e2d","0061"},/*FS39*/
					{"2cb7","0a05"},/*NRF1*/
					{"1508","1001"},/*LF20*/
					};
	int i;

	for(i=0; i<sizeof(whitelist)/sizeof(USB_BLACKLIST); i++) {
		if(strcmp(whitelist[i].vendor, vendor)==0
			&& strcmp(whitelist[i].product, product)==0) {
			return 1;
		}
	}

	return 0;
}

typedef enum{
		ST_T = 'T',
		ST_B = 'B',
		ST_D = 'D',
		ST_P = 'P',
		ST_S = 'S',
		ST_C = 'C',
		ST_I = 'I',
		ST_E = 'E',
		ST_A = 'A'
}PROC_USB_DEVICES_STATE;

/** @brief scan /proc/bus/usb/devices for device info
 * 
 * @param iface_num		out		max ifaces of usb device
 * @param vendor_id		out		vendor id of usb device
 * @param product_id	out		product id of usb device
 * @param manufacturer	out		manufacture name of usb device
 * @param product		out		product name of usb device
 *
 * @return <0 for error, =0 for not found, >0 for ok
 */
int usb_get_device_info(int *iface_num, char *vendor_id, char *product_id, char *manufacturer, char *product, int verbose)
{
	FILE *fp;
	char buf[512];
	char *p1, *p2;
	PROC_USB_DEVICES_STATE proc_state = ST_T;
	int dev_cnt = 0;
	int iface_cnt = 0;
	int max_iface = 0;
	int attached = 0;

	if(gl_modem_code == IH_FEATURE_MODEM_FG360){
		return -1;
	}

	fp = fopen(PROC_USB_DEVICES, "r");

	if(fp == NULL){
		LOG_ER("can not open %s[%d:%s]. Reboot!\n", PROC_USB_DEVICES, errno, strerror(errno));
		if(allow_reboot()){
			/*here driver is bad, should reboot the router*/
			system("reboot");
			while(1) sleep(1000);
		}
		return -1;
	}
	
	while(fgets(buf, sizeof(buf), fp)){
		if (*buf=='\n') {//new bus
#ifdef INHAND_IG9
      /*If we have find a attach device, exit!*/
			if (attached) {
				break;
			}
#endif

			if (proc_state!=ST_T && proc_state!=ST_E) {
				LOG_ER("invalid state %d for new bus!", proc_state);
			}
			proc_state = ST_T;
			continue;
		}

		if (proc_state==ST_B && *buf=='D') {
			proc_state = ST_D;
			attached = 1; //missing 'B' line means we get a device
			dev_cnt++;
			if(verbose){
				LOG_IN("got an attached device");
			}
		}else if (proc_state==ST_S && *buf=='C') {
			proc_state = ST_C;
		}else if (proc_state==ST_E && *buf=='I') {
			proc_state = ST_I;
		}else if (proc_state==ST_E && *buf=='C') {
			proc_state = ST_C;
		}else if (proc_state==ST_I && *buf=='A') {
			proc_state = ST_A;
		}

		if (*buf!=proc_state) {
			LOG_ER("invalid string: %s", buf);
			continue;
		}

		switch(proc_state) {
		case ST_T: //T:  Bus=05 Lev=00 Prnt=00 Port=00 Cnt=00 Dev#=  1 Spd=480 MxCh= 8
			if (*buf!='T') {
				LOG_DB("invalid string: %s", buf);
				break;
			}
			proc_state = ST_B;
			iface_cnt = 0;
			attached = 0;
			break;
		case ST_B: //B:  Alloc=  0/800 us ( 0%), #Int=  0, #Iso=  0
			proc_state = ST_D;
			break;
		case ST_D: //D:  Ver= 2.00 Cls=09(hub  ) Sub=00 Prot=01 MxPS=64 #Cfgs=  1
			proc_state = ST_P;
			break;
		case ST_P: //P:  Vendor=0000 ProdID=0000 Rev= 2.06
			if (attached) {
				if (vstrsep(buf+4, " ", &p1, &p2, NULL) < 2
					|| strncmp(p1, "Vendor=", 7) || strncmp(p2, "ProdID=", 7)) {
					LOG_ER("invalid string %s for state %d", buf, proc_state);
					break;
				}
#if 0				//added by shenqw, router can insert other usb devices
				//filter other usb device
				if(usb_match_blacklist(p1+7, p2+7)) {
					attached = 0;
					dev_cnt--;
				} else {
					//if (vendor_id) *vendor_id = '\0';
					//if (product_id) *product_id = '\0';
				
					if (vendor_id) strlcpy(vendor_id, p1+7, 7);
					if (product_id) strlcpy(product_id, p2+7, 7);
				}
				LOG_DB("vendor id is %s, product id is %s", p1+7, p2+7);
#else
				if(verbose){
					LOG_DB("vendor id is %s, product id is %s", p1+7, p2+7);
				}
				if(usb_match_whitelist(p1+7, p2+7)) {
					dev_cnt = 1;
					if (vendor_id) strlcpy(vendor_id, p1+7, 7);
					if (product_id) strlcpy(product_id, p2+7, 7);
					goto end;
				}

				attached = 0;
				dev_cnt = 0;
#endif
			}
			proc_state = ST_S;
			break;
		case ST_S: //S:  Manufacturer=Linux 2.6.18-4-686 ehci_hcd
				   //S:  Product=EHCI Host Controller
				   //S:  SerialNumber=0000:00:10.4
			if (attached) {
				if ((p1 = strstr(buf+4, "Manufacturer="))) {
					if (manufacturer) strcpy(manufacturer, p1+13);
					//syslog(LOG_DEBUG, "manufacturer is %s", p1+13);
					break;
				}
				if ((p1=strstr(buf+4, "Product="))) {
					if (product) strcpy(product, p1+8);
					//syslog(LOG_DEBUG, "product is %s", p1+8);
					break;
				}
				if ((p1=strstr(buf+4, "SerialNumber="))) {
					//syslog(LOG_DEBUG, "serial number is %s", p1+13);
					break;
				}
				LOG_ER("invalid string %s for state %d", buf, proc_state);
			}
			break;
		case ST_C: //C:* #Ifs= 1 Cfg#= 1 Atr=e0 MxPwr=  0mA
			if (attached) {
				if ((p1=strstr(buf+4, "#Ifs="))) {
					p1 += 5;
					/*eat " "*/
					while(*p1!=0 && *p1==' ') p1++;
					max_iface = atoi(p1);
					if (iface_num) *iface_num = max_iface;
					//syslog(LOG_DEBUG, "max interfaces is %d", max_iface);
				}
			}
			proc_state = ST_I;
			break;
		case ST_I: //I:  If#= 1 Alt= 0 #EPs= 2 Cls=ff(vend.) Sub=ff Prot=ff Driver=(none)
			if (attached) {
				if ((p1=strstr(buf+4, "If#="))) {
					p1 += 4;
					/*eat " "*/
					while(*p1!=0 && *p1==' ') p1++;
					iface_cnt = atoi(p1);
					if (iface_cnt>max_iface) {
						LOG_DB("invalid iface index %d > %d", iface_cnt, max_iface);
					}
					//syslog(LOG_DEBUG, "current interfaces is %d", iface_cnt);
				} else {
					iface_cnt = 0;
				}

				if ((p1=strstr(buf+4, "Driver="))) {
					//syslog(LOG_DEBUG, "current driver is %s", p1+7);
				}
			}
			proc_state = ST_E;
			break;
		case ST_E: //E:  Ad=81(I) Atr=03(Int.) MxPS=  16 Ivl=128ms
				   //E:  Ad=82(I) Atr=02(Bulk) MxPS=  64 Ivl=0ms
				   //E:  Ad=02(O) Atr=02(Bulk) MxPS=  64 Ivl=0ms
			break;
		case ST_A: //A:  FirstIf#= 0 IfCount= 2 Cls=02(comm.) Sub=0e Prot=00
			proc_state = ST_I;
			break;
		default:
			LOG_ER("invalid state: %d for %s", proc_state, buf);
			proc_state = ST_B;
			break;
		}
	}

end:
	fclose(fp);

	return dev_cnt;
}

int open_socket(char *port)
{
	int fd;
	int one = 1;
	int res, opt;
	socklen_t olen;
	fd_set wait_set;
	struct timeval tv;
	struct sockaddr_in sa;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd<0)return -1;

	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*) &one, (ev_socklen_t)sizeof(one)) == -1){
		LOG_ER("failed to enable SO_REUSEADDR");
		close(fd);
		return -1;
	}

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr("127.0.0.1");
	sa.sin_port = htons(atoi(port));
	
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
	fcntl(fd, F_SETFD, FD_CLOEXEC);

	if(connect(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		if(errno == EINPROGRESS){
			// make file descriptor set with socket
			FD_ZERO(&wait_set);
			FD_SET(fd, &wait_set);
			
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			// wait for socket to be writable; return after given timeout
			if(select(fd + 1, NULL, &wait_set, NULL, &tv) <= 0){
				//timeout or error
				goto error;
			}
			
			opt = 0	;
			olen = sizeof(opt);
			// check for errors in socket layer
			if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &opt, &olen) < 0 || opt){
				goto error;
			}
		}else{
			goto error;
		}
	}
	
	return fd;

error:
	LOG_ER("failed to connect localhost %s", port);
	close(fd);
	return -1;
}

int open_dev(char *dev)
{
	int fd;

	if(!dev) {
		LOG_ER("dev is null");
		return -1;
	}

	if (gl_modem_code == IH_FEATURE_MODEM_FG360) {
		fd = open_socket(dev);
	} else {
		fd = open(dev, O_RDWR|O_NONBLOCK, 0666);
	}

	return fd;
}

//@return 1:usable 0:unusable
int device_test(const char *dev, int mode, int sec)
{
	struct sysinfo s_info;
	unsigned int uptime_start, uptime_end;
	int interval;

	if(!dev || !dev[0]){
		return 0;
	}
	
	sysinfo(&s_info);
	uptime_start = s_info.uptime;//seconds from boot
	while(1){
		usleep(200*1000);
		sysinfo(&s_info);
		uptime_end = s_info.uptime;
		interval = uptime_end - uptime_start;
		if(access(dev, mode) == 0){
			return 1;
		}else if(interval >= sec){
			return 0;
		}
	}
	
	return 0;
}

#define LSPCI_OUT_FILE	"/tmp/lspci.out"
int find_pci_device(int target_vid)
{
	FILE *fp = NULL;
	char *p = NULL;
	int found = 0;
	char buf[128] = {0};
	int pci_class, pci_vid, pci_did;

	if(target_vid <= 0){
		return -1;
	}

	snprintf(buf, sizeof(buf), "lspci > "LSPCI_OUT_FILE);
	system(buf);

	fp = fopen(LSPCI_OUT_FILE, "r");
	if(NULL == fp){
		return -1;
	}

	LOG_IN("target pci vendor id: %04x", target_vid);

	while(fgets(buf, sizeof(buf), fp)){
		if((p = strstr(buf, "Class")) == NULL){
			continue;
		}
		
		pci_class = pci_vid = pci_did = 0;

		sscanf(p, "Class %x: %x:%x", &pci_class, &pci_vid, &pci_did);

		LOG_IN("pci class:%04x vid:%04x did:%04x", pci_class, pci_vid, pci_did);
		if(target_vid == pci_vid){
			found = 1;
		}
	}

	fclose(fp);

	return found;	
}

int check_socket_device(char *dev)
{
	int fd;
	
	if(gl_modem_code == IH_FEATURE_MODEM_FG360){
		//do nothing
	}else{
		return ERR_INVAL;
	}

	fd = open_dev(dev);
	if(fd<0) return -1;
	
	close(fd);
	return ERR_OK;
}

/*
 *	read from fd within the time of timeout(in ms)
 *
 */			
int read_fd(int fd, char* buf, size_t size, int timeout, int verbose)
{
	fd_set rfds;
	struct timeval tv;
	int retval;
	int actualRead = 0;

	tv.tv_sec = timeout/1000;
	tv.tv_usec = (timeout%1000)*1000;	

	buf[0] = '\0';
	
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	do{
		retval = select(fd+1, &rfds, NULL, NULL, &tv);
		if(retval<0 && errno==EINTR) continue;

		//syslog(LOG_DEBUG, "read select return:%d", retval);
		if(retval<=0) break; //timeout or error

		retval = read(fd, buf+actualRead, size-actualRead);
		if(retval<0) break;

		actualRead += retval;
		
		if(verbose)
			LOG_IN("modem response :(%d), %.*s", retval, actualRead, buf);

	}while(0);

	return actualRead;
}

/*
 *	write to fd within the time of timeout(in ms)
 *
 */			
int write_fd(int fd, char* buf, size_t size, int timeout, int verbose)
{
	fd_set wfds;
	struct timeval tv;
	int retval;
	size_t actualWrite = 0;

	tv.tv_sec = timeout/1000;
	tv.tv_usec = (timeout%1000)*1000;	

	if(verbose)
		LOG_IN("send to modem (%d): %s", (int)size, buf);

	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);

	do{
		retval = select(fd+1, NULL, &wfds, NULL, &tv);
		if(retval<0 && errno==EINTR) continue;

		//actualWrite += retval;

		//syslog(LOG_DEBUG, "write select return:%d", retval);
		if(retval<=0) break;

		//syslog(LOG_DEBUG, "zly debug write size:%d, size:%d, actualWrite:%d", size-actualWrite, size, actualWrite);
		retval = write(fd, buf+actualWrite, size-actualWrite);
		if(retval<0) break;

		actualWrite += retval;

	}while(actualWrite<size);

	//if(actualWrite<size) syslog(LOG_DEBUG, "write out %d Bytes to %d, err:%d, %s", actualWrite, fd, errno, strerror(errno));

	if(actualWrite==0) return retval;

	return (int)actualWrite;
}

int check_return2(int fd, char* buf, int len, char* want, int timeout, int verbose)
{
	int ret;
	int got = 0;
	int gotit = 0;

	/*zly: avoid override*/
	if(len>0) len -= 1;

	buf[0] = '\0';
	while(got<len && !gotit){
		ret = read_fd(fd, buf+got, len-got, timeout, verbose);
		//if(ret<0 || ret>=len) return ret;
		if(ret==0) break;
		got += ret;
		buf[got] = '\0';
		if(strstr(buf, want)) gotit = 1;
	}

	//syslog(LOG_DEBUG, "want:%s, get(%d,%d):%s", want, gotit, got, buf);

	return gotit ? got : -1;
}

int check_return3(int fd, char* buf, int len, char* want, int timeout, int verbose)
{
	int ret;
	int got = 0;
	int gotit = 0;

	/*zly: avoid override*/
	if(len>0) len -= 1;

	buf[0] = '\0';
	while(got<len && !gotit){
		ret = read_fd(fd, buf+got, len-got, timeout, verbose);
		//if(ret<0 || ret>=len) return ret;
		if(ret==0) break;
		got += ret;
		buf[got] = '\0';
		if(strstr(buf, want)) gotit = 1;
		else if(strstr(buf, "ERROR"))break; //break if got ERROR
	}

	//syslog(LOG_DEBUG, "want:%s, get(%d,%d):%s", want, gotit, got, buf);

	return gotit ? got : -1;
}

int flush_fd(int fd)
{
	char buf[64];
	int i=0;

	/*max is 10 times*/
	while(i<10 && (read_fd(fd, buf, sizeof(buf), 100, 0)>0)) i++;

	return 0;
}

/*send at command in sync mode
*/
int send_at_cmd_sync(char *dev, char* cmd, char *rsp, int len, int verbose)
{
	int fd;
	char tmp[1024];

	if(!dev || !dev[0]) return -1;

	fd = open_dev(dev);
	if(fd<0) return -1;

	/*flush auto-report data*/
	flush_fd(fd);

	write_fd(fd, cmd, strlen(cmd), 1000, verbose);
	if(check_return2(fd, tmp, sizeof(tmp), "OK", 3000, verbose)<=0){
		close(fd);
		return -2;
	}
	
	close(fd);
	//save response
	if(rsp) strlcpy(rsp, tmp, len);

	return 0;
}

int send_at_cmd_sync_timeout(char *dev, char* cmd, char* rsp, int len, int verbose, char *check_value, int timeout)
{
	int fd;
	char tmp[2048] = {0};

	if(!*dev) return -1;

	fd = open_dev(dev);
	if(fd<0) return -1;

	/*flush auto-report data*/
	flush_fd(fd);

	write_fd(fd, cmd, strlen(cmd), 1000, verbose);
	if(check_return3(fd, tmp, sizeof(tmp), check_value, timeout, verbose)<=0){
		close(fd);
		return -2;
	}
	
	close(fd);
	//save response
	if(rsp) strlcpy(rsp, tmp, len);

	return 0;
}

/*send at command in sync mode2
*/
static int send_at_cmd_sync2(char* cmd, char *rsp, int len, int verbose)
{
	char tmp[128];
	
	if(gl_at_fd < 0) return -1;
	
	write_fd(gl_at_fd, cmd, strlen(cmd), 1000, verbose);
	if(check_return2(gl_at_fd, tmp, sizeof(tmp), "OK", 3000, verbose)<=0){
		return -2;
	}
	//save response
	if(rsp) strlcpy(rsp, tmp, len);

	return 0;
}

/*send at command in sync mode3
*/
int send_at_cmd_sync3(const char *dev, char* cmd, char *rsp, int len, int verbose)
{
	int fd;
	char tmp[1024];

	if(!*dev) return -1;

	fd = open_dev(dev);
	if(fd<0) return -1;

	/*flush auto-report data*/
	flush_fd(fd);

	write_fd(fd, cmd, strlen(cmd), 1000, verbose);
	if(check_return3(fd, tmp, sizeof(tmp), "OK", 3000, verbose)<=0){
		close(fd);
		return -2;
	}
	
	close(fd);
	//save response
	if(rsp) strlcpy(rsp, tmp, len);

	return 0;
}

/* instead of usleep */
static void my_usleep(int usec)
{
	struct timeval tvstart,tvend;
	int interval;
	
	/*may be some day will appear thousand year bug.*/
	gettimeofday(&tvstart,NULL);
	while(1){
		usleep(50*1000);
		gettimeofday(&tvend,NULL);
		interval = (tvend.tv_sec - tvstart.tv_sec)*1000*1000;
		interval = interval + tvend.tv_usec - tvstart.tv_usec;
		if(interval >= usec){
			break;
		}
	}
}

/* instead of sleep, using uptime avoid problem by ntpc change time*/
static void my_sleep(int sec)
{
	struct sysinfo s_info;
	unsigned int uptime_start, uptime_end;
	int interval;
	
	sysinfo(&s_info);
	uptime_start = s_info.uptime;//seconds from boot
	while(1){
		usleep(200*1000);
		sysinfo(&s_info);
		uptime_end = s_info.uptime;
		interval = uptime_end - uptime_start;
		if(interval >= sec){
			break;
		}
	}
}

#ifdef INHAND_WR3
static void start_modem(unsigned int modem_code)
{
	int value = 1;

	gpio(GPIO_WRITE, GPIO_MIDLE, &value);
	gpio(GPIO_WRITE, GPIO_MCUPOWER, &value);

	value = 0;
	gpio(GPIO_WRITE, GPIO_MAIRPLANE, &value);

	value = 1;

	gpio(GPIO_WRITE, GPIO_MPOWER, &value);
	gpio(GPIO_WRITE, GPIO_MRST, &value);
	gpio(GPIO_WRITE, GPIO_MCONTRL, &value);

	my_usleep(10 * 1000); // 10ms

	value = 0;
	gpio(GPIO_WRITE, GPIO_MRST, &value);
	gpio(GPIO_WRITE, GPIO_MCONTRL, &value);

	my_usleep(50 * 1000); // 50ms

	value = 1;
	gpio(GPIO_WRITE, GPIO_MCONTRL, &value);

	my_usleep(600 * 1000); // 600ms

	value = 0;
	gpio(GPIO_WRITE, GPIO_MCONTRL, &value);
}

static void stop_modem(unsigned int modem_code)
{
	int value = 1;

	gpio(GPIO_WRITE, GPIO_MCONTRL, &value);
	my_usleep(900 * 1000); // 900ms

	value = 0;
	gpio(GPIO_WRITE, GPIO_MCONTRL, &value);

	my_usleep(100 * 1000); // 100ms

	gpio(GPIO_WRITE, GPIO_MPOWER, &value);
}
#endif // INHAND_WR3

int set_modem(uint64_t modem_code, int action)
{
	int value, starttime;

	/* FixME: starttime is not used here! */
	//FIXME: handle timeout
	switch(action) {
	case MODEM_START:
		LOG_IN("starting modem...");
		if (modem_code==IH_FEATURE_MODEM_EM770W
		    || modem_code==IH_FEATURE_MODEM_EM820W
		    || modem_code==IH_FEATURE_MODEM_MU609
		    || modem_code==IH_FEATURE_MODEM_ME209
		    || modem_code==IH_FEATURE_MODEM_LP41
		    || modem_code==IH_FEATURE_MODEM_ME909
		    || modem_code==IH_FEATURE_MODEM_LUA10
		    || modem_code==IH_FEATURE_MODEM_MC7710
		    || modem_code==IH_FEATURE_MODEM_MC2716
		    || modem_code==IH_FEATURE_MODEM_MC5635
		    || modem_code==IH_FEATURE_MODEM_EC20
		    || modem_code==IH_FEATURE_MODEM_EC25
		    || modem_code==IH_FEATURE_MODEM_RM500
		    || modem_code==IH_FEATURE_MODEM_RM520N
		    || modem_code==IH_FEATURE_MODEM_RM500U
		    || modem_code==IH_FEATURE_MODEM_RG500
		    || modem_code==IH_FEATURE_MODEM_FM650
		    || modem_code==IH_FEATURE_MODEM_EP06
		    || modem_code==IH_FEATURE_MODEM_U8300
		    || modem_code==IH_FEATURE_MODEM_U8300C
		    || modem_code==IH_FEATURE_MODEM_LE910
		    || modem_code==IH_FEATURE_MODEM_MC7350
			|| modem_code==IH_FEATURE_MODEM_NL668) {
#ifdef INHAND_WR3
			start_modem(modem_code);
			starttime = 20;
#else
			//give a high level to open power
			value = 1;
			gpio(GPIO_WRITE, GPIO_MPOWER, &value);
			starttime = 7;
#endif
		}else if (modem_code==IH_FEATURE_MODEM_PHS8
			|| modem_code==IH_FEATURE_MODEM_PLS8
			|| modem_code==IH_FEATURE_MODEM_ELS61
			|| modem_code==IH_FEATURE_MODEM_ELS81
			|| modem_code==IH_FEATURE_MODEM_ELS31
			|| modem_code==IH_FEATURE_MODEM_TOBYL201
			|| modem_code==IH_FEATURE_MODEM_LARAR2
			|| modem_code==IH_FEATURE_MODEM_PVS8) {
			//give a high level to open power
			value = 1;
			gpio(GPIO_WRITE, GPIO_MPOWER, &value);
			my_usleep(10*1000);//10ms
			//100ms < low pulse < 1s
			value = 1;
			gpio(GPIO_WRITE, GPIO_IGT, &value);
			my_usleep(150*1000);//150ms
			value = 0;
			gpio(GPIO_WRITE, GPIO_IGT, &value);
			starttime = 7;
		}else if(modem_code==IH_FEATURE_MODEM_FG360){
			//FIXME: add code
		}else{
			LOG_WA("unknown modem type!");
			starttime = 5;
		}
		//my_sleep(starttime);
		LOG_IN("modem is started.");
		break;
	case MODEM_STOP:
		LOG_IN("stopping modem...");
		if (modem_code==IH_FEATURE_MODEM_EM770W
		    || modem_code==IH_FEATURE_MODEM_EM820W
		    || modem_code==IH_FEATURE_MODEM_MU609
		    || modem_code==IH_FEATURE_MODEM_ME209
		    || modem_code==IH_FEATURE_MODEM_LP41
		    || modem_code==IH_FEATURE_MODEM_ME909
		    || modem_code==IH_FEATURE_MODEM_LUA10
		    || modem_code==IH_FEATURE_MODEM_MC7710
		    || modem_code==IH_FEATURE_MODEM_LE910
		    || modem_code==IH_FEATURE_MODEM_MC2716
		    || modem_code==IH_FEATURE_MODEM_MC5635
		    || modem_code==IH_FEATURE_MODEM_EC20
		    || modem_code==IH_FEATURE_MODEM_EC25
		    || modem_code==IH_FEATURE_MODEM_RM500
		    || modem_code==IH_FEATURE_MODEM_RM520N
		    || modem_code==IH_FEATURE_MODEM_RM500U
		    || modem_code==IH_FEATURE_MODEM_RG500
		    || modem_code==IH_FEATURE_MODEM_FM650
		    || modem_code==IH_FEATURE_MODEM_EP06
		    || modem_code==IH_FEATURE_MODEM_U8300
		    || modem_code==IH_FEATURE_MODEM_U8300C
		    || modem_code==IH_FEATURE_MODEM_MC7350
			|| modem_code==IH_FEATURE_MODEM_NL668) {
			//FIXME:send AT cmds through modem port
		}else if (modem_code==IH_FEATURE_MODEM_PHS8
			|| modem_code==IH_FEATURE_MODEM_PLS8
			||modem_code==IH_FEATURE_MODEM_PVS8) {
			send_at_cmd_sync(gl_commport, "AT^SMSO\r\n", NULL, 0, 0);
		}else if(modem_code==IH_FEATURE_MODEM_FG360){
			//FIXME: add code
		}else{
			LOG_WA("unknown modem type!");
		}

		LOG_IN("modem is stopped.");
		break;
	case MODEM_RESET:
		//LOG_IN("resetting modem...");
		if (modem_code==IH_FEATURE_MODEM_EM770W
		    || modem_code==IH_FEATURE_MODEM_EM820W
		    || modem_code==IH_FEATURE_MODEM_MU609
		    || modem_code==IH_FEATURE_MODEM_ME209
		    || modem_code==IH_FEATURE_MODEM_LP41
		    || modem_code==IH_FEATURE_MODEM_ME909
		    || modem_code==IH_FEATURE_MODEM_MC7710
		    || modem_code==IH_FEATURE_MODEM_LE910
		    || modem_code==IH_FEATURE_MODEM_MC2716
		    || modem_code==IH_FEATURE_MODEM_MC5635
		    //|| modem_code==IH_FEATURE_MODEM_U8300
		    || modem_code==IH_FEATURE_MODEM_U8300C
		    || modem_code==IH_FEATURE_MODEM_EC20
		    || modem_code==IH_FEATURE_MODEM_EC25
		    || modem_code==IH_FEATURE_MODEM_RM500
		    || modem_code==IH_FEATURE_MODEM_RM520N
		    || modem_code==IH_FEATURE_MODEM_RM500U
		    || modem_code==IH_FEATURE_MODEM_RG500
		    || modem_code==IH_FEATURE_MODEM_EP06
		    || modem_code==IH_FEATURE_MODEM_MC7350
			|| modem_code==IH_FEATURE_MODEM_NL668) {

		}else if(modem_code==IH_FEATURE_MODEM_U8300){
			//set high, the mrst is a low pluse.
			value = 1;
			gpio(GPIO_WRITE, GPIO_MRST, &value);
			my_usleep(100*1000);
			value = 0;
			gpio(GPIO_WRITE, GPIO_MRST, &value);
			sleep(6);
		}else if(modem_code==IH_FEATURE_MODEM_FM650){
			eval("ifconfig", "sipa_dummy0", "down");
			eval("ifconfig", "pcie0", "down");
			my_sleep(1);
			value = 1;
			gpio(GPIO_WRITE, GPIO_MRST, &value);
			my_sleep(3);
			value = 0;
			gpio(GPIO_WRITE, GPIO_MRST, &value);
			my_sleep(5);
			system("echo 1 > /sys/bus/pci/rcremove");
			system("echo 1 > /sys/bus/pci/rcrescan");
			my_sleep(2); //FIXME: need 15 seconds
		} else if (modem_code==IH_FEATURE_MODEM_PHS8 
			||modem_code==IH_FEATURE_MODEM_PLS8
			||modem_code==IH_FEATURE_MODEM_ELS61
			||modem_code==IH_FEATURE_MODEM_ELS81
			||modem_code==IH_FEATURE_MODEM_ELS31
			||modem_code==IH_FEATURE_MODEM_TOBYL201
			||modem_code==IH_FEATURE_MODEM_LARAR2
			||modem_code==IH_FEATURE_MODEM_PVS8) {
			set_modem(modem_code, MODEM_FORCE_STOP);
			my_sleep(3);
			//open power
			value = 1;
			gpio(GPIO_WRITE, GPIO_MPOWER, &value);
			my_sleep(1);
			//igt
			set_modem(modem_code, MODEM_START);
			starttime = 7;
		}else if(modem_code==IH_FEATURE_MODEM_FG360){
			set_modem(modem_code, MODEM_CFUN_RESET);
		}else {
			set_modem(modem_code, MODEM_STOP);
			my_sleep(5);
			set_modem(modem_code, MODEM_START);
		}
		break;
	case MODEM_FORCE_STOP:
		LOG_IN("force to stop modem...");
		if (modem_code==IH_FEATURE_MODEM_EM770W
		    || modem_code==IH_FEATURE_MODEM_EM820W
		    || modem_code==IH_FEATURE_MODEM_MU609
		    || modem_code==IH_FEATURE_MODEM_ME209
		    || modem_code==IH_FEATURE_MODEM_LP41
		    || modem_code==IH_FEATURE_MODEM_ME909
		    || modem_code==IH_FEATURE_MODEM_LUA10
		    || modem_code==IH_FEATURE_MODEM_MC7710
		    || modem_code==IH_FEATURE_MODEM_LE910
		    || modem_code==IH_FEATURE_MODEM_MC2716
		    || modem_code==IH_FEATURE_MODEM_MC5635
		    || modem_code==IH_FEATURE_MODEM_EC20
		    || modem_code==IH_FEATURE_MODEM_EC25
		    || modem_code==IH_FEATURE_MODEM_RM500
		    || modem_code==IH_FEATURE_MODEM_RM520N
		    || modem_code==IH_FEATURE_MODEM_RM500U
		    || modem_code==IH_FEATURE_MODEM_RG500
		    || modem_code==IH_FEATURE_MODEM_FM650
		    || modem_code==IH_FEATURE_MODEM_EP06
		    || modem_code==IH_FEATURE_MODEM_U8300
		    || modem_code==IH_FEATURE_MODEM_U8300C
		    || modem_code==IH_FEATURE_MODEM_MC7350
			|| modem_code==IH_FEATURE_MODEM_NL668) {

			if(modem_code == IH_FEATURE_MODEM_RM500){
				eval("rmmod", "pcie_mhi.ko");
				my_sleep(3);
				system("echo 0 > /sys/bus/pci/slot_reset");
				my_sleep(2);
			}else if(modem_code == IH_FEATURE_MODEM_RM500U && !ih_license_support(IH_FEATURE_PCIE2ETH_RTL8125B)){
				eval("ifconfig", "sipa_dummy0", "down");
				eval("ifconfig", "pcie0", "down");
				my_sleep(1);
				system("echo 0 > /sys/bus/pci/slot_reset");
				my_sleep(2);
			}
			//TODO: close module firstly
			/*close module power*/
#ifdef INHAND_WR3
			stop_modem(modem_code);
#else
			value = 0;
			gpio(GPIO_WRITE, GPIO_MPOWER, &value);
#endif
		}else if (modem_code==IH_FEATURE_MODEM_PHS8
			|| modem_code==IH_FEATURE_MODEM_PLS8
			|| modem_code==IH_FEATURE_MODEM_ELS61
			|| modem_code==IH_FEATURE_MODEM_ELS81
			|| modem_code==IH_FEATURE_MODEM_ELS31
			|| modem_code==IH_FEATURE_MODEM_TOBYL201
			|| modem_code==IH_FEATURE_MODEM_LARAR2
			|| modem_code==IH_FEATURE_MODEM_PVS8) {
			//TODO: close module firstly
			/*close module power*/
			value = 0;
			gpio(GPIO_WRITE, GPIO_MPOWER, &value);
		}else if(modem_code==IH_FEATURE_MODEM_FG360){
			//FIXME: add code
		}else{
			LOG_WA("unknown modem type!");
		}
		break;
	case MODEM_FORCE_RESET:
		LOG_IN("force to reset modem...");
		if (modem_code==IH_FEATURE_MODEM_EM770W
		    || modem_code==IH_FEATURE_MODEM_EM820W
		    || modem_code==IH_FEATURE_MODEM_MU609
		    || modem_code==IH_FEATURE_MODEM_ME209
		    || modem_code==IH_FEATURE_MODEM_LP41
		    || modem_code==IH_FEATURE_MODEM_ME909
		    || modem_code==IH_FEATURE_MODEM_LUA10
		    || modem_code==IH_FEATURE_MODEM_MC7710
		    || modem_code==IH_FEATURE_MODEM_LE910
		    || modem_code==IH_FEATURE_MODEM_MC2716
		    || modem_code==IH_FEATURE_MODEM_MC5635
		    || modem_code==IH_FEATURE_MODEM_EC20
		    || modem_code==IH_FEATURE_MODEM_EC25
		    || modem_code==IH_FEATURE_MODEM_RM500
		    || modem_code==IH_FEATURE_MODEM_RM520N
		    || modem_code==IH_FEATURE_MODEM_RM500U
		    || modem_code==IH_FEATURE_MODEM_RG500
		    || modem_code==IH_FEATURE_MODEM_EP06
		    || modem_code==IH_FEATURE_MODEM_U8300
		    || modem_code==IH_FEATURE_MODEM_U8300C
		    || modem_code==IH_FEATURE_MODEM_MC7350
			|| modem_code==IH_FEATURE_MODEM_NL668) {
			set_modem(modem_code, MODEM_FORCE_STOP);
			if (modem_code== IH_FEATURE_MODEM_U8300){
				my_sleep(7);
#ifdef INHAND_IDTU9
			}else if (modem_code==IH_FEATURE_MODEM_U8300C){
				my_sleep(7);
#endif
			}else if ((modem_code==IH_FEATURE_MODEM_ME909) && (ih_key_support("TH09"))){
				my_sleep(20);
			}else if (modem_code==IH_FEATURE_MODEM_LE910){
				my_sleep(25);
			}else {
				my_sleep(3);
			}
			set_modem(modem_code, MODEM_START);
			starttime = 7;
		} else if (modem_code==IH_FEATURE_MODEM_PHS8 
			||modem_code==IH_FEATURE_MODEM_PLS8
			||modem_code==IH_FEATURE_MODEM_ELS61
			||modem_code==IH_FEATURE_MODEM_ELS81
			||modem_code==IH_FEATURE_MODEM_ELS31
			||modem_code==IH_FEATURE_MODEM_TOBYL201
			||modem_code==IH_FEATURE_MODEM_LARAR2
			||modem_code==IH_FEATURE_MODEM_PVS8) {
			set_modem(modem_code, MODEM_FORCE_STOP);
			my_sleep(3);
			//open power
			value = 1;
			gpio(GPIO_WRITE, GPIO_MPOWER, &value);
			my_sleep(1);
			//igt
			set_modem(modem_code, MODEM_START);
			starttime = 10;
		}else if(modem_code==IH_FEATURE_MODEM_FM650){
			set_modem(modem_code, MODEM_RESET);
			starttime = 15;
		}else if(modem_code==IH_FEATURE_MODEM_FG360){
			set_modem(modem_code, MODEM_CFUN_RESET);
		}else{
			LOG_WA("unknown modem type!");
			starttime = 3;
		}
		//my_sleep(starttime);
		break;
	case MODEM_CFUN_RESET:
			redial_modem_function_reset(TRUE);
		break;
	case MODEM_RE_REGISTER:
		redial_modem_re_register(TRUE);
		break;
	default:
		LOG_ER("unknown modem action: %d", action);
		return -1;
	}

	return 0;
}

int check_fg360_sim_slot(char *dev, int slot, int verbose)
{
	char cmd[128] = {0};
	char rsp[128] = {0};
	char *p = NULL;
	int target = slot==SIM2 ? 0 : 1;

	if(!dev || !dev[0]){
		return -1;
	}

	send_at_cmd_sync(dev, "AT+GTSIMSWITCH?\r\n", rsp, sizeof(rsp), verbose);

	if((p = strstr(rsp, "sim card switch:"))){
		strsep(&p, ":");
		if(p && atoi(p) != target){
			snprintf(cmd, sizeof(cmd), "AT+GTSIMSWITCH=%d\r\n", target);
			send_at_cmd_sync(dev, cmd, NULL, 0, verbose);
		}
	}

	return ERR_OK;
}

void modem_dualsim_switch(uint64_t modem_code, int idx)
{
        int value;
	
	if(modem_code==IH_FEATURE_MODEM_RM500
		|| modem_code==IH_FEATURE_MODEM_RM520N
		|| modem_code==IH_FEATURE_MODEM_FM650
		|| modem_code==IH_FEATURE_MODEM_RM500U
		|| modem_code==IH_FEATURE_MODEM_RG500){
		gl_modem_cfun_reset_flag = TRUE;
	}else if(modem_code==IH_FEATURE_MODEM_FG360){
		check_fg360_sim_slot(gl_commport, idx, TRUE);
        	set_modem(modem_code, MODEM_CFUN_RESET);
		return;
	}else{
        	set_modem(modem_code, MODEM_FORCE_STOP);
	}

#ifndef INHAND_WR3
	my_sleep(3);
	if(idx == SIM1) value = 1;
	else value = 0;
	gpio(GPIO_WRITE,GPIO_SIM_SWITCH,&value);
	my_sleep(1);
#endif

	if(modem_code==IH_FEATURE_MODEM_RM500
		|| modem_code==IH_FEATURE_MODEM_RM520N
		|| modem_code==IH_FEATURE_MODEM_FM650
		|| modem_code==IH_FEATURE_MODEM_RM500U
		|| modem_code==IH_FEATURE_MODEM_RG500){
		gl_modem_cfun_reset_flag = TRUE;
	}else{
        	set_modem(modem_code, MODEM_START);
	}
	//my_sleep(7);
}
#if 0
int setup_at_cmd()
{
	//read modems.drv according vendorID, productID
	//build some at command according configure, such as select network
	//build at_cmd_lists, maybe can use a fix one
	return 0;
}
#endif

int check_at(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{	
	return ERR_AT_OK;
}

int check_ate0(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{	
	return ERR_AT_OK;
}

int modem_at_init(AT_CMD *pcmd, char *buf, int retry, MODEM_INFO *info)
{
	//close auto report
	//AT^CURC=0
	//init network select
	//AT^SYSCFG?

	return ERR_AT_OK;
}

int check_modem_imei_or_gsn(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p, *q;
	//AT_CMD* pcmd = find_at_cmd(index);

	/*mc8630 format: +GSN:*******
	 *mc39i,em770,u8300 format: *******
	*/
	if (pcmd->resp) {
		p = strstr(buf, pcmd->resp);
		if(p) {
			p += strlen(pcmd->resp);
		} else {
			p = strstr(buf, "+GSN:");
			if(p) {
				p += strlen("+GSN:");
			} else {
				p = buf;
			}
		}
	}else {
		p = buf;
	}

	while(*p) {
		if(*p>='0'&&*p<='z')
			break;
		p++;
	}
	q = p;
	while(*q) {
		if(*q<'0' || *q>'z') {
			*q = 0;
			break;
		}
		q++;
	}
	LOG_IN("modem imei: %s", p);

	strlcpy(info->imei, p, sizeof(info->imei));

	if(gl_modem_code != IH_FEATURE_MODEM_WPD200
		&& gl_modem_code != IH_FEATURE_MODEM_MC2716
		&& gl_modem_code != IH_FEATURE_MODEM_MC5635){
		if (info->imei[0] < '0' || info->imei[0] > '9'){
			bzero(info->imei, sizeof(info->imei));
			return ERR_AT_NAK;
		}
	}

	return ERR_AT_OK;


}

int check_modem_cgmi(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL, *q = NULL;

	if (pcmd->resp) {
		p = strstr(buf, pcmd->resp);
		if(p) p += strlen(pcmd->resp);
		else p = buf;
	}else {
		p = buf;
	}

	while(*p){
		if((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')){
			break;
		}
		p++;
	}

	q = strsep(&p, "\r\n");

	if(info && q){
		strlcpy(info->manufacturer, q, sizeof(info->manufacturer));
	}

	return ERR_AT_OK;
}

int check_modem_cgmm(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL, *q = NULL;

	if (pcmd->resp) {
		p = strstr(buf, pcmd->resp);
		if(p) p += strlen(pcmd->resp);
		else p = buf;
	}else {
		p = buf;
	}

	while(*p){
		if((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')){
			break;
		}
		p++;
	}

	q = strsep(&p, "\r\n");

	if(info && q){
		strlcpy(info->product_model, q, sizeof(info->product_model));
	}

	return ERR_AT_OK;
}

int check_modem_cgmr(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL, *q = NULL;

	if (pcmd->resp) {
		p = strstr(buf, pcmd->resp);
		if(p) p += strlen(pcmd->resp);
		else p = buf;
	}else {
		p = buf;
	}

	while(*p){
		if((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')){
			break;
		}
		p++;
	}

	q = strsep(&p, "\r\n");

	if(info && q){
		strlcpy(info->software_version, q, sizeof(info->software_version));
	}

	return ERR_AT_OK;
}

int check_modem_cgmr2(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL, *q = NULL;
	
	p = strstr(buf, pcmd->resp);
	if(p){
		p += strlen(pcmd->resp);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;
		
		q = strsep(&p, "\r\n");
	}

	if(info && q){
		strlcpy(info->hardware_version, q, sizeof(info->hardware_version));
	}

	return ERR_AT_OK;
}

int check_modem_hwver(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL, *q = NULL;

	if (pcmd->resp) {
		p = strstr(buf, pcmd->resp);
		if(p) p += strlen(pcmd->resp);
		else p = buf;
	}else {
		p = buf;
	}

	while(*p){
		if((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')){
			break;
		}
		p++;
	}

	q = strsep(&p, "\r\n");

	if(info && q){
		strlcpy(info->hardware_version, q, sizeof(info->hardware_version));
	}

	return ERR_AT_OK;
}

int check_modem_basic_info(MODEM_INFO *info)
{
	AT_CMD *pcmd = NULL;
	char buf[256] = {0};
	int ret = -1;

	//Manufacturer
	pcmd = find_at_cmd(AT_CMD_CGMI);
	if(pcmd){
		ret = send_at_cmd_sync(gl_commport, pcmd->atc, buf, sizeof(buf), 0);
		if(ret==0){
			pcmd->at_cmd_handle(pcmd, buf, 0, NULL, info);
		}
	}
	
	//Product model
	pcmd = find_at_cmd(AT_CMD_CGMM);
	if(pcmd){
		ret = send_at_cmd_sync(gl_commport, pcmd->atc, buf, sizeof(buf), 0);
		if(ret==0){
			pcmd->at_cmd_handle(pcmd, buf, 0, NULL, info);
		}
	}

	//Software version
	if(gl_modem_code == IH_FEATURE_MODEM_RM500U){
		pcmd = find_at_cmd(AT_CMD_GMR);
	}else if(gl_modem_code == IH_FEATURE_MODEM_RM500
		|| gl_modem_code == IH_FEATURE_MODEM_RM520N){
		pcmd = find_at_cmd(AT_CMD_QGMR);
	}else{
		pcmd = find_at_cmd(AT_CMD_CGMR);
	}

	if(pcmd){
		ret = send_at_cmd_sync(gl_commport, pcmd->atc, buf, sizeof(buf), 0);
		if(ret==0){
			pcmd->at_cmd_handle(pcmd, buf, 0, NULL, info);
		}
	}

	//Hardware version
	if(gl_modem_code == IH_FEATURE_MODEM_RM500U){
		pcmd = find_at_cmd(AT_CMD_CGMR2);
	}else{
		pcmd = find_at_cmd(AT_CMD_HWVER);
	}

	if(pcmd){
		ret = send_at_cmd_sync3(gl_commport, pcmd->atc, buf, sizeof(buf), 0);
		if(ret==0){
			pcmd->at_cmd_handle(pcmd, buf, 0, NULL, info);
		}
	}
	
	return ERR_AT_OK;
}

int check_modem_imei(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p, *q;
	//AT_CMD* pcmd = find_at_cmd(index);

	/*mc8630 format: +GSN:*******
	 *mc39i,em770,u8300 format: *******
	*/
	if (pcmd->resp) {
		p = strstr(buf, pcmd->resp);
		if(p) p += strlen(pcmd->resp);
		else p = buf;
	}else {
		p = buf;
	}

	while(*p) {
		if(*p>='0'&&*p<='z')
			break;
		p++;
	}
	q = p;
	while(*q) {
		if(*q<'0' || *q>'z') {
			*q = 0;
			break;
		}
		q++;
	}
	LOG_DB("modem imei: %s", p);

	strlcpy(info->imei, p, sizeof(info->imei));

	if(gl_modem_code != IH_FEATURE_MODEM_WPD200
		&& gl_modem_code != IH_FEATURE_MODEM_MC2716
		&& gl_modem_code != IH_FEATURE_MODEM_MC5635){
		if (info->imei[0] < '0' || info->imei[0] > '9'){
			bzero(info->imei, sizeof(info->imei));
			return ERR_AT_NAK;
		}
	}

	return ERR_AT_OK;
}

/*
 * select 2G/3G, band
 */
int select_modem_network(AT_CMD *pcmd, char *buf, int retry, MODEM_INFO *info)
{
	//FIXME: should check current status firstly
	return ERR_AT_OK;
}

static int enter_pin_code(MODEM_CONFIG *config, MODEM_INFO *info)
{
	int retval = ERR_AT_NOSIM;
	char pin_cmd[20] = {0};
	int pin_idx;

	if(info->current_sim == SIM1) pin_idx = 0;
	else pin_idx = 1;
	if(!config->pincode[pin_idx][0]){
		LOG_IN("No pincode to SIM%d",info->current_sim);
	}else{			
		if (gl_modem_code == IH_FEATURE_MODEM_TOBYL201
				|| gl_modem_code == IH_FEATURE_MODEM_FG360
				|| gl_modem_code == IH_FEATURE_MODEM_ELS81
				|| gl_modem_code == IH_FEATURE_MODEM_LARAR2) {
			sprintf(pin_cmd,"AT+CPIN=\"%s\"\r\n",config->pincode[pin_idx]);
		} else {
			sprintf(pin_cmd,"AT+CPIN=%s\r\n",config->pincode[pin_idx]);
		}

		if(send_at_cmd_sync2(pin_cmd, NULL, 0, 0) == 0) {
			info->simstatus = SIM_READY;
			retval = ERR_AT_OK;
		}
	}

	return retval;
}

static int process_unlock_modem(void)
{
	int ret = -1;
	AT_CMD* pcmd = NULL;
	unsigned int modem_code = ih_license_get_modem();

	switch (modem_code) {
		case IH_FEATURE_MODEM_EC20:
		case IH_FEATURE_MODEM_EC25:
		case IH_FEATURE_MODEM_RM500:
		case IH_FEATURE_MODEM_RM520N:
		case IH_FEATURE_MODEM_RG500:
		case IH_FEATURE_MODEM_EP06:
			pcmd = find_at_cmd(AT_CMD_CLCK);
			break;
		default:
			return 0;
	}

	if (!pcmd || !pcmd->atc) {
		syslog(LOG_ERR, "Can't find unlock cmd");
		return -2;
	}

	ret = send_at_cmd_sync2(pcmd->atc, NULL, 0, 1);
	if (ret < 0) {
		syslog(LOG_ERR, "Failed to send cmd:%s...", pcmd->atc);
		return -1;
	}

	return 0;
}

//need to update this table in future
AT_CMD* get_modem_imei_cmd(void)
{
	int i = 0;
	AT_CMD *pcmd = NULL;

	if(!gl_at_lists){
		return NULL;
	}

	while(gl_at_lists[i]!=-1){
		if(gl_at_lists[i] == AT_CMD_GSN
			|| gl_at_lists[i] == AT_CMD_ATI
			|| gl_at_lists[i] == AT_CMD_CGSN){
			pcmd = find_at_cmd(gl_at_lists[i]);
			break;
		}
		i++;
	}
	
	return pcmd;
}

//need to update this table in future
AT_CMD* get_modem_signal_cmd(void)
{
	int i = 0;
	AT_CMD *pcmd = NULL;

	if(!gl_at_lists){
		return NULL;
	}

	while(gl_at_lists[i]!=-1){
		if(gl_at_lists[i] == AT_CMD_CSQ
			|| gl_at_lists[i] == AT_CMD_CSQ1
			|| gl_at_lists[i] == AT_CMD_CCSQ
			|| gl_at_lists[i] == AT_CMD_CESQ
			|| gl_at_lists[i] == AT_CMD_VCSQ
			|| gl_at_lists[i] == AT_CMD_WCSQ
			|| gl_at_lists[i] == AT_CMD_HSTATE
			|| gl_at_lists[i] == AT_CMD_GTCCINFO
			|| gl_at_lists[i] == AT_CMD_QENG_SRVCELL){
			pcmd = find_at_cmd(gl_at_lists[i]);
			break;
		}
		i++;
	}
	
	return pcmd;
}

//need to update this table in future
AT_CMD* get_sim_iccid_cmd(void)
{
	int i = 0;
	AT_CMD *pcmd = NULL;

	if(!gl_at_lists){
		return NULL;
	}

	while(gl_at_lists[i]!=-1){
		if(gl_at_lists[i] == AT_CMD_LICCID
			|| gl_at_lists[i] == AT_CMD_HICCID
			|| gl_at_lists[i] == AT_CMD_SICCID
			|| gl_at_lists[i] == AT_CMD_ICCID
			|| gl_at_lists[i] == AT_CMD_CCID){
			pcmd = find_at_cmd(gl_at_lists[i]);
			break;
		}
		i++;
	}
	
	return pcmd;
}

int check_modem_sim(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config,  MODEM_INFO *info)
{
	char *pos,*p;
	int retval = ERR_AT_NOSIM;
	int pincount;
	char rsp[128];
	int ret = -1;
	AT_CMD *iccid_cmd = NULL;

	pos = strstr(buf, "READY");
	if(pos) {
		info->simstatus = SIM_READY;
		retval = ERR_AT_OK;
	}else {
		if(pcmd->index==AT_CMD_SCPIN) {
			//em770 format: ^CPIN: SIM PIN,3,10,3(pin time),10,3
			/*check if need pin code*/
			pos = strstr(buf, "SIM PIN");
			if(pos){//found SIM PIN
				info->simstatus = SIM_PIN;
				strsep(&pos, ",");
				strsep(&pos, ",");
				strsep(&pos, ",");
				p = pos;
				pincount = atoi(p);
				LOG_DB("pincount %d", pincount);
				if(pincount>1 && pincount<=3) {
					LOG_IN("Enter PIN code(%d left)...", pincount);
					retval = enter_pin_code(config, info);
				}else {
					LOG_IN("PIN code count=1, Donot try!");
				}			
			}
		} else if(pcmd->index==AT_CMD_WCPIN){
			pos = strstr(buf, "RUIMS:");
			if(pos) {
			}else{

			}
		}else {
			/*check if need pin code*/
			pos = strstr(buf, "SIM PIN");
			if(pos){//found SIM PIN
				info->simstatus = SIM_PIN;
				if(gl_modem_code == IH_FEATURE_MODEM_FM650
					|| gl_modem_code == IH_FEATURE_MODEM_NL668){
					if(send_at_cmd_sync2("AT+CPINR=\"SIM*\"\r\n", rsp, sizeof(rsp), 1) == 0) {
						p = strstr(rsp, "SIM PIN,");
						if(p) {
							pos = strsep(&p, ",");
							pos = strsep(&p, ",");
							pincount = atoi(pos);
							LOG_DB("pincount %d", pincount);
							if(pincount>1 && pincount<=3) {
								LOG_IN("Enter PIN code(%d left)...", pincount);
								retval = enter_pin_code(config, info);
							}else {
								LOG_IN("PIN code count=1, Donot try!");
							}			
						}
					}
					goto leave;
				}else if(gl_modem_code == IH_FEATURE_MODEM_FG360){
					if(send_at_cmd_sync2("AT+EPINC?\r\n", rsp, sizeof(rsp), 1) == 0) {
						p = strstr(rsp, "+EPINC:");
						if(p) {
							p += 7;
							pincount = atoi(p);
							LOG_DB("pincount %d", pincount);
							if(pincount>1 && pincount<=3) {
								LOG_IN("Enter PIN code(%d left)...", pincount);
								retval = enter_pin_code(config, info);
							}else {
								LOG_IN("PIN code count=1, Donot try!");
							}			
						}
					}
					goto leave;
				}

				if(send_at_cmd_sync2("AT^SPIC\r\n", rsp, sizeof(rsp), 1) == 0) {
					//phs8 format: ^SPIC: 3
					p = strstr(rsp, "SPIC:");
					if(p) {
						p += 6;
						pincount = atoi(p);
						LOG_DB("pincount %d", pincount);
						if(pincount>1 && pincount<=3) {
							LOG_IN("Enter PIN code(%d left)...", pincount);
							retval = enter_pin_code(config, info);
						}else {
							LOG_IN("PIN code count=1, Donot try!");
						}			
					}
				}else if(send_at_cmd_sync2("AT+CPNNUM\r\n", rsp, sizeof(rsp), 1) == 0) {
					//u8300 format: pin1=1;puk1=1;pin2=2;puk2=2
					if(info->current_sim == SIM1){
						p = strstr(rsp, "PIN1=");
					}else {
						p = strstr(rsp, "PIN2=");
					}
					if(p) {
						p += 5;
						pincount = atoi(p);
						LOG_DB("pincount %d", pincount);
						if(pincount>1 && pincount<=3) {
							LOG_IN("Enter PIN code(%d left)...", pincount);
							retval = enter_pin_code(config, info);
						}else {
							LOG_IN("PIN code count=1, Donot try!");
						}			
					}
				} else if (send_at_cmd_sync2("AT+UPINCNT\r\n", rsp, sizeof(rsp), 1) == 0) {
					p = strstr(rsp, "+UPINCNT:");
					if(p) {
						p += 9;
						while(*pos != 0 && *pos==' ') pos++; 
						pincount = atoi(p);
						LOG_DB("pincount %d", pincount);
						if(pincount>1 && pincount<=3) {
							LOG_IN("Enter PIN code(%d left)...", pincount);
							retval = enter_pin_code(config, info);
						}else {
							LOG_IN("PIN code count=1, Donot try!");
						}			
					}

				} else if (send_at_cmd_sync2("AT+QPINC?\r\n", rsp, sizeof(rsp), 1) == 0) {
					p = strstr(rsp, "SC");
					if (p) {
						p = strchr(p, ',');
						if (p) {
							p++;
							pincount = atoi(p);
							LOG_DB("pincount %d", pincount);
							if(pincount>1 && pincount<=3) {
								LOG_IN("Enter PIN code(%d left)...", pincount);
								retval = enter_pin_code(config, info);
							}else {
								LOG_IN("PIN code count=1, Donot try!");
							}
						}
						
					} 

				}
			} else if (strstr(buf, "PH-NETSUB PIN")) {
				info->simstatus = SIM_PIN;
				ret = process_unlock_modem();
				if(ret<0) {
					return 0;
				}

			}
		}
	}

leave:
	if(retval == ERR_AT_OK){
		//get sim iccid
		iccid_cmd = get_sim_iccid_cmd();
		if(iccid_cmd && iccid_cmd->at_cmd_handle){
			send_at_cmd_sync(gl_commport, iccid_cmd->atc, rsp, sizeof(rsp), 0);
			iccid_cmd->at_cmd_handle(pcmd, rsp, 0, config, info);
		}
	}
	
	return retval;
}

int check_modem_imsi(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p, *q;
	int cnt = 0; 
	/*parse imsi*/
	p=buf;
	while(*p) {
		if(*p>='0' && *p<='9')
			break;
		p++;
	}
	q = p;
	while(*q) {
		if(*q<'0' || *q>'9') {
			*q = 0;
			break;
		}
		q++;
		cnt++;
	}
	LOG_IN("modem imsi: %s", p);
	strlcpy(info->imsi, p, sizeof(info->imsi));

	if (cnt < 10) {
		  return ERR_AT_NAK;
	}else {
		  return ERR_AT_OK;
	}
}

int check_modem_phonenum(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p, number[32];
	int ret;

	p = strstr(buf, pcmd->resp);
	if(p) {
		/*em820w -- +CNUM: "","14530034258",129*/
		/*phs8   -- +CNUM: ,"14530034258",161*/
		p += strlen(pcmd->resp);
		/*if have ' ', eat it.*/
		//while(*p!=0 && *p==' ') p++;

		ret = sscanf(p, "%*[^,],\"%[^\"]", number);		
		if(ret>0) {
			LOG_DB("phone number: %s", number);
			strlcpy(info->phonenum, number, sizeof(info->phonenum));
		} else {
			info->phonenum[0] = '-';
		}
	} else {
		info->phonenum[0] = '\0';
	}

	return ERR_AT_OK;
}


int check_modem_vphonenum(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p=NULL, *number=NULL;

	p = strstr(buf, pcmd->resp);
	if(p!=NULL){
		p+=4;
		if(*p =='0' || !*p){
			LOG_DB("PVS8 module is inactive");
			return -1;
		}else {
			number = strsep(&p, "@");
			if(number){
				LOG_DB("Phone Number is %s", number);
				strlcpy(info->phonenum, number, sizeof(info->phonenum));
			}else {
				LOG_DB("GET Phone Number err, %s", number);
				return -1;
			}

		}
	}else {
		syslog(LOG_INFO,"PVS8 get phone number err");
		return -1;
	}

	return ERR_AT_OK;
}

#define AT_U8300C_NET_OFFSET2 		6
extern int *gl_at_lists;
extern int gl_at_index;
extern MODEM_CONFIG *get_modem_config(void);
void fixup_u8300c_at_list1(void)
{
	MODEM_CONFIG *modem_config = get_modem_config();

	switch(modem_config->network_type) {
	case MODEM_NETWORK_4G:
		gl_lte_u8300c_at_lists_const[AT_U8300C_NET_OFFSET2] = AT_CMD_4G;
		gl_lte_u8300c_at_lists[AT_U8300C_NET_OFFSET2] = AT_CMD_4G;
		break;
	case MODEM_NETWORK_3G:
		gl_lte_u8300c_at_lists_const[AT_U8300C_NET_OFFSET2] = AT_CMD_3G;
		gl_lte_u8300c_at_lists[AT_U8300C_NET_OFFSET2] = AT_CMD_3G;
		break;
	case MODEM_NETWORK_EVDO:
		gl_lte_u8300c_at_lists_const[AT_U8300C_NET_OFFSET2] = AT_CMD_EVDO;
		gl_lte_u8300c_at_lists[AT_U8300C_NET_OFFSET2] = AT_CMD_EVDO;
		break;
	case MODEM_NETWORK_2G:
		gl_lte_u8300c_at_lists_const[AT_U8300C_NET_OFFSET2] = AT_CMD_2G;
		gl_lte_u8300c_at_lists[AT_U8300C_NET_OFFSET2] = AT_CMD_2G;
		break;
	case MODEM_NETWORK_AUTO:
	default:
		gl_lte_u8300c_at_lists_const[AT_U8300C_NET_OFFSET2] = AT_CMD_AUTO;
		gl_lte_u8300c_at_lists[AT_U8300C_NET_OFFSET2] = AT_CMD_AUTO;
		break;
	}
	
	memcpy(gl_at_lists, gl_lte_u8300c_at_lists_const, sizeof(gl_lte_u8300c_at_lists_const));//reset at lists
	return;
}

#define AT_U8300C_PSRAT_POS 		7
#define AT_U8300C_LTE_REG2_OFFSET	4
#define AT_U8300C_LTE_REG_OFFSET	5
static void fixup_u8300c_at_list2(int cdma, MODEM_INFO *info)
{
	if (cdma) {
		memcpy(&(gl_at_lists[AT_U8300C_PSRAT_POS + 1]), u8300c_cdma_at_lists, 
				sizeof(u8300c_cdma_at_lists));
	}else{
		memcpy(&(gl_at_lists[AT_U8300C_PSRAT_POS +1]), u8300c_lte_at_lists, 
				sizeof(u8300c_lte_at_lists));
	}

	switch (info->network) {
		case CUR_NETWORK_4G:
			u8300c_lte_at_lists[AT_U8300C_LTE_REG2_OFFSET] = AT_CMD_CEREG2;
			u8300c_lte_at_lists[AT_U8300C_LTE_REG_OFFSET] = AT_CMD_CEREG;
			break;
		default:
			u8300c_lte_at_lists[AT_U8300C_LTE_REG2_OFFSET] = AT_CMD_CREG2;
			u8300c_lte_at_lists[AT_U8300C_LTE_REG_OFFSET] = AT_CMD_CREG;
			break;
	}

	return ;
}

int check_sim_operator(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p;
	int retval=ERR_AT_NOSIM;

	p = strstr(buf, pcmd->resp);
	if(p) {
		p += strlen(pcmd->resp);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;
		retval = ERR_AT_OK;
	} else {
		LOG_IN("not found ICCID.");
	}

	return retval;
}

/*
 *check_modem_signal
 *return: 0 		-- ok
 *	  ERR_AT_NOSIGNAL	-- error or bad signal
 */
int check_modem_signal(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p;
	int retval=ERR_AT_NOSIGNAL, sig;
	int sim_idx;
	int rxlev = 99;
	int ber = 99;
	int rscp = 255;
	int ecio = 255;
	int rsrp = 255;
	int rsrq = 255;

	SRVCELL_INFO cell = {0};
	
	sim_idx = info->current_sim - 1;
	p = strstr(buf, pcmd->resp);
	if(p) {
		p += strlen(pcmd->resp);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;
		//q = strsep(&p, "\r\n ");
		if (pcmd->index==AT_CMD_CESQ) {
			sscanf(p, "%d,%d,%d,%d,%d,%d\r\n", &rxlev, &ber, &rscp, &ecio, &rsrq, &rsrp);
			if(rxlev != 99){
				STR_FORMAT(cell.rssi, "%d", 2 * rxlev - 110);
 			}

			if(ber != 99){
				STR_FORMAT(cell.ber, "%d", ber);
 			}

			if(rscp != 255){
				STR_FORMAT(cell.rscp, "%d", rscp - 120);
			}

			if(ecio != 255){
				STR_FORMAT(cell.ecio, "%0.1f", 0.5f*ecio - 24.0f);
			}

			if(rsrq != 255){
				STR_FORMAT(cell.rsrq, "%0.1f", 0.5f*rsrq - 20.0f);
			}

			if(rsrp != 255){
				STR_FORMAT(cell.rsrp, "%d", rsrp - 140);
			}
			
			update_srvcell_info(&cell, BIT_RSSI|BIT_RSCP|BIT_ECIO|BIT_RSRQ|BIT_RSRP);
		} else {
			sig = atoi(p);
			if(sig < 100){ //RSSI
				if(sig != 99){
					rxlev = ((2 * sig) - 113) + RSSI2RXLEV_SCALE; //range: -110 ~ -48 dBm
					STR_FORMAT(cell.rssi, "%d", LIMIT(rxlev, -110, -48));
				}
				update_srvcell_info(&cell, BIT_RSSI);
			}else{ //RSCP
				if(sig != 199){
					rscp = sig - 216; //range: -115 ~ -25 dBm
					STR_FORMAT(cell.rscp, "%d", LIMIT(rscp, -115, -25));
				}
				update_srvcell_info(&cell, BIT_RSCP);
			}

		}
		sig = info->siglevel;
		/*if retry max but has low signal, pass it*/
		if(sig>0) {
			//LOG_IN("modem signal level: %d", sig);
			//FIXME: signal level
			if((config->dualsim)&&(config->csq[sim_idx])){
				if(sig<config->csq[sim_idx]) retval = ERR_AT_LOWSIGNAL;
				else retval = ERR_AT_OK;
			}else{
				if(sig >= 10) retval = ERR_AT_OK;
				//else if(retry >= pcmd->max_retry) retval = ERR_AT_OK;
				else if(retry >= 5) retval = ERR_AT_OK;
				else retval = ERR_AT_LOWSIGNAL;
			}
		} else {
			if(retry>=pcmd->max_retry) {
				LOG_IN("modem has no signal, check the antenna!");
#if 0
				if(gl_modem_code == IH_FEATURE_MODEM_U8300){
					set_modem(gl_modem_code, MODEM_RESET);
					LOG_IN("U8300 moduel has no signal, reset modeul!");
				}
#endif
				retval = ERR_AT_NOSIGNAL;
			}
		}
	} else {
		LOG_IN("not found CSQ.");
	}

	return retval;
}

void update_sig_leds(int sig)
{
#if !(defined INHAND_IR8 || defined INHAND_WR3)
	if(((sig>0) && (sig <10))
		|| ((sig>100) && (sig <120))){
		ledcon(LEDMAN_CMD_ON, LED_LEVEL0);
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
	} else if(((sig>=10) && (sig<20))
		|| ((sig>=120) && (sig <140))){
#ifdef LED_MULTI_COLOR
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
		ledcon(LEDMAN_CMD_ON, LED_LEVEL1);
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#else
		ledcon(LEDMAN_CMD_ON, LED_LEVEL0);
		ledcon(LEDMAN_CMD_ON, LED_LEVEL1);
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#endif
	} else if(sig>=20 || sig>=140){
#ifdef LED_MULTI_COLOR
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
		ledcon(LEDMAN_CMD_ON, LED_LEVEL2);
#else
		ledcon(LEDMAN_CMD_ON, LED_LEVEL0);
		ledcon(LEDMAN_CMD_ON, LED_LEVEL1);
		ledcon(LEDMAN_CMD_ON, LED_LEVEL2);
#endif
	} else {
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
		ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
	}
#endif
}

void update_sig_leds2(int sigbar)
{
	switch(sigbar){
		case SIGNAL_BAR_GREAT:
#ifdef LED_MULTI_COLOR
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
			ledcon(LEDMAN_CMD_ON, LED_LEVEL2);
#else
			ledcon(LEDMAN_CMD_ON, LED_LEVEL0);
			ledcon(LEDMAN_CMD_ON, LED_LEVEL1);
			ledcon(LEDMAN_CMD_ON, LED_LEVEL2);
#endif
			break;
		case SIGNAL_BAR_GOOD:
#ifdef LED_MULTI_COLOR
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
			ledcon(LEDMAN_CMD_ON, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#else
			ledcon(LEDMAN_CMD_ON, LED_LEVEL0);
			ledcon(LEDMAN_CMD_ON, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
#endif
			break;
		case SIGNAL_BAR_MODERATE:
		case SIGNAL_BAR_POOR:
			ledcon(LEDMAN_CMD_ON, LED_LEVEL0);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
			break;
		case SIGNAL_BAR_NONE:
		default:
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
			break;
	}
}
/*
 *check_modem_qeng_servingcell
 *return: 0 			-- ok
 *	  ERR_AT_NOSIGNAL	-- error or bad signal
 */
int check_modem_qeng_servingcell(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL, *p2 = NULL, *p3 = NULL;
	int retval=ERR_AT_NOSIGNAL;
	int sim_idx=0;
	int rat, ret;
	int rat_tmp = -1;
	char duplex[8] = {0};
	char arfcn[8] = {0};
	char arfcn_5g[8] = {0};
	char band[8] = {0};
	char band_5g[8] = {0};

	int mask = 0;
	SRVCELL_INFO cell = {0};
	char *match_str[] = {"NR5G-NSA", "NR5G-SA", "LTE", "WCDMA"};

	if(!pcmd || !buf || !config || !info){
		LOG_ER("%s, invalid parameters!!", __func__);
		return ERR_AT_NOSIGNAL;
	}
	
	sim_idx=info->current_sim-1;

	p = strstr(buf, pcmd->resp);
	if(p) {
		for(rat = 0; rat < ARRAY_COUNT(match_str); rat++){
			if((p2 = strstr(p, match_str[rat])) != NULL){
				p2 += strlen(match_str[rat]);
				if(strcmp(match_str[rat], "NR5G-NSA") == 0){
					p3 = p2;
					//search 'LTE'
					rat_tmp = rat;
					continue;
				}else{
					break;
				}
			}
		}

		rat_tmp = rat_tmp < 0 ? rat : rat_tmp;
		switch(rat_tmp){
			case 0: //NR5G-NSA
				info->network = 4;
				STR_FORMAT(cell.rat, "NSA");
				mask = MASK_ALL;
				//get lte info
				ret = sscanf(p2, "\",\"%[^\"]\",%[^,],%[^,],%[^,],%*[^,],%[^,],%[^,],%*[^,],%*[^,],%[^,],%[^,],%[^,],%[^,],%[^,],", 
						duplex, cell.mcc, cell.mnc, cell.cellid, arfcn, band, cell.lac, cell.rsrp, cell.rsrq, cell.rssi, cell.sinr);
				if((ret > 0) && ppp_debug()){
					LOG_DB("NR5G-NSA => LTE %s mcc:%s mnc:%s cellid:%s lac:%s rsrp:%s rsrq:%s sinr:%s rssi:%s earfcn:%s band:%s", 
						duplex, cell.mcc, cell.mnc, cell.cellid, cell.lac, cell.rsrp, cell.rsrq, cell.sinr, cell.rssi, arfcn, band);
				}

				//get 5gnr info
				ret = sscanf(p3, "\",%[^,],%[^,],%*[^,],%[^,],%[^,],%[^,\r\n],%[^,],%[^,],", 
						cell.mcc, cell.mnc, cell.ss_rsrp, cell.ss_sinr, cell.ss_rsrq, arfcn_5g, band_5g);
				if((ret > 0) && ppp_debug()){
					LOG_DB("NR5G-NSA mcc:%s mnc:%s ss_rsrp:%s ss_rsrq:%s ss_sinr:%s arfcn:%s band:%s", 
							cell.mcc, cell.mnc, cell.ss_rsrp, cell.ss_rsrq, cell.ss_sinr, arfcn_5g, band_5g);
					if(atoi(cell.ss_rsrp) < -156){
						LOG_DB("This may not be a real 5G signal!");
					}
				}
			
				if(band[0]){
					STR_FORMAT(cell.band, "B%s", band);
				}

				if(band_5g[0] && band[0]){
					STRCAT_FORMAT(cell.band, "+n%s", band_5g);
				}
				
				if(arfcn[0] && arfcn_5g[0]){
					STRCAT_FORMAT(cell.arfcn, "lte:%s nr:%s", arfcn, arfcn_5g);
				}
				break;
			case 1: //NR5G-SA
				info->network = 4;
				STR_FORMAT(cell.rat, "SA");
				mask = MASK_ALL;
				ret = sscanf(p2, "\",\"%[^\"]\", %[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%[^,],%[^,],%[^,],", 
						duplex, cell.mcc, cell.mnc, cell.cellid, cell.pci, cell.lac, cell.arfcn, band_5g, cell.ss_rsrp, cell.ss_rsrq, cell.ss_sinr);
				if((ret > 0) && ppp_debug()){
					LOG_DB("NR5G-SA %s mcc:%s mnc:%s cellid:%s lac:%s ss_rsrp:%s ss_rsrq:%s ss_sinr:%s pci:%s arfcn:%s band:%s", 
						duplex, cell.mcc, cell.mnc, cell.cellid, cell.lac, cell.ss_rsrp, cell.ss_rsrq, cell.ss_sinr, cell.pci, cell.arfcn, band_5g);
				}

				if(band_5g[0]){
					STR_FORMAT(cell.band, "n%s", band_5g);
				}
				break;
			case 2: //LTE
				info->network = 3;
				STR_FORMAT(cell.rat, "LTE");
				mask = BITS_UNSET(MASK_ALL, BIT_RSSI);
				ret = sscanf(p2, "\",\"%[^\"]\",%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%[^,],%[^,],%[^,],%[^,],%[^,],", 
						duplex, cell.mcc, cell.mnc, cell.cellid, cell.pci, cell.arfcn, band, cell.lac, cell.rsrp, cell.rsrq, cell.rssi, cell.sinr);
				if((ret > 0) && ppp_debug()){
					LOG_DB("LTE %s mcc:%s mnc:%s cellid:%s lac:%s rsrp:%s rsrq:%s sinr:%s rssi:%s pci:%s earfcn:%s band:%s", 
						duplex, cell.mcc, cell.mnc, cell.cellid, cell.lac, cell.rsrp, cell.rsrq, cell.sinr, cell.rssi, cell.pci, cell.arfcn, band);
				}
	
				if(band[0]){
					STR_FORMAT(cell.band, "B%s", band);
				}
				break;
			case 3: //WCDMA
				info->network = 2;
				STR_FORMAT(cell.rat, "WCDMA");
				mask = BITS_UNSET(MASK_ALL, BIT_RSSI);
				ret = sscanf(p2, "\",%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%[^,],%[^,],", 
						cell.mcc, cell.mnc, cell.lac, cell.cellid, cell.arfcn, cell.rscp, cell.ecio);
				if((ret > 0) && ppp_debug()){
					LOG_DB("WCDMA mcc:%s mnc:%s cellid:%s lac:%s uarfcn:%s rscp:%s ecio:%s", 
						cell.mcc, cell.mnc, cell.cellid, cell.lac, cell.arfcn, cell.rscp, cell.ecio);
				}
				break;
			default:
				break;
		}
	}
	
	update_srvcell_info(&cell, mask);

	if(info->siglevel > 0){
		if((config->dualsim)&&(config->csq[sim_idx])){
			if(info->siglevel<config->csq[sim_idx]) retval = ERR_AT_LOWSIGNAL;
			else retval = ERR_AT_OK;
		}else{
			if(info->siglevel >= 10) retval = ERR_AT_OK;
			else if(retry >= 5) retval = ERR_AT_OK;
			else retval = ERR_AT_LOWSIGNAL;
		}
	}else{
		if(retry >= pcmd->max_retry){
			retval = ERR_AT_NOSIGNAL;
		}
	}

	if(ppp_debug()){
		LOG_DB("modem siglevel: %d", info->siglevel);
	}

	return retval;
}

/*
 *check_modem_qeng_servingcell for Quectel 4G modems
 *return: 0 			-- ok
 *	  ERR_AT_NOSIGNAL	-- error or bad signal
 */
int check_modem_qeng_servingcell2(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL, *q = NULL, *pos = NULL;
	int rat, ret;
	char band[8] = {0};
	char duplex[8] = {0};
	int mask = 0;
	
	SRVCELL_INFO cell = {0};
	char *match_str[] = {"LTE", "WCDMA","TDSCDMA","HDR","CDMA","GSM"};

	if(!pcmd || !buf || !config || !info){
		LOG_ER("%s, invalid parameters!!", __func__);
		return ERR_AT_NOSIGNAL;
	}

	p = buf;
	while((q = strstr(p, pcmd->resp))){
		STR_SKIP(pos, q, ",", 3, leave);
		p = q;

		memset(&cell, 0, sizeof(SRVCELL_INFO));

		for(rat = 0; rat < ARRAY_COUNT(match_str); rat++){
			if(strstr(pos, match_str[rat])){
				STR_FORMAT(cell.rat, "%s", match_str[rat]);
				break;
			}
		}

		switch(rat){
			case 0: //LTE
				info->network = 3;
				mask = BITS_UNSET(MASK_ALL, BIT_RSSI);
				ret = sscanf(q, "\"%[^\"]\",%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%[^,],%[^,],%[^,],%[^,],%[^,],", 
						duplex, cell.mcc, cell.mnc, cell.cellid, cell.pci, cell.arfcn, band, cell.lac, cell.rsrp, cell.rsrq, cell.rssi, cell.sinr);
				if(ret > 0 && ppp_debug()){
					LOG_DB("LTE %s mcc:%s mnc:%s cellid:%s pci:%s earfcn:%s band:%s lac:%s rsrp:%s rsrq:%s sinr:%s rssi:%s", 
						duplex, cell.mcc, cell.mnc, cell.cellid, cell.pci, cell.arfcn, band, cell.lac, cell.rsrp, cell.rsrq, cell.sinr, cell.rssi);
				}
	
				if(band[0]){
					STR_FORMAT(cell.band, "B%s", band);
				}
				break;
			case 1: //WCDMA
				info->network = 2;
				mask = BITS_UNSET(MASK_ALL, BIT_RSSI);
				ret = sscanf(q, "%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%[^,],%[^,],", 
						cell.mcc, cell.mnc, cell.lac, cell.cellid, cell.arfcn, cell.rscp, cell.ecio);
				if(ret > 0 && ppp_debug()){
					LOG_DB("WCDMA mcc:%s mnc:%s lac:%s cellid:%s uarfcn:%s rscp:%s ecio:%s", 
						cell.mcc, cell.mnc, cell.lac, cell.cellid, cell.arfcn, cell.rscp, cell.ecio);
				}
				break;
			case 2: //TDSCDMA
				info->network = 2;
				mask = MASK_ALL;
				ret = sscanf(q, "%[^,],%[^,],%[^,],%[^,],%*[^,],%[^,],%[^,],%[^,\r\n]\r\n", 
						cell.mcc, cell.mnc, cell.lac, cell.cellid, cell.rssi, cell.rscp, cell.ecio);
				if(ret > 0 && ppp_debug()){
					LOG_DB("TDSCDMA mcc:%s mnc:%s lac:%s cellid:%s rssi:%s rscp:%s ecio:%s", 
						cell.mcc, cell.mnc, cell.lac, cell.cellid, cell.rssi, cell.rscp, cell.ecio);
				}
				break;
			case 3: //HDR
				info->network = 2;
				mask = BITS_UNSET(MASK_ALL, BIT_RSSI);
				ret = sscanf(q, "%[^,],%[^,],%[^,],%[^,],%*[^,],", 
						cell.mcc, cell.mnc, cell.lac, cell.cellid);
				if(ret > 0 && ppp_debug()){
					LOG_DB("HDR mcc:%s mnc:%s lac:%s cellid:%s", 
						cell.mcc, cell.mnc, cell.lac, cell.cellid);
				}
				break;
			case 4: //CDMA
				info->network = 1;
				mask = BITS_UNSET(MASK_ALL, BIT_RSSI);
				ret = sscanf(q, "%[^,],%[^,],%[^,],%[^,],%*[^,],", 
						cell.mcc, cell.mnc, cell.lac, cell.cellid);
				if(ret > 0 && ppp_debug()){
					LOG_DB("CDMA mcc:%s mnc:%s lac:%s cellid:%s", 
						cell.mcc, cell.mnc, cell.lac, cell.cellid);
				}
				break;
			case 5: //GSM
				info->network = 1;
				mask = MASK_ALL;
				ret = sscanf(q, "%[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%[^,],%[^,],", 
						cell.mcc, cell.mnc, cell.lac, cell.cellid, cell.band, cell.rssi);

				if(cell.band[0] == '0'){
					snprintf(cell.band, sizeof(cell.band), "1800");
				}else if(cell.band[0] == '1'){
					snprintf(cell.band, sizeof(cell.band), "1900");
				}else{
					bzero(cell.band, sizeof(cell.band));
				}
				
				if(ret > 0 && ppp_debug()){
					LOG_DB("GSM mcc:%s mnc:%s lac:%s cellid:%s band:%s rssi:%s", 
						cell.mcc, cell.mnc, cell.lac, cell.cellid, cell.band, cell.rssi);
				}
				break;
			default:
				break;
		}
	}

leave:
	update_srvcell_info(&cell, mask);

	return ERR_AT_OK;
}

int check_modem_gtccinfo(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL, *q = NULL, *q2 = NULL;
	int retval = ERR_AT_NOSIGNAL;
	int sim_idx=0;
	int is_en_dc = 0;
	int is_srvcell, rat, ret;
	char pci[4] = {0};
	char arfcn[8] = {0};
	char arfcn_5g[8] = {0};
	char rscp[8] = {0};
	char ecio[8] = {0};
	char rsrp[8] = {0};
	char rsrq[8] = {0};
	char ss_rsrp[8] = {0};
	char ss_rsrq[8] = {0};
	char ss_sinr[8] = {0};
	char band[8] = {0};
	char band_5g[8] = {0};

	int mask = 0;
	SRVCELL_INFO cell = {0};

	if(!pcmd || !buf || !config || !info){
		LOG_ER("%s, invalid parameters!!", __func__);
		return ERR_AT_NAK;
	}

	/*+GTCCINFO:
	 * LTE service cell:
	 * 1,4,460,01,EA00,71CF520,672,1BB,3,100,-2,74,74,32
	 *
	 * LTE neighbor cell:
	 *
	 * OK
	 *
	 *+GTCCINFO:
	 * UMTS service cell:
	 * 1,2,460,01,F11B,A1BE41B,2A0B,B9,1,42,52,1,52,,42
	 *
	 * UMTS neighbor cell:
	*/

	sim_idx=info->current_sim-1;

	p = strstr(buf, pcmd->resp);
	if(p){
retry:
		while((q = strsep(&p, "\r\n"))){
			if(!strchr(q, ',')){
				is_en_dc += strstr(q, "EN-DC") ? 1 : 0;
				continue;
			}
			
			STR_SKIP(q2, q, ",", 1, retry);
			is_srvcell = atoi(q2); //1: serving cell, 2: neighbor cell

			LOG_DB("is_srvcell:%d", is_srvcell);
			if(is_srvcell != 1){
				break;//
			}

			STR_SKIP(q2, q, ",", 1, retry);
			rat = atoi(q2);
			switch(rat){
				case 9: //NR5G
					info->network = 4;
					if(is_en_dc){
						mask = BITS_UNSET(MASK_ALL, BIT_RSSI);
						if(ih_license_support(IH_FEATURE_MODEM_FG360)){
							ret = sscanf(q, "%[^,],%[^,],,,%[^,],%[^,],%[^,],%*[^,],%[^,],%*[^,],%[^,],%[^\r\n]",
								cell.mcc, cell.mnc, arfcn_5g, pci, band_5g, ss_sinr, ss_rsrp, ss_rsrq);
						}else{
							ret = sscanf(q, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%[^,],%*[^,],%[^,],%[^\r\n]",
								cell.mcc, cell.mnc, cell.lac, cell.cellid, arfcn_5g, pci, band_5g, ss_sinr, ss_rsrp, ss_rsrq);
						}
						
						STR_FORMAT(cell.rat, "NSA");
						if(ss_rsrq[0] && atoi(ss_rsrq) != 255){
							if(ih_license_support(IH_FEATURE_MODEM_FG360)){
								STRCAT_FORMAT(cell.band, "+n%s", band_5g+2); //5010 means n10
							}else{
								STRCAT_FORMAT(cell.band, "+n%s", band_5g);
							}
							STR_FORMAT(cell.arfcn, "%lu", strtoul(arfcn_5g, NULL, 16));
							STR_FORMAT(cell.pci, "%lu", strtoul(pci, NULL, 16));
							STR_FORMAT(cell.ss_rsrp, "%d", atoi(ss_rsrp) - 156)	
							STR_FORMAT(cell.ss_rsrq, "%0.1f", atoi(ss_rsrq) * 0.5f - 43);
							STR_FORMAT(cell.ss_sinr, "%0.1f", atoi(ss_sinr) * 0.5f - 23);
						}

						if((ret > 0) && ppp_debug()){
							LOG_DB("NR5G-NSA mcc:%s mnc:%s cellid:%s lac:%s ss_rsrp:%s ss_rsrq:%s pci:%s arfcn:%s band:%s",
								cell.mcc, cell.mnc, cell.cellid, cell.lac, cell.ss_rsrp, cell.ss_rsrq, cell.pci, arfcn_5g, band_5g);
						}
					}else{
						mask = MASK_ALL;
						ret = sscanf(q, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%[^,],%*[^,],%[^,],%[^\r\n]",
								cell.mcc, cell.mnc, cell.lac, cell.cellid, arfcn_5g, pci, band_5g, ss_sinr, ss_rsrp, ss_rsrq);

						STR_FORMAT(cell.rat, "SA");
						if(ss_rsrq[0] && atoi(ss_rsrq) != 255){
							if(ih_license_support(IH_FEATURE_MODEM_FG360)){
								STRCAT_FORMAT(cell.band, "n%s", band_5g+2); //5010 means n10
							}else{
								STRCAT_FORMAT(cell.band, "n%s", band_5g);
							}
							STR_FORMAT(cell.arfcn, "%lu", strtoul(arfcn_5g, NULL, 16));
							STR_FORMAT(cell.pci, "%lu", strtoul(pci, NULL, 16));
							STR_FORMAT(cell.ss_rsrp, "%d", atoi(ss_rsrp) - 156)	
							STR_FORMAT(cell.ss_rsrq, "%0.1f", atoi(ss_rsrq) * 0.5f - 43);
							STR_FORMAT(cell.ss_sinr, "%0.1f", atoi(ss_sinr) * 0.5f - 23);
						}

						if((ret > 0) && ppp_debug()){
							LOG_DB("NR5G-SA mcc:%s mnc:%s cellid:%s lac:%s ss_rsrp:%s ss_rsrq:%s pci:%s arfcn:%s band:%s",
								cell.mcc, cell.mnc, cell.cellid, cell.lac, cell.ss_rsrp, cell.ss_rsrq, cell.pci, cell.arfcn, cell.band);
						}
					}
					break;
				case 4: //LTE
					info->network = is_en_dc ? 4 : 3;
					mask = BITS_UNSET(MASK_ALL, BIT_RSSI);
					ret = sscanf(q, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^\r\n]",
							cell.mcc, cell.mnc, cell.lac, cell.cellid, arfcn, pci, band, rsrp, rsrq);

					STR_FORMAT(cell.rat, "LTE");
					if(rsrq[0]){
						STR_FORMAT(cell.pci, "%lu", strtoul(pci, NULL, 16));
						STR_FORMAT(cell.arfcn, "%lu", strtoul(arfcn, NULL, 16));
						if(ih_license_support(IH_FEATURE_MODEM_FG360)
							|| ih_license_get_modem() == IH_FEATURE_MODEM_NL668){
							STR_FORMAT(cell.band, "B%d", atoi(band)-100); //110 means B10
						}else{
							STR_FORMAT(cell.band, "B%s", band);
						}
						STR_FORMAT(cell.rsrp, "%d", atoi(rsrp) - 140)	
						STR_FORMAT(cell.rsrq, "%0.1f", atoi(rsrq) * 0.5f - 20);
					}

					if((ret > 0) && ppp_debug()){
						LOG_DB("%sLTE mcc:%s mnc:%s cellid:%s lac:%s rsrp:%s rsrq:%s pci:%s earfcn:%s band:%s", is_en_dc ? "NR5G-NSA => " : "",
								cell.mcc, cell.mnc, cell.cellid, cell.lac, cell.rsrp, cell.rsrq, cell.pci, cell.arfcn, cell.band);
					}
					break;
				case 2: //WCDMA
					info->network = 2;
					mask = BITS_UNSET(MASK_ALL, BIT_RSSI);
					ret = sscanf(q, "%[^,],%[^,],%[^,],%[^,],%[^,],%*[^,],%[^,],%[^,],%[^,],",
							cell.mcc, cell.mnc, cell.lac, cell.cellid, arfcn, cell.band, ecio, rscp);

					STR_FORMAT(cell.rat, "WCDMA");
					if(rscp[0] && atoi(rscp) != 255){
						STR_FORMAT(cell.rscp, "%d", atoi(rscp) - 120)	
						STR_FORMAT(cell.ecio, "%0.1f", atoi(ecio) * 0.5f - 24.5f);
					}

					if((ret > 0) && ppp_debug()){
						LOG_DB("WCDMA mcc:%s mnc:%s cellid:%s lac:%s rscp:%s ecio:%s uarfcn:%s band:%s",
								cell.mcc, cell.mnc, cell.cellid, cell.lac, cell.rscp, cell.ecio, arfcn, cell.band);
					}
					break;
				default:
					LOG_DB("unknown rat: %d", rat);
					continue;
					break;
			}
		}
	}
	
	update_srvcell_info(&cell, mask);

	if(info->siglevel > 0){
		if((config->dualsim)&&(config->csq[sim_idx])){
			if(info->siglevel<config->csq[sim_idx]) retval = ERR_AT_LOWSIGNAL;
			else retval = ERR_AT_OK;
		}else{
			if(info->siglevel >= 10) retval = ERR_AT_OK;
			else if(retry >= 5) retval = ERR_AT_OK;
			else retval = ERR_AT_LOWSIGNAL;
		}
	}else{
		if(retry >= pcmd->max_retry){
			retval = ERR_AT_NOSIGNAL;
		}
	}

	if(ppp_debug()){
		LOG_DB("modem siglevel: %d", info->siglevel);
	}

	return retval;
}

int check_modem_gtrndis(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL; 
	int state = 0;

	if(!pcmd || !buf){
		return ERR_AT_NAK;
	}

	p = strstr(buf, pcmd->resp);
	if(p){
		p += strlen(pcmd->resp);
		while(*p == ' ')p++;

		state = atoi(p);
	}
	
	return state ? ERR_AT_OK : ERR_AT_NAK;	
}

int check_modem_qcfg_ims(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL; 
	int ims_mode = 0;
	char cmd[128] = {0};
	char *ims_str[] = {"Auto", "Enable", "Disable"};

	if(!pcmd || !buf){
		return ERR_AT_NAK;
	}

	ims_mode = (info->current_sim == SIM2) ? config->ims_mode[1] : config->ims_mode[0];

	p = strstr(buf, pcmd->resp);
	if(p){
		p += strlen(pcmd->resp);
		while(*p == ' ')p++;
		
		strsep(&p, ",");
		if(p && atoi(p) != ims_mode){
			LOG_IN("change IMS to %s mode", ims_str[ims_mode]);
			snprintf(cmd, sizeof(cmd), "AT+QCFG=\"ims\",%d\r\n", ims_mode);
			if(send_at_cmd_sync2(cmd, NULL, 0, 1) == ERR_AT_OK){
				return ERR_AT_NEED_RESET;
			}
		}
	}
	
	return ERR_AT_OK;	
}

int check_modem_qcfg_ims2(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL; 
	int ims_mode = 0;
	int ims_real = 0;
	char cmd[128] = {0};
	char *ims_str[] = {"Auto", "Enable", "Disable"};

	if(!pcmd || !buf){
		return ERR_AT_NAK;
	}

	ims_mode = (info->current_sim == SIM2) ? config->ims_mode[1] : config->ims_mode[0];

	p = strstr(buf, pcmd->resp);
	if(p){
		p += strlen(pcmd->resp);
		while(*p == ' ')p++;
		
		strsep(&p, ",");
		if(p){
			//ims mapping
			switch(ims_mode){
				case IMS_MODE_DISABLE:
					ims_real = 0;
					break;
				case IMS_MODE_ENABLE:
				case IMS_MODE_AUTO:
				default: //ims should be enabled in default mode, because we need sms feature.
					ims_real = 1;
					break;
			}

			if(ims_real != atoi(p)){
				LOG_IN("change IMS to %s mode", ims_str[ims_mode]);
				snprintf(cmd, sizeof(cmd), "AT+QCFG=\"ims\",%d\r\n", ims_real);
				//immedieately take effect
				send_at_cmd_sync2(cmd, NULL, 0, 1);
			}
		}
	}
	
	return ERR_AT_OK;	
}

int check_modem_cavims(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL; 
	int ims_mode = 0;
	int ims_real = 0;
	char cmd[128] = {0};
	char *ims_str[] = {"Auto", "Enable", "Disable"};

	if(!pcmd || !buf){
		return ERR_AT_NAK;
	}

	ims_mode = (info->current_sim == SIM2) ? config->ims_mode[1] : config->ims_mode[0];

	p = strstr(buf, pcmd->resp);
	if(p){
		p += strlen(pcmd->resp);
		while(*p == ' ')p++;
	
		//ims mapping
		switch(ims_mode){
			case IMS_MODE_DISABLE:
				ims_real = 0;
				break;
			case IMS_MODE_ENABLE:
			case IMS_MODE_AUTO:
			default: //ims should be enabled in default mode, because we need sms feature.
				ims_real = 1;
				break;
		}

		if(ims_real != atoi(p)){
			LOG_IN("change IMS to %s mode", ims_str[ims_mode]);
			snprintf(cmd, sizeof(cmd), "AT+CAVIMS=%d\r\n", ims_real);
			//immedieately take effect
			send_at_cmd_sync2(cmd, NULL, 0, 1);
		}
	}
	
	return ERR_AT_OK;	
}

#define CELLINFO_FILE	"/tmp/cellinfo"
static void update_cellinfo(MODEM_INFO *info)
{
	FILE *fp;

	fp = fopen(CELLINFO_FILE".tmp", "w");
	if (!fp) return ;

	fprintf(fp, "IMEI:%s\n", info->imei);
	fprintf(fp, "IMSI:%s\n", info->imsi);
	fprintf(fp, "LAC:%s\n", info->lac);
	fprintf(fp, "CID:%s\n", info->cellid);
	fprintf(fp, "MCCMNC:%d\n", info->carrier_code);
	fprintf(fp, "SIG:%d\n", info->siglevel);
	fflush(fp);
	fclose(fp);
	
	rename(CELLINFO_FILE".tmp", CELLINFO_FILE);
}

int compute_siglevel(int rssi)
{
	int siglevel = 0;
	
	rssi = LIMIT(rssi, -110, -48);
	siglevel = (rssi + 110) / 2;			//0 ~ 31

	return siglevel;
}

int compute_signal_info(MODEM_INFO *info)
{
	if(!info){
		return -1;
	}

	if(info->srvcell.ss_rsrp[0]){ //FIXME: test fibocom modems in the future
		info->dbm = LIMIT(atoi(info->srvcell.ss_rsrp), -140, -44);
		info->asu = info->dbm + 140;					//0 ~ 97
		if(info->srvcell.rssi[0]){
			info->siglevel = compute_siglevel(atoi(info->srvcell.rssi));
		}else{
			info->siglevel = info->asu / 3;
		}
		if(info->dbm >= -95) info->sigbar = SIGNAL_BAR_GREAT;
		else if(info->dbm >= -105)info->sigbar = SIGNAL_BAR_GOOD;
		else if(info->dbm >= -115)info->sigbar = SIGNAL_BAR_MODERATE;
		else info->sigbar = SIGNAL_BAR_POOR;
	}else if(info->srvcell.rsrp[0]){
		info->dbm = LIMIT(atoi(info->srvcell.rsrp), -140, -44);
		info->asu = info->dbm + 140;					//0 ~ 97
		if(info->srvcell.rssi[0]){
			info->siglevel = compute_siglevel(atoi(info->srvcell.rssi));
		}else{
			info->siglevel = info->asu / 3;
		}
		if(info->dbm >= -95) info->sigbar = SIGNAL_BAR_GREAT;
		else if(info->dbm >= -105)info->sigbar = SIGNAL_BAR_GOOD;
		else if(info->dbm >= -115)info->sigbar = SIGNAL_BAR_MODERATE;
		else info->sigbar = SIGNAL_BAR_POOR;
	}else if(info->srvcell.rscp[0]){
		info->dbm = LIMIT(atoi(info->srvcell.rscp), -115, -25);
		info->asu = info->dbm + 115;					//0 ~ 90
		if(info->srvcell.rssi[0]){
			info->siglevel = compute_siglevel(atoi(info->srvcell.rssi));
		}else{
			info->siglevel = info->asu / 3;
		}
		if(info->dbm >= -49) info->sigbar = SIGNAL_BAR_GREAT;
		else if(info->dbm >= -73)info->sigbar = SIGNAL_BAR_GOOD;
		else if(info->dbm >= -97)info->sigbar = SIGNAL_BAR_MODERATE;
		else info->sigbar = SIGNAL_BAR_POOR;
	}else if(info->srvcell.rssi[0]){
		info->dbm = LIMIT(atoi(info->srvcell.rssi), -110, -48);
		info->asu = (info->dbm + 110) / 2;				//0 ~ 31
		info->siglevel = info->asu;
		if(info->asu >= 12) info->sigbar = SIGNAL_BAR_GREAT;
		else if(info->asu >= 8)info->sigbar = SIGNAL_BAR_GOOD;
		else if(info->asu >= 5)info->sigbar = SIGNAL_BAR_MODERATE;
		else info->sigbar = SIGNAL_BAR_POOR;
	}else{
		info->asu = 0;
		info->dbm = -110;
		info->siglevel = 0;
		info->sigbar = SIGNAL_BAR_NONE;
	}

#ifdef INHAND_ER3
	update_sig_leds2(info->sigbar);
#else
	update_sig_leds(info->siglevel);
#endif
	update_cellinfo(info);
	
	return info->siglevel;
}

void clear_srvcell_info(int mask)
{
	SRVCELL_INFO cell;

	memset(&cell, 0, sizeof(SRVCELL_INFO));
	
	update_srvcell_info(&cell, mask);
}

int update_srvcell_info(SRVCELL_INFO *cell, int mask)
{
	char buf[64] = {0};
	static int old_network = 0;
	MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;
	
	if(!cell){
		return -1;
	}
	
	if(info->network != old_network){
		old_network = info->network;
		clear_srvcell_info(BIT_RAT);
	}

	if(mask & BIT_RAT){
		if(VALID_SRVCELL_INFO(cell->rat)){
			strlcpy(info->submode_name, cell->rat, sizeof(info->submode_name));
			strlcpy(info->srvcell.rat, cell->rat, sizeof(cell->rat));
		}else{
			bzero(info->submode_name, sizeof(info->submode_name));
			bzero(info->srvcell.rat, sizeof(info->srvcell.rat));
		}
	}

	if((mask & BIT_MCC) && (mask & BIT_MNC)){
		if(VALID_SRVCELL_INFO(cell->mcc) && VALID_SRVCELL_INFO(cell->mnc)) {
			STR_FORMAT(buf, "%s%s", cell->mcc, cell->mnc);
			info->carrier_code = atoi(buf);
			find_operator(info->carrier_code, info->carrier_str);
			strlcpy(info->srvcell.mcc, cell->mcc, sizeof(cell->mcc));
			strlcpy(info->srvcell.mnc, cell->mnc, sizeof(cell->mnc));
		}else{
			info->carrier_code = 0;
			bzero(info->carrier_str, sizeof(info->carrier_str));
			bzero(info->srvcell.mcc, sizeof(info->srvcell.mcc));
			bzero(info->srvcell.mnc, sizeof(info->srvcell.mnc));
		}
	}

	if(mask & BIT_LAC){
		if(VALID_SRVCELL_INFO(cell->lac)){
			strlcpy(info->lac, cell->lac, sizeof(info->lac));
			strlcpy(info->srvcell.lac, cell->lac, sizeof(cell->lac));
		}else{
			bzero(info->lac, sizeof(info->lac));
			bzero(info->srvcell.lac, sizeof(info->srvcell.lac));
		}
	}

	if(mask & BIT_CELLID){
		if(VALID_SRVCELL_INFO(cell->cellid)){
			strlcpy(info->cellid, cell->cellid, sizeof(info->cellid));
			strlcpy(info->srvcell.cellid, cell->cellid, sizeof(info->srvcell.cellid));
		}else{
			bzero(info->cellid, sizeof(info->cellid));
			bzero(info->srvcell.cellid, sizeof(info->srvcell.cellid));
		}
	}

	if(mask & BIT_PCI){
		if(VALID_SRVCELL_INFO(cell->pci) && RANGE_CHK(atoi(cell->pci), 0, 1007)){
			strlcpy(info->srvcell.pci, cell->pci, sizeof(cell->pci));
		}else{
			bzero(info->srvcell.pci, sizeof(info->srvcell.pci));
		}
	}

	if(mask & BIT_ARFCN){
		if(!strchr(cell->arfcn, '-')){
			strlcpy(info->srvcell.arfcn, cell->arfcn, sizeof(cell->arfcn));
		}else{
			bzero(info->srvcell.arfcn, sizeof(info->srvcell.arfcn));
		}
	}

	if(mask & BIT_BAND){
		if(VALID_SRVCELL_INFO(cell->band)){
			if(RANGE_CHK(cell->band[0], '1', '9')){
				info->band = atoi(cell->band);
			}else{
				info->band = 0;
			}
			strlcpy(info->srvcell.band, cell->band, sizeof(cell->band));
		}else{
			info->band = 0;
			bzero(info->srvcell.band, sizeof(info->srvcell.band));
		}
	}

	if(mask & BIT_RSSI){
		if(VALID_SRVCELL_INFO(cell->rssi) && RANGE_CHK(atoi(cell->rssi), -113, -48)){
			info->rssi = atoi(cell->rssi);
			strlcpy(info->srvcell.rssi, cell->rssi, sizeof(cell->rssi));
		}else{
			info->rssi = 0;
			bzero(info->srvcell.rssi, sizeof(info->srvcell.rssi));
		}
	}

	if(mask & BIT_BER){
		if(VALID_SRVCELL_INFO(cell->ber) && RANGE_CHK(atoi(cell->ber), 0, 7)){
			strlcpy(info->srvcell.ber, cell->ber, sizeof(cell->ber));
		}else{
			bzero(info->srvcell.ber, sizeof(info->srvcell.ber));
		}
	}

	if(mask & BIT_RSCP){
		if(VALID_SRVCELL_INFO(cell->rscp) && RANGE_CHK(atoi(cell->rscp), -120, -25)){
			strlcpy(info->srvcell.rscp, cell->rscp, sizeof(cell->rscp));
		}else{
			bzero(info->srvcell.rscp, sizeof(info->srvcell.rscp));
		}
	}

	if(mask & BIT_ECIO){
		if(VALID_SRVCELL_INFO(cell->ecio)){
			strlcpy(info->srvcell.ecio, cell->ecio, sizeof(cell->ecio));
		}else{
			bzero(info->srvcell.ecio, sizeof(info->srvcell.ecio));
		}
	}

	if(mask & BIT_RSRP){
		if(VALID_SRVCELL_INFO(cell->rsrp) && RANGE_CHK(atoi(cell->rsrp), -140, -44)){
			info->rsrp = atoi(cell->rsrp);
			strlcpy(info->srvcell.rsrp, cell->rsrp, sizeof(cell->rsrp));
		}else{
			info->rsrp = 0;
			bzero(info->srvcell.rsrp, sizeof(info->srvcell.rsrp));
		}
	}

	if(mask & BIT_RSRQ){
		if(VALID_SRVCELL_INFO(cell->rsrq) && RANGE_CHK(atoi(cell->rsrq), -34, 3)){
			info->rsrq = atoi(cell->rsrq);
			strlcpy(info->rsrq_str, cell->rsrq, sizeof(cell->rsrq));
			strlcpy(info->srvcell.rsrq, cell->rsrq, sizeof(cell->rsrq));
		}else{
			info->rsrq = 0; //FIXME: 0 is valid value
			bzero(info->rsrq_str, sizeof(info->rsrq_str));
			bzero(info->srvcell.rsrq, sizeof(info->srvcell.rsrq));
		}
	}

	if(mask & BIT_SINR){
		if(VALID_SRVCELL_INFO(cell->sinr) && RANGE_CHK(atoi(cell->sinr), -23, 40)){
			strlcpy(info->sinr, cell->sinr, sizeof(info->sinr));
			strlcpy(info->srvcell.sinr, cell->sinr, sizeof(cell->sinr));
		}else{
			bzero(info->sinr, sizeof(info->sinr));
			bzero(info->srvcell.sinr, sizeof(info->srvcell.sinr));
		}
	}

	if(mask & BIT_SS_RSRP){
		if(VALID_SRVCELL_INFO(cell->ss_rsrp) && RANGE_CHK(atoi(cell->ss_rsrp), -156, -31)){
			strlcpy(info->srvcell.ss_rsrp, cell->ss_rsrp, sizeof(cell->ss_rsrp));
		}else{
			bzero(info->srvcell.ss_rsrp, sizeof(info->srvcell.ss_rsrp));
		}
	}

	if(mask & BIT_SS_RSRQ){
		if(VALID_SRVCELL_INFO(cell->ss_rsrq) && RANGE_CHK(atoi(cell->ss_rsrq), -43, 20)){
			strlcpy(info->srvcell.ss_rsrq, cell->ss_rsrq, sizeof(cell->ss_rsrq));
		}else{
			bzero(info->srvcell.ss_rsrq, sizeof(info->srvcell.ss_rsrq));
		}
	}

	if(mask & BIT_SS_SINR){
		if(VALID_SRVCELL_INFO(cell->ss_sinr) && RANGE_CHK(atoi(cell->ss_sinr), -23, 40)){
			strlcpy(info->srvcell.ss_sinr, cell->ss_sinr, sizeof(cell->ss_sinr));
		}else{
			bzero(info->srvcell.ss_sinr, sizeof(info->srvcell.ss_sinr));
		}
	}

	return compute_signal_info(info);
}

static int is_lac(const char *str)
{
	char buf[64] = {0};
	char *p = buf;
	char *lac = NULL;
	int i = 0, len = 0;

	if(!str || !str[0]){
		return 0;
	}

	snprintf(buf, sizeof(buf), "%s", str);
	lac = strsep(&p, ",");

	if(*lac == '"'){
		lac++;
		lac[strlen(lac)-1] = '\0';
	}
	
	//LAC range is 0000 ~ FFFFFE
	if(strlen(lac) >= 4 && strlen(lac) <= 6){
		for(i = 0; i < strlen(lac); i++){
			if((lac[i] >= '0' && lac[i] <= '9') || (lac[i] >= 'A' && lac[i] <= 'F')){
				len++;
			}
		}
	}

	return len == strlen(lac);
}

int check_modem_reg(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	int is_urc = 0;
	char want[16] = {0};
	char *p, *q, *lac, *cellid;
	int retval=ERR_AT_NOREG, status;
	SRVCELL_INFO cell = {0};

	if(pcmd->index == AT_CMD_CREG
		|| pcmd->index == AT_CMD_CGREG
		|| pcmd->index == AT_CMD_CEREG
		|| pcmd->index == AT_CMD_C5GREG){
		snprintf(want, sizeof(want), "REG:");
	}else{
		snprintf(want, sizeof(want), "%s", pcmd->resp);
	}

	p = strstr(buf, want);
	if(p) {
		/*+CGREG: 0,1
		 *EM770W,EM820W +CGREG: 2,1, A011, 219F07
		 *MU609 +CGREG: 2,1,"A011","219F07",2
		 *LE910 +CEREG: 2,1,"4F0B","ACE501",7^M ^M OK
		 *LUA10 +CEREG: 2,1,"1095","1","1248502",7*/
		
		p += strlen(want);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;
		q = strsep(&p, ",");
		//URC format: +C5GREG: 1,"280383","FFBCF002",11,1,"01"
		is_urc = is_lac(p);
		if(is_urc){
			status = atoi(q);
		}else if(p){
			status = atoi(p);
		}else{
			status = 0;
		}
		if(status==1) retval=ERR_AT_OK;
		else if(status==5){
			if(config->roam_enable[info->current_sim-1] != TRUE) retval=ERR_AT_ROAM;
			else retval=ERR_AT_OK;
		}
		info->regstatus = status;
		//LOG_IN("modem register status: %d", status);
		//cellid
		if(!is_urc){
			strsep(&p, ",");
		}
		if(p && *p) {
			lac = strsep(&p, ",");	
			/*if have ' ', eat it.*/
			while(lac && *lac!=0 && *lac==' ') lac++;
			if (pcmd->index==AT_CMD_CEREG) {
				if (ih_license_get_modem() == IH_FEATURE_MODEM_ELS31
						|| ih_license_get_modem() == IH_FEATURE_MODEM_TOBYL201
						|| ih_license_get_modem() == IH_FEATURE_MODEM_EC20
						|| ih_license_get_modem() == IH_FEATURE_MODEM_EC25
						|| ih_license_get_modem() == IH_FEATURE_MODEM_RM500
						|| ih_license_get_modem() == IH_FEATURE_MODEM_RM520N
						|| ih_license_get_modem() == IH_FEATURE_MODEM_RM500U
						|| ih_license_get_modem() == IH_FEATURE_MODEM_RG500
						|| ih_license_get_modem() == IH_FEATURE_MODEM_FM650
						|| ih_license_get_modem() == IH_FEATURE_MODEM_FG360
						|| ih_license_get_modem() == IH_FEATURE_MODEM_EP06
						|| ih_license_get_modem() == IH_FEATURE_MODEM_LARAR2
						|| ih_license_get_modem() == IH_FEATURE_MODEM_LE910
						|| ih_license_get_modem() == IH_FEATURE_MODEM_NL668) {
					cellid = strsep(&p, ",\r\n");
				}else {
					q = strsep(&p, ",");			
					cellid = strsep(&p, ",");
				}
			}else {
				/* unknown string */
				if (ih_license_get_modem() == IH_FEATURE_MODEM_LUA10) 
					q = strsep(&p, ",");			
				cellid = strsep(&p, ",\r\n");
			}

			/*if have ' ', eat it.*/
			while(cellid && *cellid!=0 && *cellid==' ') cellid++;
			if(lac && cellid) {
				//escape ""
				if(*lac=='"') {
					lac++;
					lac[strlen(lac)-1] = '\0';
				}
				if(*cellid=='"') {
					cellid++;
					cellid[strlen(cellid)-1] = '\0';
				}
				//LOG_IN("modem lac:%s, cellid:%s", lac, cellid);
				strlcpy(cell.lac, lac, sizeof(cell.lac));
				strlcpy(cell.cellid, cellid, sizeof(cell.cellid));
				/*update lac & cellid*/
				update_srvcell_info(&cell, BIT_LAC|BIT_CELLID);
			}
		}
	} else {
		LOG_IN("not found REG.");
	}

	return retval;
}

int check_modem_nv_parameter(int verbose)
{
	char cmd[128] = {0}, rsp[128] = {0};

	if(gl_modem_code == IH_FEATURE_MODEM_EC20){
		if(ih_key_support("LQ20")){ //EC20
			snprintf(cmd, sizeof(cmd), "at+qnvfr=\"/nv/item_files/modem/mmode/operator_name\"\r\n");
			send_at_cmd_sync2(cmd, rsp, sizeof(rsp), verbose);
			if(strstr(rsp, "QNVFR: 01")){
				snprintf(cmd, sizeof(cmd), "at+qnvfw=\"/nv/item_files/modem/mmode/operator_name\",00\r\n");
				send_at_cmd_sync2(cmd, rsp, sizeof(rsp), verbose);
				//check modem version
				send_at_cmd_sync2("ATI\r\n", rsp, sizeof(rsp), verbose);
				if(strstr(rsp, "EC20CEHCR06A06M1G") == NULL){
					return 1;
				}
			}else if(strstr(rsp, "QNVFR: 00")){
				//telecom sim card need below codes
				snprintf(cmd, sizeof(cmd), "AT+COPS=2\r\n");
				send_at_cmd_sync2(cmd, rsp, sizeof(rsp), verbose);
				snprintf(cmd, sizeof(cmd), "AT+COPS=0\r\n");
				send_at_cmd_sync2(cmd, rsp, sizeof(rsp), verbose);
			}
		}
	}else if(gl_modem_code == IH_FEATURE_MODEM_U8300C){
		if(ih_key_support("TL01")){ // U9300C
			snprintf(cmd, sizeof(cmd), "at+efsrw=0,0,\"/nv/item_files/modem/mmode/operator_name\"\r\n");
			send_at_cmd_sync2(cmd, rsp, sizeof(rsp), verbose);
			if(strstr(rsp, "EFSRW:01")){
				snprintf(cmd, sizeof(cmd), "at+efsrw=1,0,\"/nv/item_files/modem/mmode/operator_name\",\"00\"\r\n");
				send_at_cmd_sync2(cmd, rsp, sizeof(rsp), verbose);
				return 1;
			}else if(strstr(rsp, "EFSRW:00")){
				//telecom sim card need below codes
				snprintf(cmd, sizeof(cmd), "AT+COPS=2\r\n");
				send_at_cmd_sync2(cmd, rsp, sizeof(rsp), verbose);
				snprintf(cmd, sizeof(cmd), "AT+COPS=0\r\n");
				send_at_cmd_sync2(cmd, rsp, sizeof(rsp), verbose);
			}
		}
	}

	return 0;
}

int check_rm500q_data_iface(char *dev, int iface_id, int verbose)
{
	char cmd[128] = {0};
	char rsp[128] = {0};
	char *p = NULL, *p2 = NULL;

	if(!dev || !dev[0]){
		return -1;
	}

	send_at_cmd_sync(dev, "AT+QCFG=\"data_interface\"\r\n", rsp, sizeof(rsp), verbose);

	if((p = strstr(rsp, "data_interface"))){
		strsep(&p, ",");
		p2 = strsep(&p, ",");
		if(p2){
			if(atoi(p2) == iface_id){
				return 1;
			}else{
				LOG_IN("change data interface to %s mode", iface_id ? "PCIE" : "USB");

				snprintf(cmd, sizeof(cmd), "AT+QCFG=\"data_interface\",%d,0\r\n", iface_id);
				send_at_cmd_sync(dev, cmd, NULL, 0, verbose);
				return 0;
			}
		}
	}

	return -1;
}

int check_rm500u_pcie_mode(char *dev, int mode, int verbose)
{
	char cmd[128] = {0};
	char rsp[128] = {0};
	char *p = NULL;
	MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;

	if(!dev || !dev[0]){
		return -1;
	}

	if(ih_license_support(IH_FEATURE_PCIE2ETH_RTL8125B)){
		return -1;
	}

	//RM500UCNAAR01A17M2G_01.001.01.0
	if(strlen(info->software_version) < 16){
		//should return if software version is unknown
		return -1;
	}

	if(p = strstr(info->software_version, "RM500UCNAAR01A")){
		p += 14;
		LOG_DB("version:A%d", atoi(p));
		if(atoi(p) < 17){
			//should return if software version is lower than A17
			return -1;
		}
	}
	
	send_at_cmd_sync(dev, "AT+QCFG=\"pcie/mode\"\r\n", rsp, sizeof(rsp), verbose);

	if((p = strstr(rsp, "+QCFG:"))){
		strsep(&p, ",");
		if(p && atoi(p) != mode){
			LOG_IN("change data interface to %s mode", mode ? "USB" : "PCIE");

			snprintf(cmd, sizeof(cmd), "AT+QCFG=\"pcie/mode\",%d\r\n", mode);
			send_at_cmd_sync(dev, cmd, NULL, 0, verbose);
			return 0;
		}
	}

	return -1;
}

int check_fm650_data_iface(char *dev, const char *iface, int verbose)
{
	char cmd[128] = {0};
	char rsp[128] = {0};
	char *p = NULL;

	if(!dev || !dev[0] || !iface || !iface[0]){
		return -1;
	}

	send_at_cmd_sync(dev, "AT+GTDEVINTF?\r\n", rsp, sizeof(rsp), verbose);

	if((p = strstr(rsp, "+GTDEVINTF:"))){
		if(strstr(p, iface)){
			return 1;
		}else{
			LOG_IN("change data interface to %s mode", !strcmp(iface, "pcie") ? "PCIE" : "USB");

			snprintf(cmd, sizeof(cmd), "AT+GTDEVINTF=%s\r\n", iface);
			send_at_cmd_sync(dev, cmd, NULL, 0, verbose);
			return 0;
		}
	}

	return -1;
}

int check_fm650_ippass(char *dev, int verbose)
{
	char rsp[128] = {0};
	char *p = NULL;
	int ippass = 0;

	if(!dev || !dev[0]){
		return -1;
	}

	send_at_cmd_sync(dev, "AT+GTIPPASS?\r\n", rsp, sizeof(rsp), verbose);

	if((p = strstr(rsp, "+GTIPPASS:"))){
		strsep(&p, ":");
		if(p){
			ippass = atoi(p);
			if(ippass != 1){
				LOG_IN("enable ippass mode");
				send_at_cmd_sync(dev, "AT+GTIPPASS=1\r\n", NULL, 0, verbose);
			}
		}
	}

	return ippass;
}

int check_rm520n_eth_setting(char *dev, int verbose)
{
	char cmd[256] = {0};
	char rsp[512] = {0};
	char *p = NULL;
	int pcie_mode = 1; //RC mode
	int reboot = 0;
	int ret = ERR_AT_NAK;
	MODEM_INFO *info = &gl_myinfo.svcinfo.modem_info;

	if(!dev || !dev[0]){
		return reboot;
	}

	//check pcie mode
	send_at_cmd_sync(dev, "AT+QCFG=\"pcie/mode\"\r\n", rsp, sizeof(rsp), verbose);
	if((p = strstr(rsp, "+QCFG:"))){
		strsep(&p, ",");
		if(p && atoi(p) != pcie_mode){
			LOG_IN("change pcie mode to %s mode", pcie_mode ? "RC" : "EP");
			snprintf(cmd, sizeof(cmd), "AT+QCFG=\"pcie/mode\",%d\r\n", pcie_mode);
			if(ERR_AT_OK == send_at_cmd_sync(dev, cmd, NULL, 0, verbose)){
				reboot++;
			}
		}
	}

	memset(cmd, 0, sizeof(cmd));
	memset(rsp, 0, sizeof(rsp));

	//check sfe
	send_at_cmd_sync_timeout(dev, "AT+QMAP=\"SFE\"\r\n", rsp, sizeof(rsp), verbose, "OK", 10000);
	if((p = strstr(rsp, "+QMAP:"))){
		strsep(&p, ",");
		if(p && !strstr(p, "enable")){
			LOG_IN("enable modem sfe");
			send_at_cmd_sync(dev, "AT+QMAP=\"SFE\",\"enable\"\r\n", NULL, 0, verbose);
		}
	}

	memset(cmd, 0, sizeof(cmd));
	memset(rsp, 0, sizeof(rsp));

	//check eth driver
	send_at_cmd_sync(dev, "AT+QETH=\"eth_driver\"\r\n", rsp, sizeof(rsp), verbose);
	if((p = strstr(rsp, "r8125"))){
		strsep(&p, ",");
		if(p && atoi(p) != 1){
			LOG_IN("enable modem rtl8125 eth driver");
			if(ERR_AT_OK == send_at_cmd_sync(dev, "AT+QETH=\"eth_driver\",\"r8125\",1\r\n", NULL, 0, verbose)){
				reboot++;
			}
		}
	}

	memset(cmd, 0, sizeof(cmd));
	memset(rsp, 0, sizeof(rsp));

	//check usb speed
	send_at_cmd_sync(dev, "AT+QCFG=\"usbspeed\"\r\n", rsp, sizeof(rsp), verbose);
	if((p = strstr(rsp, "+QCFG:"))){
		strsep(&p, ",");
		if(p++ && atoi(p) != 20){
			LOG_IN("change usb speed to usb2.0");
			if(ERR_AT_OK == send_at_cmd_sync(dev, "AT+QCFG=\"usbspeed\",\"20\"\r\n", NULL, 0, verbose)){
				reboot++;
			}
		}
	}

	return reboot;
}

static BOOL match_dynamic_ip6_addr(char *ip6_str)
{
	int i;
	int num_need_set = CFG_PER_ADDR < DHCPV6C_LIST_NUM ? CFG_PER_ADDR:DHCPV6C_LIST_NUM;
	VIF_INFO *vif = my_get_reidal_vif_info();
	IPv6_SUBNET *ip6_subnet = NULL;
	struct in6_addr ip6;

	if(!ip6_str || !inet_pton(AF_INET6, ip6_str, &ip6)){
		LOG_ER("Invalid ipv6 addr");
		return FALSE;
	}
	
	for(i = 0; i < num_need_set; i++){
		if(!IN6_IS_ADDR_UNSPECIFIED(&(vif->ip6_addr.dynamic_ifaddr[i].ip6)) && vif->ip6_addr.dynamic_ifaddr[i].prefix_len != 0){
			ip6_subnet = &(vif->ip6_addr.dynamic_ifaddr[i]);
			if(in6_prefix_equal(ip6, ip6_subnet->ip6, ip6_subnet->prefix_len)){
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

//@return  -1: error, 1: connected, 0:disconnected
int check_modem_qnetdevstatus(const char *dev, int cid, struct in_addr local_ip, int verbose)
{
	int fd = -1;
	char cmd[128] = {0};
	char rsp[512] = {0};
	char *p = NULL;
	char *pos_ip4 = NULL;
	char *pos_ip6 = NULL;
	char *ip4 = NULL;
	int status = -1;
#define QNETDEVSTATUS_STR	"QNETDEVSTATUS:"

	if(!dev || !dev[0]){
		return -1;
	}

	fd = open(dev, O_RDWR|O_NONBLOCK, 0666);
	if(fd<0) return -1;

	/*flush auto-report data*/
	flush_fd(fd);

	snprintf(cmd, sizeof(cmd), "AT+QNETDEVSTATUS=%d\r\n", cid);

	write_fd(fd, cmd, strlen(cmd), 1000, verbose);
	check_return3(fd, rsp, sizeof(rsp), "OK", 3000, verbose);
	
	if((p = strstr(rsp, QNETDEVSTATUS_STR))){
		p += strlen(QNETDEVSTATUS_STR);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;

		STR_SKIP(pos_ip4, p, ",", 1, leave);
		if(get_pdp_type_setting() != PDP_TYPE_IPV6){
			if((ip4 = inet_ntoa(local_ip)) == NULL){
				goto leave;
			}
			status = !strcmp(ip4, pos_ip4);
		}

		STR_SKIP(pos_ip6, p, ",", 6, leave);
		if(get_pdp_type_setting() != PDP_TYPE_IPV4){
			status = match_dynamic_ip6_addr(pos_ip6);
		}
	}else if(strstr(rsp, "ERROR")){
		status = 0;
	}

leave:
	close(fd);
	return status;
}

int check_modem_mpdn_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char cmd[128] = {0};
	char rsp[512] = {0};
	char *p = NULL;
	int rule_num = -1;
	int profileid = 0;
	int ippt = 0;
	int connect = 0;

	//+QMAP: "MPDN_status",0,1,1,1
	p = strstr(buf, pcmd->resp);
	if(p){
		p += strlen(pcmd->resp);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;
		//take care of channel 0
		if(sscanf(p, "\"MPDN_status\",%d,%d,%d,%d", &rule_num, &profileid, &ippt, &connect) != 4 || connect != 1){
			LOG_IN("MPDN rule 0 status is %s", connect ? "connected" : "disconnected");
			return ERR_FAILED;
		}
	}else{
		return ERR_FAILED;
	}

	return ERR_OK;
}

AT_CMD *get_at_from_custom_cmds(int net_type)
{
	int index = -1;
	AT_CMD *custom = NULL;

	if(!g_modem_custom_cmds){
		return NULL;	
	}

	switch(net_type){
		case MODEM_NETWORK_2G:
			index = AT_CMD_2G;
			break;
		case MODEM_NETWORK_3G:
			index = AT_CMD_3G;
			break;
		case MODEM_NETWORK_3G2G:
			index = AT_CMD_3G2G;
			break;
		case MODEM_NETWORK_EVDO:
			index = AT_CMD_EVDO;
			break;
		case MODEM_NETWORK_4G:
			index = AT_CMD_4G;
			break;
		case MODEM_NETWORK_5G:
			index = AT_CMD_5G;
			break;
		case MODEM_NETWORK_5G4G:
			index = AT_CMD_5G4G;
			break;
		case MODEM_NETWORK_AUTO:
		default:
			index = AT_CMD_AUTO;
			break;
	}
	
	for(custom = g_modem_custom_cmds; custom->index != -1; custom++){
		if(custom->index == index){
			break;
		}
	}
	
	return custom;
}

int check_modem_nwscanmode(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	AT_CMD *cmd = NULL;
	char cmd_buf[64] = {0};
	char *p = NULL, *q = NULL;
	int tar_status = -1, cur_status = -2;
	int network_type = 0;

	if(!pcmd || !buf || !config || !info){
		LOG_ER("%s invalid parameter!!", __func__);
		return ERR_OK;
	}

	network_type = (info->current_sim == SIM2) ? config->network_type2 : config->network_type;

	p = strstr(buf, pcmd->resp);
	if(p){
		p += strlen(pcmd->resp);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;
		q = strsep(&p, "\r\n");
		if(q) cur_status = atoi(q);

		if((cmd = get_at_from_custom_cmds(network_type)) != NULL){
			strlcpy(cmd_buf, cmd->atc, sizeof(cmd_buf));
			p = strstr(cmd_buf, pcmd->resp);
			if(p){
				strsep(&p, ",");
				q = strsep(&p, ",");
				tar_status = atoi(q);
			}
			//update nwscanmode	
			if(cur_status != tar_status){
				send_at_cmd_sync2("AT+COPS=2\r\n", NULL, 0, 0);
				sleep(3);
				send_at_cmd_sync2("AT+COPS=0\r\n", NULL, 0, 0);
				sleep(1);
				send_at_cmd_sync2(cmd->atc, NULL, 0, 1);
				sleep(2);
			}
		}
	}

	return ERR_OK;
}

int check_modem_mode_pref(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	AT_CMD *cmd = NULL;
	char cmd_buf[64] = {0};
	char *p = NULL;
	char *tar_status = NULL, *cur_status = NULL;
	int network_type = 0;

	if(!pcmd || !buf || !config || !info){
		LOG_ER("%s invalid parameter!!", __func__);
		return ERR_OK;
	}

	network_type = (info->current_sim == SIM2) ? config->network_type2 : config->network_type;

	p = strstr(buf, pcmd->resp);
	if(p){
		p += strlen(pcmd->resp);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;
		cur_status = strsep(&p, "\r\n");

		if((cmd = get_at_from_custom_cmds(network_type)) != NULL){
			strlcpy(cmd_buf, cmd->atc, sizeof(cmd_buf));
			p = strstr(cmd_buf, pcmd->resp);
			if(p){
				strsep(&p, ",");
				tar_status = strsep(&p, "\r\n");
			}
			//update mode_pref
			if(!cur_status || !tar_status || (cur_status && tar_status && strcmp(cur_status, tar_status))){
#if 0
				send_at_cmd_sync2("AT+COPS=2\r\n", NULL, 0, 0);
				sleep(3);
				send_at_cmd_sync2("AT+COPS=0\r\n", NULL, 0, 0);
				sleep(1);
#endif
				send_at_cmd_sync2(cmd->atc, NULL, 0, 1);
				sleep(2);
			}
		}
	}

	return ERR_OK;
}

int check_modem_mode_pref2(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL;
	int sim_idx = 0;
	char cmd_buf[128] = {0};
	char mode_cmd[32] = {0};
	char *tar_status = mode_cmd, *cur_status = NULL;
	char *nr5g_mode_cmd[] = {"NR5G", "NR5G-NSA", "NR5G-SA", ""};
	int network_type = 0;

	if(!pcmd || !buf || !config || !info){
		LOG_ER("%s invalid parameter!!", __func__);
		return ERR_OK;
	}

	sim_idx = info->current_sim == SIM2 ? 1 : 0;
	network_type = (info->current_sim == SIM2) ? config->network_type2 : config->network_type;

	p = strstr(buf, pcmd->resp);
	if(p){
		p += strlen(pcmd->resp);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;
		cur_status = strsep(&p, "\r\n");
		
		switch(network_type){
			case MODEM_NETWORK_5G:
				strcat(tar_status, "NR5G-SA");
				break;
			case MODEM_NETWORK_5G4G:
				strcat(tar_status, nr5g_mode_cmd[config->nr5g_mode[sim_idx]]);
				break;
			case MODEM_NETWORK_4G:
				strcat(tar_status, "LTE");
				break;
			case MODEM_NETWORK_3G:
				strcat(tar_status, "WCDMA");
				break;
			case MODEM_NETWORK_AUTO:
				strcat(tar_status, nr5g_mode_cmd[config->nr5g_mode[sim_idx]]);
				strcat(tar_status, ":LTE:WCDMA");
				break;
		}

		if(!cur_status || !tar_status || (cur_status && tar_status && strcmp(cur_status, tar_status))){
			snprintf(cmd_buf, sizeof(cmd_buf), "AT+QNWPREFCFG=\"mode_pref\",%s\r\n", tar_status);
			send_at_cmd_sync2(cmd_buf, NULL, 0, 1);
		}
	}

	return ERR_OK;
}

int check_modem_network(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p;
	int ret, carrier_code, network;
	char carrier_code_str[32] = {0};
	uint8_t cnt = 0;
	SRVCELL_INFO cell = {0};
	
	p = strstr(buf, pcmd->resp);
	if(p) {
		if (ih_license_get_modem() == IH_FEATURE_MODEM_PLS8
				|| ih_license_get_modem() == IH_FEATURE_MODEM_ELS81
				|| ih_license_get_modem() == IH_FEATURE_MODEM_ME909
				|| ih_license_get_modem() == IH_FEATURE_MODEM_LARAR2
				|| ih_license_get_modem() == IH_FEATURE_MODEM_TOBYL201
				|| ih_license_get_modem() == IH_FEATURE_MODEM_ELS61){ //liyb:PLS8 maybe have some issues when use AT+COPS=0,2.. .So we use AT+COPS=0,0...
			p += strlen(pcmd->resp);
			p = strchr(p,'\"');
			if (!p){
			    LOG_IN("can't get carrier code.");
				return -1;	
			}

			p ++;

			for (cnt = 0; cnt <= 16 && p[cnt] != '\"'; cnt ++) {
				carrier_code_str[cnt] = p[cnt];
			}
			carrier_code_str[cnt+1] = '\0';
			p = strchr(p, ',');
			if (!p){
			    LOG_IN("can't get network type.");
				return -1;	
			}

			p ++;

			network = atoi(p);
			if (carrier_code_str[0]) {
				//LOG_IN("carrier code: %s, network: %d", carrier_code_str, network);

				if(carrier_code_str[0]) memcpy(info->carrier_str ,carrier_code_str, sizeof(carrier_code_str));
				switch(network) {
					case 2:
						info->network = 2;//3G
						break;
					case 4:
						info->network = 2;//3G
						break;
					case 5:
						info->network = 2;//3G
						break;
					case 6:
						info->network = 2;//3G
						break;
					case 0:
						info->network = 1;//2G
						break;
					case 1:
						info->network = 1;//2G
						break;
					case 3:
						info->network = 1;//2G
						break;
					case 7:
						info->network = 3;//4G
						break;
					default:
						info->network = 0;
						break;
				}
			}
		}else {
			p += strlen(pcmd->resp);
			/*if have ' ', eat it.*/
			//while(*p!=0 && *p==' ') p++;
			ret = sscanf(p, "%*d,%*d,\"%d\",%d", &carrier_code, &network);
			if(ret==2) {
				//LOG_IN("carrier code: %d, network: %d", carrier_code, network);

				if(carrier_code) info->carrier_code = carrier_code;
				if(network == 2) info->network = 2;//3g
				else if(network == 0) info->network = 1;//2g
				else if(network == 7) info->network = 3;//4g /*LUA10 define*/
				else if(network == 100) info->network = 2;//CDMA/EVDO
				else if(network >= 10 && network <= 13)info->network = 4;//5g
				
				if(info->carrier_code >= 100000){ //MNC is three digits
					STR_FORMAT(cell.mcc, "%d", carrier_code/1000);
					STR_FORMAT(cell.mnc, "%03d", carrier_code%1000);
				}else{ //MNC is two digits
					STR_FORMAT(cell.mcc, "%d", carrier_code/100);
					STR_FORMAT(cell.mnc, "%02d", carrier_code%100);
				}

				update_srvcell_info(&cell, BIT_MCC|BIT_MNC);
			}
		} 
	} else {
		LOG_IN("not found COPS.");
		return -1;
	}
	return ERR_AT_OK;
}

int check_modem_sysinfo(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p, submode_name[32];
	int ret, sysmode, submode, roamstatus, regStatus;
	char *submode_str[] = {"","GSM","GPRS","EDGE","WCDMA","HSDPA","HSUPA","","","HSPA+"};
	char *cdma_submode[]={"","AMPS","CDMA","GSM/GPRS","HDR","WCDMA","GPS","GSM/WCDMA","CDMA/HDR HYBRID"};
	
	p = strstr(buf, pcmd->resp);
	if(p) {
		p += strlen(pcmd->resp);
		if(pcmd->index==AT_CMD_SYSINFOEX) {
			/*^SYSINFOEX:2,3,0,1,,3,"WCDMA",41,"WCDMA"*/
			//LOG_IN("%s", p);
			ret = sscanf(p, "%*d,%*d,%*d,%*d,,%d,%*[^,],%*d,\"%[^\"]", &sysmode, submode_name);
			if(ret>0) { 
				//LOG_IN("%d, %s", sysmode, submode_name);
				strlcpy(info->submode_name, submode_name, sizeof(info->submode_name));
				if(sysmode == 3) info->network = 2;//3g
				else if(sysmode == 1) info->network = 1;//2g
				else if(sysmode == 6) info->network = 3;//me909 4g-lte
			}
			if ((config->network_type != MODEM_NETWORK_AUTO) && (config->network_type != MODEM_NETWORK_3G2G) && (config->network_type != info->network)) {
				return ERR_AT_NAK;
			}
		} else if(pcmd->index==AT_CMD_SYSINFO) {
			/*^SYSINFO:2,3,0,5,1,,4*/
			//LOG_IN("%s", p);
			ret = sscanf(p, "%*d,%*d,%*d,%d,%*d,,%d", &sysmode, &submode);
			if(ret>0) { 
				//LOG_IN("%d, %d", sysmode, submode);
				if(submode<10) {
					strlcpy(info->submode_name, submode_str[submode], sizeof(info->submode_name));
				}
				if(sysmode == 5) info->network = 2;//3g
				else if(sysmode == 3) info->network = 1;//2g
			}
		} else if(pcmd->index==AT_CMD_VSYSINFO) {
			//mc7216 evdo: ^SYSINFO:<srv_status>,<srv_domain>,<roam_status>,<sys_mode>,<sim_state>
			//^SYSINFO:2,3,0,8,1
			ret = sscanf(p, "%d,%*d,%d,%d,%*d", &sysmode, &roamstatus, &submode);
			if(ret>0) { 
				//LOG_IN("vsysinfo %d, %d, %s", sysmode, roamstatus, cdma_submode[submode]);
				if(sysmode == 2){
					if(!roamstatus) info->regstatus = 1; //registred
					else info->regstatus = 5;//registred roamming
				}
				if(submode<9) strlcpy(info->submode_name, cdma_submode[submode], sizeof(info->submode_name));
				if(submode == 4) info->network = 2;//3g
				else if(submode == 2) info->network = 1;//2g
				else info->network = 0;//auto
			}
		}if (pcmd->index==AT_CMD_WSYSINFO){
			ret = sscanf(p, "%d,%*d,%*d,%d,%*d", &regStatus, &submode);
			if(ret>0) {
				//regStatus = atoi(p);
				if (regStatus == 2){ 
					info->regstatus = 1;
				} else{
					info->regstatus = 2;
					return ERR_AT_NAK;
				}
			}
			LOG_IN("modem register status: %d", info->regstatus);
			LOG_IN("modem register mode: %d", submode);

			if(submode == 4) info->network = 2;//3g
			else if(submode == 2) info->network = 1;//2g
			else info->network = 0;//auto
 		}
	}

	return ERR_AT_OK;
}

typedef struct{
	char *network;
	int id;
}NWMODE_ST;

int check_modem_network_sel_u8300c(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *pos;
	int i = 0;
	NWMODE_ST submode[] = {
				{"LTE TDD", 3},
				{"LTE FDD", 3},
				{"TDSCDMA", 2},
				{"HSPA+", 2},
				{"HSUPA", 2},
				{"HSDPA", 2},
				{"WCDMA", 2},
				{"UMTS", 2},
				{"EVDO", 2},
				{"HDR", 2},
				{"CDMA", 1},
				{"GPRS", 1},
				{"EDGE", 1},
				{"GSM",  1},
				{"NONE", 0},
				{NULL, 0}
			};

	pos = strstr(buf, pcmd->resp);
	if(pos) {
		pos += strlen(pcmd->resp);	
		while((submode[i].network != NULL) && strstr(pos, submode[i].network)==NULL) i++;	
		if(submode[i].network == NULL){
			if (ih_license_support(IH_FEATURE_MODEM_U8300C)) {
				//fixup_u8300c_at_list2(0);
			}
			strlcpy(info->submode_name, "NONE", sizeof(info->submode_name));	
			return ERR_AT_OK;
		}else {
			info->network = submode[i].id;
			if (strcmp(info->submode_name, submode[i].network) && gl_at_index >= 9){
				  strlcpy(info->submode_name, submode[i].network, sizeof(info->submode_name));	
				  auto_select_net_apn(info->imsi, 1);
			}else {
				  strlcpy(info->submode_name, submode[i].network, sizeof(info->submode_name));	
			}
		}
	}else{
		LOG_DB("network info type is NONE!");
		strlcpy(info->submode_name, "NONE", sizeof(info->submode_name));	
		return ERR_AT_NOREG;	
	}
/*
	if(config->network_type && info->network != config->network_type){
		LOG_IN("network type %d is not correspendant to config %d", info->network, config->network_type);
		return ERR_AT_NOREG;	
	}
*/
	if(!info->network ||strcmp(info->submode_name ,"NONE") == 0) {
		LOG_DB("network info type is none");
		return ERR_AT_NOREG;	
	}
#if 1
	if (ih_license_support(IH_FEATURE_MODEM_U8300C)) {
		if ((strcmp(submode[i].network, "EVDO") == 0) 
			|| (strcmp(submode[i].network, "HDR") == 0)
			|| (strcmp(submode[i].network, "CDMA") == 0)) //EVDO or CDMA
			fixup_u8300c_at_list2(1, info);
		else
			fixup_u8300c_at_list2(0, info);
	}
#endif

	return ERR_AT_OK;
}

int check_modem_network_sel(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *pos;
	int i = 0;
	int net_index= 0;
	char *submode_str[] = {"LTE TDD", //0, 4G
				"LTE FDD",
				"TDSCDMA",//2, 3G
				"HSPA+",
				"HSUPA",
				"HSDPA",
				"WCDMA",
				"UMTS", 
				"EVDO",
				"HDR",
				"CDMA", //10, 2G
				"GPRS",
				"EDGE",
				"GSM",
				"NONE"};

	pos = strstr(buf, pcmd->resp);
	if(pos) {
		pos += strlen(pcmd->resp);	
		while(i<(sizeof(submode_str)/sizeof(char *)) && strstr(pos, submode_str[i])==NULL) i++;	
		if(i == (sizeof(submode_str)/sizeof(char *))){
			LOG_DB("network type is none");	
			return ERR_AT_NAK;
		}else {
			info->regstatus = 1;
			if(i>=0 && i<2) {
				info->network = 3;
			}else if(i>=2 && i<10){
				info->network = 2;
			}else if(i>=10 && i< ((sizeof(submode_str)/sizeof(char *)) - 1)){
				info->network = 1;
			}else {
				info->network = 0;
			}		
			strlcpy(info->submode_name, submode_str[i], sizeof(info->submode_name));	
			//LOG_IN("network type is %s", submode_str[i]);	
			net_index = i;
		}
	}else{
		LOG_DB("network info type is none");
		return ERR_AT_NAK;	
	}
		
	if ((config->network_type && info->network != config->network_type)){
		if (ih_license_support(IH_FEATURE_MODEM_U8300C)) {
			if (config->network_type!=MODEM_NETWORK_EVDO || net_index != 8) {
				LOG_IN("network type %d is not correspendant to config %d", info->network, config->network_type);
				return ERR_AT_NAK;	
			}	
		}else {
			LOG_IN("network type %d is not correspendant to config %d", info->network, config->network_type);
			return ERR_AT_NAK;	
		}
	}
	if(!info->network){
		LOG_DB("network info type is none");
		return ERR_AT_NAK;	
	}
	return ERR_AT_OK;
}

int check_modem_cellid(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p, *q, lac[8], cellid[16];
	int ret;
	SRVCELL_INFO cell = {0};

	p = strstr(buf, pcmd->resp);
	if(p) {
		/*+CGREG: 2,1, A011, 219F07"*/
		/*MU609 +CGREG: 2,1,"A011","219F07",2*/
		p += strlen(pcmd->resp);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;
		q = strsep(&p, "\r\n");//del \r\n
		//LOG_IN("%s", q);
		ret = sscanf(q, "%*d,%*d, %[^,], %[^,]", lac, cellid);		
		if(ret>0) {
			p = lac;
			q = cellid;
			//escape ""
			if(*p=='"') {
				p++;
				p[strlen(p)-1] = '\0';
			}
			if(*q=='"') {
				q++;
				q[strlen(q)-1] = '\0';
			}
			//LOG_IN("lac:%s, cellid:%s", p, q);
			strlcpy(cell.lac, p, sizeof(cell.lac));
			strlcpy(cell.cellid, q, sizeof(cell.cellid));
			update_srvcell_info(&cell, BIT_LAC|BIT_CELLID);
		}
	}

	return ERR_AT_OK;
}

int check_modem_smoni(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p, *q; 
	int ret=0;
	int mask = 0;
	SRVCELL_INFO cell = {0};

	p = strstr(buf, pcmd->resp);
	if(p) {
		if(IH_FEATURE_MODEM_PVS8 == gl_modem_code){
		//1x Syntax:
		//^SMONI: CH,BC,PN,MCC,MNC,SID,NID, CELL, CLASS, PZID,LAT,LONG,PREV,RX,ECIO
		//Example:
		//^SMONI: 507,0,108,310,0,6,8,4850,0,5,0,0,6,-70,11
		//evdo Syntax:
		//^SMONI:
		//CH BC,PN,RX
		//Example:
		//^SMONI:
		//925,1,108,-76
		//1x and evdo Syntax:
		//^SMONI: CH,BC,PN,MCC,MNC,SID,NID,CELL,CLASS,PZID,LAT,LONG,PREV,RX,ECIO
		//CH,BC,PN,RX
		//Example:
		//^SMONI: 507,0,108,310,0,6,8,4850,0,5,0,0,6,-70,11 // (1xRTT)
		//925,1,108,-76 // (EvDO)
			p += strlen(pcmd->resp);
			/*if have ' ', eat it.*/
			while(*p!=0 && *p==' ') p++;
			q = strstr(p, "Searching");
			if(q != NULL){
				LOG_IN("Searching 1xRTT Network");
			}
			q = strstr(p, "\nSearching");
			if(q != NULL){
				LOG_IN("Searching EvDO Network");
			}
		}else {
			//^SMONI: 2G,733,-58,460,01,03FE,CDF0,47,47,1,3,G,LIMSRV
			//^SMONI: 3G,10713,413,-11.0,-83,460,01,A011,0219F07,10,21,LIMSRV
			p += strlen(pcmd->resp);
			/*if have ' ', eat it.*/
			while(*p!=0 && *p==' ') p++;
			//LOG_IN("%s", p);

			if (strcasestr(p, "SEARCH")) {
				return ERR_AT_NAK;
			} else if (strcasestr(p, "Searching")) {
				return ERR_AT_NAK;
			}

			q = strsep(&p, ",");
			if (!q) {
				LOG_IN("Searching..");
				return ERR_AT_NAK;
			}

			if(strcmp(q, "3G")==0) {
				info->network = 2;
				mask = BIT_LAC|BIT_CELLID|BIT_RSRP|BIT_RSRQ|BIT_SINR; //need to clear rsrp rsrq sinr
				ret = sscanf(p, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^,]", cell.lac, cell.cellid);
			}else if (strcmp(q, "4G")==0){
				info->network = 3;
				mask = BIT_LAC|BIT_CELLID|BIT_RSRP|BIT_RSRQ|BIT_SINR;
				ret = sscanf(p, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^,],%*[^,],%*[^,],%[^,],%[^,]", 
						cell.lac, cell.cellid, cell.rsrp, cell.rsrq);
			}else {
				info->network = 1;
				mask = BIT_LAC|BIT_CELLID|BIT_RSRP|BIT_RSRQ|BIT_SINR; //need to clear rsrp rsrq sinr
				ret = sscanf(p, "%*[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^,]", cell.lac, cell.cellid);
			}
			if(ret>0) { 
				update_srvcell_info(&cell, mask);
			} else {
				return ERR_AT_NAK;
			}
		}
	}

	return ERR_AT_OK;
}

int get_band_by_arfcn(int network, int arfcn)
{
    int band = UNKNOWN_BAND;
    //float rel_band = 0.0f;
    //log_d("get_band_by_arfcn: arfcn: %d",arfcn);
    switch(network){
        case 1:
            if((arfcn >= 975 && arfcn<=1023) || (arfcn <= 124)){
                band = GSM_BAND_900;
            }
            else if(arfcn >= 512 && arfcn <= 885 ){
                band = GSM_BAND_1800;
            }
            else if(arfcn >= 128 && arfcn <= 251){
                band = GSM_BAND_850;
            }
            else{
                band = 0xFFFF;
            }
            
            break;
        case 2:
            if(arfcn>=9612 && arfcn <= 9888){
                /**WCDMA 频点号arfcn = 中心频率*5 refer TS25.101**/
                //rel_band = arfcn/5.0f;
                //log_d("WCDMA Uplink band: %.2fMHz",rel_band);
                band = WCDMA_BAND_2100;
            }
            else if(arfcn >= 10562 && arfcn <= 10838){
                /**WCDMA 频点号arfcn = 中心频率*5 refer TS25.101**/
                //rel_band = arfcn/5.0f;
                //log_d("WCDMA Downlink band: %.2fMHz",rel_band);
                band = WCDMA_BAND_2100;
            }
            else if((arfcn >= 9262 && arfcn <= 9538) ){
                /**WCDMA 频点号arfcn = 中心频率*5 refer TS25.101**/
                //rel_band = arfcn/5.0f;
                //log_d("WCDMA Uplink band: %.2fMHz",rel_band);
                band = WCDMA_BAND_1900;
            }
            else if(arfcn>= 12 && arfcn <= 287){
                /**WCDMA 频点号arfcn = 5*(中心频率-1850.1MHz) refer TS25.101**/
                //rel_band = arfcn/5.0f + 1850.1f;
                //log_d("WCDMA Uplink band: %.2fMHz",rel_band);
                band = WCDMA_BAND_1900;
            }
            else if(arfcn>=9662 && arfcn <=9938){
                /**WCDMA 频点号arfcn = 中心频率*5 refer TS25.101**/
                //rel_band = arfcn/5.0f;
                //log_d("WCDMA Downlink band: %.2fMHz",rel_band);
                band = WCDMA_BAND_1900;
            }
            else if(arfcn>=412 && arfcn <=687){
                /**WCDMA 频点号arfcn = 5*(中心频率-1850.1MHz) refer TS25.101**/
                //rel_band = arfcn/5.0f + 1850.1f;
                //log_d("WCDMA Downlink band: %.2fMHz",rel_band);
                band = WCDMA_BAND_1900;
            }
            else if( arfcn >= 4132 && arfcn <= 4233){
                /**WCDMA 频点号arfcn = 中心频率*5 refer TS25.101**/
                //rel_band = arfcn/5.0f;
                //log_d("WCDMA Uplink band: %.2fMHz",rel_band);
                band = WCDMA_BAND_850;
            }
            else if( arfcn >= 782 && arfcn <= 862 ){
                /**WCDMA 频点号arfcn = 5*(中心频率-670.1MHz) refer TS25.101**/
                //rel_band = arfcn/5.0f + 670.1f;
                //log_d("WCDMA Uplink band: %.2fMHz",rel_band);
                band = WCDMA_BAND_850;
            } 
            else if( arfcn >= 4357 && arfcn <= 4458 ){
                /**WCDMA 频点号arfcn = 中心频率*5 refer TS25.101**/
                //rel_band = arfcn/5.0f;
                //log_d("WCDMA Downlink band: %.2fMHz",rel_band);
                band = WCDMA_BAND_850;
            }
            else if( arfcn >= 1007 && arfcn <= 1087 ){
                /**WCDMA 频点号arfcn = 5*(中心频率-670.1MHz) refer TS25.101**/
                //rel_band = arfcn/5.0f + 670.1f;
                //log_d("WCDMA Uplink band: %.2fMHz",rel_band);
                band = WCDMA_BAND_850;
            }
            else if((arfcn >= 2712 && arfcn <= 2863) ){
                /**WCDMA 频点号arfcn = 5*(中心频率 - 340MHz) refer TS25.101**/
                //rel_band = arfcn/5.0f + 340;
                band = WCDMA_BAND_900;
                //log_d("WCDMA Uplink band: %.2fMHz",rel_band);
            }
            else if(arfcn >= 2937 && arfcn <= 3088){
                /**WCDMA 频点号arfcn = 5*(中心频率 - 340MHz) refer TS25.101**/
                //rel_band = arfcn/5.0f + 340;
                band = WCDMA_BAND_900;
                //log_d("WCDMA Downlink band: %.2fMHz",rel_band);
            } 
            else{
                band = UNKNOWN_BAND;
            }
            
            break;
        case 3:
            /**根据3GPP规范3GPP TS 36.101: **/
            /**FDL = FDL_low + 0.1(NDL - NOffs-DL)**/
            /**FUL = FUL_low + 0.1(NUL - NOffs-UL)**/
            if(arfcn >= 37750 && arfcn <= 38249){
                //rel_band = 2570 + 0.1f*(arfcn - 37750);
                band = TDD_LTE_BAND_38;
                //log_d("TDD-LTE band: %.2fMHz",rel_band);
            }
            else if(arfcn >= 38250 && arfcn <= 38649){
                //rel_band = 1880 + 0.1f*(arfcn - 38250);
                band = TDD_LTE_BAND_39;
                //log_d("TDD-LTE band: %.2fMHz",rel_band);
            }
            else if(arfcn >= 38650 && arfcn <= 39649){
                //rel_band = 2300 + 0.1f * (arfcn - 38650);
                band = TDD_LTE_BAND_40;
                //log_d("TDD-LTE band: %.2fMHz",rel_band);
            }
            else if(arfcn <= 599){
                //rel_band = 2110 + 0.1f*(arfcn - 0);
                band = FDD_LTE_BAND_1;
                //log_d("FDD-LTE Downlink band: %.2fMHz",rel_band);
            }
            else if(arfcn >= 18000 && arfcn <= 18599){
                //rel_band = 1920 + 0.1f*(arfcn - 18000);
                band = FDD_LTE_BAND_1;
                //log_d("FDD-LTE Uplonk band: %.2fMHz",rel_band);
            }
	    else if(arfcn >= 600 && arfcn <= 1199){
                //rel_band = 2110 + 0.1f*(arfcn - 0);
                band = FDD_LTE_BAND_2;
                //log_d("FDD-LTE Downlink band: %.2fMHz",rel_band);
            }
	    
            else if(arfcn >= 1200 && arfcn <= 1949){
                //rel_band = 1805 + 0.1f * (arfcn - 1200);
                band = FDD_LTE_BAND_3;
                //log_d("FDD-LTE Downlink band: %.2fMHz",rel_band);
            }
            else if(arfcn >= 19200 && arfcn <= 19949){
                //rel_band = 1710 + 0.1f * (arfcn - 19200);
                band = FDD_LTE_BAND_3;
                //log_d("FDD-LTE Uplink band: %.2fMHz",rel_band);
            }
	    else if(arfcn >= 1950 && arfcn <= 2399){
                //rel_band = 1710 + 0.1f * (arfcn - 19200);
                band = FDD_LTE_BAND_4;
                //log_d("FDD-LTE Uplink band: %.2fMHz",rel_band);
            }
	    else if(arfcn >= 2400 && arfcn <= 2649){
                //rel_band = 1710 + 0.1f * (arfcn - 19200);
                band = FDD_LTE_BAND_5;
                //log_d("FDD-LTE Uplink band: %.2fMHz",rel_band);
            }
	    else if(arfcn >= 2650 && arfcn <= 2749){
                //rel_band = 1710 + 0.1f * (arfcn - 19200);
                band = FDD_LTE_BAND_6;
                //log_d("FDD-LTE Uplink band: %.2fMHz",rel_band);
            }
	    else if(arfcn >= 2750 && arfcn <= 3449){
                //rel_band = 1710 + 0.1f * (arfcn - 19200);
                band = FDD_LTE_BAND_7;
                //log_d("FDD-LTE Uplink band: %.2fMHz",rel_band);
            }
            else if(arfcn >= 3450 && arfcn <= 3799){
                //rel_band = 925 + 0.1f * (arfcn - 3450);
                band = FDD_LTE_BAND_8;
                //log_d("FDD-LTE Downlink band: %.2fMHz",rel_band);
            }
            else if(arfcn >= 21450 && arfcn <= 21799){
                //rel_band = 880 + 0.1f * (arfcn - 21450);
                band = FDD_LTE_BAND_8;
                //log_d("FDD-LTE Uplink band: %.2fMHz",rel_band);
            }
	    else if(arfcn >= 6150 && arfcn <= 6449){
                //rel_band = 1710 + 0.1f * (arfcn - 19200);
                band = FDD_LTE_BAND_20;
                //log_d("FDD-LTE Uplink band: %.2fMHz",rel_band);
            }
            else{
                band = UNKNOWN_BAND;
            }
            break;
        default:
            band = UNKNOWN_BAND;
            break;
    }
    return band;
}
int check_modem_monsc(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p, *q;
	int arfcn = 0;
	int network = 0;
	int ret=0;
//^MONSC: LTE,460,01,450,7031502,107,8106,-102,-7,-75
	p = strstr(buf, pcmd->resp);
	if(p) {
		p += strlen(pcmd->resp);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;

		q = strsep(&p, ",");
		ret = sscanf(p, "%*[^,],%*[^,],%d", &arfcn);
		if(strcmp(q, "LTE")==0) {
			network = 3;
		}else if (strcmp(q, "WCDMA")==0){
			network = 2;
		}else {
			network = 1;
		}

		if(ret >0 && arfcn >= 0) { 
			LOG_IN("arfcn:%d", arfcn);
			info->band = get_band_by_arfcn(network, arfcn);
			LOG_IN("band:%d", info->band);
		}
	}

	return ERR_AT_OK;
}

int check_pls8_connection_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL;
	int status = 0;
	
	if (!buf) {
		return ERR_AT_NAK;	
	}

	p = strstr(buf, pcmd->resp);
	if(p) {
		p = strchr(buf,',' ) +1;
		if (p){
			status = atoi(p);
			if (1 == status) {
				return ERR_AT_OK;	
			}else {
				return ERR_AT_NAK;	
			}
		}
	}else {
		return ERR_AT_NAK;	
	}
	return ERR_AT_NAK;	
}

int check_wpd200_rssi_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *pos = NULL;
	int rssi = 0;
	SRVCELL_INFO cell = {0};
	
	if (NULL == buf){
		LOG_WA("Cant't get network state!");
		return ERR_AT_NAK;
	}
	
	pos = strstr(buf, pcmd->resp);
	if (pos)
	pos += strlen(pcmd->resp);
	pos = strchr(pos, ',') + 1;
	pos = strchr(pos, ',') + 1;
	pos = strchr(pos, ',') + 1;
	pos = strchr(pos, ',') + 1;
	pos = strchr(pos, ',') + 2;
	
	rssi = -atoi(pos) + RSSI2RXLEV_SCALE;
	STR_FORMAT(cell.rssi, "%d", LIMIT(rssi, -110, -48));

	update_srvcell_info(&cell, BIT_RSSI);

	return info->siglevel <=0 ? ERR_AT_NAK : ERR_AT_OK;
}

int check_wpd200_net_code(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL;
	char *number = NULL;

	p = strstr(buf, pcmd->resp);
	if(p) {
		p = strchr(p, ':') +1;
		number = strtok(p, "\r");
		if (number) {
			if (pcmd->index==AT_CMD_MCC){
				bzero(info->carrier_str , sizeof(info->carrier_str));
				info->carrier_str[0] = '\0';
				sprintf(info->carrier_str, "%s", number);
			}else {
				strcat(info->carrier_str, number);
				LOG_IN("Carrier_code:%s", info->carrier_str);
			}
		}
	} else {
		bzero(info->carrier_str , sizeof(info->carrier_str));
		info->carrier_str[0] = '\0';
	}

	return ERR_AT_OK;
}

int check_ps_attachment_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL;
	int result = -1;
	p = strstr(buf, pcmd->resp);
	if(p) {
		p = strchr(p, ':') + 1;
		if(!p) {
			return ERR_AT_NAK;
		}
		
		result = atoi(p);
		if (1 == result){	
		//	LOG_IN("Attach ps domain successful!");
			return ERR_AT_OK;
		}else {
			return ERR_AT_NAK;
		}
			
	}

	return ERR_AT_NAK;
}

int check_fb78_signal_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p, *q; 
	char *rat, *rsrp, *sinr, *rsrq;
	int i;
	SRVCELL_INFO cell = {0};
	
	if(!pcmd || !pcmd->resp || !buf){
		return ERR_AT_NAK;
	}	
	p = strstr(buf, pcmd->resp);
	if(p) {	
		p += strlen(pcmd->resp);
		while(*p!=0 && *p==' ')p++;
		/* mode = 2*/	
		if(!p || *p!='2'){
			return ERR_AT_NAK;
		}
		q = strsep(&p, ",");
		if(!q){
			return ERR_AT_NAK;
		}
		rat = q + 3;
		/* rat must be 4*/
		if(!rat || *rat!='4'){
			return ERR_AT_NAK;
		}	
		/* svc */
		q = strsep(&p, ",");
		/* mcc */
		q = strsep(&p, ",");
		/* mnc */
		q = strsep(&p, "\r\n");
		/* rsrp */
		for(i = 0; i < 11; i++){
			q = strsep(&p, ",");
			if(!q){
				return ERR_AT_NAK;
			}
		}	
		rsrp = q;
		/* rsrq */
		rsrq = strsep(&p, ",");
		/* sinr */
		sinr = strsep(&p, ",");
		
		if(rsrp && (atoi(rsrp) != 255)){
			STR_FORMAT(cell.rsrp, "%d", atoi(rsrp) - 141);
		}
		
		if(rsrq && (atoi(rsrq) != 255)){
			STR_FORMAT(cell.rsrq, "%0.1f", atoi(rsrq)*0.5f - 20.0f);
		}

		if(sinr){
			STR_FORMAT(cell.rsrp, "%s", sinr);
		}

		update_srvcell_info(&cell, BIT_RSRP|BIT_RSRQ|BIT_SINR);
	}

	return ERR_AT_OK;
}

int check_lp41_connection_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL;
	int result = -1;
	p = strstr(buf, pcmd->resp);
	if(p) {
		p = strchr(p, ':') + 1;
		if(!p) {
			return ERR_AT_NAK;
		}
		
		p = strchr(p,',') + 1;
		if (!p) {
			return ERR_AT_NAK;
		}
		result = atoi(p);
		if (1 == result){	
			return ERR_AT_OK;
		}else {
			return ERR_AT_NAK;
		}
	}

	return ERR_AT_NAK;
}

int check_lp41_ipaddr_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL;
	char *q = NULL;
	char *cur_ipaddr = NULL;
	VIF_INFO *pinfo = NULL;

	p = strstr(buf, pcmd->resp);
	if(p) {
		p = strchr(p, '"');
		if(!p) {
			return ERR_AT_NAK;
		}

		p ++;
		
		if (strchr(p, ',')) {
			q = strsep(&p, ",");	
		}else {
			q = strsep(&p, "\"");	
		}
		if (!q) {
			return ERR_AT_NAK;
		}
	//	LOG_IN("ip in modem : %s", q);
		
		pinfo = my_get_reidal_vif_info();
		if (!pinfo) {
			return ERR_AT_NAK;
		}

		cur_ipaddr = inet_ntoa(pinfo->local_ip);
		
	//	LOG_IN("current ip : %s", q);

		if (!cur_ipaddr) {
			return ERR_AT_NAK;
		}

		if(strcmp(cur_ipaddr, q)==0) {
			return ERR_AT_OK;
		}else {
			LOG_WA("IP address was changed , the newest IP is %s, while we are using %s", q, cur_ipaddr);
			return ERR_AT_NAK;
		}
	}

	return ERR_AT_NAK;
}

int check_lp41_signal_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *pos = NULL;
	int rssi = 0;
	int rsrp = 0;
	SRVCELL_INFO cell = {0};
	
	if (NULL == buf){
		LOG_WA("Cant't get network signal status!");
		return ERR_AT_NAK;
	}
	
	pos = strstr(buf, pcmd->resp);
	if (!pos) {
		return ERR_AT_NAK;
	}
	
	pos = strstr(buf, "RSRP");
	if (pos) {
		while (pos && *pos != ',' && (*pos < '0' || *pos > '9')) pos++;
		if (*pos == ',') {
			rsrp = 0;
		}else {
			rsrp = -atoi(pos);
			STR_FORMAT(cell.rsrp, "%d", rsrp);
		}	
	}else {
		rsrp = 0;
	}

	pos = strstr(buf, "RSSI");
	if (pos) {
		while (pos && *pos != '\n' && (*pos < '0' || *pos > '9')) pos++;
		if (!pos || *pos == '\n') {
			rssi = 0;
		}else {
			rssi = -atoi(pos) + RSSI2RXLEV_SCALE;
			STR_FORMAT(cell.rsrp, "%d", LIMIT(rssi, -110, -48));
		}
	}else {
		rssi = 0;
	}

	update_srvcell_info(&cell, BIT_RSSI|BIT_RSRP);

	return info->siglevel <= 0 ? ERR_AT_NAK : ERR_AT_OK;
}

int check_ecm_connection_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL;
	int result = -1;
	p = strstr(buf, pcmd->resp);
	if(p) {
		p = strchr(p, ':');
		if(!p) {
			return ERR_AT_NAK;
		}

		p++;	

		while(*p!=0 && *p==' ') p++;
		if (!p) {
			return ERR_AT_NAK;
		}

		result = atoi(p);
		if (1 == result){	
			return ERR_AT_OK;
		}else {
			return ERR_AT_NAK;
		}
	}

	return ERR_AT_NAK;
}

int check_check_sim_iccid(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info) 
{
	char *p = NULL, *q = NULL;

	p=buf;

	if (pcmd->resp) {
		p = strstr(buf, pcmd->resp);
	}

	while(p && *p) {
		if(*p>='0' && *p<='9')
			break;
		p++;
	}
	
	if (!p) {
		  return ERR_AT_NAK;
	}
	
	if (gl_modem_code==IH_FEATURE_MODEM_ELS31) {
		q = strsep(&p, "\"");
	}else {
		q = strsep(&p, "\r\n");
	}

	if (!q) {
		  return ERR_AT_NAK;
	}

	LOG_IN("modem iccid: %s", q);

	if (strlen(q) < 16) { //should be 19 or 20
		  return ERR_AT_NAK;
	}	

	strlcpy(info->iccid, q, sizeof(info->iccid));

	return ERR_AT_OK;
}

int check_els_sica_connection_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info) 
{
	char *p = NULL; 

	p=buf;

	p = strstr(buf, pcmd->resp);

	if (!p) {
		  return ERR_AT_NAK;
	}

	if (strstr(p, "3,1")) {
		  return ERR_AT_OK;
	}

	return ERR_AT_NAK;
}

int check_modem_apn_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info) 
{
	char apn_buf[512] = {0};
	char tmp_buf[64] = {0};
	char *pos = NULL;
	char *apn = NULL;

	int cid = -1;

	pos = buf;

	while(pos){
		if (!pos){
			break;
		}

		pos = strstr(pos, "CGDCONT:");
		if (!pos){
			break;
		}
		
		pos += strlen("CGDCONT:");
		
		while (pos && *pos ==' ') {
			pos++;	
		}

		cid = atoi(pos);
		if (cid < 0 || cid > MAX_NUMBER_PROFILES) {
			break;	
		}

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

		pos++;

		apn = strsep(&pos, "\"");
		if (!apn || ('\0' == *apn)) {
			cid = -1;
			continue;
		}
		
		snprintf(tmp_buf, sizeof(tmp_buf), "[%d: %s]", cid, apn);
		strlcat(apn_buf+strlen(apn_buf), tmp_buf, sizeof(apn_buf));
		cid = -1;
		apn = NULL;
	};

	//syslog(LOG_INFO, "apn status:%s", apn_buf);
	if(apn_buf[0]) {
		snprintf(info->apns, sizeof(info->apns), "%s", apn_buf);
	}else {
		bzero(info->apns, sizeof(info->apns));
		return ERR_AT_NAK;
	}

	return ERR_AT_OK;
}

int check_modem_cgpaddr_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info) 
{
	char *pos = NULL;

	pos = strstr(buf, pcmd->resp);
	if (!pos) {
		  return ERR_AT_NAK;
	}

	if (pcmd->index==AT_CMD_CGPADDR3
			|| pcmd->index==AT_CMD_CGPADDR2) {
		pos = strchr(pos, '\"');
		if (!pos) {
			return ERR_AT_NAK;
		}

		pos++;
		if (*pos=='\"') {
			return ERR_AT_NAK;
		}
		//TODO: confirm if the IP address is valid.
		return ERR_AT_OK;
	}
	 
	return ERR_AT_OK;
}

int check_modem_cgpaddr(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	int cid = -1;
	char *p = NULL;
	char ip_s[64] = {0};
	char ipv4_s[16] = {0};
	char ipv6_s[64] = {0};
	VIF_INFO *pinfo = &(gl_myinfo.svcinfo.vif_info);

	if(!pcmd){
		return ERR_INVAL;
	}

	//+CGPADDR: 1,"10.83.237.147","36.8.132.105.3.0.13.212.23.92.135.164.184.168.39.99"
	//+CGPADDR: 1,"10.130.113.53,36.8.132.105.3.80.86.243.23.93.93.2.66.79.157.154"
	//+CGPADDR: 1,"10.130.113.53"
	//+CGPADDR: 1,"36.8.132.105.3.80.86.243.23.93.93.2.66.79.157.154"
	p = strstr(buf, pcmd->resp);
	if(p){
		p += strlen(pcmd->resp);
		/*if have ' ', eat it.*/
		while(*p!=0 && *p==' ') p++;

		if(sscanf(p, "%d,\"%[0-9.],%[0-9.]\"", &cid, ipv4_s, ipv6_s) == 3 && cid == gl.default_cid){
			goto leave;
		}

		memset(ipv4_s, 0, sizeof(ipv4_s));
		memset(ipv6_s, 0, sizeof(ipv6_s));
		if(sscanf(p, "%d,\"%[0-9.]\",\"%[0-9.]\"", &cid, ipv4_s, ipv6_s) == 3 && cid == gl.default_cid){
			goto leave;
		}

		memset(ipv4_s, 0, sizeof(ipv4_s));
		memset(ipv6_s, 0, sizeof(ipv6_s));
		if(sscanf(p, "%d,\"%[0-9.]\"", &cid, ip_s) == 2 && cid == gl.default_cid){
			if(strlen(ip_s) > 16){
				strlcpy(ipv6_s, ip_s, sizeof(ipv6_s));
			}else{
				strlcpy(ipv4_s, ip_s, sizeof(ipv4_s));
			}
			goto leave;
		}

	}

	return ERR_AT_NOVALIDIP;

leave:
	if(VIF_IS_LINK(pinfo->status)){
		if(ipv4_s[0] && strcmp(inet_ntoa(pinfo->local_ip), ipv4_s)){
			LOG_IN("IP address was changed, we are using %s, the newest IP is %s", inet_ntoa(pinfo->local_ip), ipv4_s);
			return ERR_AT_NOVALIDIP;
		}else if(ipv6_s[0] && strncmp(ipv6_s, "0.0.0.0", 7)){
			//TODO: ipv6
		}
	}else if((ipv4_s[0] && strncmp(ipv4_s, "0.0.0.0", 7)) || (ipv6_s[0] && strncmp(ipv6_s, "0.0.0.0", 7))){
		//do nothing
	}else{
		return ERR_AT_NOVALIDIP;
	}

	return ERR_AT_OK;
}

int check_ublox_umnoconf(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info) 
{
	char *pos = NULL;

	if (!pcmd->resp) {
		return ERR_AT_NAK;
	}

	pos = strstr(buf, pcmd->resp);
	if (!pos) {
		  return ERR_AT_NAK;
	}

	return ERR_AT_OK;
}

int check_modem_ssrvset_actsrvset_10(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL;
	int status = 0;
	char rsp[64] = {0};

	if (!buf) {
		goto set_ssrvset;
	}

	p = strstr(buf, pcmd->resp);
	if (p) {
		p += strlen(pcmd->resp);	
		while(*p!=0 && *p==' ') p++;
		if (p){
			status = atoi(p);
			if (10 == status) {
				return ERR_AT_OK;	
			}else {
				goto set_ssrvset;
			}
		}
	} else {
		goto set_ssrvset;
	}

set_ssrvset:
	send_at_cmd_sync2("AT^SSRVSET=\"actSrvSet\", 10\r\n", rsp, sizeof(rsp), 1);

	return ERR_AT_NEED_RESET;	
}

int get_modem_preferred_operator_list(char *dev, int verbose)
{
	FILE *fp = NULL;
	int fd, ret = 0;
	char rsp[1024] = {0};

	if(!dev || !dev[0]){
		return -1;
	}

	fp = fopen(MODEM_PREFERRED_OPERATOR_LIST_FILE, "w");
	if(!fp) return -1;

	fd = open(dev, O_RDWR|O_NONBLOCK, 0666);
	if(fd < 0){
		if(fp){
			fclose(fp);
		}
		return -1;
	}

	/*flush auto-report data*/
	flush_fd(fd);

	write_fd(fd, "AT+CPOL?\r\n", strlen("AT+CPOL?\r\n"), 1000, verbose);
retry:
	bzero(rsp, sizeof(rsp));
	check_return2(fd, rsp, sizeof(rsp), "OK", 2000, verbose);

	if(!rsp[0] || strstr(rsp, "ERROR")){
		//error occurred, may no at response, need pin code or no sim card
		ret -= 1;
	}

	fprintf(fp, "%s", rsp);
	if(strlen(rsp) == (sizeof(rsp) - 1)){
		goto retry;
	}

	fclose(fp);

	close(fd);

	return ret;
}

int check_modem_ctzu_status(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL;

	if(!buf || !pcmd){
		return -1;
	}

	p = strstr(buf, pcmd->resp);
	if (p) {
		p += strlen(pcmd->resp);	
		if(atoi(p)){
			return 1;
		}
	}
		
	return 0;
}

int enable_ipv4v6_for_tmobile_profile(char *dev, int verbose)
{
	int ret = 0;
	AT_CMD *pcmd = NULL;

	if(!dev || !dev[0]){
		return -1;
	}

	if(ih_key_support("NRQ1") && gl_modem_code == IH_FEATURE_MODEM_RM520N){
		pcmd = find_at_cmd(AT_CMD_QCPRFMOD);
		if(pcmd){
			ret = send_at_cmd_sync3(dev, pcmd->atc, NULL, 0, verbose);
		}
	}

	return ret;	
}

int enable_modem_auto_time_zone_update(char *dev, int verbose)
{
	int ret = 0;
	char buf[64] = {0};
	AT_CMD *pcmd = NULL;

	if(!dev || !dev[0]){
		return -1;
	}

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
		|| gl_modem_code == IH_FEATURE_MODEM_LE910){

		pcmd = find_at_cmd(AT_CMD_CTZU);
		if(pcmd){
			ret = send_at_cmd_sync3(dev, pcmd->atc, buf, sizeof(buf), verbose);
			if((ret == 0) && (pcmd->at_cmd_handle(pcmd, buf, 0, NULL, NULL) <= 0)){
				send_at_cmd_sync(dev, "AT+CTZU=1\r\n", NULL, 0, verbose);
			}
		}
	}

	return ret;	
}

int check_modem_nr5g_disable_mode(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	char *p = NULL;
	int nr5g_mode = NR5G_MODE_AUTO;
	char cmd_str[128] = {0};
	char *nr5g_mode_str[] = {"NSA/SA", "NSA", "SA"};
	int network_type = 0;

	if(!pcmd || !buf || !config){
		return ERR_AT_NAK;
	}

	network_type = (info->current_sim == SIM2) ? config->network_type2 : config->network_type;

	p = strstr(buf, pcmd->resp);
	if(p){
		strsep(&p, ",");
		if(!p) return ERR_AT_NAK;
		
		//nr5g mode mapping	
		switch(network_type){
			case MODEM_NETWORK_AUTO:
			case MODEM_NETWORK_5G4G:
				nr5g_mode = config->nr5g_mode[info->current_sim == SIM2 ? 1 : 0];
				break;
			case MODEM_NETWORK_5G: //5G-SA
			default:
				nr5g_mode = NR5G_MODE_AUTO;
				break;
		}

		if(nr5g_mode != atoi(p)){
			LOG_IN("change nr5g mode to %s", nr5g_mode_str[nr5g_mode]);

			snprintf(cmd_str, sizeof(cmd_str), "AT+QNWPREFCFG=\"nr5g_disable_mode\",%d\r\n", nr5g_mode);
			send_at_cmd_sync_timeout(gl_commport, cmd_str, NULL, 0, ppp_debug(), "OK", 3000);	
		}
	}
	
	return ERR_AT_OK;
}

int check_nr5g_mode(char *dev, int verbose)
{
	int ret = 0;
	char buf[128] = {0};
	AT_CMD *pcmd = NULL;
	MODEM_CONFIG *modem_config = get_modem_config();

	if(!dev || !dev[0]){
		return -1;
	}

	if(gl_modem_code == IH_FEATURE_MODEM_RM500
		|| gl_modem_code == IH_FEATURE_MODEM_RM520N
		|| gl_modem_code == IH_FEATURE_MODEM_RG500){
		pcmd = find_at_cmd(AT_CMD_QNR5G_MODE);
	}

	if(pcmd){
		ret = send_at_cmd_sync(dev, pcmd->atc, buf, sizeof(buf), verbose);
		if((ret == 0) && pcmd->at_cmd_handle){
			return pcmd->at_cmd_handle(pcmd, buf, 0, modem_config, &gl_myinfo.svcinfo.modem_info);
		}
	}

	return ret;
}

int check_modem_cclk_update_time(AT_CMD *pcmd, char *buf, int retry, MODEM_CONFIG *config, MODEM_INFO *info)
{
	int ret = -1;
	char *p = NULL;
	char s[128] = {0};
	char dir[8] = {0};
	time_t tm_old, tm_now, diff;
	int year, mon, day, hour, min, sec, zone, dst;

	if(!buf || !pcmd || !pcmd->resp[0]){
		return -1;
	}

	//+CCLK: "20/07/07,08:09:56+32"
	p = strstr(buf, pcmd->resp);
	if(p){
		p += strlen(pcmd->resp);	
		if(strstr(p, "\"")){
			strsep(&p, "\"");
			p = strsep(&p, "\"");
		}
		
		if(!p || sscanf(p, "%d/%d/%d,%d:%d:%d%[-+]%d,%d", &year, &mon, &day, &hour, &min, &sec, dir, &zone, &dst) < 6){
			LOG_DB("failed to parse command(%s) response.", pcmd->atc);
			goto err;
		}
		
		//check validation of time
		if(year < 1900){
			//80 means 1980 or 2080, 1980 may be the module's factory delivery value
			if(year > 79){
				LOG_DB("the time of year is invalid.");
				goto err;
			}
			year += 2000;
		}
		
		//year must be equal to or greater than 2020(current year)
		if(year < 2022){
			LOG_DB("the time of year is invalid.");
			goto err;
		}
		
		tm_old = time(0);
		snprintf(s, sizeof(s), "date -u -s %d.%d.%d-%d:%d:%d", year, mon, day, hour, min, sec);
		system(s);
		
		tm_now = time(0);
		diff = tm_now - tm_old;
		if(diff != 0){
			saveRtc();
			strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S %z", localtime(&tm_now));
			syslog(LOG_INFO, "time updated: %s [%s%lds]", s, diff > 0 ? "+" : "", diff);
			return 1;
		}else{
			strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S %z", localtime(&tm_now));
			syslog(LOG_INFO, "current time is %s.", s);
			return 0;
		}
	}

err:	
	if(ret < 0){
		LOG_DB("failed to update system time, command(%s) response:%s", pcmd->atc, buf);
	}

	return ret;
}

