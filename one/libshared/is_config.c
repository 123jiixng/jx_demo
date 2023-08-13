#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "build_info.h"
#include "shared.h"
#include "ih_errno.h"
#include "bootenv.h"
#include "ih_logtrace.h"
#include "nm_ipc.h"
#include "sql_db.h"
#include "record_ipc.h"

#define IS_CONFIG_MAGIC		0x7EE7
#define IS_CONFIG_HEAD_SIZE 	8
#define NOR_SECTER_SIZE		64*1024
#define IS_CONFIG_SIZE		4*(NOR_SECTER_SIZE)
#define IS_CONFIG_DATA_SIZE 	((IS_CONFIG_SIZE) - (IS_CONFIG_HEAD_SIZE))

//xml backup config
#define XML_BACKUP_CONFIG_FILE   "/var/backups/config/backup_config.xml"

#define PORTAL_BACKUP_PATH             "/var/backups/captive_portal"
#define AGENT_PORTAL_BACKUP_PATH       "/var/backups/wifidog"

struct IS_CONFIG {
	uns32 crc;
	uns32 len;
	char data[IS_CONFIG_DATA_SIZE];
};

static struct IS_CONFIG *is_config = NULL;

static int do_relocate_is_config(char *path,char *mtd_name)
{
	FILE *pf;
	char mtd_part[64];
	uns32 crc;

	if (get_part(mtd_name,mtd_part,sizeof(mtd_part)) == NULL) {
		return ERR_NOENT;
	}
	
	pf = fopen(mtd_part,"r");
	if (pf == NULL) {
		LOG_ER("unable to open %s",mtd_part);
		return ERR_NOENT;
	}

	fread(is_config,IS_CONFIG_SIZE,1,pf);
	fclose(pf);

	/* check config format */
	if (is_config->len > IS_CONFIG_DATA_SIZE) {
		LOG_ER("invalid length");
		return ERR_INVAL;
	}

	crc = uboot_crc32(is_config->data,is_config->len);
	if (crc != is_config->crc) {
		LOG_ER("crc error");
		return ERR_INVAL;
	}

	pf = fopen(path,"w");
	if (pf == NULL) {
		LOG_ER("unable to open %s",path);
		return ERR_NOENT;
	}

	fwrite(is_config->data,is_config->len,1,pf);
	fclose(pf);
	
	return ERR_OK;
}

int relocate_is_config(char *path)
{	
	int ret = ERR_OK,fd;
	int i;

	if (!path) return ERR_INVAL;

	is_config = (struct IS_CONFIG *)malloc(IS_CONFIG_SIZE);
	if (is_config == NULL) {
		LOG_ER("no memory");
		return ERR_NOMEM;
	}

	if (do_relocate_is_config(path,CONFIG_PART) != ERR_OK) {
		goto use_backup;
	}

	free(is_config);
	return ERR_OK;
	
use_backup:
	if (do_relocate_is_config(path,BACKUPCONFIG_PART) != ERR_OK) {
		goto err_exit;
	}

	/* ���������ô洢���������� */
	fd = part_open(CONFIG_PART);
	if (fd >= 0) {
#if 1
		for (i = 1; i <= (IS_CONFIG_SIZE)/(NOR_SECTER_SIZE); i++){
			if (is_config->len + IS_CONFIG_HEAD_SIZE <= i*NOR_SECTER_SIZE) {
				part_erase(fd, 0, i, 0, 1);
				part_write(fd, 0, (const unsigned char *)is_config, i*NOR_SECTER_SIZE, 1);
				break;				
			}			
		}
#else
		if(is_config->len + IS_CONFIG_HEAD_SIZE <= IS_CONFIG_SIZE){
			part_erase(fd, 0, (IS_CONFIG_SIZE)/(NOR_SECTER_SIZE), 0, 0);
			part_write(fd, 0, (const unsigned char *)is_config, IS_CONFIG_SIZE, 1);
		}
#endif
		
		part_close(fd);
	}

	free(is_config);
	return ERR_OK;
err_exit:
	free(is_config);
	return ret;
}

int save_is_config(char *path)
{
	FILE *pf;
	int len,fd,i;
	
	if (!path) return ERR_INVAL;	

	is_config = (struct IS_CONFIG *)malloc(IS_CONFIG_SIZE);
	if (is_config == NULL) {
		LOG_ER("no memory");
		return ERR_NOMEM;
	}

	memset(is_config,0xff,IS_CONFIG_SIZE);

	pf = fopen(path,"r");
	if (pf == NULL) {
		LOG_ER("unable to open %s",path);
		free(is_config);
		return ERR_NOENT;
	}

	len = fread(is_config->data,1,IS_CONFIG_DATA_SIZE,pf);
	fclose(pf);
  	if (len <= 0) {
		free(is_config);
		return -1;
  	}

	is_config->len = len;
	is_config->crc = uboot_crc32(is_config->data,is_config->len);
	
	fd = part_open(BACKUPCONFIG_PART);
	if (fd >= 0) {
#if 1
		for (i = 1; i <= (IS_CONFIG_SIZE)/(NOR_SECTER_SIZE); i++){
			if (is_config->len + IS_CONFIG_HEAD_SIZE <= i*NOR_SECTER_SIZE) {
				part_erase(fd,0,i,0,1);
				part_write(fd,0,(const unsigned char *)is_config,i*NOR_SECTER_SIZE,1);
				break;				
			}			
		}
#else
		if(is_config->len + IS_CONFIG_HEAD_SIZE <= IS_CONFIG_SIZE){
			part_erase(fd, 0, (IS_CONFIG_SIZE)/(NOR_SECTER_SIZE), 0, 1);
			part_write(fd, 0, (const unsigned char *)is_config, IS_CONFIG_SIZE, 1);
		}
#endif
		
		part_close(fd);
	}

	fd = part_open(CONFIG_PART);
	if (fd >= 0) {
#if 1
		for (i = 1; i <= (IS_CONFIG_SIZE)/(NOR_SECTER_SIZE); i++){
			if (is_config->len + IS_CONFIG_HEAD_SIZE <= i*NOR_SECTER_SIZE) {
				part_erase(fd,0,i,0,1);
				part_write(fd,0,(const unsigned char *)is_config,i*NOR_SECTER_SIZE,1);
				break;				
			}			
		}
#else
		if(is_config->len + IS_CONFIG_HEAD_SIZE <= IS_CONFIG_SIZE){
			part_erase(fd, 0, (IS_CONFIG_SIZE)/(NOR_SECTER_SIZE), 0, 1);
			part_write(fd, 0, (const unsigned char *)is_config, IS_CONFIG_SIZE, 1);
		}
#endif
		
		part_close(fd);
	}

	free(is_config);
	return ERR_OK;
}

