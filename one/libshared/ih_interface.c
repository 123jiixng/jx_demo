#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#include <net/if.h>
#include <net/if_arp.h>
#ifdef HAVE_NET_ETHERNET_H
# include <net/ethernet.h>
#endif
#include <netinet/in.h> 
#include <arpa/inet.h>

#include <dirent.h>  

#include "ih_ipc.h"
#include "ih_types.h"
#include "ih_logtrace.h"

#include "ih_cmd.h"
#include "sw_ipc.h"
#include "strings.h"
#include "if_common.h"
#include "vif_shared.h"
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////



#define FAST_FUNC
#define UNUSED_PARAM

#define xmalloc(size)	malloc(size)
/* Octets in one Ethernet addr, from <linux/if_ether.h> */
#define ETH_ALEN     6

/* Useful for defeating gcc's alignment of "char message[]"-like data */
#if 1 /* if needed: !defined(arch1) && !defined(arch2) */
#ifdef ALIGN1
#undef ALIGN1
#endif
#ifdef ALIGN2
#undef ALIGN2
#endif
#ifdef ALIGN4
#undef ALIGN4
#endif
#define ALIGN1 __attribute__((aligned(1)))
#define ALIGN2 __attribute__((aligned(2)))
#define ALIGN4 __attribute__((aligned(4)))
#else
/* Arches which MUST have 2 or 4 byte alignment for everything are here */
# define ALIGN1
# define ALIGN2
# define ALIGN4
#endif

#define	DEF_WIFI_VLAN	15

struct aftype {
	const char *name;
	const char *title;
	int af;
	int alen;
	char*       FAST_FUNC (*print)(unsigned char *);
	const char* FAST_FUNC (*sprint)(struct sockaddr *, int numeric);
	int         FAST_FUNC (*input)(/*int type,*/ const char *bufp, struct sockaddr *);
	void        FAST_FUNC (*herror)(char *text);
	int         FAST_FUNC (*rprint)(int options);
	int         FAST_FUNC (*rinput)(int typ, int ext, char **argv);
	/* may modify src */
	int         FAST_FUNC (*getmask)(char *src, struct sockaddr *mask, char *name);
};
/* This structure defines hardware protocols and their handlers. */
struct hwtype {
	const char *name;
	const char *title;
	int type;
	int alen;
	char* FAST_FUNC (*print)(unsigned char *);
	int   FAST_FUNC (*input)(const char *, struct sockaddr *);
	int   FAST_FUNC (*activate)(int fd);
	int suppress_null_addr;
};


#ifdef IFNAMSIZ
#undef IFNAMSIZ
#endif
#define IFNAMSIZ 36	//FIXME 32 is not enough


struct user_net_device_stats {
	unsigned long long rx_packets;	/* total packets received       */
	unsigned long long tx_packets;	/* total packets transmitted    */
	unsigned long long rx_bytes;	/* total bytes received         */
	unsigned long long tx_bytes;	/* total bytes transmitted      */
	unsigned long rx_errors;	/* bad packets received         */
	unsigned long tx_errors;	/* packet transmit problems     */
	unsigned long rx_dropped;	/* no space in linux buffers    */
	unsigned long tx_dropped;	/* no space available in linux  */
	unsigned long rx_multicast;	/* multicast packets received   */
	unsigned long rx_compressed;
	unsigned long tx_compressed;
	unsigned long collisions;

	/* detailed rx_errors: */
	unsigned long rx_length_errors;
	unsigned long rx_over_errors;	/* receiver ring buff overflow  */
	unsigned long rx_crc_errors;	/* recved pkt with crc error    */
	unsigned long rx_frame_errors;	/* recv'd frame alignment error */
	unsigned long rx_fifo_errors;	/* recv'r fifo overrun          */
	unsigned long rx_missed_errors;	/* receiver missed packet     */
	/* detailed tx_errors */
	unsigned long tx_aborted_errors;
	unsigned long tx_carrier_errors;
	unsigned long tx_fifo_errors;
	unsigned long tx_heartbeat_errors;
	unsigned long tx_window_errors;
};

struct interface {
	struct interface *next, *prev;
	char name[IFNAMSIZ];                    /* interface name        */
	short type;                             /* if type               */
	short flags;                            /* various flags         */
	int metric;                             /* routing metric        */
	int mtu;                                /* MTU value             */
	int tx_queue_len;                       /* transmit queue length */
	struct ifmap map;                       /* hardware setup        */
	struct sockaddr addr;                   /* IP address            */
	struct sockaddr dstaddr;                /* P-P IP address        */
	struct sockaddr broadaddr;              /* IP broadcast address  */
	struct sockaddr netmask;                /* IP network mask       */
	int has_ip;
	char hwaddr[32];                        /* HW address            */
	int statistics_valid;
	struct user_net_device_stats stats;     /* statistics            */
	int keepalive;                          /* keepalive value for SLIP */
	int outfill;                            /* outfill value for SLIP */
};


#if ENABLE_FEATURE_HWIB
/* #include <linux/if_infiniband.h> */
# undef INFINIBAND_ALEN
# define INFINIBAND_ALEN 20
#endif

#if ENABLE_FEATURE_IPV6
# define HAVE_AFINET6 1
#else
# undef HAVE_AFINET6
#endif

#define _PATH_PROCNET_DEV               "/proc/net/dev"
#define _PATH_PROCNET_IFINET6           "/proc/net/if_inet6"

#ifdef HAVE_AFINET6
#ifndef _LINUX_IN6_H
/*
 * This is from linux/include/net/ipv6.h
 */
struct in6_ifreq {
	struct in6_addr ifr6_addr;
	uint32_t ifr6_prefixlen;
	unsigned int ifr6_ifindex;
};
#endif
#endif /* HAVE_AFINET6 */

/* Defines for glibc2.0 users. */
#ifndef SIOCSIFTXQLEN
# define SIOCSIFTXQLEN      0x8943
# define SIOCGIFTXQLEN      0x8942
#endif

/* ifr_qlen is ifru_ivalue, but it isn't present in 2.0 kernel headers */
#ifndef ifr_qlen
# define ifr_qlen        ifr_ifru.ifru_mtu
#endif

#ifndef HAVE_TXQUEUELEN
# define HAVE_TXQUEUELEN 1
#endif

#ifndef IFF_DYNAMIC
# define IFF_DYNAMIC     0x8000 /* dialup device with changing addresses */
#endif


