/*
 * $Id$ --
 *
 *   Misc routines
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <signal.h>
#include <wait.h>
#include <jansson.h>

#include "ih_cmd.h"
#include "ih_ipc.h"
#include "gps_ipc.h"
#include "obd_ipc.h"
#include "pwr_mgmt_ipc.h"
#include "shutils.h"
#include "shared.h"
#include "version.h"
#include "proto/ethernet.h"
#include "alert_def.h"
#include "hash.h"

struct _proto_defines{
	int id;
	char *proto;
};

static struct _proto_defines proto_defines[] = {
	{PROTO_STATIC,		"static"},
	{PROTO_DHCP,		"dhcp"},
	{PROTO_DISABLED,	"disabled"},
	{PROTO_INVALID,		NULL}
};

/**
 * Parse proto string to proto id
 * @param	proto	proto string
 * @return	proto id
 */
int parse_proto(const char* proto)
{
	struct _proto_defines *p;

	if(!proto || !*proto) return PROTO_INVALID;

	for(p=proto_defines; p->proto; p++){
		if(strcmp(proto, p->proto)==0) return p->id;
	}

	return PROTO_INVALID;
}

void add_ip_addr(char *iface, char *ip, char *netmask)
{
	char ip_netmask[64];

	memset(ip_netmask, 0, sizeof(ip_netmask));
	snprintf(ip_netmask, sizeof(ip_netmask), "%s/%d", ip, netmask_aton(netmask));

	//LOG_IN("ip addr add %s dev %s", ip_netmask, iface);
	eval("ip", "addr", "add", ip_netmask, "dev", iface);
	//eval("ifconfig", iface, ip, "netmask", netmask);
}

void del_ip_addr(char *iface, char *ip, char *netmask)
{
	char ip_netmask[64];

	memset(ip_netmask, 0, sizeof(ip_netmask));
	snprintf(ip_netmask, sizeof(ip_netmask), "%s/%d", ip, netmask_aton(netmask));
	//LOG_IN("ip addr del %s dev %s", ip_netmask, iface);
	eval("ip", "addr", "del", ip_netmask, "dev", iface);
	//eval("ifconfig", iface, "0.0.0.0");
}


void flush_ip_addr(char *iface)
{
	eval("ip", "-4", "addr", "flush", "dev", iface);
	eval("ip", "-6", "addr", "flush", "dev", iface, "scope", "global");
}

void flush_ip6_addr(char *iface)
{
	eval("ip", "-6", "addr", "flush", "dev", iface, "scope", "global");
}

void flush_ip4_addr(char *iface)
{
	eval("ip", "-4", "addr", "flush", "dev", iface);
}

/**
 * Parse proto id to proto string
 * @param	proto	proto id
 * @return	proto string
 */
char* get_proto_name(int id)
{
	struct _proto_defines *p;

	for(p=proto_defines; p->proto; p++){
		if(p->id==id) return p->proto;
	}

	return NULL;
}


/**
 * Convert dot-seperated netmask to bit length
 * such as : 255.255.255.0 => 24
 * such as : 255.255.255.255 => 32
 * @param	netmask		dot-seperated netmask
 * @return	bit length, or -1 for bad netmask
 */
int netmask_aton(const char* netmask)
{
	unsigned int n = inet_addr(netmask);
	int i = 0;

	/*add by zly*/
	if(strcmp(netmask, "255.255.255.255")==0) return 32;

	if(n==-1){
		syslog(LOG_ERR, "invalid netmask %s", netmask);
		return -1; //invalid netmask
	}

	n = ntohl(n);
	while(n&0x80000000){
		n = n << 1;
		i++;
	}

	if(n){//invalid netmask
		syslog(LOG_ERR, "invalid netmask %s", netmask);
		return -1;
	}

	return i;
}

/**
 * Convert in_addr netmask to bit length
 * such as : 255.255.255.0 => 24
 * such as : 255.255.255.255 => 32
 * @param	netmask		dot-seperated netmask
 * @return	bit length, or -1 for bad netmask
 */
int netmask_inaddr2len(const struct in_addr netmask)
{
	unsigned int n = netmask.s_addr;
	int i = 0;

	if(n == 0xffffffff) return 32;

	n = ntohl(n);
	while(n&0x80000000){
		n = n << 1;
		i++;
	}

	if(n){//invalid netmask
		syslog(LOG_ERR, "invalid netmask %08x", netmask.s_addr);
		return -1;
	}

	return i;
}

/* Maskbit. */
static const u_char maskbit[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0,
			         0xf8, 0xfc, 0xfe, 0xff};