void backup_config_file(void)
{
	f_copy(SHADOW_DB_FILE, SHADOW_CONFIG_FILE_BACKUP);
	sync_file_write(SHADOW_CONFIG_FILE_BACKUP);
	LOG_DB("backup config file.\n");

	return;
}

int restore_backup_config(void)
{
	if (validate_is_config(STARTUP_CONFIG_FILE_BACKUP) == TRUE) {
		f_copy(STARTUP_CONFIG_FILE_BACKUP, STARTUP_CONFIG_FILE);
		save_is_config(STARTUP_CONFIG_FILE);
		LOG_IN("restored backup config!\n");

		return ERR_OK;
	}else{
		LOG_IN("restore backup config but no valid one found.\n");
	}

	return -1;
}


void restore_is_config(void)
{
	int fd;
	char cmd[64] = {0};
	
	backup_config_file();

#if 0
	/*need to remount after erasing*/
	fd = part_open(BACKUPCONFIG_PART);
	if (fd >= 0) {
		//zly: last one sector for certs
		part_erase(fd,0,-1,0,1);
		part_close(fd);
	}

	fd = part_open(CONFIG_PART);
	if (fd >= 0) {
		part_erase(fd,0,-1,0,1);
		part_close(fd);
	}
#endif
}

BOOL validate_is_config(char *filename)
{
	char buf[1024];
	FILE *pf;

	if ((pf = fopen(filename,"r")) == NULL) return FALSE;
	while (fgets(buf,sizeof(buf),pf) != NULL);
	fclose(pf);
	if (!strstr(buf,CONFIG_END_SECTION)) return FALSE; 

	return TRUE;
}

BOOL check_hw_reset_factory(void)
{
	int value, i, j;
	BOOL ret=FALSE;

	/*check if recover default configuration by zly
	 *1.press button until error led on.
	 *2.not press button, error led will off
	 *3.press button until error led flash
	*/
	gpio(GPIO_READ, GPIO_RESET, &value);
	if(value == 0) {//vertial button -- low level effective
		usleep(500*1000);
		for(j=0; j<100; j++) {
			gpio(GPIO_READ, GPIO_RESET, &value);
			if(value!=0) goto skip_factory_default;
		}
		for(j=0; j<20; j++) {
			usleep(15*1000);
			gpio(GPIO_READ, GPIO_RESET, &value);
			if(value!=0) goto skip_factory_default;
		}
		usleep(129*1000);
		for(j=0; j<100; j++) {
			gpio(GPIO_READ, GPIO_RESET, &value);
			if(value!=0) goto skip_factory_default;
		}
#if !(defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_ER3)
		ledcon(LEDMAN_CMD_ON, LED_ERROR);
#else
		system_ledcon(SYSTEM_CONFIG_RESET_BEGIN);
#endif
		for(i=0; i<10; i++) {//10s
			sleep(1);
			gpio(GPIO_READ, GPIO_RESET, &value);
			if(value) {
				usleep(500*1000);
				for(j=0; j<100; j++) {
					gpio(GPIO_READ, GPIO_RESET, &value);
					if(!value) goto skip_factory_default;
				}
				usleep(60*1000);
				for(j=0; j<20; j++) {
					usleep(7*1000);
					gpio(GPIO_READ, GPIO_RESET, &value);
					if(!value) goto skip_factory_default;
				}
				usleep(467*1000);
				for(j=0; j<100; j++) {
					gpio(GPIO_READ, GPIO_RESET, &value);
					if(!value) goto skip_factory_default;
				}
				break;
			}
		}
#if !(defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_ER3)
		ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#else
		system_ledcon(SYSTEM_CONFIG_RESET_STAGE1);
#endif
		if(value) {
			for(i=0; i<10; i++) {//10s
				sleep(1);
				gpio(GPIO_READ, GPIO_RESET, &value);
				if(value==0) {
					usleep(500*1000);
					for(j=0; j<100; j++) {
						gpio(GPIO_READ, GPIO_RESET, &value);
						if(value!=0) goto skip_factory_default;
					}
					for(j=0; j<26; j++) {
						usleep(11*1000);
						gpio(GPIO_READ, GPIO_RESET, &value);
						if(value!=0) goto skip_factory_default;
					}
					usleep(23*1000);
					for(j=0; j<100; j++) {
						gpio(GPIO_READ, GPIO_RESET, &value);
						if(value!=0) goto skip_factory_default;
					}
					break;
				}
			}
			if(value == 0) {
#if defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_ER3
				system_ledcon(SYSTEM_CONFIG_RESET_BEGIN);
				sleep(2);
#endif
				ret = TRUE;
			}
		}
	}
skip_factory_default:
#if !(defined INHAND_IR8 || defined INHAND_ER6 || defined INHAND_ER3)
	ledcon(LEDMAN_CMD_OFF, LED_ERROR);
#endif

	return ret;
}