/* Display an Internet socket address. */
static const char* FAST_FUNC INET_sprint(struct sockaddr *sap, int numeric)
{
	static char buff[20]; /* defaults to NULL */

	memset(buff, 0, sizeof(buff));
	
	if (sap->sa_family == 0xFFFF || sap->sa_family == 0)
		return "[NONE SET]";
	snprintf(buff, sizeof(buff), "%s", inet_ntoa(((struct sockaddr_in *) sap)->sin_addr));
	return buff;
}
#if 0
#ifdef UNUSED_AND_BUGGY
static int INET_getsock(char *bufp, struct sockaddr *sap)
{
	char *sp = bufp, *bp;
	unsigned int i;
	unsigned val;
	struct sockaddr_in *sock_in;

	sock_in = (struct sockaddr_in *) sap;
	sock_in->sin_family = AF_INET;
	sock_in->sin_port = 0;

	val = 0;
	bp = (char *) &val;
	for (i = 0; i < sizeof(sock_in->sin_addr.s_addr); i++) {
		*sp = toupper(*sp);

		if ((unsigned)(*sp - 'A') <= 5)
			bp[i] |= (int) (*sp - ('A' - 10));
		else if (isdigit(*sp))
			bp[i] |= (int) (*sp - '0');
		else
			return -1;

		bp[i] <<= 4;
		sp++;
		*sp = toupper(*sp);

		if ((unsigned)(*sp - 'A') <= 5)
			bp[i] |= (int) (*sp - ('A' - 10));
		else if (isdigit(*sp))
			bp[i] |= (int) (*sp - '0');
		else
			return -1;

		sp++;
	}
	sock_in->sin_addr.s_addr = htonl(val);

	return (sp - bufp);
}
#endif

static int FAST_FUNC INET_input(/*int type,*/ const char *bufp, struct sockaddr *sap)
{
	return INET_resolve(bufp, (struct sockaddr_in *) sap, 0);
/*
	switch (type) {
	case 1:
		return (INET_getsock(bufp, sap));
	case 256:
		return (INET_resolve(bufp, (struct sockaddr_in *) sap, 1));
	default:
		return (INET_resolve(bufp, (struct sockaddr_in *) sap, 0));
	}
*/
}
#endif


static const struct aftype inet_aftype = {
	.name   = "inet",
	.title  = "DARPA Internet",
	.af     = AF_INET,
	.alen   = 4,
	#if 1
	.sprint = INET_sprint,
	.input  = NULL,//INET_input,
	#else
	.sprint = NULL,
	.input  = NULL,
	#endif
};

#ifdef HAVE_AFINET6
#if 0
/* Display an Internet socket address. */
/* dirty! struct sockaddr usually doesn't suffer for inet6 addresses, fst. */
static const char* FAST_FUNC INET6_sprint(struct sockaddr *sap, int numeric)
{
	static char *buff;

	free(buff);
	if (sap->sa_family == 0xFFFF || sap->sa_family == 0)
		return "[NONE SET]";
	buff = INET6_rresolve((struct sockaddr_in6 *) sap, numeric);
	return buff;
}

#ifdef UNUSED
static int INET6_getsock(char *bufp, struct sockaddr *sap)
{
	struct sockaddr_in6 *sin6;

	sin6 = (struct sockaddr_in6 *) sap;
	sin6->sin6_family = AF_INET6;
	sin6->sin6_port = 0;

	if (inet_pton(AF_INET6, bufp, sin6->sin6_addr.s6_addr) <= 0)
		return -1;

	return 16;			/* ?;) */
}
#endif

static int FAST_FUNC INET6_input(/*int type,*/ const char *bufp, struct sockaddr *sap)
{
	return INET6_resolve(bufp, (struct sockaddr_in6 *) sap);
/*
	switch (type) {
	case 1:
		return (INET6_getsock(bufp, sap));
	default:
		return (INET6_resolve(bufp, (struct sockaddr_in6 *) sap));
	}
*/
}
#endif
static const struct aftype inet6_aftype = {
	.name   = "inet6",
	.title  = "IPv6",
	.af     = AF_INET6,
	.alen   = sizeof(struct in6_addr),
	#if 0
	.sprint = INET6_sprint,
	.input  = INET6_input,
	#else
	.sprint = NULL,
	.input  = NULL,
	#endif
};

#endif /* HAVE_AFINET6 */
#if 1
/* Display an UNSPEC address. */
static char* FAST_FUNC UNSPEC_print(unsigned char *ptr)
{
#if 1
	static char buff[100];

	char * pos;
	unsigned int i;
	
	pos = buff;
	for (i = 0; i < sizeof(struct sockaddr); i++) {
		/* careful -- not every libc's sprintf returns # bytes written */
		sprintf(pos, "%02X-", (*ptr++ & 0377));
		pos += 3;
	}	
	/* Erase trailing "-".  Works as long as sizeof(struct sockaddr) != 0 */
	*--pos = '\0';
#else
	static char *buff;

	char *pos;
	unsigned int i;

	if (!buff)
		buff = xmalloc(sizeof(struct sockaddr) * 3 + 1);
	pos = buff;
	for (i = 0; i < sizeof(struct sockaddr); i++) {
		/* careful -- not every libc's sprintf returns # bytes written */
		sprintf(pos, "%02X-", (*ptr++ & 0377));
		pos += 3;
	}
	/* Erase trailing "-".  Works as long as sizeof(struct sockaddr) != 0 */
	*--pos = '\0';
#endif	
	return buff;
}

/* Display an UNSPEC socket address. */
static const char* FAST_FUNC UNSPEC_sprint(struct sockaddr *sap, int numeric UNUSED_PARAM)
{
	if (sap->sa_family == 0xFFFF || sap->sa_family == 0)
		return "[NONE SET]";
	return UNSPEC_print((unsigned char *)sap->sa_data);
}
#endif

static const struct aftype unspec_aftype = {
	.name   = "unspec",
	.title  = "UNSPEC",
	.af     = AF_UNSPEC,
	.alen   = 0,
	#if 1
	.print  = UNSPEC_print,
	.sprint = UNSPEC_sprint,
	#else
	.sprint = NULL,
	.input  = NULL,
	#endif
};

static const struct aftype *const aftypes[] = {
	&inet_aftype,
#ifdef HAVE_AFINET6
	&inet6_aftype,
#endif
	&unspec_aftype,
	NULL
};

/* Check our protocol family table for this family. */
static const struct aftype *get_afntype(int af)
{
	const struct aftype *const *afp;

	afp = aftypes;
	while (*afp != NULL) {
		if ((*afp)->af == af)
			return *afp;
		afp++;
	}
	return NULL;
}