/* Convert masklen into IP address's netmask. */
void masklen2ip (int masklen, struct in_addr *netmask)
{
  u_char *pnt;
  int bit;
  int offset;

  if (!netmask) {
  	LOG_ER("netmask is null");
  	return;
  }

  memset (netmask, 0, sizeof (struct in_addr));
  pnt = (unsigned char *) netmask;

  offset = masklen / 8;
  bit = masklen % 8;
  
  while (offset--)
    *pnt++ = 0xff;

  if (bit)
    *pnt = maskbit[bit];
}


#ifdef WIN32
/**
* Parse address string to in_addr struct
* @param	cp		address in standard numbers-and-dots notation
* @param	inp		in_addr to be stored
* @return	nonzero if  the  address  is valid, zero if not.
*/
int
inet_aton(const char *cp, struct in_addr *inp)
{
	inp->s_addr = inet_addr(cp);

	if(inp->s_addr==-1) return 0;

	return inp->s_addr;
}
#endif//WIN32


/*
 *Get ipv4 subnet str from ip/mask
 * */
void ipaddr_to_subnet(char *ipaddr, char *netmask, char *subnet_str)
{
	struct in_addr in_ip, in_netmask, subnet;

	inet_aton(ipaddr, &in_ip);
	inet_aton(netmask, &in_netmask);

	subnet.s_addr = in_ip.s_addr&in_netmask.s_addr;

	memcpy(subnet_str, inet_ntoa(subnet), INET_ADDRSTRLEN);
}

/*
descrption: format ip/mask string: 
	input: 123.123.123.123/24 or 123.123.123.123/255.255.255.255
	output: in_addr of 123.123.123.123, and masklen 24
*/
int ipmask_aton(char *ipmask, struct in_addr *ipaddr, int *masklen)
{

	char *ip, *mask;
	struct in_addr in;
	int mask_len1;
	int ret;

	if (!ipmask || !strlen(ipmask)) {
		return ERR_INVALID;
	}

	ip = ipmask;
	mask = strstr(ipmask, "/");
	if (mask) {
		*mask = '\0';
	}

	ret = inet_aton(ip, &in);
	if(ret==0) {
		LOG_IN("invalid ip address string");
		if (mask) {
			*mask = '/';
		}

		return ERR_INVALID;
	}

	if (mask) {
		*mask = '/';
		mask++;

		if (strspn (mask, "0123456789") != strlen (mask)) {
			mask_len1 = netmask_aton(mask);
		} else {
			mask_len1 = atoi(mask);
		}
	} else {
		mask_len1 = 32;
	}

	if (mask_len1 < 0 || mask_len1 > 32) {
		LOG_IN("invalid mask len %d", mask_len1);
		return ERR_INVALID;
	}

	ipaddr->s_addr = in.s_addr;
	*masklen = mask_len1;

	return ERR_OK;
}

/*
descrption: format subnet string: 
	input: 123.123.123.123/24 or 123.123.123.123/255.255.255.255
	output: in_addr of 123.123.123.0, and masklen 24
*/
int subnet_aton(char *ipmask, struct in_addr *subnet, int *masklen)
{

	char *ip, *mask;
	struct in_addr in, in_mask;	
	int mask_len1;
	int ret;

	if (!ipmask || !strlen(ipmask)) {
		return ERR_INVALID;
	}

	ip = ipmask;
	mask = strstr(ipmask, "/");
	if (!mask) {
		return ERR_INVALID;
	}

	*mask = '\0';

	ret = inet_aton(ip, &in);
	if(ret==0) {
		LOG_IN("invalid ip address string");
		return ERR_INVALID;
	}

	*mask = '/';
	mask++;

	if (strspn (mask, "0123456789") != strlen (mask)) {
		mask_len1 = netmask_aton(mask);
	} else {
		mask_len1 = atoi(mask);
	}

	if (mask_len1 < 0 || mask_len1 > 32) {
		LOG_IN("invalid mask len %d", mask_len1);
		return ERR_INVALID;
	}

	masklen2ip (mask_len1, &in_mask);
	subnet->s_addr = in.s_addr&in_mask.s_addr;
	*masklen = mask_len1;

	return ERR_OK;
}
void notice_set(const char *path, const char *format, ...)
{
	char p[256];
	char buf[2048];
	va_list args;

	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	
#ifdef WIN32
	mkdir("notice");
	snprintf(p, sizeof(p), "notice/%s", path);
#else
	mkdir("/var/notice", 0755);
	snprintf(p, sizeof(p), "/var/notice/%s", path);
#endif//WIN32

	f_write_string(p, buf, 0, 0);
	if (buf[0]) syslog(LOG_INFO, "notice: %s", buf);
}

// -----------------------------------------------------------------------------