int monitor_reset_button(void)
{
	int value, i, j;

	gpio(GPIO_READ, GPIO_RESET, &value);
	if(value == 0) {//vertial button -- low level effective
		usleep(500*1000);
		for(j=0; j<100; j++) {
			gpio(GPIO_READ, GPIO_RESET, &value);
			if(value!=0) goto do_nothing;
		}
		for(j=0; j<20; j++) {
			usleep(15*1000);
			gpio(GPIO_READ, GPIO_RESET, &value);
			if(value!=0) goto do_nothing;
		}
		usleep(129*1000);
		for(j=0; j<100; j++) {
			gpio(GPIO_READ, GPIO_RESET, &value);
			if(value!=0) goto do_nothing;
		}
		ledcon(LEDMAN_CMD_ON, LED_ERROR);
		for(i=0; i<10; i++) {//10s
			sleep(1);
			gpio(GPIO_READ, GPIO_RESET, &value);
			if(value) {
				usleep(500*1000);
				for(j=0; j<100; j++) {
					gpio(GPIO_READ, GPIO_RESET, &value);
					if(!value) goto do_nothing;
				}
				usleep(60*1000);
				for(j=0; j<20; j++) {
					usleep(7*1000);
					gpio(GPIO_READ, GPIO_RESET, &value);
					if(!value) goto do_nothing;
				}
				usleep(467*1000);
				for(j=0; j<100; j++) {
					gpio(GPIO_READ, GPIO_RESET, &value);
					if(!value) goto do_nothing;
				}
				break;
			}
		}

		ledcon(LEDMAN_CMD_OFF, LED_ERROR);
	}else {
		goto do_nothing;
	}
	return RST_BUTTON_DISABLE_REDIAL;

do_nothing:
	return RST_BUTTON_DO_NOTHING;
}

//change string to upper
char* strupr_d(char* src)
{
	char * pos = src;
	while (*pos != '\0')
	{
		if (*pos >= 'a' && *pos <= 'z')
			*pos -= 32;
		pos++;
	}
	return src;
}

//some modems support dhcp option of interface mtu
BOOL modem_support_dhcp_option_mtu(void)
{
	if(ih_key_support("FS39")
		|| ih_license_support(IH_FEATURE_MODEM_EC20)
		|| ih_license_support(IH_FEATURE_MODEM_EC25)
		|| ih_license_support(IH_FEATURE_MODEM_EP06)
		|| ih_license_support(IH_FEATURE_MODEM_FG360)
		|| ih_license_support(IH_FEATURE_MODEM_RM520N)
		|| ih_license_support(IH_FEATURE_MODEM_RM500)){
		return TRUE;
	}

	return FALSE;	
}

//return true if the modem support ipv6
BOOL modem_support_ipv6(void)
{
	if(ih_key_support("NRQ1")
		|| ih_key_support("NRQ2")){
		return TRUE;
	}

	return FALSE;	
}

//return true if the modem support dual-apn
BOOL modem_support_dual_apn(void)
{
	if(ih_key_support("NRQ3")
		|| ih_key_support("FQ39")){
		return TRUE;
	}

	return FALSE;	
}