static const struct hwtype unspec_hwtype = {
	.name =		"unspec",
	.title =	"UNSPEC",
	.type =		-1,
	#if 1
	.print =	UNSPEC_print
	#else
	.print =	NULL
	#endif
};

static const struct hwtype loop_hwtype = {
	.name =		"loop",
	.title =	"Local Loopback",
	.type =		ARPHRD_LOOPBACK
};

#if 1
/* Display an Ethernet address in readable format. */
static char* FAST_FUNC ether_print(unsigned char *ptr)
{
	static char buff[20];

	memset(buff, 0, sizeof(buff));

	snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X",
			 (ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
			 (ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377));
	
	return buff;
}

//static int FAST_FUNC ether_input(const char *bufp, struct sockaddr *sap);
#endif


static const struct hwtype ether_hwtype = {
	.name  = "ether",
	.title = "Ethernet",
	.type  = ARPHRD_ETHER,
	.alen  = ETH_ALEN,
	#if 1
	.print = ether_print,
	.input = NULL,//ether_input
	#else
	.print = NULL,
	.input = NULL
	#endif
};

#if 0
static unsigned hexchar2int(char c)
{
	if (isdigit(c))
		return c - '0';
	c &= ~0x20; /* a -> A */
	if ((unsigned)(c - 'A') <= 5)
		return c - ('A' - 10);
	return ~0U;
}

/* Input an Ethernet address and convert to binary. */
static int FAST_FUNC ether_input(const char *bufp, struct sockaddr *sap)
{
	unsigned char *ptr;
	char c;
	int i;
	unsigned val;

	sap->sa_family = ether_hwtype.type;
	ptr = (unsigned char*) sap->sa_data;

	i = 0;
	while ((*bufp != '\0') && (i < ETH_ALEN)) {
		val = hexchar2int(*bufp++) * 0x10;
		if (val > 0xff) {
			errno = EINVAL;
			return -1;
		}
		c = *bufp;
		if (c == ':' || c == 0)
			val >>= 4;
		else {
			val |= hexchar2int(c);
			if (val > 0xff) {
				errno = EINVAL;
				return -1;
			}
		}
		if (c != 0)
			bufp++;
		*ptr++ = (unsigned char) val;
		i++;

		/* We might get a semicolon here - not required. */
		if (*bufp == ':') {
			bufp++;
		}
	}
	return 0;
}
#endif

static const struct hwtype ppp_hwtype = {
	.name =		"ppp",
	.title =	"Point-to-Point Protocol",
	.type =		ARPHRD_PPP
};

#if ENABLE_FEATURE_IPV6
static const struct hwtype sit_hwtype = {
	.name =			"sit",
	.title =		"IPv6-in-IPv4",
	.type =			ARPHRD_SIT,
	.print =		UNSPEC_print,
	.suppress_null_addr =	1
};
#endif

#if ENABLE_FEATURE_HWIB
/* Input an Infiniband address and convert to binary. */
int FAST_FUNC in_ib(const char *bufp, struct sockaddr *sap)
{
	sap->sa_family = ib_hwtype.type;
//TODO: error check?
	hex2bin((char*)sap->sa_data, bufp, INFINIBAND_ALEN);
# ifdef HWIB_DEBUG
	fprintf(stderr, "in_ib(%s): %s\n", bufp, UNSPEC_print(sap->sa_data));
# endif
	return 0;
}
#endif

#if ENABLE_FEATURE_HWIB
static const struct hwtype ib_hwtype = {
	.name  = "infiniband",
	.title = "InfiniBand",
	.type  = ARPHRD_INFINIBAND,
	.alen  = INFINIBAND_ALEN,
	.print = UNSPEC_print,
	.input = in_ib,
};
#endif


static const struct hwtype *const hwtypes[] = {
#if 1
	&loop_hwtype,
#endif	
	&ether_hwtype,
	&ppp_hwtype,
	&unspec_hwtype,
#if 1		
#if ENABLE_FEATURE_IPV6
	&sit_hwtype,
#endif
#if ENABLE_FEATURE_HWIB
	&ib_hwtype,
#endif
#endif
	NULL
};

#ifdef IFF_PORTSEL
static const char *const if_port_text[] = {
	/* Keep in step with <linux/netdevice.h> */
	"unknown",
	"10base2",
	"10baseT",
	"AUI",
	"100baseT",
	"100baseTX",
	"100baseFX",
	NULL
};
#endif



/* Check our hardware type table for this type. */
const struct hwtype* FAST_FUNC get_hwtype(const char *name)
{
	const struct hwtype *const *hwp;

	hwp = hwtypes;
	while (*hwp != NULL) {
		if (!strcmp((*hwp)->name, name))
			return (*hwp);
		hwp++;
	}
	return NULL;
}

/* Check our hardware type table for this type. */
const struct hwtype* FAST_FUNC get_hwntype(int type)
{
	const struct hwtype *const *hwp;

	hwp = hwtypes;
	while (*hwp != NULL) {
		if ((*hwp)->type == type)
			return *hwp;
		hwp++;
	}
	return NULL;
}

/* return 1 if address is all zeros */
static int hw_null_address(const struct hwtype *hw, void *ap)
{
	int i;
	unsigned char *address = (unsigned char *) ap;

	for (i = 0; i < hw->alen; i++)
		if (address[i])
			return 0;
	return 1;
}

static const char TRext[] ALIGN1 = "\0\0\0Ki\0Mi\0Gi\0Ti";

static void print_bytes_scaled(unsigned long long ull, const char *end)
{
	unsigned long long int_part;
	const char *ext;
	unsigned int frac_part;
	int i;

	frac_part = 0;
	ext = TRext;
	int_part = ull;
	i = 4;
	do {
		if (int_part >= 1024) {
			frac_part = ((((unsigned int) int_part) & (1024-1)) * 10) / 1024;
			int_part /= 1024;
			ext += 3;	/* KiB, MiB, GiB, TiB */
		}
		--i;
	} while (i);

	ih_cmd_rsp_print("X bytes:%llu (%llu.%u %sB)%s", ull, int_part, frac_part, ext, end);
}