void set_action(int a)
{
	int r = 3;
	while (f_write(ACTION_FILE, &a, sizeof(a), 0, 0) != sizeof(a)) {
		sleep(1);
		if (--r == 0) return;
	}
	if (a != ACT_IDLE) sleep(2);
}

int check_action(void)
{
	int a = ACT_IDLE;
	int r = 3;
	int len;
	
	do{
		len = f_read(ACTION_FILE, &a, sizeof(a));
		if(len<0) return ACT_IDLE;
		if(len==sizeof(a)) break;

		sleep(1);
		if (--r == 0) return ACT_UNKNOWN;
	}while (f_read(ACTION_FILE, &a, sizeof(a)) != sizeof(a));

	return a;
}

int wait_action_idle(int n)
{
	while (n-- > 0) {
		if (check_action() == ACT_IDLE) return 1;
		sleep(1);
	}
	return 0;
}


int
schedule_reboot(int ndelay)
{
	char buf[16];
	pid_t pid;

	sprintf(buf, "%d", ndelay);
	pid = fork();
	if(pid == 0) {
		execl("/sbin/reboot","reboot", buf, NULL);
	}

	return 0;
}

int
diagnose(void)
{
	FILE *fp;

	if(!(fp = fopen(DIAG_OUTPUT_TMP, "w"))){
		syslog(LOG_ERR, "failed to write to diagnose file!");
		return -1;
	}

	fprintf(fp, "################# DIAGNOSE BEGIN ##############\n");
	fprintf(fp, "name        : %s\n", INHAND_PRODUCT_NAME);
	fprintf(fp, "oem name    : %s\n", INHAND_OEM_NAME);
	fprintf(fp, "version     : %s\n", INHAND_PRODUCT_VERSION);
	fprintf(fp, "ext version : %s\n", INHAND_BUILD_EXT_VERSION);
	fprintf(fp, "build time  : %s\n", INHAND_PRODUCT_BUILD_TIME);
	fprintf(fp, "build type  : %s\n", INHAND_BUILD_TYPE);
//	fprintf(fp, "build info  : %s\n", INHAND_BUILD_INFO);
	fprintf(fp, "---------------------------------------------\n");
	fprintf(fp, "      * model\n");
	fprintf(fp, "%s\n", g_sysinfo.model_name);
	fprintf(fp, "---------------------------------------------\n");
	fprintf(fp, "      * oem\n");
	fprintf(fp, "%s\n", g_sysinfo.oem_name);
	fprintf(fp, "---------------------------------------------\n");
	fprintf(fp, "      * product number\n");
	fprintf(fp, "%s\n", g_sysinfo.product_number);
	fprintf(fp, "---------------------------------------------\n");
	fprintf(fp, "      * serial number\n");
	fprintf(fp, "%s\n", g_sysinfo.serial_number);

	fclose(fp);

#ifndef CLOUD_WEB
	/* wucl added running config & startup config */
	system("echo -------- date ----------->>"DIAG_OUTPUT_TMP);
	system("date >>" DIAG_OUTPUT_TMP);
	system("echo -------- running config ----------->>"DIAG_OUTPUT_TMP);
	system("cli -O /tmp/dia_running.conf");
	system("cat /tmp/dia_running.conf>>"DIAG_OUTPUT_TMP);
	unlink("/tmp/dia_running.conf");

	system("echo -------- startup config ----------->>"DIAG_OUTPUT_TMP);
	system("cat "STARTUP_CONFIG_FILE">>" DIAG_OUTPUT_TMP);
#else
	system("echo -------- running config----------->>"DIAG_OUTPUT_TMP);
	system("cat "CONF_FILE">>" DIAG_OUTPUT_TMP);

	system("echo >>"DIAG_OUTPUT_TMP);	//blank line
	system("echo -------- shadow db ----------->>"DIAG_OUTPUT_TMP);
	system("shadowdb >>" DIAG_OUTPUT_TMP);
#endif

	system("diagnose.sh >> " DIAG_OUTPUT_TMP);

	return 0;
}

int diagnose_save(void)
{
	char timestamp_buf[64] = {0};
	char filename[128] = {0};
	time_t ts_start;
	time_t ts_end;
	int ret;
	int status;

	LOG_IN("diagnose_save...");
	ts_start = time(NULL);
	diagnose();
	
	strftime(timestamp_buf, sizeof(timestamp_buf), "%Y%m%dT%H%M%SZ", gmtime(&ts_start));
	snprintf(filename, sizeof(filename), "%s%s%s.dat", BACKUP_DIAG_DIR, "diagnose-", timestamp_buf);
	f_copy(DIAG_OUTPUT_TMP, filename);
	sync_file_write(filename);

	unlink(DIAG_OUTPUT_TMP);

	status = system("diag_clean.sh");
	if(WIFEXITED(status)){
		ret = WEXITSTATUS(status);
		if(ret == 0){
			//LOG_WA("exec diag_clean.sh OK.");
		}else{
			LOG_WA("failed to exec diag_clean.sh: %d(%s)", ret, strerror(ret));
		}
	}else if(WIFSIGNALED(status)){
		LOG_WA("diag_clean.sh exited by signal with status %d\n", WTERMSIG(status));
	}

	ts_end = time(NULL);
	LOG_IN("diagnose_save Done. Spent time: %s\n", age(ts_start, ts_end));

	return 0;
}

