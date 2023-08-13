#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "shared.h"
#include "validate.h"
#include "ih_errno.h"
#include "strings.h"
#include "ih_logtrace.h"
#include "bootenv.h"

#define ETH_ALEN 6

int ip_set_addr(const char *iface,const char *ip,const char *submask)
{
	char buf[MAX_SYS_CMD_LEN];
	int got = 0;
	
	if (!iface || !*iface)	return ERR_INVAL;

	buf[0] = 0;

	strcat(buf,"ifconfig ");
	strcat(buf,iface);
	if (ip && *ip) {
		strcat(buf," ");
		strcat(buf,ip);
		got = 1;
	}

	if (submask && *submask) {
		strcat(buf," netmask ");
		strcat(buf,submask);
		got = 1;
	}

	if (got) return system(buf);

	return ERR_FAILED;
}

int ip_set_default_gw(const char *gw)
{
	if (gw && *gw) {
		char buf[MAX_SYS_CMD_LEN] = "route add default gw ";

		strcat(buf,gw);

		system("route del default");
		return system(buf);
	} else {
		system("route del default");
		return ERR_OK;
	}
}

#define MAC_DNS		10
#define DNS_FILE	"/etc/resolv.conf"
int ip_set_dns(const char *dns)
{
	char *buf,*p,*p1;
	int got = 0;

	if (!dns || !*dns || strcmp(dns,"0.0.0.0") == 0) {
		unlink(DNS_FILE);
		return ERR_OK;
	}
	
	buf = malloc(1024);
	if (!buf)	return ERR_NOMEM;
	p1 = strdup2(dns);
	if (!p1) {
		free(buf);
		return ERR_NOMEM;
	}

	buf[0] = 0;
	p = p1;
	SKIP_WHITE(p);
	trim_str(p);

	strcat(buf,"nameserver");
	p = strtok((char *)dns," ");
	while (p && got<=MAC_DNS) {
		strcat(buf," ");
		strcat(buf,p);
		got++;

		p = strtok(NULL," ");
	}

	if (got) {
		strcat(buf,"\n");
		if(f_write(DNS_FILE,buf,strlen(buf),0,0) != strlen(buf))
			got = 0;
	}

	free(buf);
	free(p1);

	return got?ERR_OK:ERR_INVAL;
}


static int str_to_mac_addr(char *mac_str, char *mac, unsigned short size)
{
	char *p, *q;
	int i = 0;
	unsigned int data;
	
	p = mac_str;
	q = strstr(p, ":");

	while(q && (i < (size - 1)) && (i < (ETH_ALEN - 1))){
		*q = '\0';
		sscanf(p, "%x", &data);
		mac[i] = data;
		*q = ':';
		p = q + 1;
		q = strstr(p, ":");
		i++;
	}

	sscanf(p, "%x", &data);
	mac[i] = data;
	return 0;
}

int get_mac_addr(const uns8 *mac,unsigned short size)
{
	char mac_str[64];
	
	if (!mac || size<ETH_ALEN) return -1;

	memset(mac_str, 0, sizeof(mac_str));
	if (bootenv_get("ethaddr", mac_str, sizeof(mac_str))){
		str_to_mac_addr(mac_str, (char *)mac, size);
		return 0;
	}
	
	LOG_ER("cannot get mac address of eth0");
	return -1;
}

/***********************************************************/
/* collect ethernet info                                   */
/***********************************************************/
char *raw_mac_to_dot_mac(char dot_mac[], const uint8_t raw_mac[6])
{
	sprintf(dot_mac, "%02X:%02X:%02X:%02X:%02X:%02X",                                                                                                                                                          
			raw_mac[0], raw_mac[1], raw_mac[2],
 			raw_mac[3], raw_mac[4], raw_mac[5]);

    return dot_mac;
}

int get_mac_addr2(const uns8 *mac,unsigned short size)
{
	char mac_str[64];
	
	if (!mac || size<ETH_ALEN) return -1;

	memset(mac_str, 0, sizeof(mac_str));
	if (bootenv_get("ethaddr2", mac_str, sizeof(mac_str))){
		str_to_mac_addr(mac_str, (char *)mac, size);
		return 0;
	}
	
	LOG_ER("cannot get mac address of eth1");
	return -1;
}

int get_iface_mac(const char *ifname, unsigned char *mac, int len)
{  
	int sk = -1;  
	struct ifreq ifreq;

	if(!ifname || !mac || len < 6){
		return -1;
	}

	sk = socket(AF_INET, SOCK_DGRAM, 0);  
	if (sk < 0){  
		syslog(LOG_WARNING, "create socket failed, error %s(%d)", strerror(errno), errno);  
		return -1;  
	}

	strcpy(ifreq.ifr_name, ifname);  
	if (ioctl(sk, SIOCGIFHWADDR, &ifreq) < 0){  
		syslog(LOG_WARNING, "ioctl failed to get ineterface MAC, error %s(%d)", strerror(errno), errno);  
		close(sk);  
		return -1;
	}

	memcpy(mac, (unsigned char *)ifreq.ifr_hwaddr.sa_data, 6);

	close(sk);  

	return 0;  
}