typedef int smallint;
typedef unsigned smalluint;
#if 0
static int ih_if_readconf(void)
{
	int numreqs = 30;
	struct ifconf ifc;
	struct ifreq *ifr;
	int n, err = -1;
	int skfd;

	ifc.ifc_buf = NULL;

	/* SIOCGIFCONF currently seems to only work properly on AF_INET sockets
	   (as of 2.1.128) */
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0) {
		LOG_ER("error: no inet socket available");
		return -1;
	}

	for (;;) {
		ifc.ifc_len = sizeof(struct ifreq) * numreqs;
		//ifc.ifc_buf = xrealloc(ifc.ifc_buf, ifc.ifc_len);
		ifc.ifc_buf = realloc(ifc.ifc_buf, ifc.ifc_len);

		//if (ioctl_or_warn(skfd, SIOCGIFCONF, &ifc) < 0) {
		if (ioctl(skfd, SIOCGIFCONF, &ifc) < 0) {
			goto out;
		}
		if (ifc.ifc_len == (int)(sizeof(struct ifreq) * numreqs)) {
			/* assume it overflowed and try again */
			numreqs += 10;
			continue;
		}
		break;
	}

	ifr = ifc.ifc_req;
	for (n = 0; n < ifc.ifc_len; n += sizeof(struct ifreq)) {
		add_interface(ifr->ifr_name);
		ifr++;
	}
	err = 0;

 out:
	close(skfd);
	free(ifc.ifc_buf);
	return err;
}
#endif

static int ih_procnetdev_version(char *buf)
{
	if (strstr(buf, "compressed"))
		return 2;
	if (strstr(buf, "bytes"))
		return 1;
	return 0;
}
static char* skip_whitespace(char *s)
{
	while (*s == ' ' || *s == '\t') ++s;

	return s;
}
static char *ih_get_name(char *name, char *p)
{
	/* Extract <name> from nul-terminated p where p matches
	 * <name>: after leading whitespace.
	 * If match is not made, set name empty and return unchanged p
	 */
	char *nameend;
	char *namestart = skip_whitespace(p);

	nameend = namestart;
	while (*nameend && *nameend != ':' && !isspace(*nameend))
		nameend++;
	if (*nameend == ':') {
		if ((nameend - namestart) < IFNAMSIZ) {
			memcpy(name, namestart, nameend - namestart);
			name[nameend - namestart] = '\0';
			p = nameend;
		} else {
			/* Interface name too large */
			name[0] = '\0';
		}
	} else {
		/* trailing ':' not found - return empty */
		name[0] = '\0';
	}
	return p + 1;
}




#if INT_MAX == LONG_MAX
static const char *const ss_fmt[] = {
	"%n%llu%u%u%u%u%n%n%n%llu%u%u%u%u%u",
	"%llu%llu%u%u%u%u%n%n%llu%llu%u%u%u%u%u",
	"%llu%llu%u%u%u%u%u%u%llu%llu%u%u%u%u%u%u"
};
#else
static const char *const ss_fmt[] = {
	"%n%llu%lu%lu%lu%lu%n%n%n%llu%lu%lu%lu%lu%lu",
	"%llu%llu%lu%lu%lu%lu%n%n%llu%llu%lu%lu%lu%lu%lu",
	"%llu%llu%lu%lu%lu%lu%lu%lu%llu%llu%lu%lu%lu%lu%lu%lu"
};

#endif

static void get_dev_fields(char *bp, struct interface *ife, int procnetdev_vsn)
{
	memset(&ife->stats, 0, sizeof(struct user_net_device_stats));

	sscanf(bp, ss_fmt[procnetdev_vsn],
		   &ife->stats.rx_bytes, /* missing for 0 */
		   &ife->stats.rx_packets,
		   &ife->stats.rx_errors,
		   &ife->stats.rx_dropped,
		   &ife->stats.rx_fifo_errors,
		   &ife->stats.rx_frame_errors,
		   &ife->stats.rx_compressed, /* missing for <= 1 */
		   &ife->stats.rx_multicast, /* missing for <= 1 */
		   &ife->stats.tx_bytes, /* missing for 0 */
		   &ife->stats.tx_packets,
		   &ife->stats.tx_errors,
		   &ife->stats.tx_dropped,
		   &ife->stats.tx_fifo_errors,
		   &ife->stats.collisions,
		   &ife->stats.tx_carrier_errors,
		   &ife->stats.tx_compressed /* missing for <= 1 */
		   );

	if (procnetdev_vsn <= 1) {
		if (procnetdev_vsn == 0) {
			ife->stats.rx_bytes = 0;
			ife->stats.tx_bytes = 0;
		}
		ife->stats.rx_multicast = 0;
		ife->stats.rx_compressed = 0;
		ife->stats.tx_compressed = 0;
	}
}


static int ih_if_read_proc(struct interface *ife)
{
	static smallint proc_read;

	FILE *fh;
	char buf[512];
	int err, procnetdev_vsn;
	char *target = ife->name;
	if (proc_read)
		return 0;
	if (!target)
		proc_read = 1;

	//fh = fopen_or_warn(_PATH_PROCNET_DEV, "r");
	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh) {
		LOG_ER("should call if_readconf()");
		return 0;
//		return if_readconf();
	}
	fgets(buf, sizeof buf, fh);	/* eat line */
	fgets(buf, sizeof buf, fh);

	procnetdev_vsn = ih_procnetdev_version(buf);

	err = 0;
	while (fgets(buf, sizeof buf, fh)) {
		char *s, name[128];

		s = ih_get_name(name, buf);
		if (target && !strcmp(target, name)){
			get_dev_fields(s, ife, procnetdev_vsn);
			ife->statistics_valid = 1;
			break;
		}
	}
	if (ferror(fh)) {
		LOG_ER(_PATH_PROCNET_DEV);
		//bb_perror_msg(_PATH_PROCNET_DEV);
		err = -1;
		proc_read = 0;
	}
	fclose(fh);
	return err;
}


/* Fetch the interface configuration from the kernel. */
static int ih_if_fetch(struct interface *ife)
{
	struct ifreq ifr;
	char *ifname = ife->name;
	int skfd;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (skfd < 0){
		LOG_ER("can't open socket for interface fectch");
		return -1;
	}
	
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));  
	if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
		close(skfd);
		return -1;
	}
	ife->flags = ifr.ifr_flags;

	memset(ife->hwaddr, 0, 32);
	if (ioctl(skfd, SIOCGIFHWADDR, &ifr) >= 0)
		memcpy(ife->hwaddr, ifr.ifr_hwaddr.sa_data, 8);

	ife->type = ifr.ifr_hwaddr.sa_family;

	ife->metric = 0;
	if (ioctl(skfd, SIOCGIFMETRIC, &ifr) >= 0)
		ife->metric = ifr.ifr_metric;

	ife->mtu = 0;
	if (ioctl(skfd, SIOCGIFMTU, &ifr) >= 0)
		ife->mtu = ifr.ifr_mtu;

	memset(&ife->map, 0, sizeof(struct ifmap));
