/*
 * $Id$ --
 *
 *   LED routines
 *
 * Copyright (c) 2001-2010 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 06/04/2010
 * Author: Jianliang Zhang
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "build_info.h"
#include "shared.h"
#include "ih_types.h"
#include "license.h"
#include "strings.h"
#include "files.h"
#include "ledcon.h"
#include "ih_logtrace.h"

#define strcasecmp stricmp

/* Convert GPIO signal to GPIO pin number */
#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

#if 1
#ifdef INHAND_IP812
LED_GPIO_LIST led_list[]={
	{LED_WARN,"warn",0},
	{LED_STATUS,"status",0},
	{LED_ERROR,"error",0},
	{LED_VPN,"vpn",0},
	{LED_WLAN,"wlan",0},
	{LED_LEVEL0,"level0",0},
	{LED_LEVEL1,"level1",0},
	{LED_LEVEL2,"level2",0},
	{LED_WWAN,"wwan",0}
};
#elif (defined INHAND_IDTU9 || defined INHAND_IG5)
LED_GPIO_LIST led_list[]={
	{LED_STATUS,"status",0},
	{LED_WARN,"warn",0},
	{LED_ERROR,"warn",0},	//IDTU9 ERROR and WARN are the same one
	{LED_NET,"net",0}
};
LED_GPIO_LIST ext_led_list[]={};
#elif defined (INHAND_IG9)
LED_GPIO_LIST led_list[]={
	{LED_WARN,"warn",0},
	{LED_STATUS,"status",0},
	{LED_ERROR,"error",0},
	{LED_WLAN,"wlan",0},
	{LED_LEVEL0,"level0",0},
	{LED_LEVEL1,"level1",0},
	{LED_LEVEL2,"level2",0},
	{LED_WWAN,"wwan",0},
	{LED_USR1,"usr1",0},
	{LED_USR2,"usr2",0},
	{LED_USR3,"app",0},
	{LED_GNSS,"gnss",0},
	{LED_TF,"tf",0}
};

LED_GPIO_LIST ext_led_list[]={};
#elif defined(INHAND_VG9)
LED_GPIO_LIST led_list[]={
	{LED_WARN,"system_blue",0},
	{LED_STATUS,"system_green",0},
	{LED_ERROR,"system_red",0},
	{LED_VPN,"u2_green",0},
	{LED_WLAN,"u2_yellow",0},
	{LED_LEVEL0,"signal_red",0},
	{LED_LEVEL1,"signal_blue",0},
	{LED_LEVEL2,"signal_green",0},
	{LED_CELL_R,"cellular_red",0},
	{LED_CELL_G,"cellular_green",0},
	{LED_CELL_B,"cellular_blue",0},
	{LED_USR1,"u1_red",0},
	{LED_USR2,"u2_red",0},
	{LED_USR3,"u1_green",0},
	{LED_GNSS,"gnss_green",0}
};

LED_GPIO_LIST ext_led_list[]={};
#elif defined(INHAND_IR8)
LED_GPIO_LIST led_list[]={
	{LED_WARN,"system_green",0},
	{LED_STATUS,"system_blue",0},
	{LED_ERROR,"system_red",0},
	{LED_NETWORK_R,"network_red",0},
	{LED_NETWORK_G,"network_green",0},
	{LED_NETWORK_B,"network_blue",0},
};

LED_GPIO_LIST ext_led_list[]={};
#elif defined(INHAND_ER6)
LED_GPIO_LIST led_list[]={
	{LED_WARN,"system:yellow",0},
	{LED_STATUS,"system:green",0},
	{LED_ERROR,"system:red",0},
	{LED_NETWORK_R,"network:red",0},
	{LED_NETWORK_G,"network:green",0},
	{LED_NETWORK_Y,"network:yellow",0},
	{LED_LEVEL0,"signal:red",0},
	{LED_LEVEL1,"signal:yellow",0},
	{LED_LEVEL2,"signal:green",0},
};

LED_GPIO_LIST ext_led_list[]={};
#elif defined(INHAND_ER3)
LED_GPIO_LIST led_list[]={
	{LED_WARN,"system_blue",0},
	{LED_STATUS,"system_green",0},
	{LED_ERROR,"system_red",0},
	{LED_LEVEL0,"signal_red",0},
	{LED_LEVEL1,"signal_blue",0},
	{LED_LEVEL2,"signal_green",0},
	{LED_CELL_R,"cellular_red",0},
	{LED_CELL_G,"cellular_green",0},
	{LED_CELL_B,"cellular_blue",0},
	{LED_ENABLE,"status_led",0}
};

LED_GPIO_LIST ext_led_list[]={};
#else
LED_GPIO_LIST led_list[]={
	{LED_WARN,"warn",0},
	{LED_STATUS,"status",0},
	{LED_ERROR,"error",0},
	{LED_VPN,"vpn",0},
	{LED_WLAN,"wlan",0},
	{LED_LEVEL0,"level0",0},
	{LED_LEVEL1,"level1",0},
	{LED_LEVEL2,"level2",0}
};

LED_GPIO_LIST ext_led_list[]={
	{LED_WARN,"ext_warn", 0},
	{LED_STATUS,"ext_status",0},
	{LED_ERROR,"error",0},
	{LED_VPN,"vpn", 0},
	{LED_WLAN,"wlan", 0},
	{LED_LEVEL0,"ext_level0", 0},
	{LED_LEVEL1,"ext_level1", 0},
	{LED_LEVEL2,"ext_level2", 0}
};

#endif
#else
LED_GPIO_LIST led_list[]={
	{LED_USR1,"usr1",0},
	{LED_USR2,"usr2",0},
	{LED_USR3,"usr3",0},
	{LED_USR4,"usr4",0}
};
#endif

