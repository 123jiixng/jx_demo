/*the main file of inhand ipv6 icmp function. 
*
* Copyright (c) 2001-2012 InHand Networks, Inc.
*
* PROPRIETARY RIGHTS of InHand Networks are involved in the
* subject matter of this material.	All manufacturing, reproduction,
* use, and sales rights pertaining to this subject matter are governed
* by the license agreement.  The recipient of this software implicitly
* accepts the terms of the license.
*
* Creation Date: 2016-07-10
* Author: Puxc
* Description:
*	   Provide API for control icmp detecting
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "ih_logtrace.h"
#include "ih_ipc.h"
#include "if_common.h"
#include "vif_shared.h"
#include "ipv6_icmp.h"

#define DEFDATALEN	56
#define MAXIPLEN	60
#define MAXICMPLEN	76
#define MINICMPLEN	8
/* common routines */

//****************************************************
int icmp6_socket_create(void)
{
	int sock;

	sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);

	if (sock < 0) {
		LOG_IN("%s:Socket err[%d]:%s\n",__FUNCTION__, errno, strerror(errno));
	}

	return sock;
}

int icmp6_socket_init(struct in6_addr src)
{
	int sock;
	struct icmp6_filter filt;

	sock = icmp6_socket_create();
	if(sock<0){
		LOG_ER("%s: Can't create socket!",__FUNCTION__);
		return -1;
	}

	if (!IN6_IS_ADDR_UNSPECIFIED(&src)) {
		struct sockaddr_in6 myaddr;

		bzero((char *)&myaddr, sizeof(myaddr));
		myaddr.sin6_family = AF_INET6;
		myaddr.sin6_port = 0;
		memcpy(&myaddr.sin6_addr, &src, sizeof(struct in6_addr));

		if (bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0){
			LOG_ER("%s: Can't bind source address %s err[%d]:%s",
					__FUNCTION__,inet6_ntoa(myaddr.sin6_addr), errno, strerror(errno));
			/*bind fail, should close the created sock*/
			close(sock);
			return -2;
		}
	}

	ICMP6_FILTER_SETBLOCKALL(&filt);

	ICMP6_FILTER_SETPASS(ICMP6_DST_UNREACH, &filt);
	ICMP6_FILTER_SETPASS(ICMP6_TIME_EXCEEDED, &filt);
	ICMP6_FILTER_SETPASS(ICMP6_PARAM_PROB, &filt);
	ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY, &filt);
	ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT, &filt);

	if (setsockopt(sock, IPPROTO_ICMPV6, ICMP6_FILTER, (char*)&filt, sizeof(filt)) == -1) {
		LOG_ER("WARNING: setsockopt(ICMP_FILTER) err[%d]:%s", errno, strerror(errno));
	}

	LOG_DB("init sock:%d",sock);
	return sock;
}

int icmp6_send_request(int sock, struct in6_addr dst, unsigned int datasize, unsigned short id, unsigned short *pseq)
{
	struct sockaddr_in6 pingaddr;
	struct icmp6_hdr *icmph;
	int  c, packetlen;
	char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];

	memset(&pingaddr, 0, sizeof(struct sockaddr_in6));
	memcpy(&pingaddr.sin6_addr, &dst, sizeof(struct in6_addr));
	pingaddr.sin6_port = htons(IPPROTO_ICMPV6);
	pingaddr.sin6_family = AF_INET6;

	icmph = (struct icmp6_hdr *)packet;
	icmph->icmp6_type = ICMP6_ECHO_REQUEST;
	icmph->icmp6_code = 0;
	icmph->icmp6_cksum = 0;
	icmph->icmp6_id = id;
	icmph->icmp6_seq = htons(*pseq);

	packetlen = datasize + 8;
	*pseq += 1;

	gettimeofday((struct timeval *)&packet[8],
		(struct timezone *)NULL);

	LOG_DB("send sock:%d,addr:[%s]",sock,inet6_ntoa(pingaddr.sin6_addr));
	c = sendto(sock, packet, packetlen, 0,
			(struct sockaddr *) &pingaddr, sizeof(struct sockaddr_in6));

	if (c < 0 || c != packetlen){
		LOG_ER("%s: failed to send icmp6 packet to:%s  err[%d]:%s\n",
				__FUNCTION__,inet6_ntoa(pingaddr.sin6_addr), errno, strerror(errno));
		return -4;
	}

	return 0;
}

static void set_route_metric(char *iface, char *metric){
	if(!iface || !metric){
		LOG_ER("set route metric error (null pointer)");
		return;
	}

	eval("ip", "-6", "route", "del", "ff00::/8", "dev", iface);
	eval("ip", "-6", "route", "add", "ff00::/8", "dev", iface, "metric", metric);
}