int get_iface_mtu(const char *ifname, int *mtu)
{  
	int sk = -1;  
	struct ifreq ifreq;

	if(!ifname || !mtu){
		return -1;
	}

	sk = socket(AF_INET, SOCK_DGRAM, 0);  
	if (sk < 0){  
		syslog(LOG_WARNING, "create socket failed, error %s(%d)", strerror(errno), errno);  
		return -1;  
	}

	strcpy(ifreq.ifr_name, ifname);  
	if (ioctl(sk, SIOCGIFMTU, &ifreq) < 0){  
		syslog(LOG_WARNING, "ioctl failed to get MTU of ineterface %s, error %s(%d)", ifname, strerror(errno), errno);  
		close(sk);  
		return -1;
	}

	*mtu = ifreq.ifr_mtu;

	close(sk);  

	return 0;  
}

/* curl string describing error number
 * ARGS:
 * 	errnum: error number
 * RETURN:
 *	string
*/
static char *curl_strcode [] = {
	//0~4
	"", "Unsupported protocol", "Failed to initialize",
	"URL malformed", "",
	//5~9
	"Couldn't resolve proxy", "Couldn't resolve host", "Failed to connect to host",
	"FTP weird server reply", "FTP access denied",
	//10~14
	"", "FTP weird PASS reply", "",
	"FTP weird PASV reply", "FTP weird 227 format",
	//15~19
	"FTP can't get host", "", "FTP couldn't set binary",
	"Partial file",	"FTP couldn't download/access the given file",
	//20~24
	"", "FTP quote error", "HTTP page not retrieved",
	"Write error", "",
	//25~29
	"FTP couldn't STOR file", "Read error", "Out of memory",
	"Operation timeout", "",
	//30~34
	"FTP PORT failed", "FTP couldn't use REST", "", 
	"HTTP range error", "HTTP post error",
	//35~39
	"", "FTP bad download resume",	"FILE couldn't read file",
	"", "",
	//40~44
	"", "", "",
	"", "",
	//45~49
	"", "", "",
	"", "",
	//50~54
	"", "", "",
	"", "",
	//55~59
	"", "", "",
	"", "",
	//60~64
	"", "", "",
	"", "",
	//65~69
	"", "", "The user name, password, or similar was not accepted",
	"File not found on TFTP server", "",
	//70~74
	"", "", "",
	"File already exists", "No such user",
	//75~79
	"", "", "",
	"The resource does not exist", "",
	//80~84
	"", "", "",
	"", "Unknown error",
};
char *curl_strerror(int errnum)
{
	int n = sizeof(curl_strcode)/sizeof(char *);

	if(errnum >= n || errnum < 0) errnum = n-1;

	return curl_strcode[errnum];
}

/* curl tftp/ftp upload (sync mode)
 * ARGS:
 * 	type: transfer protocol
 * 	from: source file
 *	addr: ip/domain
 * 	file: destination file
 * 	userpass: user:password
 * 	timeout: operation timeout
*/
int curl_upload(CURL_PROTO type, char *from, char *addr, char *file, char *userpass, int timeout)
{
	int i=0;
	char *argv[16], to[64], tm[16];

	argv[i++] = "curl";
	argv[i++] = "-T";
	argv[i++] = from;
	if(type==CURL_TFTP) {
		snprintf(to, sizeof(to), "tftp://%s/%s", addr, file);
	} else {
		snprintf(to, sizeof(to), "ftp://%s/%s", addr, file);
	}
	argv[i++] = to;
	if(type==CURL_FTP && userpass && *userpass) {
		argv[i++] = "-u";//--user
		argv[i++] = userpass;
	}
	if(timeout>0) {
		snprintf(tm, sizeof(tm), "%d", timeout);
		argv[i++] = "-m";//timeout
		argv[i++] = tm;
	}
	argv[i++] = NULL;

	return _eval(argv, NULL, 0, NULL);
}

/* curl tftp/ftp download (sync mode)
 * ARGS:
 * 	type: transfer protocol
 *	addr: ip/domain
 * 	file: source file
 * 	to: destination file
 * 	userpass: user:password
 * 	timeout: operation timeout
*/
int curl_download(CURL_PROTO type, char *addr, char *file, char *to, char *userpass, int timeout)
{
	int i=0;
	char *argv[16], from[64], tm[16];

	argv[i++] = "curl";
	if(type==CURL_TFTP) {
		snprintf(from, sizeof(from), "tftp://%s/%s", addr, file);
	} else {
		snprintf(from, sizeof(from), "ftp://%s/%s", addr, file);
	}
	argv[i++] = from;
	argv[i++] = "-o";
	argv[i++] = to;
	if(type==CURL_FTP && userpass && *userpass) {
		argv[i++] = "-u";//--user
		argv[i++] = userpass;
	}
	if(timeout>0) {
		snprintf(tm, sizeof(tm), "%d", timeout);
		argv[i++] = "-m";//timeout
		argv[i++] = tm;
	}
	argv[i++] = NULL;

	return _eval(argv, NULL, 0, NULL);
}