#ifdef SIOCGIFMAP
	if (ioctl(skfd, SIOCGIFMAP, &ifr) == 0)
		ife->map = ifr.ifr_map;
#endif

#ifdef HAVE_TXQUEUELEN
	ife->tx_queue_len = -1;	/* unknown value */
	if (ioctl(skfd, SIOCGIFTXQLEN, &ifr) >= 0)
		ife->tx_queue_len = ifr.ifr_qlen;
#else
	ife->tx_queue_len = -1;	/* unknown value */
#endif

	ifr.ifr_addr.sa_family = AF_INET;
	memset(&ife->addr, 0, sizeof(struct sockaddr));
	if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0) {
		ife->has_ip = 1;
		ife->addr = ifr.ifr_addr;
		memset(&ife->dstaddr, 0, sizeof(struct sockaddr));
		if (ioctl(skfd, SIOCGIFDSTADDR, &ifr) >= 0)
			ife->dstaddr = ifr.ifr_dstaddr;

		memset(&ife->broadaddr, 0, sizeof(struct sockaddr));
		if (ioctl(skfd, SIOCGIFBRDADDR, &ifr) >= 0)
			ife->broadaddr = ifr.ifr_broadaddr;

		memset(&ife->netmask, 0, sizeof(struct sockaddr));
		if (ioctl(skfd, SIOCGIFNETMASK, &ifr) >= 0)
			ife->netmask = ifr.ifr_netmask;
	}

	close(skfd);
	return 0;
}

static void ife_print6(char *iface)
{
	FILE *f;
	char addr6[40], devname[20];
	struct sockaddr_in6 sap;
	int plen, scope, dad_status, if_idx;
	char addr6p[8][5];

	f = fopen(_PATH_PROCNET_IFINET6, "r");
	if (f == NULL)
		return;

	while (fscanf
		   (f, "%4s%4s%4s%4s%4s%4s%4s%4s %08x %02x %02x %02x %20s\n",
			addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4],
			addr6p[5], addr6p[6], addr6p[7], &if_idx, &plen, &scope,
			&dad_status, devname) != EOF
	) {
		if (!strcmp(devname, iface)) {
			sprintf(addr6, "%s:%s:%s:%s:%s:%s:%s:%s",
					addr6p[0], addr6p[1], addr6p[2], addr6p[3],
					addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
			inet_pton(AF_INET6, addr6,
					  (struct sockaddr *) &sap.sin6_addr);
			sap.sin6_family = AF_INET6;
			ih_cmd_rsp_print("inet6 addr: %s/%d",
				   inet6_ntoa(sap.sin6_addr),
				   plen);
			ih_cmd_rsp_print(" Scope:");
			switch (scope & IPV6_ADDR_SCOPE_MASK) {
			case 0:
				ih_cmd_rsp_print("Global");
				break;
			case IPV6_ADDR_LINKLOCAL:
				ih_cmd_rsp_print("Link");
				break;
			case IPV6_ADDR_SITELOCAL:
				ih_cmd_rsp_print("Site");
				break;
			case IPV6_ADDR_COMPATv4:
				ih_cmd_rsp_print("Compat");
				break;
			case IPV6_ADDR_LOOPBACK:
				ih_cmd_rsp_print("Host");
				break;
			default:
				ih_cmd_rsp_print("Unknown");
			}
			ih_cmd_rsp_print("\n          ");
		}
	}
	fclose(f);
}