#if 1
#ifdef INHAND_IR8_LEGACY
LED_GPIO_LIST gpio_list[]={
	{GPIO_MPOWER,"mpower", 13},
	{GPIO_MRST,"mrst", 11},
	{GPIO_SIM_SWITCH,"simswitch", 12},
	{GPIO_RESET,"reset", 39},
	{GPIO_SSD,"ssdpower", 14},
};
#else
#ifdef INHAND_IP812
LED_GPIO_LIST gpio_list[]={
	{GPIO_MPOWER,"mpower", 13},
	{GPIO_MRST,"mrst", 11},
	{GPIO_SIM_SWITCH,"simswitch", 74},
	{GPIO_RESET,"reset", 32},
	{GPIO_SHUTDOWN,"shutdown", 33},
	{GPIO_SSD,"ssdpower", 14},
};
#elif defined(INHAND_IG9)
LED_GPIO_LIST gpio_list[]={
	{GPIO_MPOWER,"mpower", 488},
	{GPIO_MRST,"mrst", 489},
	{GPIO_SIM_SWITCH,"simswitch", 490},
	{GPIO_RESET,"reset",GPIO_TO_PIN(2,4)}
};
#elif defined(INHAND_VG9)
LED_GPIO_LIST gpio_list[]={
	{GPIO_MPOWER,"mpower",63},
	{GPIO_MRST,"mrst", 61},
	{GPIO_SIM_SWITCH,"simswitch",62},
	{GPIO_RESET,"reset",18},
};
#elif defined(INHAND_IR8)
LED_GPIO_LIST gpio_list[]={
	{GPIO_MPOWER,"mpower",63},
	{GPIO_MRST,"mrst", 61},
	{GPIO_SIM_SWITCH,"simswitch",62},
	{GPIO_RESET,"reset",18},
};
#elif defined(INHAND_ER6)
LED_GPIO_LIST gpio_list[]={
	{GPIO_MPOWER,"mpower", 13},
	{GPIO_MRST,"mrst", 14},
	{GPIO_SIM_SWITCH,"simswitch", 15},
	{GPIO_RESET,"reset", 18},
};
#elif defined(INHAND_WR3)
LED_GPIO_LIST gpio_list[]={
	{GPIO_MPOWER,"mpower", 8},
	{GPIO_MRST,"mrst", 9},
	{GPIO_MCONTRL,"mcontrl", 134},
	{GPIO_MIDLE,"midle", 3},
	{GPIO_MAIRPLANE,"mairplane", 4},
	{GPIO_SIM_SWITCH,"simswitch", 1},
	{GPIO_MCUPOWER,"mcupower", 2},
	{GPIO_MANTSEL,"mantselect", 49},
	{GPIO_ESPPOWER,"esp32power", 129},
	{GPIO_ESPCONTROL,"esp32control", 128},
	{GPIO_ESPANTSEL,"esp32antselect", 48},
	{GPIO_WIFI_FG,"wifioverflowflag", 50},
	{GPIO_5G_FG,"5goverflowflag", 51}
};
#elif defined(INHAND_ER3)
//FIXME ER3 how
LED_GPIO_LIST gpio_list[]={
	{},{},{},
	{GPIO_RESET,"reset", 7},
};
#else
LED_GPIO_LIST gpio_list[]={
	{GPIO_MPOWER,"mpower",GPIO_TO_PIN(3,21)},
	{GPIO_MRST,"mrst",GPIO_TO_PIN(3,20)},
	{GPIO_SIM_SWITCH,"simswitch",GPIO_TO_PIN(2,23)},
	{GPIO_RESET,"reset",GPIO_TO_PIN(0,20)},
	{GPIO_PHY1RST,"phy1rst",GPIO_TO_PIN(3,18)},
	{GPIO_PHY2RST,"phy2rst",GPIO_TO_PIN(3,19)},
	{GPIO_232_485,"232-485",GPIO_TO_PIN(0,7)}
};
#endif
#endif
#else
LED_GPIO_LIST gpio_list[]={
	{0,NULL,0}
};
#endif

int write_sys_file(char* filename, int value_int, char* value_string, int format)
{
	FILE *file = NULL;
	
	file = fopen(filename,"w");
	if(file == NULL){
		printf("Write file open %s error\n",filename);
		return 0;
	}
	if(format == FORMAT_INT) fprintf(file,"%d",value_int);
	else if(format == FORMAT_STRING) fprintf(file,"%s",value_string);
	fclose(file);
	return 0;	
}

int read_sys_file(char* filename)
{
	FILE *file = NULL;
	int value;

	if((file=fopen(filename,"r"))==NULL){
		printf("Read sys file open %s error\n",filename);
		return -1;
	}
	fscanf(file, "%d", &value);
	fclose(file);

	return value;	
}

BOOL is_pca9554_ctrl_led(int led) 
{
	if (led==LED_WARN
		//|| led==LED_ERROR
		|| led==LED_LEVEL0
		|| led==LED_LEVEL1
		|| led==LED_LEVEL2
		|| led==LED_STATUS) {
		return TRUE;
	}

	return FALSE;
}

#ifdef INHAND_ER3
/*whilt set  "AT+GTTESTMODE=1", don't ledcon handle*/
#define FIBO_TEST_MODE_FILE_PATH "/tmp/fibo_testmode_switch"
#define TESTMODE_ON "on"
#define TESTMODE_OFF "off"

 int check_fibocom_testmode(void)
{
	FILE *fp;
	char sta[3+1]={0};
	int mode =0;

	if( (fp=fopen(FIBO_TEST_MODE_FILE_PATH,"r")) == NULL ){
		goto error;
	}

	if(fgets(sta, 3, fp) == NULL){
		goto error;
	}

	if(!strncmp(sta,TESTMODE_ON,strlen(TESTMODE_ON)))
	{
		mode = 1;
	}else
		mode = 0;

	//printf("check test mode: %d\n",mode);
error:
	if(NULL != fp)
		fclose(fp);

	return mode;
}

#endif

/*
 *	function to set leds' status
 *	input:
 *		cmd	:	led command
 *		led	:	led id
 *	return:
 *		0	:	ok
 *		-1	:	error
 */