int specify_factory_config_by_model(char *model, char *oem, char *product)
{
	FILE *fp;
	int ret;

#if (defined INHAND_VG9 || defined INHAND_IR8)
	char ssid0[32] = {0}; //2g
	char ssid1[32] = {0}; //5g
	char sn[32] = {0}; //sn
	int len = 0;
#endif

	fp = fopen(STARTUP_CONFIG_FILE, "w");
	if(!fp) return -1;

	fprintf(fp, "#factory config\n");
	fprintf(fp, "!\n");

#if (defined INHAND_IR8)
    fprintf(fp, "interface vlan 1\n");
    //fprintf(fp, "  ip address 192.168.2.1 255.255.255.0\n");
    //fprintf(fp, "  ip dhcp-server range 192.168.2.2 192.168.2.100\n");
    //fprintf(fp, "  ip dhcp-server enable\n");
    fprintf(fp, "  ip nat inside\n");
    fprintf(fp, "!\n");

    fprintf(fp, "!\n");
    fprintf(fp, "interface cellular 1\n");
    fprintf(fp, "  ip nat outside\n");

    fprintf(fp, "!\n");
    fprintf(fp, "interface vlan 4010\n");
    fprintf(fp, "  ip nat outside\n");

    fprintf(fp, "!\n");
    fprintf(fp, "interface vlan 4011\n");
    fprintf(fp, "  ip nat outside\n");

    fprintf(fp, "#firewall config\n");
    fprintf(fp, "!\n");
    fprintf(fp, "ip snat inside list 100 interface cellular 1\n");
    fprintf(fp, "access-list 100 permit ip any any\n");
    fprintf(fp, "ip snat inside list 101 interface vlan 4010\n");
    fprintf(fp, "access-list 101 permit ip any any\n");
    fprintf(fp, "ip snat inside list 102 interface vlan 4011\n");
    fprintf(fp, "access-list 102 permit ip any any\n");

	fprintf(fp, CONFIG_END_SECTION);
	fclose(fp);

	ret = save_is_config(STARTUP_CONFIG_FILE);

	return ret;
#endif

	if(strcmp(oem, "global")==0
	    || strcmp(oem, "blank")==0
	    || strcmp(oem, "inhand-us")==0
	    || strcmp(oem, "welotec-oem")==0
	    || strcmp(oem, "welotec")==0) {
		fprintf(fp, "#system config\n");
		fprintf(fp, "language English\n");
		if(strcmp(oem, "welotec")==0 || strcmp(oem, "welotec-oem")==0) {
			if(strcmp(oem, "welotec")==0) {
				fprintf(fp, "ip domain-name welotec-router.com\n");
			}else{
				fprintf(fp, "ip domain-name my-router.com\n");
			}
			fprintf(fp, "clock timezone CET-1CEST,M3.5.0/2,M10.5.0/3\n");
		}
	}
	
	if(ih_key_support("FQ88")) {
		fprintf(fp, "clock timezone UTC-9\n");
	}

#ifdef INHAND_IDTU9
	fprintf(fp, "hostname DTU\n");
	fprintf(fp, "username admin privilege 15 password admin@123\n");
	fprintf(fp, "username config privilege 12 password config@123\n");
	fprintf(fp, "username audit privilege 1 password audit@123\n");
	fprintf(fp, "enable password 123456\n");
	fprintf(fp, "remote-login retry 8\n");
	fprintf(fp, "console inhand-tool\n");
#endif
	fprintf(fp, "service password-encryption\n");

#if (defined INHAND_IG9 || defined INHAND_IG5)
	fprintf(fp, "hostname EdgeGateway\n");
#endif

#if (!(defined INHAND_IP812))
	if(ih_license_support_new(IH_FEATURE_WLAN_AP) || ih_license_support_new(IH_FEATURE_WLAN_STA)){
		fprintf(fp, "#ethernet config\n");
		fprintf(fp, "!\n");
#if !(defined INHAND_VG9 || defined INHAND_IR8)
		if(ih_license_support_new(IH_FEATURE_ETH2_KSZ9893)){
			fprintf(fp, "interface gigabitethernet 0/1\n");
		}else{
			fprintf(fp, "interface fastethernet 0/1\n");
		}
		fprintf(fp, "  ip address 192.168.1.1 255.255.255.0\n");
		fprintf(fp, "  ip dhcp-server range 192.168.1.2 192.168.1.100\n");
		fprintf(fp, "  no ip dhcp-server enable\n");
		fprintf(fp, "  track l2-state\n");
		if (ih_license_support_new(IH_FEATURE_MODEM_NONE)){ 
			fprintf(fp, "  ip nat outside\n");
		} else {
			fprintf(fp, "  ip nat inside\n");
		}
#endif

		fprintf(fp, "#bridge config\n");
		fprintf(fp, "!\n");
		fprintf(fp, "bridge 1\n");

		fprintf(fp, "#dot11 SSID config\n");
		fprintf(fp, "!\n");
		if(strcmp(oem, "welotec")==0 || strcmp(oem, "welotec-oem")==0){
			fprintf(fp, "dot11 ssid TK800\n");
		}else{
#if (defined INHAND_VG9 || defined INHAND_IR8)
			if(bootenv_get("wlanaddr", ssid0, sizeof(ssid0))){
				strupr_d(ssid0);
				fprintf(fp, "dot11 ssid %s-%c%c%c%c%c%c\n", DEFAULT_SSID, 
						ssid0[9], ssid0[10], ssid0[12], ssid0[13], ssid0[15], ssid0[16]);
			} else {
				fprintf(fp, "dot11 ssid %s\n", DEFAULT_SSID);
			}
#else
			fprintf(fp, "dot11 ssid %s\n", DEFAULT_SSID);
#endif
		}
#if (defined INHAND_VG9 || defined INHAND_IR8)
		fprintf(fp, "  authentication key-management wpa 2\n");
		fprintf(fp, "  guest-mode\n");
		if(bootenv_get("serialnumber", sn, sizeof(sn)) && (len = strlen(sn)) >= 8){
			fprintf(fp, "  wpa-psk ascii %s\n", sn+(len-8));
		} else {
			fprintf(fp, "  wpa-psk ascii %s\n", DEFAULT_WIFI_PASSWD); //if fail, use default password 12345678 for wpa2-psk

		}
#else
		fprintf(fp, "  authentication open\n");
#endif

		fprintf(fp, "#dot11 dot11radio config\n");
		fprintf(fp, "!\n");
		fprintf(fp, "interface dot11radio 1\n");
		fprintf(fp, "  bridge-group 1\n");
#if (defined INHAND_VG9 || defined INHAND_IR8)
		fprintf(fp, "  channel 0\n"); //auto channel
		fprintf(fp, "  radio-type dot11ng\n");
#else
		fprintf(fp, "  channel 11\n");
		fprintf(fp, "  radio-type dot11gn\n");
#endif
		if(strcmp(oem, "welotec")==0 || strcmp(oem, "welotec-oem")==0){
			fprintf(fp, "  ssid TK800\n");
		}else{
#if (defined INHAND_VG9 || defined INHAND_IR8)
			if(bootenv_get("wlanaddr", ssid0, sizeof(ssid0))){
				strupr_d(ssid0);
				fprintf(fp, "  ssid %s-%c%c%c%c%c%c\n", DEFAULT_SSID, 
						ssid0[9], ssid0[10], ssid0[12], ssid0[13], ssid0[15], ssid0[16]);
			} else {
				fprintf(fp, "  ssid %s\n", DEFAULT_SSID);
			}
			fprintf(fp, "  encryption mode ciphers aes-ccm\n"); 
			fprintf(fp, "  802.11n bandwidth 20\n"); 
#else
			fprintf(fp, "  ssid %s\n", DEFAULT_SSID);
#endif
		}
#if !(defined INHAND_VG9 || defined INHAND_IR8)
		fprintf(fp, "  shutdown\n");
#endif

#if (defined INHAND_VG9 || defined INHAND_IR8)
		fprintf(fp, "!\n");
		if(strcmp(oem, "welotec")==0 || strcmp(oem, "welotec-oem")==0){
			fprintf(fp, "dot11 ssid TK800-5G\n");
		}else{
			if(bootenv_get("wlanaddr_5g", ssid1, sizeof(ssid1))){
				strupr_d(ssid1);
				fprintf(fp, "dot11 ssid %s-5G-%c%c%c%c%c%c\n", DEFAULT_SSID, 
						ssid1[9], ssid1[10], ssid1[12], ssid1[13], ssid1[15], ssid1[16]);
			} else {
				fprintf(fp, "dot11 ssid %s-5G\n", DEFAULT_SSID);
			}
		}
		fprintf(fp, "  authentication key-management wpa 2\n");
		fprintf(fp, "  guest-mode\n");
		if(bootenv_get("serialnumber", sn, sizeof(sn)) && (len = strlen(sn)) >= 8){
			fprintf(fp, "  wpa-psk ascii %s\n", sn+(len-8));
		} else {
			fprintf(fp, "  wpa-psk ascii %s\n", DEFAULT_WIFI_PASSWD); //if fail, use default password 12345678 for wpa2-psk

		}

		fprintf(fp, "!\n");
		fprintf(fp, "interface dot11radio 2\n");
		fprintf(fp, "  bridge-group 1\n");
		if(strcmp(oem, "welotec")==0 || strcmp(oem, "welotec-oem")==0){
			fprintf(fp, "  ssid TK800-5G\n");
		}else{
			fprintf(fp, "  channel 36\n"); //auto channel
			fprintf(fp, "  radio-type dot11ac\n");
			if(bootenv_get("wlanaddr_5g", ssid1, sizeof(ssid1))){
				strupr_d(ssid1);
				fprintf(fp, "  ssid %s-5G-%c%c%c%c%c%c\n", DEFAULT_SSID, 
						ssid1[9], ssid1[10], ssid1[12], ssid1[13], ssid1[15], ssid1[16]);
			} else {
				fprintf(fp, "  ssid %s-5G\n", DEFAULT_SSID);
			}
			fprintf(fp, "  encryption mode ciphers aes-ccm\n"); 
			fprintf(fp, "  802.11n bandwidth 80\n"); 
		}
#endif

		fprintf(fp, "#bridge config\n");
		fprintf(fp, "!\n");
		if(ih_license_support_new(IH_FEATURE_ETH2_KSZ9893)){
			fprintf(fp, "interface gigabitethernet 0/2\n");
		}else{
			fprintf(fp, "interface vlan 1\n");
		}
		fprintf(fp, "  bridge-group 1\n");

		fprintf(fp, "#bridge interface config\n");
		fprintf(fp, "!\n");
		fprintf(fp, "interface bridge 1\n");
		fprintf(fp, "  ip address 192.168.2.1 255.255.255.0\n");
		fprintf(fp, "  ip dhcp-server range 192.168.2.2 192.168.2.100\n");
		fprintf(fp, "  ip dhcp-server enable\n");
		fprintf(fp, "  ip nat inside\n");
	}else {
		if (ih_license_support_new(IH_FEATURE_SINGLE_ETH)) {
				fprintf(fp, "#ethernet interface config\n");
			  fprintf(fp, "!\n");
				fprintf(fp, "interface fastethernet 0/1\n");
				fprintf(fp, "  ip address 192.168.1.1 255.255.255.0\n");
				fprintf(fp, "  ip dhcp-server range 192.168.1.2 192.168.1.100\n");
				fprintf(fp, "  ip dhcp-server enable\n");
				fprintf(fp, "  track l2-state\n");
				fprintf(fp, "  ip nat inside\n");
		} else if (ih_license_support_new(IH_FEATURE_ETH2_OMAP) || ih_license_support_new(IH_FEATURE_ETH2_KSZ9893)) {
			if((!strcmp(oem, "welotec") || strcmp(oem, "welotec-oem")==0)
				&& !ih_license_support_new(IH_FEATURE_ETH2_KSZ9893)){
				fprintf(fp, "#bridge config\n");
				fprintf(fp, "!\n");
				fprintf(fp, "bridge 1\n");
				fprintf(fp, "interface fastethernet 0/1\n");
				fprintf(fp, "  bridge-group 1\n");

				fprintf(fp, "!\n");
				fprintf(fp, "interface fastethernet 0/2\n");
				fprintf(fp, "  bridge-group 1\n");

				fprintf(fp, "#bridge interface config\n");
				fprintf(fp, "!\n");
				fprintf(fp, "interface bridge 1\n");
				fprintf(fp, "  ip address 192.168.2.1 255.255.255.0\n");
				fprintf(fp, "  ip address 192.168.1.1 255.255.255.0 secondary\n");
				fprintf(fp, "  ip dhcp-server range 192.168.2.2 192.168.2.100\n");
				fprintf(fp, "  ip dhcp-server enable\n");
				fprintf(fp, "  ip nat inside\n");
			} else {
				fprintf(fp, "#ethernet interface config\n");
				fprintf(fp, "!\n");
				if(ih_license_support_new(IH_FEATURE_ETH2_KSZ9893)){
					fprintf(fp, "interface gigabitethernet 0/1\n");
				}else{
					fprintf(fp, "interface fastethernet 0/1\n");
				}
				fprintf(fp, "  ip address 192.168.1.1 255.255.255.0\n");
				fprintf(fp, "  ip dhcp-server range 192.168.1.2 192.168.1.100\n");
				fprintf(fp, "  no ip dhcp-server enable\n");
				fprintf(fp, "  track l2-state\n");
				fprintf(fp, "  ip nat inside\n");

				fprintf(fp, "!\n");
				if(ih_license_support_new(IH_FEATURE_ETH2_KSZ9893)){
					fprintf(fp, "interface gigabitethernet 0/2\n");
				}else{
					fprintf(fp, "interface fastethernet 0/2\n");
				}
				fprintf(fp, "  ip address 192.168.2.1 255.255.255.0\n");
				fprintf(fp, "  ip dhcp-server range 192.168.2.2 192.168.2.100\n");
				fprintf(fp, "  ip dhcp-server enable\n");
				fprintf(fp, "  track l2-state\n");
				fprintf(fp, "  ip nat inside\n");
			}
		} else {
#ifndef INHAND_IDTU9
			fprintf(fp, "#ethernet interface config\n");
			fprintf(fp, "!\n");
#if !(defined INHAND_VG9 || defined INHAND_IR8)
			fprintf(fp, "interface fastethernet 0/1\n");
			fprintf(fp, "  ip address 192.168.1.1 255.255.255.0\n");
			fprintf(fp, "  ip dhcp-server range 192.168.1.2 192.168.1.100\n");
			fprintf(fp, "  no ip dhcp-server enable\n");
			fprintf(fp, "  track l2-state\n");
			fprintf(fp, "  ip nat inside\n");
#endif

			fprintf(fp, "#vlan interface config\n");
			fprintf(fp, "!\n");
			fprintf(fp, "interface vlan 1\n");
			fprintf(fp, "  ip address 192.168.2.1 255.255.255.0\n");
			fprintf(fp, "  ip dhcp-server range 192.168.2.2 192.168.2.100\n");
			fprintf(fp, "  ip dhcp-server enable\n");
			fprintf(fp, "  ip nat inside\n");
#endif
		}
	}

	fprintf(fp, "#Management services config\n");
	fprintf(fp, "!\n");
	if (!strcmp(oem, "global-ge")){
#ifndef NEW_WEBUI
		fprintf(fp, "ip http server port 80\n");
		fprintf(fp, "ip http access enable\n");
#endif
		fprintf(fp, "access-list 182 permit tcp any any eq 80 log\n");
		fprintf(fp, "ip https server port 443\n");
		fprintf(fp, "ip https access enable\n");
		fprintf(fp, "access-list 182 permit tcp any any eq 443 log\n");
		fprintf(fp, "ip telnet server port 23\n");
		fprintf(fp, "ip telnet access enable\n");
		fprintf(fp, "access-list 182 permit tcp any any eq 23 log\n");
		fprintf(fp, "ip ssh server port 22\n");
		fprintf(fp, "ip ssh access enable\n");
		fprintf(fp, "access-list 182 permit tcp any any eq 22 log\n");
		fprintf(fp, "interface cellular 1\n");
		fprintf(fp, "ip access-group 182 admin\n");
	} else {
#ifndef NEW_WEBUI
#ifdef INHAND_IDTU9
		fprintf(fp, "no ip http server\n");
#else
		fprintf(fp, "ip http server port 80\n");
#endif
#endif
		fprintf(fp, "ip https server port 443\n");
		fprintf(fp, "ip https access enable\n");
		fprintf(fp, "no ip telnet server\n");
		fprintf(fp, "access-list 192 permit tcp any any eq 443 log\n");
		fprintf(fp, "access-list 192 deny tcp any any eq 80\n");
#if (defined INHAND_VG9 || defined INHAND_IR8)
		//for restapi
		fprintf(fp, "access-list 192 deny tcp any any eq 6000\n");
		fprintf(fp, "access-list 192 deny tcp any any eq 60000\n");
		//for local_broker
		fprintf(fp, "access-list 192 deny tcp any any eq 1085\n");
#endif
		fprintf(fp, "access-list 192 deny tcp any any eq 23\n");
		fprintf(fp, "access-list 192 deny tcp any any eq 22\n");
		fprintf(fp, "access-list 192 deny tcp any any eq 53\n");
		fprintf(fp, "access-list 192 deny udp any any eq 53\n");
		fprintf(fp, "interface cellular 1\n");
		fprintf(fp, "ip access-group 192 admin\n");
	}
#else /* IR800 or IP800 */
	fprintf(fp, "#switch vlan config\n");
	fprintf(fp, "!\n");
	fprintf(fp, "interface vlan 1\n");
	fprintf(fp, "  ip address 192.168.2.1 255.255.255.0\n");
	fprintf(fp, "  ip dhcp-server range 192.168.2.2 192.168.2.100\n");
	fprintf(fp, "  ip dhcp-server enable\n");
	fprintf(fp, "  ip nat inside\n");
	if (ih_license_support_new(IH_FEATURE_MODEM_NONE)) {
		/*use Vlan10 interface as WAN*/
		fprintf(fp, "#vlan interface config\n");
		fprintf(fp, "!\n");
		fprintf(fp, "interface vlan 10\n");
		fprintf(fp, "  ip address 192.168.1.1 255.255.255.0\n");
		fprintf(fp, "  ip nat outside\n");

		fprintf(fp, "#ethernet config\n");
		fprintf(fp, "!\n");
		fprintf(fp, "interface fastethernet 1/1\n");
		fprintf(fp, "  switchport access vlan 10\n");
	}

	fprintf(fp, "#dot11 SSID config\n");
	fprintf(fp, "!\n");
	if(strcmp(oem, "welotec")==0 || strcmp(oem, "welotec-oem")==0){
		fprintf(fp, "dot11 ssid TK800\n");
	}else{
		fprintf(fp, "dot11 ssid InPortal800\n");
	}
	fprintf(fp, "  authentication open\n");

	fprintf(fp, "#dot11 dot11radio config\n");
	fprintf(fp, "!\n");
	fprintf(fp, "interface dot11radio 1\n");
	fprintf(fp, "  channel 4\n");
	fprintf(fp, "  radio-type dot11gn\n");
	if(strcmp(oem, "welotec")==0 || strcmp(oem, "welotec-oem")==0){
		fprintf(fp, "  ssid TK800\n");
	}else{
		fprintf(fp, "  ssid InPortal800\n");
	}
	fprintf(fp, "  ip address 192.168.3.1 255.255.255.0\n");
	fprintf(fp, "  ip dhcp-server range 192.168.3.2 192.168.3.100\n");
	fprintf(fp, "  ip dhcp-server enable\n");
	fprintf(fp, "  ip nat inside\n");

	fprintf(fp, "#nginx web server config\n");
	fprintf(fp, "!\n");
	fprintf(fp, "nginx enable\n");

	fprintf(fp, "#portal config\n");
	fprintf(fp, "!\n");
	fprintf(fp, "portal interface dot11radio 1\n");
	fprintf(fp, "portal homepage http://192.168.3.1\n");
	fprintf(fp, "portal splash-form splash-noclick.html\n");
	fprintf(fp, "portal enable\n");
#endif
	if (!ih_license_support_new(IH_FEATURE_MODEM_NONE)) {
		fprintf(fp, "#cellular config\n");
		fprintf(fp, "!\n");
		if(strcmp(oem, "welotec")==0 || strcmp(oem, "welotec-oem")==0) {
			if(product[0] == 'V' || product[0] == 'v'){
				if(product[1] == 'S' || product[1] == 's'){		
					fprintf(fp, "cellular 1 cdma profile 1 #777 ipv4 auto\n");		
				}else {	
					fprintf(fp, "cellular 1 cdma profile 1 #777 ipv4 auto CARD CARD\n");						
				}
			}else {
				fprintf(fp, "cellular 1 gsm profile 1 internet.t-d1.de *99***1# ipv4v6 auto tm tm\n");		
			}
			fprintf(fp, "!\n");
		}else {
			if(product[0] == 'V' || product[0] == 'v'){
				if(product[1] == 'S' || product[1] == 's'){		
					fprintf(fp, "cellular 1 cdma profile 1 #777 ipv4  auto\n");		
				}else {	
					fprintf(fp, "cellular 1 cdma profile 1 #777 ipv4 auto CARD CARD\n");						
				}	
				fprintf(fp, "!\n");
			}else {
				if(ih_key_support("TL00")) {
					fprintf(fp, "cellular 1 gsm profile 1 cmnet *99***1# ipv4v6 auto gprs gprs\n");
					//FIXME MERGE
				}else if(ih_key_support("FT10")
					|| ih_key_support("FS28")){
					fprintf(fp, "cellular 1 gsm profile 1 VZWINTERNET *99***3# auto gprs gprs\n");
				}else if (ih_key_support("FH26")) {
					fprintf(fp, "cellular 1 gsm profile 1 VZWINTERNET *99***1# auto gprs gprs\n");
				}else if (ih_key_support("FA41")) {
					fprintf(fp, "cellular 1 gsm profile 1 3gnet *99***1# auto gprs gprs\n");
				}else {
					fprintf(fp, "cellular 1 gsm profile 1 3gnet *99***1# ipv4 auto gprs gprs\n");
				}
				fprintf(fp, "!\n");
			}
		}

		fprintf(fp, "#cellular interface config\n");
		fprintf(fp, "!\n");
		fprintf(fp, "interface cellular 1\n");
		fprintf(fp, "  ip address static netmask 255.255.255.255\n");
		fprintf(fp, "  ppp ipcp dns request\n");
		if(modem_support_dhcp_option_mtu()){
			fprintf(fp, "  dhcp option mtu request\n");
		}
		fprintf(fp, "  ip nat outside\n");
		//fprintf(fp, "  ip tcp adjust-mss 1360\n");

		fprintf(fp, "#static route config\n");
		fprintf(fp, "!\n");
		fprintf(fp, "ip route 0.0.0.0 0.0.0.0 cellular 1 255\n");
		fprintf(fp, "ipv6 route ::/0  cellular 1\n");

		fprintf(fp, "#firewall config\n");
		fprintf(fp, "!\n");
		fprintf(fp, "ip snat inside list 100 interface cellular 1\n");
		fprintf(fp, "access-list 100 permit ip any any\n");
	} else {
#if (!(defined INHAND_IP812))
		fprintf(fp, "#static route config\n");
		fprintf(fp, "!\n");
		if(ih_license_support_new(IH_FEATURE_ETH2_KSZ9893)){
			fprintf(fp, "ip route 0.0.0.0 0.0.0.0 gigabitethernet 0/1 192.168.1.254\n");
		}else{
			fprintf(fp, "ip route 0.0.0.0 0.0.0.0 fastethernet 0/1 192.168.1.254\n");
		}

		fprintf(fp, "#firewall config\n");
		fprintf(fp, "!\n");
		if(ih_license_support_new(IH_FEATURE_ETH2_KSZ9893)){
			fprintf(fp, "interface gigabitethernet 0/1\n");
		}else{
			fprintf(fp, "interface fastethernet 0/1\n");
		}
		fprintf(fp, "  ip nat outside\n");

		fprintf(fp, "!\n");
		if(ih_license_support_new(IH_FEATURE_ETH2_KSZ9893)){
			fprintf(fp, "ip snat inside list 100 interface gigabitethernet 0/1\n");
		}else{
			fprintf(fp, "ip snat inside list 100 interface fastethernet 0/1\n");
		}
		fprintf(fp, "access-list 100 permit ip any any\n");
#else
		fprintf(fp, "#static route config\n");
		fprintf(fp, "!\n");
		fprintf(fp, "ip route 0.0.0.0 0.0.0.0 vlan 10 192.168.1.254\n");

		fprintf(fp, "#firewall config\n");
		fprintf(fp, "!\n");
		fprintf(fp, "ip snat inside list 100 interface vlan 10\n");
		fprintf(fp, "access-list 100 permit ip any any\n");
#endif
	}

	fprintf(fp, "#tcp mss config\n");
	fprintf(fp, "!\n");
	fprintf(fp, "ip tcp adjust-mss 1360\n");

#if (!(defined INHAND_IP812))
	fprintf(fp, "#sntp client config\n");
	fprintf(fp, "!\n");
	if(!ih_key_support("FQ88")) {
#ifndef INHAND_IDTU9
		fprintf(fp, "sntp-client\n");
#endif
	}

	fprintf(fp, "sntp-client server 0.pool.ntp.org port 123\n");
	fprintf(fp, "sntp-client server 1.pool.ntp.org port 123\n");
	fprintf(fp, "sntp-client server 2.pool.ntp.org port 123\n");
	fprintf(fp, "sntp-client server 3.pool.ntp.org port 123\n");

	fprintf(fp, "#ntp server config\n");
	fprintf(fp, "!\n");
	fprintf(fp, "ntp server 0.pool.ntp.org\n");
	fprintf(fp, "ntp server 1.pool.ntp.org\n");
	fprintf(fp, "ntp server 2.pool.ntp.org\n");
	fprintf(fp, "ntp server 3.pool.ntp.org\n");
#endif

#if (defined INHAND_VG9)
	if (ih_license_support_new(IH_FEATURE_GPS)) {
		fprintf(fp, "!\n");
		fprintf(fp, "gps enable\n");
	}

	fprintf(fp, "!\n");
	fprintf(fp, "#powerMgmt config\n");
	fprintf(fp, "power-management shutdown-delay 30\n");
	fprintf(fp, "power-management standby-mode 1\n");
	fprintf(fp, "power-management standby-check-interval 20\n");
	fprintf(fp, "power-management standby-voltage 90\n");
	fprintf(fp, "power-management standby-resume-voltage 105\n");

	fprintf(fp, "!\n");
	fprintf(fp, "#io config\n");
	fprintf(fp, "no io pull-up digital-input 1\n");        
	fprintf(fp, "no io pull-up digital-input 2\n");        
	fprintf(fp, "no io pull-up digital-input 3\n");        
	fprintf(fp, "no io pull-up digital-input 4\n");        
	fprintf(fp, "no io pull-up digital-input 5\n");        
	fprintf(fp, "no io pull-up digital-input 6\n");        

	fprintf(fp, "no io pull-up digital-output 1\n");        
	fprintf(fp, "no io pull-up digital-output 2\n");        
	fprintf(fp, "no io pull-up digital-output 3\n");        
	fprintf(fp, "no io pull-up digital-output 4\n");        

	fprintf(fp, "io output digital 1 low\n");        
	fprintf(fp, "io output digital 2 low\n");        
	fprintf(fp, "io output digital 3 low\n");        
	fprintf(fp, "io output digital 4 low\n");        

#endif

	fprintf(fp, CONFIG_END_SECTION);
	fclose(fp);

	ret = save_is_config(STARTUP_CONFIG_FILE);

	return ret;
}