static void ih_ife_print(struct interface *ptr, char *iface)
{
	const struct aftype *ap;
	const struct hwtype *hw;
	int hf;
	int can_compress = 0;

	ap = get_afntype(ptr->addr.sa_family);
	if (ap == NULL)
		ap = get_afntype(0);

	hf = ptr->type;

	if (hf == ARPHRD_CSLIP || hf == ARPHRD_CSLIP6)
		can_compress = 1;

	hw = get_hwntype(hf);
	if (hw == NULL)
		hw = get_hwntype(-1);

	ih_cmd_rsp_print("%-9s Link encap:%s  ", ptr->name, hw->title);
	/* For some hardware types (eg Ash, ATM) we don't print the
	   hardware address if it's null.  */
	if (hw->print != NULL
	 && !(hw_null_address(hw, ptr->hwaddr) && hw->suppress_null_addr)
	) {
		ih_cmd_rsp_print("HWaddr %s  ", hw->print((unsigned char *)ptr->hwaddr));
		//printf("HWaddr %s  ", hw->print((unsigned char *)ptr->hwaddr));
	}
#ifdef IFF_PORTSEL
	if (ptr->flags & IFF_PORTSEL) {
		ih_cmd_rsp_print("Media:%s", if_port_text[ptr->map.port] /* [0] */);
		if (ptr->flags & IFF_AUTOMEDIA)
			ih_cmd_rsp_print("(auto)");
	}
#endif	
	ih_cmd_rsp_print("\n");


	if (ptr->has_ip) {
		ih_cmd_rsp_print("          %s addr:%s ", ap->name,
			   ap->sprint(&ptr->addr, 1));
		if (ptr->flags & IFF_POINTOPOINT) {
			ih_cmd_rsp_print(" P-t-P:%s ", ap->sprint(&ptr->dstaddr, 1));
		}
		if (ptr->flags & IFF_BROADCAST) {
			ih_cmd_rsp_print(" Bcast:%s ", ap->sprint(&ptr->broadaddr, 1));
		}
		ih_cmd_rsp_print(" Mask:%s\n", ap->sprint(&ptr->netmask, 1));
	}	
	
	ih_cmd_rsp_print("          ");
	/* DONT FORGET TO ADD THE FLAGS IN ife_print_short, too */
	
	ife_print6(iface);

	if (ptr->flags == 0) {
		ih_cmd_rsp_print("[NO FLAGS] ");
	} else {
		static const char ife_print_flags_strs[] ALIGN1 =
			"UP\0"
			"BROADCAST\0"
			"DEBUG\0"
			"LOOPBACK\0"
			"POINTOPOINT\0"
			"NOTRAILERS\0"
			"RUNNING\0"
			"NOARP\0"
			"PROMISC\0"
			"ALLMULTI\0"
			"SLAVE\0"
			"MASTER\0"
			"MULTICAST\0"
#ifdef HAVE_DYNAMIC
			"DYNAMIC\0"
#endif
			;
		static const unsigned short ife_print_flags_mask[] ALIGN2 = {
			IFF_UP,
			IFF_BROADCAST,
			IFF_DEBUG,
			IFF_LOOPBACK,
			IFF_POINTOPOINT,
			IFF_NOTRAILERS,
			IFF_RUNNING,
			IFF_NOARP,
			IFF_PROMISC,
			IFF_ALLMULTI,
			IFF_SLAVE,
			IFF_MASTER,
			IFF_MULTICAST
#ifdef HAVE_DYNAMIC
			,IFF_DYNAMIC
#endif
		};
		const unsigned short *mask = ife_print_flags_mask;
		const char *str = ife_print_flags_strs;
		do {
			if (ptr->flags & *mask) {
				ih_cmd_rsp_print("%s ", str);
			}
			mask++;
			str += strlen(str) + 1;
		} while (*str);
	}	

	/* DONT FORGET TO ADD THE FLAGS IN ife_print_short */
	ih_cmd_rsp_print(" MTU:%d  Metric:%d", ptr->mtu, ptr->metric ? ptr->metric : 1);
#ifdef SIOCSKEEPALIVE
	if (ptr->outfill || ptr->keepalive)
		ih_cmd_rsp_print("  Outfill:%d  Keepalive:%d", ptr->outfill, ptr->keepalive);
#endif
	ih_cmd_rsp_print("\n");	

	/* If needed, display the interface statistics. */

	if (ptr->statistics_valid) {
		/* XXX: statistics are currently only printed for the primary address,
		 *      not for the aliases, although strictly speaking they're shared
		 *      by all addresses.
		 */
		ih_cmd_rsp_print("          ");

		ih_cmd_rsp_print("RX packets:%llu errors:%lu dropped:%lu overruns:%lu frame:%lu\n",
			   ptr->stats.rx_packets, ptr->stats.rx_errors,
			   ptr->stats.rx_dropped, ptr->stats.rx_fifo_errors,
			   ptr->stats.rx_frame_errors);
		if (can_compress)
			ih_cmd_rsp_print("             compressed:%lu\n",
				   ptr->stats.rx_compressed);
		ih_cmd_rsp_print("          ");
		ih_cmd_rsp_print("TX packets:%llu errors:%lu dropped:%lu overruns:%lu carrier:%lu\n",
			   ptr->stats.tx_packets, ptr->stats.tx_errors,
			   ptr->stats.tx_dropped, ptr->stats.tx_fifo_errors,
			   ptr->stats.tx_carrier_errors);
		ih_cmd_rsp_print("          collisions:%lu ", ptr->stats.collisions);
		if (can_compress)
			ih_cmd_rsp_print("compressed:%lu ", ptr->stats.tx_compressed);
		if (ptr->tx_queue_len != -1)
			ih_cmd_rsp_print("txqueuelen:%d ", ptr->tx_queue_len);
		ih_cmd_rsp_print("\n          R");
		print_bytes_scaled(ptr->stats.rx_bytes, "  T");
		print_bytes_scaled(ptr->stats.tx_bytes, "\n");
	}

	//ih_cmd_rsp_print("\n");
}


void ih_if_basic_display1(IF_INFO *if_info, char *display_name)
{
	char name[IFNAMSIZ];
	struct interface ife;

	if (!if_info || !display_name) {
		return;
	}

	memset(name, 0, sizeof(name));
	memset(&ife, 0, sizeof(struct interface));
	
	if (vif_get_sys_name(if_info, name) != IH_ERR_OK){
		return;
	}

	strncpy(ife.name, name, IFNAMSIZ);
	ih_if_fetch(&ife);
	ih_if_read_proc(&ife);
	snprintf(ife.name, IFNAMSIZ, "%s", display_name);
	
	ih_ife_print(&ife, name);
}

void ih_if_get_display_name(IF_INFO *if_info, char *display_name)
{
	if (!if_info || !display_name) {
		return;
	}

	switch(if_info->type){
	case IF_TYPE_FE:
		snprintf(display_name, IFNAMSIZ, "FastEthernet%d/%d", if_info->slot, if_info->port);
		break;
	case IF_TYPE_GE:
		snprintf(display_name, IFNAMSIZ, "GigabiteEhernet%d/%d", if_info->slot, if_info->port);
		break;
	case IF_TYPE_CELLULAR:
		snprintf(display_name, IFNAMSIZ, "Cellular%d", if_info->port);
		break;
	case IF_TYPE_TUNNEL:
		snprintf(display_name, IFNAMSIZ, "Tunnel%d", if_info->port);
		break;
	case IF_TYPE_VXLAN:
		snprintf(display_name, IFNAMSIZ, "Vxlan%d", if_info->port);
		break;
	case IF_TYPE_SVI:
#if 0
		if(if_info->port == DEF_WIFI_VLAN){
			snprintf(display_name, IFNAMSIZ, "Wlan1");
		}else {
			snprintf(display_name, IFNAMSIZ, "Vlan%d", if_info->port);
		}		
#else
		snprintf(display_name, IFNAMSIZ, "Vlan%d", if_info->port);
#endif
		break;	
	case IF_TYPE_LO:
		snprintf(display_name, IFNAMSIZ, "Loopback%d", if_info->port);
		break;			
	case IF_TYPE_DIALER:
		snprintf(display_name, IFNAMSIZ, "Dialer%d", if_info->port);
		break;
	case IF_TYPE_VP:
		snprintf(display_name, IFNAMSIZ, "Virtual-PPP%d", if_info->port);
		break;
	case IF_TYPE_OPENVPN:
        if (if_info->port > 10) {
            snprintf(display_name, IFNAMSIZ, "Openvpn%s", "-Server");
        } else {
            snprintf(display_name, IFNAMSIZ, "Openvpn%d", if_info->port);
        }
		break;
	case IF_TYPE_SUB_FE:
		snprintf(display_name, IFNAMSIZ, "FastEthernet%d/%d.%d", if_info->slot, if_info->port, if_info->sid);
		break;
	case IF_TYPE_SUB_GE:
		snprintf(display_name, IFNAMSIZ, "GigabiteEhernet%d/%d.%d", if_info->slot, if_info->port, if_info->sid);
		break;
	case IF_TYPE_SUB_CELLULAR:
		snprintf(display_name, IFNAMSIZ, "Cellular%d.%d", if_info->port, if_info->sid);
		break;
	case IF_TYPE_DOT11:
		if((ih_license_support(IH_FEATURE_WLAN_MTK) && ih_license_support(IH_FEATURE_IR9)) ||
			((ih_license_support(IH_FEATURE_WLAN_AP) || ih_license_support(IH_FEATURE_WLAN_STA)) && ih_license_support(IH_FEATURE_IG9)))
			snprintf(display_name, IFNAMSIZ, "Dot11Radio%d", if_info->port);
		else 
			snprintf(display_name, IFNAMSIZ, "Wlan%d", if_info->port);
		break;
	case IF_TYPE_SUB_DOT11:
		if((ih_license_support(IH_FEATURE_WLAN_MTK) && ih_license_support(IH_FEATURE_IR9)) ||
			((ih_license_support(IH_FEATURE_WLAN_AP) || ih_license_support(IH_FEATURE_WLAN_STA)) && ih_license_support(IH_FEATURE_IG9)))
			snprintf(display_name, IFNAMSIZ, "Dot11Radio%d.%d", if_info->port, if_info->sid);
		else 
			snprintf(display_name, IFNAMSIZ, "Wlan%d.%d", if_info->port, if_info->sid);
		break;
	case IF_TYPE_BRIDGE:
		snprintf(display_name, IFNAMSIZ, "Bridge %d", if_info->port);
		break;
	case IF_TYPE_VE:
		snprintf(display_name, IFNAMSIZ, "Virtual-eth%d", if_info->port);
		break;
	case IF_TYPE_CAN:
		snprintf(display_name, IFNAMSIZ, "CAN%d", if_info->port);
		break;
	default:
		LOG_ER("Bad interface type %d", if_info->type);
		return;
	}

	return;
}