int ledcon_handle(int cmd, int led)
{
	int idx;
	char filename[256] = {0};
	int value = LED_VALUE_OFF;
	int read_value;
	int n_leds = 0;
	int i = 0;

	if((led<0)||(led>=LED_MAX)){
		return -1;
	}

	if (ih_license_support(IH_FEATURE_EXT_GPIO)) {
		n_leds = sizeof(ext_led_list)/sizeof(ext_led_list[0]);
	}else {
		n_leds = sizeof(led_list)/sizeof(led_list[0]);
	}

	for(i = 0; i < n_leds;i++){
		if (ih_license_support(IH_FEATURE_EXT_GPIO)) {
			if(ext_led_list[i].idx == led){
				idx = i;
				break;
			}
		}else{
			if(led_list[i].idx == led){
				idx = i;
				break;
			}
		}
	}

	if(i >= n_leds){
		return -1;
	}

	switch(cmd){
	case LEDMAN_CMD_ON:
	case LEDMAN_CMD_OFF:
		if (ih_license_support(IH_FEATURE_EXT_GPIO)) {
			sprintf(filename, SYS_LED_PATH "%s/brightness",ext_led_list[idx].name);
		}else {
			sprintf(filename, SYS_LED_PATH "%s/brightness",led_list[idx].name);
		}

		if(cmd == LEDMAN_CMD_ON){
			value = LED_VALUE_OFF;
			write_sys_file(filename, value, NULL,FORMAT_INT);
			value = LED_VALUE_ON;
			write_sys_file(filename, value, NULL,FORMAT_INT);
		}
		else if(cmd == LEDMAN_CMD_OFF) value = LED_VALUE_OFF;
		write_sys_file(filename, value, NULL,FORMAT_INT);
		break;
	case LEDMAN_CMD_FLASH:
	case LEDMAN_CMD_FLASH_FAST:
		if (ih_license_support(IH_FEATURE_EXT_GPIO)) {
			sprintf(filename, SYS_LED_PATH "%s/trigger", ext_led_list[idx].name);
		}else {
			sprintf(filename, SYS_LED_PATH "%s/trigger", led_list[idx].name);
		}
		write_sys_file(filename, 0, "timer", FORMAT_STRING);

		if (ih_license_support(IH_FEATURE_EXT_GPIO)) {
			sprintf(filename, SYS_LED_PATH "%s/delay_on", ext_led_list[idx].name);
		}else {
			sprintf(filename, SYS_LED_PATH "%s/delay_on", led_list[idx].name);
		}
		read_value = read_sys_file(filename);
		if(cmd == LEDMAN_CMD_FLASH) value = LED_VALUE_FLASH;
		else if(cmd == LEDMAN_CMD_FLASH_FAST) value = LED_VALUE_FLASH_FAST;
		if(read_value != value) write_sys_file(filename, value, NULL, FORMAT_INT);
	
		if (ih_license_support(IH_FEATURE_EXT_GPIO)) {
			sprintf(filename, SYS_LED_PATH "%s/delay_off", ext_led_list[idx].name);
		}else {
			sprintf(filename, SYS_LED_PATH "%s/delay_off", led_list[idx].name);
		}	
		read_value = read_sys_file(filename);
		if(read_value != value) write_sys_file(filename, value, NULL, FORMAT_INT);
		break;
	case LEDMAN_CMD_FLASH_OFF:
		if (ih_license_support(IH_FEATURE_EXT_GPIO)) {
			sprintf(filename, SYS_LED_PATH "%s/trigger", ext_led_list[idx].name);
		}else {
			sprintf(filename, SYS_LED_PATH "%s/trigger", led_list[idx].name);
		}
		write_sys_file(filename, 0, "none", FORMAT_STRING);
		break;
	default:
		break;
	}
	return 0;
}

/*
 *	function to set leds' status
 *	input:
 *		cmd	:	led command
 *		led	:	led id
 *	return:
 *		0	:	ok
 *		-1	:	error
 */
int ledcon(int cmd, int led)
{
#ifdef INHAND_ER3
	if (check_fibocom_testmode()) {
		syslog(LOG_INFO, "fibo test mode enabled");
		return -1;
	}
#endif

	return ledcon_handle(cmd, led);
}

int get_led_status(int led, int *status)
{
	int idx;
	char filename[256] = {0};
	int read_value = -1;

	if(!status){
		return -1;
	}	

	if((led<0)||(led>=LED_MAX)) return -1;
	else idx = led;

	/*the led is flash if this file is exist*/
	if (ih_license_support(IH_FEATURE_EXT_GPIO)) {
		sprintf(filename, SYS_LED_PATH "%s/delay_on", ext_led_list[idx].name);
	}else {
		sprintf(filename, SYS_LED_PATH "%s/delay_on", led_list[idx].name);
	}

	if(!access(filename, F_OK)){
		read_value = read_sys_file(filename);
		if(read_value == LED_VALUE_FLASH){
			*status =	LEDMAN_CMD_FLASH;
		}else if(read_value == LED_VALUE_FLASH_FAST){
			*status =	LEDMAN_CMD_FLASH_FAST;
		}

		return 0;
	}

	if (ih_license_support(IH_FEATURE_EXT_GPIO)) {
		sprintf(filename, SYS_LED_PATH "%s/brightness",ext_led_list[idx].name);
	}else {
		sprintf(filename, SYS_LED_PATH "%s/brightness",led_list[idx].name);
	}
	read_value = read_sys_file(filename);
	if(read_value == LED_VALUE_ON){
		*status =	LEDMAN_CMD_ON;
	}else if(read_value == LED_VALUE_OFF){
		*status =	LEDMAN_CMD_OFF;
	}

	return 0;
}

int system_ledcon(SYSTEM_LED_STATE state)
{
	switch(state){
		case SYSTEM_OFF:
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_OFF, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
			break;

		case SYSTEM_STARTING:
#if (defined INHAND_ER6 || defined INHAND_ER3)
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_OFF, LED_STATUS);
			ledcon(LEDMAN_CMD_ON, LED_ERROR);
#else
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_FLASH, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#endif
			break;

		case SYSTEM_STARTUP_FAULT:
#if (defined INHAND_ER6 || defined INHAND_ER3)
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_OFF, LED_STATUS);
			ledcon(LEDMAN_CMD_FLASH, LED_ERROR);
#else
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_OFF, LED_STATUS);
			ledcon(LEDMAN_CMD_ON, LED_ERROR);
#endif
			break;

		case SYSTEM_STARTUP_OK:
#if (defined INHAND_ER6)
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_FLASH, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#elif (defined INHAND_ER3)
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_ON, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#else
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_ON, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#endif
			break;

		case SYSTEM_CONFIG_RESET_BEGIN:
#if (defined INHAND_ER6 || defined INHAND_ER3)
			ledcon(LEDMAN_CMD_ON, LED_WARN);
			ledcon(LEDMAN_CMD_OFF, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#else
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_ON, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#endif
			break;

		case SYSTEM_CONFIG_RESET_STAGE1:
#if (defined INHAND_ER6 || defined INHAND_ER3)
			ledcon(LEDMAN_CMD_FLASH_FAST, LED_WARN);
			ledcon(LEDMAN_CMD_OFF, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#else
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_FLASH_FAST, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#endif
			break;

		case SYSTEM_CONFIG_RESET_STAGE2:
#if (defined INHAND_ER6 || defined INHAND_ER3)
			ledcon(LEDMAN_CMD_ON, LED_WARN);
			ledcon(LEDMAN_CMD_OFF, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#else
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_FLASH, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#endif
			break;

		case SYSTEM_CONFIG_RESET_OK:
			ledcon(LEDMAN_CMD_OFF, LED_WARN);
			ledcon(LEDMAN_CMD_OFF, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
			break;

		case SYSTEM_FW_UPGRADE:
			ledcon(LEDMAN_CMD_FLASH, LED_WARN);
			ledcon(LEDMAN_CMD_OFF, LED_STATUS);
			ledcon(LEDMAN_CMD_OFF, LED_ERROR);
			break;

		case SYSTEM_REBOOT:
			break;

		default:
			break;
	}

	return 0;
}

int cellular_ledcon(CELL_LED_STATE state)
{
	switch(state){
		case CELL_OFF:
			ledcon(LEDMAN_CMD_OFF, LED_CELL_R);
			ledcon(LEDMAN_CMD_OFF, LED_CELL_G);
			ledcon(LEDMAN_CMD_OFF, LED_CELL_B);
			break;

		case CELL_CONNECTING:
#if defined(INHAND_ER3)
			ledcon(LEDMAN_CMD_FLASH, LED_CELL_R);
#else
			ledcon(LEDMAN_CMD_ON, LED_CELL_R);
#endif
			ledcon(LEDMAN_CMD_OFF, LED_CELL_G);
			ledcon(LEDMAN_CMD_OFF, LED_CELL_B);
			break;

		case CELL_CONNECTING_FAULT:
			ledcon(LEDMAN_CMD_FLASH, LED_CELL_R);
			ledcon(LEDMAN_CMD_OFF, LED_CELL_G);
			ledcon(LEDMAN_CMD_OFF, LED_CELL_B);
			break;

		case CELL_CONNECTED:
			ledcon(LEDMAN_CMD_OFF, LED_CELL_R);
			ledcon(LEDMAN_CMD_OFF, LED_CELL_G);
			ledcon(LEDMAN_CMD_ON, LED_CELL_B);
			break;

		case CELL_CONNECTED_5G:
			ledcon(LEDMAN_CMD_OFF, LED_CELL_R);
			ledcon(LEDMAN_CMD_ON, LED_CELL_G);
			ledcon(LEDMAN_CMD_OFF, LED_CELL_B);
			break;

		default:
			break;
	}

	return 0;
}

int cellular_signal_ledcon(CELL_SIGNAL_LEVEL sig_level)
{
	switch(sig_level){
		case CELL_SIGNAL_NONE:

			ledcon(LEDMAN_CMD_OFF, LED_LEVEL0);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
			break;

		case CELL_SIGNAL_LEVEL1:
			ledcon(LEDMAN_CMD_ON, LED_LEVEL0);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL1);
			ledcon(LEDMAN_CMD_OFF, LED_LEVEL2);
			break;

		case CELL_SIGNAL_LEVEL2:
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

		case CELL_SIGNAL_LEVEL3:
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

		default:
			break;
	}

	return 0;
}

int led_warn_status = -1;
int led_err_status = -1;
int led_sys_status = -1;
void ledcon_upgrade_start(void)
{
#if (defined INHAND_VG9 || defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_ER3)
	get_led_status(LED_WARN, &led_warn_status);
	get_led_status(LED_ERROR, &led_err_status);
	get_led_status(LED_STATUS, &led_sys_status);
#if !(defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_ER3)
	ledcon(LEDMAN_CMD_OFF, LED_STATUS);
	ledcon(LEDMAN_CMD_OFF, LED_ERROR);
	ledcon(LEDMAN_CMD_FLASH, LED_WARN);
#else
	system_ledcon(SYSTEM_FW_UPGRADE);
#endif // INHAND_IR8
#else
	/*we should control warn and err leds to indicate upgrade progress*/
	get_led_status(LED_WARN, &led_warn_status);
	get_led_status(LED_ERROR, &led_err_status);
	ledcon(LEDMAN_CMD_FLASH, LED_WARN);
	ledcon(LEDMAN_CMD_FLASH, LED_ERROR);
#endif

	return;
}

void ledcon_upgrade_end(void)
{
#if (defined INHAND_VG9 || defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_ER3)
	ledcon(led_warn_status, LED_WARN);
	ledcon(led_err_status, LED_ERROR);
	ledcon(led_sys_status, LED_STATUS);
#else
	if(led_warn_status >= LEDMAN_CMD_ON 
		&& led_warn_status >= LEDMAN_CMD_ON){
		ledcon(led_warn_status, LED_WARN);
		ledcon(led_err_status, LED_ERROR);
	}
#endif

	return;
}

#ifdef INHAND_ER3

/*/sys/bus/platform/devices/10005000.pinctrl/mt_gpio
 * PIN:(MODE)(DIR)(DOUT)(DIN)(DRV)(SMT)(IES)(PULLEN)(PULLSEL)[R1 R0]
 * 000:0000001110
 * 001:0000001110
 * 002:0000001110
 * 003:0000001110
 *
 * get the value of gpio 1 by shell
 * grep 001 /sys/bus/platform/devices/10005000.pinctrl/mt_gpio | cut -b 8,8
 */

#define MT_GPIO_FILE	"/sys/bus/platform/devices/10005000.pinctrl/mt_gpio"
#define MT_GPIO_BASE_BITS	4	 //skip 3 GPIO bits and 1 ':' bit
#define MT_GPIO_MODE_BIT	(MT_GPIO_BASE_BITS + 0)
#define MT_GPIO_DIR_BIT		(MT_GPIO_BASE_BITS + 1)
#define MT_GPIO_DOUT_BIT	(MT_GPIO_BASE_BITS + 2)
#define MT_GPIO_DIN_BIT		(MT_GPIO_BASE_BITS + 3)

#define MT_GPIO_MODE_GPIO	'0'
#define MT_GPIO_DIR_IN		'0'
#define MT_GPIO_DIR_OUT		'1'

/*this function is used for ER3 (ODU2002, based on FG360) gpio set*/
static int _gpio_er3(int cmd, int gpio_idx, int *value)
{
	FILE *fp = NULL;
	char buff[256] = {0};
	char gpio_id_base[8] = {0};
	char bit = 0;
	int gpio_value;
	int ret = -1;

	BOOL gpio_dir_in = TRUE;
	BOOL flag = FALSE;

	fp = fopen(MT_GPIO_FILE, "r");
	if (!fp) {
		syslog(LOG_INFO, "open %s failed",  MT_GPIO_FILE);
		return -1;
	}

	snprintf(gpio_id_base, sizeof(gpio_id_base), "%03d:", gpio_idx);

	while(fgets(buff, sizeof(buff), fp)){
		if (strncmp(buff, gpio_id_base, strlen(gpio_id_base)) == 0) {
			flag = TRUE;
			break;
		}
	}

	fclose(fp);

	if (!flag) {
		syslog(LOG_INFO, "get gpio base [%s] failed",  gpio_id_base);
		return -1;
	}

	bit = buff[MT_GPIO_MODE_BIT];
	if (bit != MT_GPIO_MODE_GPIO) {
		syslog(LOG_INFO, "gpio [%s] is not in gpio mode",  gpio_id_base);
		//return -1;
	}

	bit = buff[MT_GPIO_DIR_BIT];
	if (bit != MT_GPIO_DIR_IN) {
		gpio_dir_in = FALSE;
	}

	syslog(LOG_INFO, "gpio [%s] is dir %s",  gpio_id_base, gpio_dir_in? "in": "out");

	switch(cmd){
	case GPIO_READ:
		if (gpio_dir_in) {
			bit = buff[MT_GPIO_DIN_BIT];
			if (bit == '1') {
				*value = 1;
			} else {
				*value = 0;
			}
			syslog(LOG_INFO, "gpio [%s] value is %d",  gpio_id_base, *value);
		}
		break;
	case GPIO_WRITE:
		if (gpio_dir_in) {
			syslog(LOG_INFO, "gpio [%s] don't set value to din",  gpio_id_base);
			break;
		}

		if(*value) {
			gpio_value = 1;
		} else {
			gpio_value = 0;
		}

		snprintf(buff, sizeof(buff), "echo out %d %d > %s", gpio_idx, gpio_value, MT_GPIO_FILE);
		ret = system(buff);
		syslog(LOG_INFO, "gpio [%s] set value is %d ret %d",  gpio_id_base, gpio_value, ret);

		break;
	default:
		break;
	}

	return 0;
}
#endif

/*
 *	function to set gpio
 *	input:
 *		cmd	:	gpio command
 *		idx	:	gpio idx
 *	return:
 *		0	:	ok
 *		-1	:	error
 */
int gpio(int cmd, int gpio_idx, int *value)
{
	int idx;
	char filename[256] = {0};
	int gpio_value;

	if((gpio_idx<0)||(gpio_idx>=GPIO_MAX)) return -1;
	else idx = gpio_idx;

#ifdef INHAND_ER3
	return _gpio_er3(cmd, gpio_list[idx].pin_idx, value);
#else

#if 0
	for(idx = 0;idx < sizeof(gpio_list)/sizeof(gpio_list[0]);idx++){
		if(gpio_list[idx].idx == gpio_idx) break;
	}
	if(idx >= sizeof(gpio_list)/sizeof(gpio_list[0])) return -1;
#endif
	switch(cmd){
	case GPIO_READ:
	case GPIO_WRITE:
#if 0
		sprintf(filename, SYS_GPIO_PATH "export");
		write_sys_file(filename, gpio_list[idx].pin_idx, NULL, FORMAT_INT);
		sprintf(filename, SYS_GPIO_PATH "gpio%d/direction",gpio_list[idx].pin_idx);
		if(cmd == GPIO_READ) write_sys_file(filename, 0, "in", FORMAT_STRING);
		else if(cmd == GPIO_WRITE) write_sys_file(filename, 0, "out", FORMAT_STRING);
#endif
		
		sprintf(filename, SYS_GPIO_PATH "gpio%d/value",gpio_list[idx].pin_idx);
		if(cmd == GPIO_READ) *value = read_sys_file(filename);
		else if(cmd == GPIO_WRITE){
			if(*value) gpio_value = 1;
			else gpio_value = 0;
			write_sys_file(filename, gpio_value, NULL, FORMAT_INT);
		}	
#if 0
		sprintf(filename, SYS_GPIO_PATH "unexport");
		write_sys_file(filename, gpio_list[idx].pin_idx, NULL, FORMAT_INT);
#endif
		break;
	default:
		break;
	}
	return 0;
#endif
}

#if (defined INHAND_IDTU9 || defined INHAND_IG5)
ENUM_STR led_state_str[] = {
	{"ST_START_UP",			ST_START_UP},
	{"ST_DAIL_OTHER_ERROR",	ST_DAIL_OTHER_ERROR},
	{"ST_DAIL_MODEM_ERROR",	ST_DAIL_MODEM_ERROR},
	{"ST_DAIL_NOSIM",		ST_DAIL_NOSIM},
	{"ST_DAILING",			ST_DAILING},
	{"ST_DAIL_UP",			ST_DAIL_UP},
	{"ST_VPN_UP",			ST_VPN_UP},
	{"ST_DTU_CONNECTED",	ST_DTU_CONNECTED},
	{NULL,					-1}
};

struct LED_EVT_TASKS{
	int state;
	int event;
	int state_next;
} led_evt_tasks[] =
{
	{ST_START_UP,			EV_DAIL_START,			ST_DAILING},		
	{ST_START_UP,			EV_VPN_UP,				ST_VPN_UP},	//not using cellular
	{ST_START_UP,			EV_DTU_CONNECTED,		ST_DTU_CONNECTED},	//not using celluar and vpn

	{ST_DAILING,			EV_DAIL_DOWN,			ST_START_UP},
	{ST_DAILING,			EV_DAIL_OTHER_ERROR,	ST_DAIL_OTHER_ERROR},
	{ST_DAILING,			EV_DAIL_MODEM_ERROR,	ST_DAIL_MODEM_ERROR},	
	{ST_DAILING,			EV_DAIL_NOSIM,			ST_DAIL_NOSIM},
	{ST_DAILING,			EV_DAIL_UP,				ST_DAIL_UP},

	{ST_DAIL_OTHER_ERROR,	EV_DAIL_DOWN,			ST_START_UP},		
	{ST_DAIL_OTHER_ERROR,	EV_DAIL_START,			ST_DAILING},
	{ST_DAIL_OTHER_ERROR,	EV_DAIL_MODEM_ERROR,	ST_DAIL_MODEM_ERROR},
	{ST_DAIL_OTHER_ERROR,	EV_DAIL_NOSIM,			ST_DAIL_NOSIM},

	{ST_DAIL_MODEM_ERROR,	EV_DAIL_DOWN,			ST_START_UP},		
	{ST_DAIL_MODEM_ERROR,	EV_DAIL_START,			ST_DAILING},	
	{ST_DAIL_MODEM_ERROR,	EV_DAIL_OTHER_ERROR,	ST_DAIL_OTHER_ERROR},
	{ST_DAIL_MODEM_ERROR,	EV_DAIL_NOSIM,			ST_DAIL_NOSIM},

	{ST_DAIL_NOSIM,			EV_DAIL_DOWN,			ST_START_UP},			
	{ST_DAIL_NOSIM,			EV_DAIL_START,			ST_DAILING},
	{ST_DAIL_NOSIM,			EV_DAIL_MODEM_ERROR,	ST_DAIL_MODEM_ERROR},
	{ST_DAIL_NOSIM,			EV_DAIL_OTHER_ERROR,	ST_DAIL_OTHER_ERROR},

	{ST_DAIL_UP,			EV_VPN_UP,				ST_VPN_UP},
	{ST_DAIL_UP,			EV_DAIL_DOWN,			ST_DAILING},
	{ST_DAIL_UP,			EV_DTU_CONNECTED,		ST_DTU_CONNECTED},//not using vpn

	{ST_VPN_UP,				EV_DTU_CONNECTED,		ST_DTU_CONNECTED},
	{ST_VPN_UP,				EV_VPN_DOWN,			ST_DAIL_UP},
	{ST_VPN_UP,				EV_DAIL_DOWN,			ST_DAILING},
	{ST_VPN_UP,				EV_DAIL_START,			ST_DAILING},

	{ST_DTU_CONNECTED,		EV_DTU_DISCONNECTED,	ST_VPN_UP},
	{ST_DTU_CONNECTED,		EV_DAIL_DOWN,			ST_DAILING},
	{ST_DTU_CONNECTED,		EV_DAIL_START,			ST_DAILING}
};

struct LED_HANDLER{
	int state;
	int net_action;
	int status_action;
	int warn_action;
} led_handlers[] = 
{
	//state					net_led					status_led			warn_led
	{ST_START_UP,			LEDMAN_CMD_OFF,		LEDMAN_CMD_FLASH,		LEDMAN_CMD_OFF},				
	{ST_DAILING,			LEDMAN_CMD_OFF,		LEDMAN_CMD_FLASH_FAST,	LEDMAN_CMD_OFF},	
	{ST_DAIL_OTHER_ERROR,	LEDMAN_CMD_OFF,		LEDMAN_CMD_FLASH_FAST,	LEDMAN_CMD_FLASH},
	{ST_DAIL_MODEM_ERROR,	LEDMAN_CMD_OFF,		LEDMAN_CMD_FLASH_FAST,	LEDMAN_CMD_FLASH_FAST},	
	{ST_DAIL_NOSIM,			LEDMAN_CMD_OFF,		LEDMAN_CMD_FLASH_FAST,	LEDMAN_CMD_ON},
	{ST_DAIL_UP,			LEDMAN_CMD_ON,		LEDMAN_CMD_FLASH_FAST,	LEDMAN_CMD_OFF},
	{ST_VPN_UP,				LEDMAN_CMD_ON,		LEDMAN_CMD_FLASH,		LEDMAN_CMD_OFF},
	{ST_DTU_CONNECTED,		LEDMAN_CMD_FLASH,	LEDMAN_CMD_FLASH,		LEDMAN_CMD_OFF}
};

int ledcon_by_event(int event)
{
	int i, state, updated = FALSE;
	char *state_str = NULL;

	state_str = file2str(LED_STAUTS_FILE);
	state = get_num_by_string(state_str, led_state_str);

	if (state_str) {
		free(state_str);
	}

	if (state < 0) {
		//init state
		state = ST_START_UP;
		updated = TRUE;
	}

	for (i = 0; i < ASIZE(led_evt_tasks); i++)
	{
		if (led_evt_tasks[i].state == state &&
				led_evt_tasks[i].event == event)
		{
			state = led_evt_tasks[i].state_next;
			updated = TRUE;
		}
	}

	//update new state to file
	if (updated) {
		state_str = get_string_by_num(state, led_state_str);
		if (state_str && *state_str) {
			f_write_string(LED_STAUTS_FILE, state_str, 0, 0);

			for (i = 0; i < ASIZE(led_handlers); i++)
			{
				if (led_handlers[i].state == state)
				{
					//init LED status
					ledcon(LEDMAN_CMD_OFF, LED_NET);
					ledcon(LEDMAN_CMD_OFF, LED_STATUS);
					ledcon(LEDMAN_CMD_OFF, LED_WARN);

					ledcon(led_handlers[i].net_action, LED_NET);
					ledcon(led_handlers[i].status_action, LED_STATUS);
					ledcon(led_handlers[i].warn_action, LED_WARN);
				}
			}

		}
	}

	return 1;
}
#elif defined INHAND_IR8 || defined INHAND_ER6
ENUM_STR led_state_str[] = {
	{"ST_DAIL_DISABLE",		ST_DAIL_DISABLE},
	{"ST_START_UP",			ST_START_UP},
	{"ST_DAIL_OTHER_ERROR",	ST_DAIL_OTHER_ERROR},
	{"ST_DAIL_MODEM_ERROR",	ST_DAIL_MODEM_ERROR},
	{"ST_DAIL_NOSIM",		ST_DAIL_NOSIM},
	{"ST_DAILING",			ST_DAILING},
	{"ST_DAIL_UP",			ST_DAIL_UP},
	{"ST_ETH_DISABLE",		ST_ETH_DISABLE},
	{"ST_ETH_UP",			ST_ETH_UP},
	{"ST_ETH_DAIL_UP",		ST_ETH_DAIL_UP},
	{"ST_ETH_DOWN",			ST_ETH_DOWN},
	{"ST_ETH_DAIL_DISABLE",	ST_ETH_DAIL_DISABLE},
	{NULL,					-1}
};

ENUM_STR led_event_str[] = {
	{"EV_DAIL_DISABLE",		EV_DAIL_DISABLE},
	{"EV_DAIL_START",		EV_DAIL_START},
	{"EV_DAIL_DOWN",		EV_DAIL_DOWN},
	{"EV_DAIL_OTHER_ERROR",	EV_DAIL_OTHER_ERROR},
	{"EV_DAIL_MODEM_ERROR",	EV_DAIL_MODEM_ERROR},
	{"EV_DAIL_NOSIM",		EV_DAIL_NOSIM},
	{"EV_DAIL_UP",			EV_DAIL_UP},
	{"EV_ETH_DISABLE",		EV_ETH_DISABLE},
	{"EV_ETH_UP",			EV_ETH_UP},
	{"EV_ETH_DOWN",			EV_ETH_DOWN},
	{NULL,					-1}
};

struct LED_EVT_TASKS{
	int state;
	int event;
	int state_next;
} led_evt_tasks[] =
{
	{ST_DAIL_DISABLE,		EV_DAIL_START,			ST_START_UP},
	{ST_DAIL_DISABLE,		EV_ETH_UP,				ST_ETH_UP},
	{ST_DAIL_DISABLE,		EV_ETH_DOWN,			ST_ETH_DOWN},
	{ST_DAIL_DISABLE,		EV_ETH_DISABLE,			ST_ETH_DAIL_DISABLE},

	{ST_START_UP,			EV_DAIL_START,			ST_DAILING},
	{ST_START_UP,			EV_ETH_UP,				ST_ETH_UP},	
	{ST_START_UP,			EV_ETH_DOWN,			ST_DAILING},		
	{ST_START_UP,			EV_DAIL_DISABLE,		ST_DAIL_DISABLE},
	{ST_START_UP,			EV_ETH_DISABLE,			ST_DAILING},

	{ST_DAILING,			EV_DAIL_DOWN,			ST_START_UP},			
	{ST_DAILING,			EV_DAIL_OTHER_ERROR,	ST_DAIL_OTHER_ERROR},
	{ST_DAILING,			EV_DAIL_MODEM_ERROR,	ST_DAIL_MODEM_ERROR},	
	{ST_DAILING,			EV_DAIL_NOSIM,			ST_DAIL_NOSIM},
	{ST_DAILING,			EV_DAIL_UP,				ST_DAIL_UP},
	{ST_DAILING,			EV_ETH_UP,				ST_ETH_UP},
	{ST_DAILING,			EV_ETH_DOWN,			ST_DAILING},
	{ST_DAILING,			EV_DAIL_DISABLE,		ST_DAIL_DISABLE},
	{ST_DAILING,			EV_ETH_DISABLE,			ST_START_UP},

	{ST_DAIL_OTHER_ERROR,	EV_DAIL_DOWN,			ST_START_UP},
	{ST_DAIL_OTHER_ERROR,	EV_DAIL_START,			ST_DAILING},
	{ST_DAIL_OTHER_ERROR,	EV_DAIL_MODEM_ERROR,	ST_DAIL_MODEM_ERROR},
	{ST_DAIL_OTHER_ERROR,	EV_DAIL_NOSIM,			ST_DAIL_NOSIM},
	{ST_DAIL_OTHER_ERROR,	EV_ETH_UP,				ST_ETH_UP},
	{ST_DAIL_OTHER_ERROR,	EV_ETH_DOWN,			ST_DAIL_OTHER_ERROR},
	{ST_DAIL_MODEM_ERROR,	EV_DAIL_DISABLE,		ST_DAIL_DISABLE},
	{ST_DAIL_OTHER_ERROR,	EV_ETH_DISABLE,			ST_DAIL_OTHER_ERROR},

	{ST_DAIL_MODEM_ERROR,	EV_DAIL_DOWN,			ST_START_UP},
	{ST_DAIL_MODEM_ERROR,	EV_DAIL_START,			ST_DAILING},	
	{ST_DAIL_MODEM_ERROR,	EV_DAIL_OTHER_ERROR,	ST_DAIL_OTHER_ERROR},
	{ST_DAIL_MODEM_ERROR,	EV_DAIL_NOSIM,			ST_DAIL_NOSIM},
	{ST_DAIL_MODEM_ERROR,	EV_ETH_UP,				ST_ETH_UP},
	{ST_DAIL_MODEM_ERROR,	EV_ETH_DOWN,			ST_DAIL_MODEM_ERROR},	
	{ST_DAIL_MODEM_ERROR,	EV_DAIL_DISABLE,		ST_DAIL_DISABLE},
	{ST_DAIL_MODEM_ERROR,	EV_ETH_DISABLE,			ST_DAIL_MODEM_ERROR},

	{ST_DAIL_NOSIM,			EV_DAIL_DOWN,			ST_START_UP},
	{ST_DAIL_NOSIM,			EV_DAIL_START,			ST_DAILING},
	{ST_DAIL_NOSIM,			EV_DAIL_MODEM_ERROR,	ST_DAIL_MODEM_ERROR},
	{ST_DAIL_NOSIM,			EV_DAIL_OTHER_ERROR,	ST_DAIL_OTHER_ERROR},
	{ST_DAIL_NOSIM,			EV_ETH_UP,				ST_ETH_UP},
	{ST_DAIL_NOSIM,			EV_ETH_DOWN,			ST_DAIL_NOSIM},
	{ST_DAIL_NOSIM,			EV_DAIL_DISABLE,		ST_DAIL_DISABLE},
	{ST_DAIL_NOSIM,			EV_ETH_DISABLE,			ST_DAIL_NOSIM},

	{ST_DAIL_UP,			EV_DAIL_DOWN,			ST_START_UP},
	{ST_DAIL_UP,			EV_DAIL_START,			ST_DAILING},
	{ST_DAIL_UP,			EV_DAIL_MODEM_ERROR,	ST_DAIL_MODEM_ERROR},
	{ST_DAIL_UP,			EV_DAIL_OTHER_ERROR,	ST_DAIL_OTHER_ERROR},
	{ST_DAIL_UP,			EV_ETH_UP,				ST_ETH_DAIL_UP},
	{ST_DAIL_UP,			EV_ETH_DOWN,			ST_DAIL_UP},
	{ST_DAIL_UP,			EV_DAIL_DISABLE,		ST_DAIL_DISABLE},
	{ST_DAIL_UP,			EV_ETH_DISABLE,			ST_DAIL_UP},

	{ST_ETH_UP,				EV_ETH_DOWN,			ST_ETH_DOWN},
	{ST_ETH_UP,				EV_DAIL_UP,				ST_ETH_DAIL_UP},
	{ST_ETH_UP,				EV_DAIL_DISABLE,		ST_ETH_UP},
	{ST_ETH_UP,				EV_ETH_DISABLE,			ST_ETH_DISABLE},

	{ST_ETH_DAIL_UP,		EV_ETH_DOWN,			ST_DAIL_UP},
	{ST_ETH_DAIL_UP,		EV_DAIL_START,			ST_ETH_UP},
	{ST_ETH_DAIL_UP,		EV_DAIL_OTHER_ERROR,	ST_ETH_UP},
	{ST_ETH_DAIL_UP,		EV_DAIL_MODEM_ERROR,	ST_ETH_UP},
	{ST_ETH_DAIL_UP,		EV_DAIL_NOSIM,			ST_ETH_UP},
	{ST_ETH_DAIL_UP,		EV_DAIL_DISABLE,		ST_ETH_UP},
	{ST_ETH_DAIL_UP,		EV_ETH_DISABLE,			ST_DAIL_UP},

	{ST_ETH_DOWN,			EV_ETH_UP,				ST_ETH_UP},
	{ST_ETH_DOWN,			EV_DAIL_START,			ST_DAILING},
	{ST_ETH_DOWN,			EV_DAIL_OTHER_ERROR,	ST_DAIL_OTHER_ERROR},
	{ST_ETH_DOWN,			EV_DAIL_MODEM_ERROR,	ST_DAIL_MODEM_ERROR},
	{ST_ETH_DOWN,			EV_DAIL_NOSIM,			ST_DAIL_NOSIM},
	{ST_ETH_DOWN,			EV_DAIL_UP,				ST_DAIL_UP},
	{ST_ETH_DOWN,			EV_DAIL_DISABLE,		ST_DAIL_DISABLE},
	{ST_ETH_DOWN,			EV_ETH_DISABLE,			ST_ETH_DISABLE},

	{ST_ETH_DISABLE,		EV_DAIL_DISABLE,		ST_ETH_DAIL_DISABLE},
	{ST_ETH_DISABLE,		EV_DAIL_START,			ST_START_UP},
	{ST_ETH_DISABLE,		EV_DAIL_UP,				ST_DAIL_UP},
	{ST_ETH_DISABLE,		EV_ETH_UP,				ST_ETH_UP},

	{ST_ETH_DAIL_DISABLE,	EV_ETH_UP,				ST_ETH_UP},
	{ST_ETH_DAIL_DISABLE,	EV_DAIL_START,			ST_START_UP},
};

struct LED_HANDLER{
	int state;
#ifdef INHAND_ER6
	int yellow_action;
#else
	int blue_action;
#endif
	int green_action;
	int red_action;
} led_handlers[] =
{
#ifdef INHAND_ER6
	//state					yellow_led			green_led				red_led
	{ST_DAIL_DISABLE,		LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_OFF},
	{ST_START_UP,			LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_ON},
	{ST_DAILING,			LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_FLASH},
	{ST_DAIL_OTHER_ERROR,	LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_FLASH_FAST},
	{ST_DAIL_MODEM_ERROR,	LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_FLASH_FAST},
	{ST_DAIL_NOSIM,			LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_FLASH_FAST},
	{ST_DAIL_UP,			LEDMAN_CMD_OFF,		LEDMAN_CMD_ON,			LEDMAN_CMD_OFF},
	{ST_ETH_DISABLE,		LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_OFF},
	{ST_ETH_UP,				LEDMAN_CMD_ON,		LEDMAN_CMD_OFF,			LEDMAN_CMD_OFF},
	{ST_ETH_DAIL_UP,		LEDMAN_CMD_ON,		LEDMAN_CMD_OFF,			LEDMAN_CMD_OFF},
	{ST_ETH_DOWN,			LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_FLASH_FAST},
	{ST_ETH_DAIL_DISABLE,	LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_OFF}
#else
	//state					blue_led			green_led				red_led
	{ST_DAIL_DISABLE,		LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_OFF},
	{ST_START_UP,			LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_ON},
	{ST_DAILING,			LEDMAN_CMD_OFF,		LEDMAN_CMD_FLASH,		LEDMAN_CMD_OFF},	
	{ST_DAIL_OTHER_ERROR,	LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_FLASH_FAST},
	{ST_DAIL_MODEM_ERROR,	LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_FLASH_FAST},	
	{ST_DAIL_NOSIM,			LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_FLASH_FAST},
	{ST_ETH_DISABLE,		LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_OFF},
	{ST_DAIL_UP,			LEDMAN_CMD_OFF,		LEDMAN_CMD_ON,			LEDMAN_CMD_OFF},
	{ST_ETH_UP,				LEDMAN_CMD_ON,		LEDMAN_CMD_OFF,			LEDMAN_CMD_OFF},
	{ST_ETH_DAIL_UP,		LEDMAN_CMD_ON,		LEDMAN_CMD_OFF,			LEDMAN_CMD_OFF},
	{ST_ETH_DOWN,			LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_FLASH_FAST},
	{ST_ETH_DAIL_DISABLE,	LEDMAN_CMD_OFF,		LEDMAN_CMD_OFF,			LEDMAN_CMD_OFF}
#endif
};

int ledcon_by_event(int event)
{
	int i, state, updated = FALSE;
	char *state_str = NULL;

	state_str = file2str(LED_STAUTS_FILE);
	state = get_num_by_string(state_str, led_state_str);

	if (state < 0) {
		//init state
		state = ST_START_UP;
		updated = TRUE;
	}

	for (i = 0; i < ASIZE(led_evt_tasks); i++)
	{
		if (led_evt_tasks[i].state == state &&
				led_evt_tasks[i].event == event)
		{
			state = led_evt_tasks[i].state_next;
			updated = TRUE;
			break;
		}
	}

	if (state_str) {
		if (updated) {
			LOG_IN("current LED STATE is %s event %s", 
					state_str, get_string_by_num(event, led_event_str));
		}

		free(state_str);
	}

	//update new state to file
	if (updated) {
		state_str = get_string_by_num(state, led_state_str);
		LOG_IN("update LED STATE to %s by event %s", 
				state_str, get_string_by_num(event, led_event_str));

		if (state_str && *state_str) {
			f_write_string(LED_STAUTS_FILE, state_str, 0, 0);

			for (i = 0; i < ASIZE(led_handlers); i++)
			{
				if (led_handlers[i].state == state)
				{
					//init LED status
					ledcon(LEDMAN_CMD_OFF, LED_NETWORK_R);
					ledcon(LEDMAN_CMD_OFF, LED_NETWORK_G);
#ifdef INHAND_ER6
					ledcon(LEDMAN_CMD_OFF, LED_NETWORK_Y);
#else
					ledcon(LEDMAN_CMD_OFF, LED_NETWORK_B);
#endif

#ifdef INHAND_ER6
					ledcon(led_handlers[i].yellow_action, LED_NETWORK_Y);
#else
					ledcon(led_handlers[i].blue_action, LED_NETWORK_B);
#endif
					ledcon(led_handlers[i].green_action, LED_NETWORK_G);
					ledcon(led_handlers[i].red_action, LED_NETWORK_R);
				}
			}

		}
	}

	return 1;
}
#endif