int modprobe(const char *mod)
{
	return eval("modprobe", "-s", (char *)mod);
}

int modprobe_r(const char *mod)
{
	return eval("modprobe", "-r", (char *)mod);
}


/*
 * Convert Ethernet address string representation to binary data
 * @param	a	string in xx:xx:xx:xx:xx:xx notation
 * @param	e	binary data
 * @return	TRUE if conversion was successful and FALSE otherwise
 */
int ether_atoe(const char *a, unsigned char *e)
{
	char *x;
	int i;

	i = 0;
	while (1) {
		e[i++] = (unsigned char) strtoul(a, &x, 16);
		if (a == x) break;
		if (i == ETHER_ADDR_LEN) return 1;
		if (*x == 0) break;
		a = x + 1;
	}
	//memset(e, 0, sizeof(e));
	return 0;
}

/*
 * Convert Ethernet address binary data to string representation
 * @param	e	binary data
 * @param	a	string in xx:xx:xx:xx:xx:xx notation
 * @return	a
 */
char *ether_etoa(const unsigned char *e, char *a)
{
	sprintf(a, "%02X:%02X:%02X:%02X:%02X:%02X", e[0], e[1], e[2], e[3], e[4], e[5]);
	return a;
}

time_t get_uptime(void)
{
#ifdef WIN32
	return time(NULL);
#else
	struct sysinfo si;
	sysinfo(&si);

	return si.uptime;
#endif
}

//convert seconds to # days, ##:##:##
char *second_to_day(time_t time)
{
	static char buf[32];
	unsigned int days, hh, mm, ss, tmp;
	
	days = time/86400;
	tmp = time%86400;
	hh = tmp/3600;
	tmp = tmp%3600;
	mm = tmp/60;
	ss = tmp%60;

	sprintf(buf, "%u day%s, %02u:%02u:%02u", days, ((days<=1)?"":"s"), hh, mm, ss);

	return buf;
}

#define YEAR_IN_SEC (12*MONTH_IN_SEC)
#define MONTH_IN_SEC (30*DAY_IN_SEC)
#define DAY_IN_SEC (24*HOUR_IN_SEC)
#define HOUR_IN_SEC (60*MINUTE_IN_SEC)
#define MINUTE_IN_SEC 60

char *add_str(int length, char *str)
{
	return ((length > 1) && str)? str: "";
}

char *add_s(int count)
{
	return (count > 1)? "s": "";
}

char *age(time_t birth, time_t now)
{
	double time_diff = 0.0f;
	long int time_left = 0;
	static char age_buf[128] = {0};
	int years = 0;
	int months = 0;
	int days = 0;
	int hours = 0;
	int minutes = 0;
	int seconds = 0;
	int print_len = 0;

	time_diff = difftime(now, birth);
	time_left = (long int)time_diff;

	years = time_left / YEAR_IN_SEC;
	time_left = time_left % YEAR_IN_SEC;

	months = time_left / MONTH_IN_SEC;
	time_left = time_left % MONTH_IN_SEC;

	days = time_left / DAY_IN_SEC;
	time_left = time_left % DAY_IN_SEC;

	hours = time_left / HOUR_IN_SEC;
	time_left = time_left % HOUR_IN_SEC;

	minutes = time_left / MINUTE_IN_SEC;
	time_left = time_left % MINUTE_IN_SEC;

	seconds = time_left; 

	if(years || print_len){
		print_len += snprintf(age_buf+print_len, sizeof(age_buf)-print_len, "%s%ld year%s", add_str(print_len, ","), (long int)years, add_s(years));	
	}

	if(months || print_len){
		print_len += snprintf(age_buf+print_len, sizeof(age_buf)-print_len, "%s%ld month%s", add_str(print_len, ","), (long int)months, add_s(months));	
	}

	if(days || print_len){
		print_len += snprintf(age_buf+print_len, sizeof(age_buf)-print_len, "%s%ld day%s", add_str(print_len, ","), (long int)days, add_s(days));	
	}

	if(hours || print_len){
		print_len += snprintf(age_buf+print_len, sizeof(age_buf)-print_len, "%s%ld hour%s", add_str(print_len, ","), (long int)hours, add_s(hours));	
	}

	if(minutes || print_len){
		print_len += snprintf(age_buf+print_len, sizeof(age_buf)-print_len, "%s%ld minute%s", add_str(print_len, ","), (long int)minutes, add_s(minutes));	
	}

	if(seconds || print_len){
		print_len += snprintf(age_buf+print_len, sizeof(age_buf)-print_len, "%s%ld second%s", add_str(print_len, ","), (long int)seconds, add_s(seconds));	
	}

	if(!print_len){
		print_len += snprintf(age_buf+print_len, sizeof(age_buf)-print_len, "%s%ld second%s", add_str(print_len, ","), (long int)seconds, add_s(seconds));	
	}

	return age_buf;
}