int icmp6_send_rs(int sock, char *iface)
{
	struct sockaddr_in6 rs_addr;
	struct nd_router_solicit *icmph;     /* router solicitation */
	int  c, packetlen;
	char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN] = {0};
	int hoplimit = 255;

	if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char*)&hoplimit, sizeof(hoplimit)) == -1) {
		LOG_ER("WARNING: setsockopt(ICMP_MULTICAST_HOPS) err[%d]:%s\n", errno, strerror(errno));
		return -1;
	}

	memset(&rs_addr, 0, sizeof(struct sockaddr_in6));
	inet_pton(AF_INET6, ALL_ROUTER_ADDR, &rs_addr.sin6_addr);
	rs_addr.sin6_port = htons(IPPROTO_ICMPV6);
	rs_addr.sin6_family = AF_INET6;

	icmph = (struct nd_router_solicit *)packet;
	icmph->nd_rs_type = ND_ROUTER_SOLICIT;
	icmph->nd_rs_code = 0;
	icmph->nd_rs_cksum = 0;
	icmph->nd_rs_reserved = 0;

	packetlen = 8;
	
	if(iface && iface[0]){
		set_route_metric(iface, "1");
	}

	LOG_DB("%s send rs sock:%d,addr:[%s]", iface, sock, inet6_ntoa(rs_addr.sin6_addr));
	c = sendto(sock, packet, packetlen, 0,
			(struct sockaddr *) &rs_addr, sizeof(struct sockaddr_in6));

	if(iface && iface[0]){
		set_route_metric(iface, "256");
	}

	if (c < 0 || c != packetlen){
		LOG_ER("%s: failed to send icmp6 packet to:%s  err[%d]:%s\n",
				__FUNCTION__,inet6_ntoa(rs_addr.sin6_addr), errno, strerror(errno));
		return -2;
	}

	return 0;
}

int icmp6_gethostbyname(char *name, struct in6_addr *addr)
{
	struct addrinfo hints, *ai;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	if (getaddrinfo(name, NULL, &hints, &ai)) {
		LOG_IN("get icmp6 addr info, error %d[%s]", errno, strerror(errno));
		return -2;
	}

	LOG_IN("get icmp6 addr [%s]", inet6_ntoa(((struct sockaddr_in6 *)(ai->ai_addr))->sin6_addr));

	memcpy(addr, &((struct sockaddr_in6 *)(ai->ai_addr))->sin6_addr, sizeof(struct in6_addr));

	return 0;
}

int icmp6_recv_rsp(int sock, struct in6_addr src, unsigned short id)
{
	struct icmp6_hdr *icmph;
	int cc;
	char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];
	char addrbuf[128];
	char ans_data[256];
	struct iovec iov;
	struct msghdr msg;
	struct sockaddr_in6 *from;
	int packetlen;

	packetlen = MAXICMPLEN;

	iov.iov_base = (char *)packet;
	iov.iov_len = packetlen;
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = addrbuf;
	msg.msg_namelen = sizeof(addrbuf);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = ans_data;
	msg.msg_controllen = sizeof(ans_data);

	cc = recvmsg(sock, &msg, 0);
	if (cc < 0) {
		syslog(LOG_WARNING,
				"%s: recvfrom returned error %d (%s)\n",
				__FUNCTION__, errno, strerror (errno));
		return -1;
	}

	from = (struct sockaddr_in6 *)addrbuf;
	icmph = (struct icmp6_hdr *)packet;

	if (cc < MINICMPLEN) {
		LOG_ER("packet too short (%d bytes) from %s", 
				cc, inet6_ntoa(from->sin6_addr));
		return -2;
	}

	if(!IN6_ARE_ADDR_EQUAL(&from->sin6_addr, &src)) {
		return -3;
	}

	if (icmph->icmp6_type != ICMP6_ECHO_REPLY) {
		return -4;
	}

	if (icmph->icmp6_id != id) {
		/* Received icmpv6 packet is for other task */
		return -5;
	}

	return 0;
}