int restore_backups_dir(void)
{
	char cmd[64] = {0};
	int ret;

	unlink("/var/backups/ipsec.conf");
	unlink("/var/backups/ipsec.secrets");
	/* if remove ROOTCA_DB_PATH directory but not reboot, files can not be imported to this directory any more */
	snprintf(cmd, sizeof(cmd), "rm -rf %s*", ROOTCA_DB_PATH);
	ret = system(cmd);
	if(ret < 0) {
		LOG_ER("Execute command failed: %s,ret:%d", cmd, ret);
		return -1;
	}

	if(!access("/var/backups/superadm", F_OK)){
		memset(cmd, 0, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "rm -rf %s", "/var/backups/superadm");
		ret = system(cmd);
		if(ret < 0) {
			LOG_ER("Execute command failed: %s,ret:%d", cmd, ret);
			return -1;
		}
	}

	if(!access(PORTAL_BACKUP_PATH, F_OK)){
		snprintf(cmd, sizeof(cmd), "rm -rf %s", PORTAL_BACKUP_PATH);
		ret = system(cmd);
		if(ret < 0) {
			LOG_ER("Execute command failed: %s,ret:%d", cmd, ret);
			return -1;
		}
	}

	if(!access(AGENT_PORTAL_BACKUP_PATH, F_OK)){
		snprintf(cmd, sizeof(cmd), "rm -rf %s", AGENT_PORTAL_BACKUP_PATH);
		ret = system(cmd);
		if(ret < 0) {
			LOG_ER("Execute command failed: %s,ret:%d", cmd, ret);
			return -1;
		}
	}

	if(f_exists(STARTUP_TZ_FILE)){
		if(unlink(STARTUP_TZ_FILE)){
			LOG_ER("Can't unlink timezone file");
			return -1;
		}
	}

	return 0;
}