//duplicate memory
void *
mem_dup(void *p, int len)
{
	void *p2;

	if(!p || len<=0) return NULL;
	p2 = malloc(len);
	if(!p2) return NULL;
	memcpy(p2, p, len);

	return p2;
}

//duplicate argv array
char **
argv_dup(const char **argv)
{
	int i;
	char **args = NULL;

	for(i=0; argv[i]; i++) ;

	//TRACE("argv_dup: len: %d", i);

	args = malloc((i+1)*sizeof(char *));
	if(!args) return NULL;

	for(i=0; argv[i]; i++){
		args[i] = strdup2(argv[i]);
		//TRACE("argv_dup: src: %s, dst: %s", argv[i], args[i]);
		if(!args[i]) {
			syslog(LOG_ERR, "out of memory, failed to duplicate args %s", argv[i]);
			for(i=0; args[i]; i++) free(args[i]);
			free(args);
			return NULL;
		}
	}
	args[i] = NULL;

	return args;
}

//free argv array
void
argv_free(char **argv)
{
	int i;

	if(!argv) return;

	for(i=0; argv[i]; i++) free(argv[i]);
	free(argv);
}

//ih_srand: use hw_random + urandom to rand seed
void ih_srand(unsigned int seed)
{
	int fd;
	unsigned int entropy[2];

	/* /dev/hw_random */
	fd = open("/dev/hw_random", O_RDONLY);
	if(fd>=0) {
		read(fd, (char *)entropy, 8);
		seed += entropy[0] + entropy[1];
		close(fd);
	}
	/* /dev/urandom */
	fd = open("/dev/urandom", O_RDONLY);
	if(fd>=0) {
		read(fd, (char *)entropy, 8);
		seed += entropy[0] + entropy[1];
		close(fd);
	}

	srand(seed);
}

//get a string of random mac address
int get_random_hw_addr(char *hw_addr, int size, int is_inhand)
{
	int i = 0;
	unsigned char random[6]	= {0};

	if(!hw_addr || size < 18){
		return -1;
	}

	ih_srand(time(NULL));

	memset(hw_addr, 0, size);
	for(i = 0; i < 6; i++){
		random[i] = rand() % 255;
	}

	//Reference kernel implementation: eth_hw_addr_random()
	random[0] &= 0xfe; /* clear multicast bit */
	random[0] |= 0x02; /* set local assignment bit (IEEE802) */

	if(is_inhand){
		strlcpy(hw_addr, "00:18:05", size);
		for(i = 3; i < 6; i++){
			snprintf(hw_addr + strlen(hw_addr), size - strlen(hw_addr), ":%02x", random[i]);
		}
	}else{
		for(i = 0; i < 6; i++){
			snprintf(hw_addr + strlen(hw_addr), size - strlen(hw_addr), "%s%02x", i ? ":" : "", random[i]);
		}
	}

	return 0;
}

/*
 * send SIGTERM to pid and wait for at last timeout (in ms)
 * if it does not exit, send SIGKILL
 */
int stop_or_kill(pid_t pid, int timeout)
{
	int status;

	kill(pid, SIGTERM);
	while(timeout > 0) {
		usleep(100*1000);
		if (waitpid(pid, &status, WNOHANG)==pid) return 0;
		timeout -= 100;
	}
	kill(pid, SIGKILL);
	return 0;
}

int gnss_dr_enabled(struct gps_info gps_info)
{
	if((gps_info.dr.gnss_fix == 1) || (gps_info.dr.gnss_fix == 4)){
		return 1;
	}

	return 0;
}

int gnss_prefer_pvt_location_enabled(struct gps_info gps_info)
{
	return gps_info.prefer_pvt_location;
}

int gnss_dr_fix(struct gps_info *gps_info)
{
	if(!gps_info){
		return -1;
	}

	if(!gnss_prefer_pvt_location_enabled(*gps_info) && !gnss_dr_enabled(*gps_info)){
		return -1;
	}

	gps_info->pos.latitude  = gps_info->dr.latitude;
	gps_info->pos.longitude = gps_info->dr.longitude;
	gps_info->pos.altitude  = gps_info->dr.altitude;

	return 0;
}