void ih_if_basic_display(IF_INFO *if_info)
{
	char name[IFNAMSIZ];
	struct interface ife;

	memset(name, 0, sizeof(name));
	memset(&ife, 0, sizeof(struct interface));
	
	if (vif_get_sys_name(if_info, name) != IH_ERR_OK){
		return;
	}

	strncpy(ife.name, name, IFNAMSIZ);
	ih_if_fetch(&ife);
	ih_if_read_proc(&ife);
	switch(if_info->type){
	case IF_TYPE_FE:
		snprintf(ife.name, IFNAMSIZ, "FastEthernet%d/%d", if_info->slot, if_info->port);
		break;
	case IF_TYPE_GE:
		snprintf(ife.name, IFNAMSIZ, "GigabiteEhernet%d/%d", if_info->slot, if_info->port);
		break;
	case IF_TYPE_CELLULAR:
		snprintf(ife.name, IFNAMSIZ, "Cellular%d", if_info->port);
		break;
	case IF_TYPE_TUNNEL:
		snprintf(ife.name, IFNAMSIZ, "Tunnel%d", if_info->port);
		break;
	case IF_TYPE_VXLAN:
		snprintf(ife.name, IFNAMSIZ, "Vxlan%d", if_info->port);
		break;
	case IF_TYPE_SVI:
#if 0
		if(if_info->port == DEF_WIFI_VLAN){
			snprintf(ife.name, IFNAMSIZ, "Wlan1");
		}else {
			snprintf(ife.name, IFNAMSIZ, "Vlan%d", if_info->port);
		}		
#else
		snprintf(ife.name, IFNAMSIZ, "Vlan%d", if_info->port);
#endif
		break;	
	case IF_TYPE_LO:
		snprintf(ife.name, IFNAMSIZ, "Loopback%d", if_info->port);
		break;			
	case IF_TYPE_DIALER:
		snprintf(ife.name, IFNAMSIZ, "Dialer%d", if_info->port);
		break;
	case IF_TYPE_VP:
		snprintf(ife.name, IFNAMSIZ, "Virtual-PPP%d", if_info->port);
		break;
	case IF_TYPE_OPENVPN:
        if (if_info->port > 10) {
            snprintf(ife.name, IFNAMSIZ, "Openvpn%s", "-Server");
        } else {
            snprintf(ife.name, IFNAMSIZ, "Openvpn%d", if_info->port);
        }
		break;
	case IF_TYPE_SUB_FE:
		snprintf(ife.name, IFNAMSIZ, "FastEthernet%d/%d.%d", if_info->slot, if_info->port, if_info->sid);
		break;
	case IF_TYPE_SUB_GE:
		snprintf(ife.name, IFNAMSIZ, "GigabiteEhernet%d/%d.%d", if_info->slot, if_info->port, if_info->sid);
		break;
	case IF_TYPE_SUB_CELLULAR:
		snprintf(ife.name, IFNAMSIZ, "Cellular%d.%d", if_info->port, if_info->sid);
		break;
	case IF_TYPE_DOT11:
		if((ih_license_support(IH_FEATURE_WLAN_MTK) && ih_license_support(IH_FEATURE_IR9)) ||
			((ih_license_support(IH_FEATURE_WLAN_AP) || ih_license_support(IH_FEATURE_WLAN_STA)) && ih_license_support(IH_FEATURE_IG9)))
			snprintf(ife.name, IFNAMSIZ, "Dot11Radio%d", if_info->port);
		else 
			snprintf(ife.name, IFNAMSIZ, "Wlan%d", if_info->port);
		break;
	case IF_TYPE_SUB_DOT11:
		if((ih_license_support(IH_FEATURE_WLAN_MTK) && ih_license_support(IH_FEATURE_IR9)) ||
			((ih_license_support(IH_FEATURE_WLAN_AP) || ih_license_support(IH_FEATURE_WLAN_STA)) && ih_license_support(IH_FEATURE_IG9)))
			snprintf(ife.name, IFNAMSIZ, "Dot11Radio%d.%d", if_info->port, if_info->sid);
		else 
			snprintf(ife.name, IFNAMSIZ, "Wlan%d.%d", if_info->port, if_info->sid);
		break;
	case IF_TYPE_BRIDGE:
		snprintf(ife.name, IFNAMSIZ, "Bridge %d", if_info->port);
		break;
	case IF_TYPE_VE:
		snprintf(ife.name, IFNAMSIZ, "Virtual-eth%d", if_info->port);
		break;
	case IF_TYPE_CAN:
		snprintf(ife.name, IFNAMSIZ, "CAN%d", if_info->port);
		break;
	default:
		LOG_ER("Bad interface type %d", if_info->type);
		return;
	}
	
	ih_ife_print(&ife, name);
}