static int get_ra_prefix_info(char *radata, int len, struct in6_addr *prefix, int *prefix_len)
{
	struct nd_router_advert *icmph;       	/* router advertisement */
	struct nd_opt_hdr *opt;            	/* Neighbor discovery option header */
	struct nd_opt_prefix_info *prefix_info; /* prefix information */
	int prefix_size = sizeof(struct nd_opt_prefix_info);
	int opt_size = sizeof(struct nd_opt_hdr);
	int idx, got = 0;
	
	if(!radata || !prefix || !prefix_len){
		LOG_WA("get ra prefix info error (null pointer)");
		return -1;
	}
	
	icmph = (struct nd_router_advert *)radata;
	if(icmph->nd_ra_type != ND_ROUTER_ADVERT){
		LOG_WA("packet isn't router advertisment");
		return -2;
	}

	idx = sizeof(struct nd_router_advert);
	while((idx + opt_size) < len){
		opt = (struct nd_opt_hdr *)&radata[idx];
		if(opt->nd_opt_type == ND_OPT_PREFIX_INFORMATION && (idx + prefix_size) <= len){
			prefix_info = (struct nd_opt_prefix_info *)&radata[idx];
			memcpy(prefix, &prefix_info->nd_opt_pi_prefix, sizeof(struct in6_addr));
			*prefix_len = prefix_info->nd_opt_pi_prefix_len;
			got = 1;
			break;
		}
		idx += (opt->nd_opt_len * 8); 
	}	

	return got;	
}

#define _PATH_PROCNET_IFINET6   "/proc/net/if_inet6"
static int if_check_prefix(char *iface, struct in6_addr ra_prefix, int prefix_len)
{
	FILE *f;
	char addr6[40], devname[20];
	struct sockaddr_in6 sap;
	struct in6_addr prefix;
	int plen, scope, dad_status, if_idx;
	char addr6p[8][5];
	int ret = 0;

	if(!iface){
		LOG_ER("check interface prefix error (null pointer)");
		return -1;
	}

	f = fopen(_PATH_PROCNET_IFINET6, "r");
	if (f == NULL){
		LOG_ER("can't open %s", _PATH_PROCNET_IFINET6);
		return -2;
	}

	while (fscanf(f, "%4s%4s%4s%4s%4s%4s%4s%4s %08x %02x %02x %02x %20s\n",
			addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4],
			addr6p[5], addr6p[6], addr6p[7], &if_idx, &plen, &scope,
			&dad_status, devname) != EOF)
       	{
		if (iface[0] && !strcmp(devname, iface) && scope == 0) {
			sprintf(addr6, "%s:%s:%s:%s:%s:%s:%s:%s",
					addr6p[0], addr6p[1], addr6p[2], addr6p[3],
					addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
			inet_pton(AF_INET6, addr6,(struct sockaddr *) &sap.sin6_addr);

			in6_addr_2prefix(sap.sin6_addr, prefix_len, &prefix);
			if(IN6_ARE_ADDR_EQUAL(&prefix, &ra_prefix)){
				ret = 1;
				break;
			}
		}
	}
	fclose(f);

	return ret;
}

int icmp6_recv_ra(int sock, char *iface)
{
	struct nd_router_advert *icmph;       /* router advertisement */
	struct in6_addr ra_prefix;
	int prefix_len, cc;
	char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];
	char addrbuf[128];
	char ans_data[256];
	struct iovec iov;
	struct msghdr msg;
	struct sockaddr_in6 *from;
	int packetlen;

	if(!iface && !iface[0]){
		LOG_WA("interface is unspecified");
		return -1;
	}

	packetlen = MAXICMPLEN;

	iov.iov_base = (char *)packet;
	iov.iov_len = packetlen;
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = addrbuf;
	msg.msg_namelen = sizeof(addrbuf);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = ans_data;
	msg.msg_controllen = sizeof(ans_data);

	cc = recvmsg(sock, &msg, 0);
	if (cc < 0) {
		syslog(LOG_WARNING,
				"%s: recvfrom returned error %d (%s)\n",
				__FUNCTION__, errno, strerror (errno));
		return -2;
	}

	from = (struct sockaddr_in6 *)addrbuf;
	icmph = (struct nd_router_advert *)packet;

	if(cc < MINICMPLEN){
		LOG_ER("packet too short (%d bytes) from %s", 
				cc, inet6_ntoa(from->sin6_addr));
		return -3;
	}
	
	if(icmph->nd_ra_type != ND_ROUTER_ADVERT){
		LOG_ER("packet isn't router advertisement");
		return -4;
	}
	
	if(get_ra_prefix_info(packet, cc, &ra_prefix, &prefix_len) != 1){
		LOG_ER("not get prefix info");
		return -5;
	}
	
	if(if_check_prefix(iface, ra_prefix, prefix_len) != 1){
		LOG_ER("compare prefix error, ra prefix is %s", inet6_ntoa(ra_prefix));
		return -6;
	}

	return 0;
}