int get_gps_info(struct gps_info *info)
{	
	IPC_MSG *rsp = NULL;
    int ret = 0; 
    int flag = 1; 

	if(NULL == info){
		return IH_ERR_PARAM;
	}

    ret = ih_ipc_send_wait_rsp(IH_SVC_GPS, DEFAULT_CMD_TIMEOUT, IPC_MSG_GPS_SVCINFO_REQ, (char *)&flag, sizeof(flag), &rsp);
    if (ret) {
        LOG_ER("Request io output val failed[%d]!", ret);
        return IH_ERR_FAILED;
    }     

    memcpy(info, (struct gps_info *)rsp->body, sizeof(struct gps_info));
	ih_ipc_free_msg(rsp);

	gnss_dr_fix(info);

#if 0   
    LOG_IN("gps info latitude:%.8f longitude:%.8f altitude:%f speed:%f, Age: %s", info->pos.latitude, info->pos.longitude, info->pos.altitude, info->speed.n_h_speed, age(info->timestamp, time(NULL)));
#endif  

    return IH_ERR_OK;
	
}

int get_gnss_lat_lon_alt(position pos, double *lat, double *lon, double *alt)
{
	int dir = 0;

	if(pos.latitude > 0.0){
		if(pos.n_s_indicator != 'N'){
			dir = -1;
		}else{
			dir = 1;
		}

		if(lat){
			*lat = pos.latitude * dir;
		}
	}

	if(pos.longitude > 0.0){
		if(pos.e_w_indicator != 'E'){
			dir = -1;
		}else{
			dir = 1;
		}

		if(lon){
			*lon = pos.longitude * dir;
		}
	}

	if(alt){
		*alt = pos.altitude;
	}

	return 0;
}

int get_obd_info(OBD_BASIC_INFO *info)
{
	IPC_MSG *rsp = NULL;
	int ret = 0; 
	int flag = 1; 

	if(NULL == info){
		return IH_ERR_PARAM;
	}

	ret = ih_ipc_send_wait_rsp(IH_SVC_OBD, DEFAULT_CMD_TIMEOUT, IPC_MSG_OBD_INFO_REQ, (char *)&flag, sizeof(flag), &rsp);
	if (ret) {
		LOG_ER("Request obd info failed[%d]!", ret);
		return IH_ERR_FAILED;
	}

	memcpy(info, (OBD_BASIC_INFO *)rsp->body, sizeof(OBD_BASIC_INFO));
	ih_ipc_free_msg(rsp);

	return IH_ERR_OK;
}

int io_set_handle(IO_SET_MSG io_set)
{
	IPC_MSG *rsp = NULL;
	int ret = -1;
	int io_set_status = -1;

	ret = ih_ipc_send_wait_rsp(IH_SVC_PWR_MGMT, 3500, IPC_MSG_IO_SET_REQ, (char *)&io_set, sizeof(IO_SET_MSG), &rsp);
	if(ret){
		LOG_WA("request set io failed[%d]!", ret);
		return ERR_FAILED;
	}

	memcpy(&io_set_status, (int *)rsp->body, sizeof(io_set_status));

	ih_ipc_free_msg(rsp);

	return io_set_status;
}

int get_one_wire_info(PWR_MGMT_BOARD_1WIRE_DATA *info)
{
	IPC_MSG *rsp = NULL;
	int ret = 0; 
	int flag = 1; 

	if(NULL == info){
		return IH_ERR_PARAM;
	}

	ret = ih_ipc_send_wait_rsp(IH_SVC_PWR_MGMT, DEFAULT_CMD_TIMEOUT, IPC_MSG_PWR_MGMT_1WIRE_DATA_REQ, (char *)&flag, sizeof(flag), &rsp);
	if(ret){
		LOG_ER("Request 1-wire info failed[%d]!", ret);
		return IH_ERR_FAILED;
	}

	memcpy(info, (PWR_MGMT_BOARD_1WIRE_DATA *)rsp->body, sizeof(PWR_MGMT_BOARD_1WIRE_DATA));

	ih_ipc_free_msg(rsp);

	return IH_ERR_OK;
}