#ifdef ZYB_DEBUG
static void ife_print(struct interface *ptr)
{
	const struct aftype *ap;
	const struct hwtype *hw;
	int hf;
	int can_compress = 0;

	ap = get_afntype(ptr->addr.sa_family);
	if (ap == NULL)
		ap = get_afntype(0);

	hf = ptr->type;

	if (hf == ARPHRD_CSLIP || hf == ARPHRD_CSLIP6)
		can_compress = 1;

	hw = get_hwntype(hf);
	if (hw == NULL)
		hw = get_hwntype(-1);

	ih_cmd_rsp_print("%-9s Link encap:%s  ", ptr->name, hw->title);
	/* For some hardware types (eg Ash, ATM) we don't print the
	   hardware address if it's null.  */
	if (hw->print != NULL
	 && !(hw_null_address(hw, ptr->hwaddr) && hw->suppress_null_addr)
	) {
		ih_cmd_rsp_print("HWaddr %s  ", hw->print((unsigned char *)ptr->hwaddr));
	}
#ifdef IFF_PORTSEL
	if (ptr->flags & IFF_PORTSEL) {
		ih_cmd_rsp_print("Media:%s", if_port_text[ptr->map.port] /* [0] */);
		if (ptr->flags & IFF_AUTOMEDIA)
			ih_cmd_rsp_print("(auto)");
	}
#endif
	ih_cmd_rsp_print("\n");
	

	if (ptr->has_ip) {
		ih_cmd_rsp_print("          %s addr:%s ", ap->name,
			   ap->sprint(&ptr->addr, 1));
		if (ptr->flags & IFF_POINTOPOINT) {
			printf(" P-t-P:%s ", ap->sprint(&ptr->dstaddr, 1));
		}
		if (ptr->flags & IFF_BROADCAST) {
			printf(" Bcast:%s ", ap->sprint(&ptr->broadaddr, 1));
		}
		printf(" Mask:%s\n", ap->sprint(&ptr->netmask, 1));
	}

	ife_print6(ptr);

	printf("          ");
	/* DONT FORGET TO ADD THE FLAGS IN ife_print_short, too */

	if (ptr->flags == 0) {
		printf("[NO FLAGS] ");
	} else {
		static const char ife_print_flags_strs[] ALIGN1 =
			"UP\0"
			"BROADCAST\0"
			"DEBUG\0"
			"LOOPBACK\0"
			"POINTOPOINT\0"
			"NOTRAILERS\0"
			"RUNNING\0"
			"NOARP\0"
			"PROMISC\0"
			"ALLMULTI\0"
			"SLAVE\0"
			"MASTER\0"
			"MULTICAST\0"
#ifdef HAVE_DYNAMIC
			"DYNAMIC\0"
#endif
			;
		static const unsigned short ife_print_flags_mask[] ALIGN2 = {
			IFF_UP,
			IFF_BROADCAST,
			IFF_DEBUG,
			IFF_LOOPBACK,
			IFF_POINTOPOINT,
			IFF_NOTRAILERS,
			IFF_RUNNING,
			IFF_NOARP,
			IFF_PROMISC,
			IFF_ALLMULTI,
			IFF_SLAVE,
			IFF_MASTER,
			IFF_MULTICAST
#ifdef HAVE_DYNAMIC
			,IFF_DYNAMIC
#endif
		};
		const unsigned short *mask = ife_print_flags_mask;
		const char *str = ife_print_flags_strs;
		do {
			if (ptr->flags & *mask) {
				printf("%s ", str);
			}
			mask++;
			str += strlen(str) + 1;
		} while (*str);
	}

	/* DONT FORGET TO ADD THE FLAGS IN ife_print_short */
	printf(" MTU:%d  Metric:%d", ptr->mtu, ptr->metric ? ptr->metric : 1);
#ifdef SIOCSKEEPALIVE
	if (ptr->outfill || ptr->keepalive)
		printf("  Outfill:%d  Keepalive:%d", ptr->outfill, ptr->keepalive);
#endif
	//bb_putchar('\n');

	/* If needed, display the interface statistics. */

	if (ptr->statistics_valid) {
		/* XXX: statistics are currently only printed for the primary address,
		 *      not for the aliases, although strictly speaking they're shared
		 *      by all addresses.
		 */
		printf("          ");

		printf("RX packets:%llu errors:%lu dropped:%lu overruns:%lu frame:%lu\n",
			   ptr->stats.rx_packets, ptr->stats.rx_errors,
			   ptr->stats.rx_dropped, ptr->stats.rx_fifo_errors,
			   ptr->stats.rx_frame_errors);
		if (can_compress)
			printf("             compressed:%lu\n",
				   ptr->stats.rx_compressed);
		printf("          ");
		printf("TX packets:%llu errors:%lu dropped:%lu overruns:%lu carrier:%lu\n",
			   ptr->stats.tx_packets, ptr->stats.tx_errors,
			   ptr->stats.tx_dropped, ptr->stats.tx_fifo_errors,
			   ptr->stats.tx_carrier_errors);
		printf("          collisions:%lu ", ptr->stats.collisions);
		if (can_compress)
			printf("compressed:%lu ", ptr->stats.tx_compressed);
		if (ptr->tx_queue_len != -1)
			printf("txqueuelen:%d ", ptr->tx_queue_len);
		printf("\n          R");
		print_bytes_scaled(ptr->stats.rx_bytes, "  T");
		print_bytes_scaled(ptr->stats.tx_bytes, "\n");
	}

	if (ptr->map.irq || ptr->map.mem_start
	 || ptr->map.dma || ptr->map.base_addr
	) {
		printf("          ");
		if (ptr->map.irq)
			printf("Interrupt:%d ", ptr->map.irq);
		if (ptr->map.base_addr >= 0x100)	/* Only print devices using it for
											   I/O maps */
			printf("Base address:0x%lx ",
				   (unsigned long) ptr->map.base_addr);
		if (ptr->map.mem_start) {
			printf("Memory:%lx-%lx ", ptr->map.mem_start,
				   ptr->map.mem_end);
		}
		if (ptr->map.dma)
			printf("DMA chan:%x ", ptr->map.dma);
		//bb_putchar('\n');
	}
	//bb_putchar('\n');
}
#endif	

BOOL ih_check_interface_exist(char *name)
{
	DIR *dirptr = NULL;
	char dir[64] = {0};
	
	if (!name) {
		LOG_ER("interface name is null!");
		return FALSE;
	} 

	snprintf(dir, sizeof(dir), "/sys/class/net/%s", name);

	if ((dirptr = opendir(dir)) == NULL) {  
		return FALSE;  
	}

	closedir(dirptr);
	return TRUE;  
}