//clear startup xml config, the datastore will reinit after system reboot
int restore_xml_config(void)
{
#if 0
    system("echo > /tmp/test.xml");
    system("sysrepocfg -I/tmp/test.xml -d startup");
#endif
    system("rm -rf /var/backups/sysrepo");

	return 0;
}

int restore_xml_config_from_backup_config(void)
{
    system("sysrepocfg -I"XML_BACKUP_CONFIG_FILE);

	return 0;
}

//backup xml config file which can link up and connect to NM server
int backup_xml_config(void)
{
    system("sysrepocfg -X > "XML_BACKUP_CONFIG_FILE);

	return 0;
}

/*
 * Services has its own database, DO CLEAR WHEN DO FACTORY RESET
 */
typedef struct {
	char name[32];       // name of service
	char db_path[128];   // path of database
} DATABASE_SERVICE;

static DATABASE_SERVICE db_services[] = {
	{"NetworkManager", NM_DB_FILE_PATH},
	{"events", EVENTS_DB_FILE},
	{"ih_record",      RECORD_DB_FILE_PATH},
};

int restore_database(void)
{
#ifndef NEW_WEBUI
	int i;
	char cmd[512];

	for (i = 0; i < sizeof(db_services)/sizeof(db_services[0]); i++) {
		snprintf(cmd, sizeof(cmd), "killall %s; rm -f %s", db_services[i].name, db_services[i].db_path);
		system(cmd);
	}

	return 0;
#else
	/* InhandTag: clear configs in shadow.db */
	if(f_exists(SHADOW_DB_FILE)) {
		if (unlink(SHADOW_DB_FILE)) {
			LOG_ER("Can't unlink shadow db, restore failed.");
			goto EXIT_END;
		}
	}
	if(f_exists(SHADOW_BACKUP_DB_FILE )) {
        if (unlink(SHADOW_BACKUP_DB_FILE )) {
            LOG_ER("Can't unlink shadow backup db, restore failed.");
            goto EXIT_END;
        }
    }
	syslog(LOG_INFO, "restore=> clear config shadow db succeeded.");
	/* InhandTag: clear msg in NM.db */
	if(f_exists(NM_DB_FILE_PATH)) {
		if (unlink(NM_DB_FILE_PATH)) {
			LOG_ER("Can't unlink NM db, restore failed.");
			goto EXIT_END;
		}
	}
	syslog(LOG_INFO, "restore=> clear NM msg db succeeded.");
	/* InhandTag: clear events in events.db */
	if(f_exists(EVENTS_DB_FILE)) {
		if (unlink(EVENTS_DB_FILE)) {
			LOG_ER("Can't unlink events db, restore failed.");
			goto EXIT_END;
		}
	}
	syslog(LOG_INFO, "restore=> clear events db succeeded.");
	/* InhandTag: clear events in ih_record.db */
	if(f_exists(RECORD_DB_FILE_PATH)) {
		if (unlink(RECORD_DB_FILE_PATH)) {
			LOG_ER("Can't unlink ih_record db, restore failed.");
			goto EXIT_END;
		}
	}
	syslog(LOG_INFO, "restore=> clear record db succeeded.");

	if(f_exists(JSON_PRESIST_DATA_PATH)) {
		if (unlink(JSON_PRESIST_DATA_PATH)) {
			LOG_ER("Can't unlink service db, restore failed.");
			goto EXIT_END;
		}
	}
	syslog(LOG_INFO, "restore=> clear service db succeeded.");
	return 0;

EXIT_END:
	return -1;
#endif
}