int event_meta_set(ALERT_OPER op, EVENT_META *meta)
{
	int ret = 0; 

	if(!meta || ((op != ALERT_RAISE) && (op != ALERT_CLEAR))){
		return -1;
	}

	if(op == ALERT_RAISE){
		meta->event_active = 1;
		meta->start_timestamp = time(NULL);

		memset(&meta->start_point, 0, sizeof(struct gps_info));
		ret = get_gps_info(&meta->start_point); 
		if(ret != IH_ERR_OK){
			LOG_IN("Can not get GPS info. ret=%d", ret);
		}
	}else{
		meta->event_active = 0;
		meta->end_timestamp = time(NULL);

		memset(&meta->end_point, 0, sizeof(struct gps_info));
		ret = get_gps_info(&meta->end_point); 
		if(ret != IH_ERR_OK){
			LOG_IN("Can not get GPS info. ret=%d.", ret);
		}
	}

	return 0;
}

int data_meta_set(DATA_META *meta, int valid)
{
	int ret = 0; 

	if(!meta){
		return -1;
	}

	meta->valid = valid? 1: 0;
	meta->timestamp = time(NULL);

	return ret;
}

char crc8(char crc, char *message, int len)
{
	char i = 0;

	if((message == NULL) || (len <= 0)) {
		return 0;
	}

	while(len--) {
		crc ^= *message++;
		for(i = 0;i < 8;i++) {
			if(crc & 0x01){
				crc = (crc >> 1) ^ 0x8c;
			}else {
				crc >>= 1;
			}
		}
	}

	return crc; 
}

//return ERR_OK means IP, ERR_FAILED means Domain name
int is_valid_ip(const char *host)
{
    int count = 0, i = 0, num1, num2, num3, num4;
    
    if(!host)
        return ERR_FAILED;

    for(i = 0; i < strlen(host); i++)
    {
        if(host[i] == '.'){
            count++;
            continue;
        }else if(host[i] < '0' || host[i] > '9'){
            return ERR_FAILED;
        }
    }

    if(count != 3)
        return ERR_FAILED;

    if(4 == sscanf(host, "%d.%d.%d.%d", &num1, &num2, &num3, &num4))
    {
        if(0 < num1 && num1 <= 255
           && 0 <= num2 && num2 <= 255
           && 0 <= num3 && num3 <= 255
           && 0 <= num4 && num4 <= 255){
            return ERR_OK;
        }
    }

    return ERR_FAILED;
}

int ih_create_uuid(int idx, ih_uuid_t *uuid)
{
	char ih_uuid_t[IH_UUID_MAX];
	long int timestamp = 0;

	if (!uuid) {
		LOG_IN("uuid is null");
		return ERR_INVALID;
	}

	memset(uuid, 0, sizeof(ih_uuid_t));
	timestamp = time(NULL);

	snprintf(uuid->_uuid, IH_UUID_MAX, "%04x%08lx%04x", idx, timestamp, rand()%(0xffff));
	LOG_DB("get uuid : %s", uuid->_uuid);

	return ERR_OK;

}

time_t ih_system_uptime_get(void)
{
    struct timespec time = {0, 0};

    if(0 == clock_gettime(CLOCK_MONOTONIC, &time)){
        return time.tv_sec;
    }

    return 0;
}

int ih_create_hash_uuid(int idx, char *msg, ih_uuid_t *uuid)
{
	char hash[IH_UUID_HASH_LEN + 1] = {0};

	if(!msg || !uuid){
		LOG_ER("%s parameter error", __func__);
		return ERR_INVALID;
	}

	memset(uuid, 0, sizeof(ih_uuid_t));
	HASH_STR_SHA1(msg, hash, sizeof(hash));
	if(strlen(hash) != IH_UUID_HASH_LEN){
		return ERR_INVALID;
	}

	snprintf(uuid->_uuid, IH_UUID_MAX, "%04x%s", idx, hash);
	LOG_DB("get uuid : %s", uuid->_uuid);

	return ERR_OK;
}

/*
 * handle :"-A" express ADD
 *         "-D" express DEL
*/
int fw_permit_service_handle(char *handle, char *proto, char *port)
{
    if(!handle || !proto || !port){
        LOG_DB("bad para!");
        return ERR_INVALID;
    }

    eval("iptables", handle, "PERMIT-SERVICE", "-p", proto, "-m", "multiport", "--dports", port, "-j", "ACCEPT"); 

    return ERR_OK;
}

int system_debug(char *cmd, char *buf, int len)
{
    FILE *fp = NULL;
    char buff[1024] = {0};
    int count = 0;
    int readcount = 0;
 
    if((!cmd) || (!buf)){
        LOG_ER("bad cmd string or return value saved buffer");
        return -1;
    }
   
    fp = popen(cmd, "r");
    if(fp){
        readcount = 0;
        memset(buff, 0x0, sizeof(buff));
        while((readcount < sizeof(buff)) && (!feof(fp))) {
            count = fread(buff+readcount, sizeof(char), sizeof(buff)-readcount, fp);
            if (count >= 0) {
                readcount += count;
            }
        }
        
        pclose(fp);

        memcpy(buf, buff, len < strlen(buff)? len: strlen(buff));
    }else{
        LOG_DB("popen exec failed");
        return -1;
    }
   
    return 0;
}

/*
* add the escape char to a string which have some special char like '&', '$', '`'...(may cause command execution)
*
* @param str point to string which maybe need to add escape char
*
* @param exec_str point to string after add escape char
*/
int escape_char_add(const char *str, char *exec_str, int len)
{
	char special_chr[] = {'`', '$', '&', ';', '|', '-', '\\', '[', ']', '@', '%', '^', '"'};
	int i = 0, j = 0, k = 0;

	if(str == NULL || exec_str == NULL){
		LOG_ER("invalid params...");
		return ERR_FAILED;
	}

	for(i = 0; i < len; i++){
		for(j = 0; j < sizeof(special_chr); j++){
			if (str[i] == special_chr[j]){
				exec_str[k] = '\\';
				k++;
				//when use grep,$ means the end of line,^ means the start of line
				if(str[i] == '$'){
					exec_str[k] = '\\';
					k++;
					exec_str[k] = '\\';
					k++;
				}
				break;
			}
		}

		exec_str[k] = str[i];
		k++;
	}

	exec_str[k] = '\0';

	return ERR_OK;
}

//iface is linux interface name like: ath100, vlan4010....
int uplink_interface_snat_set(char *iface, int handle)
{
    if(!iface){
        LOG_DB("interface error");
        return -1;
    }

    LOG_IN("%s SNAT of interface %s", (handle)?("Add"):("Delete"), iface);

    //always delete rule before add
    if(handle){
        eval("iptables", "-t", "nat", "-w", "3", "-D", "POST-DEFSNAT", "-o", iface,
             "-j", "MASQUERADE");
    }

    eval("iptables", "-t", "nat", "-w", "3", (handle)?("-A"):("-D"), "POST-DEFSNAT", 
         "-o", iface, "-j", "MASQUERADE");

    return 0;
}
typedef struct cpu_occupy_ //定义一个cpu occupy的结构体
{
	char name[20]; //定义一个char类型的数组名name有20个元素
	unsigned int user; //定义一个无符号的int类型的user
	unsigned int nice; //定义一个无符号的int类型的nice
	unsigned int system; //定义一个无符号的int类型的system
	unsigned int idle; //定义一个无符号的int类型的idle
	unsigned int iowait;
	unsigned int irq;
	unsigned int softirq;
}cpu_occupy_t;

static double cal_cpuoccupy (cpu_occupy_t *o, cpu_occupy_t *n)
{

	double od, nd;
	double id, sd;
	double cpu_use ;
	
	od = (double) (o->user + o->nice + o->system +o->idle+o->softirq+o->iowait+o->irq);//第一次(用户+优先级+系统+空闲)的时间再赋给od
	nd = (double) (n->user + n->nice + n->system +n->idle+n->softirq+n->iowait+n->irq);//第二次(用户+优先级+系统+空闲)的时间再赋给od

	id = (double) (n->idle); //用户第一次和第二次的时间之差再赋给id
	sd = (double) (o->idle) ; //系统第一次和第二次的时间之差再赋给sd
	if((nd-od) != 0){
		cpu_use =100.0 - ((id-sd))/(nd-od)*100.00; //((用户+系统)乖100)除(第一次和第二次的时间差)再赋给g_cpu_used
	}else{
		cpu_use = 0;
	}

	return cpu_use;
}

static void get_cpuoccupy (cpu_occupy_t *cpust)	
{
	FILE *fd;
	int n;
	char buff[256];
	cpu_occupy_t *cpu_occupy;
	
	cpu_occupy=cpust;

	fd = fopen ("/proc/stat", "r");
	if(fd == NULL){
		LOG_ER("fopen /proc/stat err,cal cpu use err");
		return;
	}

	fgets (buff, sizeof(buff), fd);	
	sscanf (buff, "%s %u %u %u %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice,&cpu_occupy->system, &cpu_occupy->idle ,&cpu_occupy->iowait,&cpu_occupy->irq,&cpu_occupy->softirq);
	
	fclose(fd);

}

double get_sysCpuUsage()
{

	cpu_occupy_t cpu_stat1;
	cpu_occupy_t cpu_stat2;
	double cpu;

	get_cpuoccupy((cpu_occupy_t *)&cpu_stat1);

	sleep(1);

	//第二次获取cpu使用情况
	get_cpuoccupy((cpu_occupy_t *)&cpu_stat2);

	//计算cpu使用率
	cpu = cal_cpuoccupy ((cpu_occupy_t *)&cpu_stat1, (cpu_occupy_t *)&cpu_stat2);

	return cpu;

}
