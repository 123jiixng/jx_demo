/*
 * $Id$ --
 *
 *   ipv6 common functions
 *
 * Copyright (c) 2001-2015 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 12/01/2015
 * Author: wucl 
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "ih_logtrace.h"
#include "is_port_map.h"
#include "ih_cmd.h"
#include "sw_ipc.h"
#include "console.h"
#include "cli_api.h"
#include "ih_adm.h"
#include "if_common.h"
#include "agent_ipc.h"
#include "ih_vif.h"
#include "vif_shared.h"

char odhcp6c_caller_name[ODHCP6C_MAX][128] = {
	[ODHCP6C_WLAN]="wlan-odhcp6c-event",
	[ODHCP6C_BRIDGE]="bridge-odhcp6c-event",
	[ODHCP6C_WWAN]="wwan-odhcp6c-event",
	[ODHCP6C_INF]="inf-odhcp6c-event"
};

unsigned int ipv6_addr_scope2type(unsigned scope)
{
	switch(scope) {
	case IPV6_ADDR_SCOPE_NODELOCAL:
		return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_NODELOCAL) |
			IPV6_ADDR_LOOPBACK);
	case IPV6_ADDR_SCOPE_LINKLOCAL:
		return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL) |
			IPV6_ADDR_LINKLOCAL);
	case IPV6_ADDR_SCOPE_SITELOCAL:
		return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_SITELOCAL) |
			IPV6_ADDR_SITELOCAL);
	}
	return IPV6_ADDR_SCOPE_TYPE(scope);
}

int ipv6_addr_type(const struct in6_addr *addr)
{
	__be32 st;

	st = addr->s6_addr32[0];

	/* Consider all addresses with the first three bits different of
	   000 and 111 as unicasts.
	 */
	if ((st & htonl(0xE0000000)) != htonl(0x00000000) &&
	    (st & htonl(0xE0000000)) != htonl(0xE0000000))
		return (IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));

	if ((st & htonl(0xFF000000)) == htonl(0xFF000000)) {
		/* multicast */
		/* addr-select 3.1 */
		return (IPV6_ADDR_MULTICAST |
			ipv6_addr_scope2type(IPV6_ADDR_MC_SCOPE(addr)));
	}

	if ((st & htonl(0xFFC00000)) == htonl(0xFE800000))
		return (IPV6_ADDR_LINKLOCAL | IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL));		/* addr-select 3.1 */
	if ((st & htonl(0xFFC00000)) == htonl(0xFEC00000))
		return (IPV6_ADDR_SITELOCAL | IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_SITELOCAL));		/* addr-select 3.1 */
	if ((st & htonl(0xFE000000)) == htonl(0xFC000000))
		return (IPV6_ADDR_UNICAST |
			IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));			/* RFC 4193 */

	if ((addr->s6_addr32[0] | addr->s6_addr32[1]) == 0) {
		if (addr->s6_addr32[2] == 0) {
			if (addr->s6_addr32[3] == 0)
				return IPV6_ADDR_ANY;

			if (addr->s6_addr32[3] == htonl(0x00000001))
				return (IPV6_ADDR_LOOPBACK | IPV6_ADDR_UNICAST |
					IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL));	/* addr-select 3.4 */

			return (IPV6_ADDR_COMPATv4 | IPV6_ADDR_UNICAST |
				IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));	/* addr-select 3.3 */
		}

		if (addr->s6_addr32[2] == htonl(0x0000ffff))
			return (IPV6_ADDR_MAPPED |
				IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));	/* addr-select 3.3 */
	}

	return (IPV6_ADDR_UNICAST |
		IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));	/* addr-select 3.4 */
}


/* Utility function for making IPv6 address string. */
const char *inet6_ntoa(struct in6_addr addr)
{
	static char buf[INET6_ADDRSTRLEN];

	inet_ntop(AF_INET6, &addr, buf, INET6_ADDRSTRLEN);

	return buf;
}

/*
 *Get ipv6 prefix by in6 addr and prefix length
    0 ---failed
    1 ---success
 * */
int in6_addr_2prefix(struct in6_addr ipv6_addr, int len, struct in6_addr *ipv6_prefix)
{
    int a, b;

    if(!ipv6_prefix) {
        return 0;
    }

	if (len < 0 || len > 128) {
		return 0;
	}

	/*FIX issue: while len == 128, a=16, ipv6_prefix->s6_addr[a] will overflow*/
	if (len == 128) {
		memcpy(ipv6_prefix->s6_addr, ipv6_addr.s6_addr, sizeof(struct in6_addr));
		return 1;
	}

    a = len >> 3;
    b = len & 0x7;

    bzero((void *)ipv6_prefix, sizeof(struct in6_addr));

    memcpy(ipv6_prefix->s6_addr, ipv6_addr.s6_addr, a);
    ipv6_prefix->s6_addr[a] = ipv6_addr.s6_addr[a] & (0xff00 >> b);

    return 1;
}

/*
* Utility function to convert string to IPv6 address and prefixlen
*
* return value: 
*	0 --- failed 
*	1 --- success
*
*/
int str_to_ip6addr_prefixlen(const char *str, struct in6_addr *ip6, int *prefix_len)
{
	char *slash = NULL;
	char *cp = NULL;
	int plen = 0;
	int ret = 0;

	if(!str || !ip6 || !prefix_len){
		return 0; 
	}

	slash = strchr (str, '/');

	/* If string doesn't contain `/' treat it as address only. */
	if (slash == NULL) {
		ret = inet_pton (AF_INET6, str, ip6);
		if (ret == 0){
			return 0;
		}
		*prefix_len = IPV6_MAX_BIT_LEN;
	}else{
		cp = malloc((slash - str) + 1);
		if(!cp){
			LOG_ER("no memory available\n");
			return 0;
		}
		strlcpy (cp, str, (slash - str) + 1);
		ret = inet_pton (AF_INET6, cp, ip6);
		free (cp);
		if (ret == 0){
			return 0;
		}

		plen = (u_char) atoi (++slash);
		if (plen > IPV6_MAX_BIT_LEN){
			return 0;
		}
		*prefix_len = plen;
	}

	return ret;
}

/*
* Utility function to convert string to IPv6 prefix and prefixlen
* return value: 
*	0 --- failed 
*	1 --- success
*/
int str_to_in6_prefix(char *str, struct in6_addr *prefix, int *len)
{
    int ret;
    int tmp_len;
    struct in6_addr addr;

    if (!str || !prefix || !len) {
        return 0;
    }

    ret = str_to_ip6addr_prefixlen(str, &addr, &tmp_len);
    if (ret) {
        ret = in6_addr_2prefix(addr, tmp_len, prefix);
    }
	*len = tmp_len;

    return ret;
}

/*
 *Get prefix string by in6_addr and prefix length
 * */
char *in6_addr2str(struct in6_addr ipv6_addr, int length)
{
    static char addr[INET6_ADDRSTRLEN];
    char tmpaddr[INET6_ADDRSTRLEN] = {0};

    bzero(addr, INET6_ADDRSTRLEN);

    inet_ntop(AF_INET6, &ipv6_addr, tmpaddr, INET6_ADDRSTRLEN);
    snprintf(addr, INET6_ADDRSTRLEN, "%s/%d", tmpaddr, length);

    return addr;
}

int in6_prefix_equal(struct in6_addr addr1, struct in6_addr addr2, int prefix_len)
{
	struct in6_addr prf1, prf2;

	in6_addr_2prefix(addr1, prefix_len, &prf1);
	in6_addr_2prefix(addr2, prefix_len, &prf2);

	if(IN6_ARE_ADDR_EQUAL(&prf1, &prf2)){
		return TRUE;
	}

	return FALSE;
}

int add_ip6_addr(char *iface, char *ip6, int prefix_len)
{
	char ip_prefix[IPV6_ADDR_PREFIX_STR_LEN] = {0};

	snprintf(ip_prefix, sizeof(ip_prefix), "%s/%d", ip6, prefix_len);

	LOG_IN("ipv6 addr add %s dev %s", ip_prefix, iface);
	return eval("ip", "-f", "inet6", "addr", "add", ip_prefix, "dev", iface);
}

int del_ip6_addr(char *iface, char *ip6, int prefix_len)
{
	char ip_prefix[IPV6_ADDR_PREFIX_STR_LEN] = {0};

	snprintf(ip_prefix, sizeof(ip_prefix), "%s/%d", ip6, prefix_len);

	LOG_IN("ip addr del %s dev %s", ip_prefix, iface);
	return eval("ip", "-f", "inet6", "addr", "del", ip_prefix, "dev", iface);
}
/* generate link local address */
int iface_down_up(char *iface)
{
#if 0
	iface_add_link_local_addr(iface);
#endif
	LOG_IN("ip link set %s down", iface);
	eval("ip", "link", "set", iface, "down");

	LOG_IN("ip link set %s up", iface);
	eval("ip", "link", "set", iface, "up");

	return IH_ERR_OK;
}

int form_eui64_xid(unsigned char *mac_addr, unsigned char *eui)
{

	if(!mac_addr || !eui){
		return -1;
	}

        memcpy(eui, mac_addr, 3);
        memcpy(eui + 5, mac_addr + 3, 3);

	eui[3] = 0xFF;
	eui[4] = 0xFE;
	eui[0] ^= 2;

	return 0;
}

int iface_add_link_local_addr(char *iface)
{
	unsigned char mac[6] = {0};
	struct in6_addr addr;

	memset(&addr, 0, sizeof(struct in6_addr));
	addr.s6_addr32[0] = htonl(0xFE800000);

	get_iface_mac(iface, mac, sizeof(mac));  
	form_eui64_xid(mac, &addr.s6_addr[8]);

	add_ip6_addr(iface, (char *)inet6_ntoa(addr), 64);

	return ERR_OK;
}

int add_ipv6_str_interface_id_to_address (struct in6_addr *address, char *id) 
{
	char buf[48] = {'\0'};	
	struct in6_addr tmp_ip6, prefix;
	int i = 0;
	int ret = 0;

	if (!id || !address) {
		return ERR_INVAL;
	}

	snprintf(buf, sizeof(buf), "2001:0:0:0:");

	strncat(buf, id, sizeof(buf)-strlen("2001:0:0:0:")-1);
	ret = inet_pton(AF_INET6, buf, &tmp_ip6);
	if (ret == 0){
		return ERR_INVAL;
	}
	
	in6_addr_2prefix(*address, 64, &prefix);
	
	for(i = 8; i < sizeof(prefix.s6_addr); i++) {
		if (tmp_ip6.s6_addr[i]) {
			prefix.s6_addr[i] |= tmp_ip6.s6_addr[i];
		}
	}

	memcpy(address->s6_addr, prefix.s6_addr, sizeof(address->s6_addr));

	return ERR_OK;	
}

int vif_add_link_local_addr(VIF_INFO *vif)
{
	char iface[IF_CMD_NAME_SIZE] = {0};
	unsigned char mac[6] = {0};
	struct in6_addr addr;

	if(!vif){
		return ERR_OK;
	}

	memset(iface, 0, sizeof(iface));
	if(vif_get_sys_name(&(vif->if_info), iface) != IH_ERR_OK){
		return ERR_SRCH;
	}

	memset(&addr, 0, sizeof(struct in6_addr));
	addr.s6_addr32[0] = htonl(0xFE800000);

	get_iface_mac(iface, mac, sizeof(mac));  
	if(XID_EUI64 == vif->ip6_addr.xid_method){
		form_eui64_xid(mac, &addr.s6_addr[8]);
	}

	add_ip6_addr(iface, (char *)inet6_ntoa(addr), 64);

	return ERR_OK;
}

void iface_ipv6_conf_write(char *iface, char *name, char *value)
{
	char buff[PROC_IPV6_IFACE_CONF_LEN] = {0};

	if(!iface || !name || !value){
		return;
	}

	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff), "echo %s > /proc/sys/net/ipv6/conf/%s/%s", value, iface, name);
	system(buff);

	return;
}

void enable_ipv6_forward(int enable)
{
	iface_ipv6_conf_write("all", "forwarding", enable? "1": "0");
	return;
}

void enable_ipv6(int enable)
{
	enable_ipv6_forward(enable);
	return;
}

/* enable/disable IPv6 autoconf. iface is system interface name */
void enable_ipv6_autoconf(char *iface, int enable)
{
	iface_ipv6_conf_write(iface, "autoconf", enable? "1": "0");
	return;
}

int iface_has_valid_ip6_address(IPv6_IFADDR *ip6_ifaddr)
{
	int i = 0;
	int count = 0;

	if(!ip6_ifaddr){
		return 0;
	}

	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
		if (!IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->static_ifaddr[i].ip6)){
			count++;
		}

		if (!IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->dynamic_ifaddr[i].ip6)){
			count++;
		}
	}
	
	return count;
}

int iface_has_valid_static_ip6_address(IPv6_IFADDR *ip6_ifaddr)
{
	int i = 0;
	int count = 0;

	if(!ip6_ifaddr){
		return 0;
	}

	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
		if (!IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->static_ifaddr[i].ip6)){
			count++;
		}
	}
	
	return count;
}

int iface_has_valid_dynamic_ip6_address(IPv6_IFADDR *ip6_ifaddr)
{
	int i = 0;
	int count = 0;

	if(!ip6_ifaddr){
		return 0;
	}

	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
		if (!IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->dynamic_ifaddr[i].ip6)){
			count++;
		}
	}
	
	return count;
}

int iface_has_global_ip6_address(IPv6_IFADDR *ip6_ifaddr)
{
	int i = 0;
	int count = 0;

	if(!ip6_ifaddr){
		return 0;
	}

	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
		if (ipv6_addr_type(&ip6_ifaddr->static_ifaddr[i].ip6) & IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL)){
			count++;
		}

		if (ipv6_addr_type(&ip6_ifaddr->dynamic_ifaddr[i].ip6) & IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL)){
			count++;
		}
	}
	
	return count;
}

/* add interface's IPv6 addresses by using ip command. */
int iface_add_ip6_addr(char *iface, IPv6_IFADDR *ip6_ifaddr)
{
	int i = 0;

	if(!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR;
	}

//	iface_down_up(iface);

	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){

		if(IN6_IS_ADDR_UNSPECIFIED(&ip6_ifaddr->static_ifaddr[i].ip6)){
			continue;
		}

		add_ip6_addr(iface, (char *)inet6_ntoa(ip6_ifaddr->static_ifaddr[i].ip6), ip6_ifaddr->static_ifaddr[i].prefix_len);
	}

	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){

		if(IN6_IS_ADDR_UNSPECIFIED(&ip6_ifaddr->dynamic_ifaddr[i].ip6)){
			continue;
		}

		add_ip6_addr(iface, (char *)inet6_ntoa(ip6_ifaddr->dynamic_ifaddr[i].ip6), ip6_ifaddr->dynamic_ifaddr[i].prefix_len);
	}

	return IH_ERR_OK;
}


int iface_showrun_ip6_addr_cli(char *iface, IPv6_IFADDR *ip6_ifaddr)
{
	int i = 0;
	int first = 1;

	if(!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR;
	}

	ih_cmd_rsp_print("  #IPv6 config\n");

	if (ip6_ifaddr->odhcp6c_enable){
		if(STATEFUL_PD == ip6_ifaddr->dhcp6c_state){
			ih_cmd_rsp_print("  ipv6 address dhcp-pd %s\n", ip6_ifaddr->pd_label);
		}else if(STATELESS == ip6_ifaddr->dhcp6c_state){
			ih_cmd_rsp_print("  ipv6 address autoconfig\n");
		}else{
			ih_cmd_rsp_print("  ipv6 address dhcp\n");
		}
	}else{
		if(ip6_ifaddr->static_if_id_enable && ip6_ifaddr->static_interface_id[0]){
			ih_cmd_rsp_print("  ipv6 address static interface-id %s\n", ip6_ifaddr->static_interface_id);
		}

		if(PD_USER == ip6_ifaddr->pd_role){
			if(ip6_ifaddr->subnet_position){
				ih_cmd_rsp_print("  ipv6 address follow-prefix %s position %d\n", ip6_ifaddr->pd_label, ip6_ifaddr->subnet_position);
			}else{
				ih_cmd_rsp_print("  ipv6 address follow-prefix %s\n", ip6_ifaddr->pd_label);
			}

		}else{
			for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
				if(IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->static_ifaddr[i].ip6)){
					continue;
				}
				ih_cmd_rsp_print("  ipv6 address %s/%d\n", inet6_ntoa(ip6_ifaddr->static_ifaddr[i].ip6), ip6_ifaddr->static_ifaddr[i].prefix_len);

				if(first){
					first = 0;
				}
			}
		}
	}
	return IH_ERR_OK;
}
/* static dns dynamic dns ra dns */
#define DNS_NUM CFG_PER_ADDR*3
void merge_dns(struct in6_addr* dns, int dns_num, IPv6_IFADDR *ip6_ifaddr){
	int i = 0;
	int j = 0;

	if(NULL == dns || NULL == ip6_ifaddr){
		return;
	}
	for(i+=j,j=0; j<CFG_PER_ADDR && i<dns_num ;j++){
		if(IN6_IS_ADDR_UNSPECIFIED(&ip6_ifaddr->static_dns[j])){
			break;
		}
		memcpy(&dns[i+j], &ip6_ifaddr->static_dns[j], sizeof(dns[i+j]));
	}
	for(i+=j,j=0; j<CFG_PER_ADDR && i<dns_num;j++){
		if(IN6_IS_ADDR_UNSPECIFIED(&ip6_ifaddr->dhcp_dns[j])){
			break;
		}
		memcpy(&dns[i+j], &ip6_ifaddr->dhcp_dns[j], sizeof(dns[i+j]));
	}
	for(i+=j,j=0; j<CFG_PER_ADDR && i<dns_num;j++){
		if(IN6_IS_ADDR_UNSPECIFIED(&ip6_ifaddr->ra_dns[j])){
			break;
		}
		memcpy(&dns[i+j], &ip6_ifaddr->ra_dns[j], sizeof(dns[i+j]));
	}
	return;
}

int iface_showrun_ip6_addr_json(char *iface, IPv6_IFADDR *ip6_ifaddr)
{
	int i = 0;
	int first = 1;
	time_t dhcpv6_remaining_lease;
	struct in6_addr	dns[DNS_NUM];

	if(!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR;
	}

	memset(dns, 0, sizeof(dns));
	merge_dns(dns, DNS_NUM, ip6_ifaddr);
	ih_cmd_rsp_print("[");//1
	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
	    ih_cmd_rsp_print("%s[", first? "": ",");
		ih_cmd_rsp_print("'%s/%d',", inet6_ntoa(ip6_ifaddr->static_ifaddr[i].ip6), ip6_ifaddr->static_ifaddr[i].prefix_len);
		ih_cmd_rsp_print("'%d',", ip6_ifaddr->odhcp6c_enable?1:0);
		ih_cmd_rsp_print("'%s',", inet6_ntoa(dns[i]));
		dhcpv6_remaining_lease = 0;
		ih_cmd_rsp_print("'%ld',", dhcpv6_remaining_lease);
		ih_cmd_rsp_print("'%d'", ip6_ifaddr->static_ifaddr[i].mtu);
		ih_cmd_rsp_print(",'%s'", "static");
	    ih_cmd_rsp_print("]");
		if(first){
			first = 0;
		}
	}

	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
	    ih_cmd_rsp_print("%s[", first? "": ",");
		ih_cmd_rsp_print("'%s/%d',", inet6_ntoa(ip6_ifaddr->dynamic_ifaddr[i].ip6), ip6_ifaddr->dynamic_ifaddr[i].prefix_len);
		ih_cmd_rsp_print("'%d',", ip6_ifaddr->odhcp6c_enable?1:3); /* 0-static 1-dynamic(DHCP) 2-pppoe 3-follow prefix */
		ih_cmd_rsp_print("'%s',", inet6_ntoa(dns[i]));
		dhcpv6_remaining_lease = (ip6_ifaddr->odhcp6c_enable )? (ip6_ifaddr->dynamic_ifaddr[i].preferred_lifetime - (get_uptime() - ip6_ifaddr->dynamic_ifaddr[i].preferred_got_time)): 0;
		if(dhcpv6_remaining_lease < 0){/* bug need fix when if down*/
			dhcpv6_remaining_lease = 0;
		}
		ih_cmd_rsp_print("'%ld',", dhcpv6_remaining_lease);
		ih_cmd_rsp_print("'%d'", ip6_ifaddr->dynamic_ifaddr[i].mtu);
		ih_cmd_rsp_print(",'%s'", "dynamic");
	    ih_cmd_rsp_print("]");
		if(first){
			first = 0;
		}
	}

	ih_cmd_rsp_print("]");
	return IH_ERR_OK;
}


/* common show running for interface's IPv6 information. */
int iface_showrun_ip6_addr(char *iface, IPv6_IFADDR *ip6_ifaddr, int flags)
{
	if(!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR;
	}

    if(IH_CMD_FLAG_JSON == flags){
		iface_showrun_ip6_addr_json(iface, ip6_ifaddr);
	}else{
		iface_showrun_ip6_addr_cli(iface, ip6_ifaddr);
	}

	return IH_ERR_OK;
}
//* add IPv6 address and its prefixlen to interface's IPv6 structure. */
int iface_set_static_ip6_addr(char *iface, IPv6_IFADDR *ip6_ifaddr, struct in6_addr ip6, int prefix_len)
{
	IPv6_NET_PARAMETERS parameter;
	
	if(!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR; 
	}
	
	bzero(&parameter, sizeof(IPv6_NET_PARAMETERS));

	parameter.ifaddr.ip6 = ip6;
	parameter.flag |= IPV6_SET_IPADDR;

	parameter.ifaddr.prefix_len = prefix_len;
	parameter.flag |= IPV6_SET_PREFIX_LEN;

	return (iface_set_all_ip6_parameters(iface, ip6_ifaddr, parameter));
}

//* add IPv6 address and its prefixlen to interface's IPv6 structure. */
int iface_set_dynamic_ip6_addr(char *iface, IPv6_IFADDR *ip6_ifaddr, struct in6_addr ip6, int prefix_len)
{
	IPv6_NET_PARAMETERS parameter;
	
	if(!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR; 
	}
	
	bzero(&parameter, sizeof(IPv6_NET_PARAMETERS));

	parameter.ifaddr.ip6 = ip6;
	parameter.flag |= IPV6_SET_DYN_IPADDR;

	parameter.ifaddr.prefix_len = prefix_len;
	parameter.flag |= IPV6_SET_PREFIX_LEN;

	return (iface_set_all_ip6_parameters(iface, ip6_ifaddr, parameter));
}

/* clear IPv6 address and its prefixlen from interface's IPv6 structure. */
int iface_clear_static_ip6_addr(char *iface, IPv6_IFADDR *ip6_ifaddr, struct in6_addr ip6, int prefix_len)
{
	int i = 0;
	int count = 0;

	if(!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR; 
	}
	
	if (ip6_ifaddr->odhcp6c_enable) {
		ih_cmd_rsp_print("%% DHCPv6 client is enabled on this interface!");
		return IH_ERR_FAILED;
	}

	if(PD_USER == ip6_ifaddr->pd_role){
		ih_cmd_rsp_print("%% Follow prefix is enabled on this interface!");
		return IH_ERR_FAILED;
	}

	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){

		if ((!memcmp(&(ip6_ifaddr->static_ifaddr[i].ip6), &ip6, sizeof(struct in6_addr))) 
			&& (ip6_ifaddr->static_ifaddr[i].prefix_len == prefix_len)){
			memset(&ip6_ifaddr->static_ifaddr[i], 0, sizeof(ip6_ifaddr->static_ifaddr[i]));
			LOG_IN("interface %s clear static IPv6 address: %s/%d", iface, inet6_ntoa(ip6), prefix_len);
			count++;
		}
	}

	if(count){
		return IH_ERR_OK;
	}

	return IH_ERR_NOT_FOUND;
}

/* clear dynamic IPv6 prefix and its prefixlen from interface's IPv6 structure. */
int iface_clear_dynamic_ip6_prefix(char *iface, IPv6_IFADDR *ip6_ifaddr, struct in6_addr prefix, int prefix_len)
{
	int i = 0;

	if(!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR; 
	}
	
	if (ip6_ifaddr->odhcp6c_enable) {
		return IH_ERR_OK;
	}

	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){

		if (in6_prefix_equal(ip6_ifaddr->dynamic_ifaddr[i].ip6, prefix, prefix_len) 
			&& (ip6_ifaddr->dynamic_ifaddr[i].prefix_len == prefix_len)){
			LOG_IN("interface %s clear dynamic IPv6 address: %s/%d", iface, inet6_ntoa(ip6_ifaddr->dynamic_ifaddr[i].ip6), prefix_len);
			memset(&ip6_ifaddr->dynamic_ifaddr[i], 0, sizeof(ip6_ifaddr->dynamic_ifaddr[i]));
		}
	}

	return IH_ERR_OK;
}

/*set all given gateway to interface's ipv6 structure*/
int iface_set_ip6_gateway(char *iface, IPv6_IFADDR *ip6_ifaddr, struct in6_addr gw, struct in6_addr addr, int prefix_len) 
{
	IPv6_NET_PARAMETERS parameter;
	bzero(&parameter, sizeof(IPv6_NET_PARAMETERS));

	parameter.ifaddr.gateway = gw;
	parameter.flag |= IPV6_SET_GATEWAY;

	parameter.ifaddr.ip6 = addr;
	parameter.flag |= IPV6_SET_IPADDR;

	parameter.ifaddr.prefix_len = prefix_len;
	parameter.flag |= IPV6_SET_PREFIX_LEN;

	return (iface_set_all_ip6_parameters(iface, ip6_ifaddr, parameter));
	
}

BOOL in6_addr_xid_is_zero(struct in6_addr ip6)
{
	if(ip6.s6_addr32[2] || ip6.s6_addr32[3]){
		return FALSE;
	}

	return TRUE;
}

/*set all given parameters to interface's ipv6 structure*/
int iface_set_all_ip6_parameters(char *iface, IPv6_IFADDR *ip6_ifaddr, IPv6_NET_PARAMETERS parameter)
{
	int i = 0;
	int dest_idx = -1;
	int already_exist = 0;
	unsigned char mac[6] = {0};
	IPv6_SUBNET *ifaddr = NULL;

	if (!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR; 
	}

	if (parameter.flag & IPV6_SET_LL_ADDR){
		ip6_ifaddr->ll_ip6 = parameter.ll_ip6;
	}

	if (!(parameter.flag & IPV6_SET_SUBNET_PARA)) {
		return IH_ERR_OK;
	}

	if (parameter.flag & IPV6_SET_DYN_IPADDR){
		ifaddr = ip6_ifaddr->dynamic_ifaddr;
	}else if (parameter.flag & IPV6_SET_IPADDR){
		ifaddr = ip6_ifaddr->static_ifaddr;
	}else{
		LOG_DB("invalid parameter.flag %x", parameter.flag);
		return IH_ERR_NULL_PTR; 
	}

	if(in6_addr_xid_is_zero(parameter.ifaddr.ip6)){
		get_iface_mac(iface, mac, sizeof(mac));  
		if(XID_EUI64 == ip6_ifaddr->xid_method){
			form_eui64_xid(mac, &parameter.ifaddr.ip6.s6_addr[8]);
		}
	}

	for (i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
		if ((!memcmp(&(ifaddr[i].ip6), &parameter.ifaddr.ip6, sizeof(struct in6_addr))) 
			&& (ifaddr[i].prefix_len == parameter.ifaddr.prefix_len)){
			already_exist = 1;
			LOG_IN("interface %s already has IPv6 address: %s/%d", iface, inet6_ntoa(parameter.ifaddr.ip6), parameter.ifaddr.prefix_len);
			dest_idx = i;
			break;
		}if (dest_idx < 0 && IN6_IS_ADDR_UNSPECIFIED (&ifaddr[i].ip6)){
			dest_idx = i;
		}
	}

	if (dest_idx < 0) {
		LOG_IN("interface %s has no space to add IPv6 address: %s/%d", iface, inet6_ntoa(parameter.ifaddr.ip6), parameter.ifaddr.prefix_len);
		return IH_ERR_FULL; 
	}

	if ((parameter.flag & IPV6_SET_IPADDR) 
		|| (parameter.flag & IPV6_SET_PREFIX_LEN)){
		if (!already_exist){
			LOG_IN("interface %s add IPv6 address: %s/%d", iface, inet6_ntoa(parameter.ifaddr.ip6), parameter.ifaddr.prefix_len);
			ifaddr[dest_idx].ip6 = parameter.ifaddr.ip6;
			ifaddr[dest_idx].prefix_len = parameter.ifaddr.prefix_len;
		}
	}

	if (parameter.flag & IPV6_SET_GATEWAY){
		ifaddr[dest_idx].gateway = parameter.ifaddr.gateway;
		LOG_IN("interface %s add IPv6 gateway: %s", iface, inet6_ntoa(parameter.ifaddr.gateway));
	}

	if (parameter.flag & IPV6_SET_DNS){
		for(i=0;i<CFG_PER_ADDR;i++){
			if(IN6_IS_ADDR_UNSPECIFIED(&(ip6_ifaddr->static_dns[i]))){	
				ip6_ifaddr->static_dns[i] = parameter.static_dns[i]; 
			}
			LOG_IN("interface %s add IPv6 dns: %s", iface, inet6_ntoa(parameter.static_dns[i]));
		}
	}

	if (parameter.flag & IPV6_SET_MTU){
		ifaddr[dest_idx].mtu = parameter.ifaddr.mtu;
		LOG_IN("interface %s add IPv6 interface mtu: %d", iface, parameter.ifaddr.mtu);
	}

	if (parameter.flag & IPV6_SET_HOPLIMIT){
		ifaddr[dest_idx].hop_limit = parameter.ifaddr.hop_limit;
		LOG_IN("interface %s add IPv6 interface hop limit: %d", iface, parameter.ifaddr.hop_limit);
	}

	return IH_ERR_OK;
}

/* clear all IPv6 address from interface's IPv6 structure. */
int iface_clear_all_ip6_addr(char *iface, IPv6_IFADDR *ip6_ifaddr)
{
	if(!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR; 
	}

	LOG_IN("interface %s clear all IPv6 address", iface);

	memset(ip6_ifaddr->static_ifaddr, 0, sizeof(ip6_ifaddr->static_ifaddr));
	memset(ip6_ifaddr->dynamic_ifaddr, 0, sizeof(ip6_ifaddr->dynamic_ifaddr));

	return IH_ERR_OK;
}
 
/* clear all static IPv6 address from interface's IPv6 structure. */
int iface_clear_all_static_ip6_addr(char *iface, IPv6_IFADDR *ip6_ifaddr)
{
	if(!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR; 
	}

	LOG_IN("interface %s clear all static IPv6 address", iface);

	memset(ip6_ifaddr->static_ifaddr, 0, sizeof(ip6_ifaddr->static_ifaddr));

	return IH_ERR_OK;
}
 
/* clear all dynamic IPv6 address from interface's IPv6 structure. */
int iface_clear_all_dynamic_ip6_addr(char *iface, IPv6_IFADDR *ip6_ifaddr)
{
	if(!iface || !ip6_ifaddr){
		return IH_ERR_NULL_PTR; 
	}

	LOG_IN("interface %s clear all dynamic IPv6 address", iface);

	memset(ip6_ifaddr->dynamic_ifaddr, 0, sizeof(ip6_ifaddr->dynamic_ifaddr));

	return IH_ERR_OK;
}
 
int iface_find_static_ip6_addr(char *iface, IPv6_IFADDR *ip6_ifaddr, struct in6_addr ip6, int prefix_len)
{
	int i = 0;
	int count = 0;

	if(!iface || !ip6_ifaddr){
		return 0; 
	}
	
	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){

		if ((!memcmp(&(ip6_ifaddr->static_ifaddr[i].ip6), &ip6, sizeof(struct in6_addr))) 
			&& (ip6_ifaddr->static_ifaddr[i].prefix_len == prefix_len)){
			count++;
		}
	}

	return count;
}

int iface_find_dynamic_ip6_addr(char *iface, IPv6_IFADDR *ip6_ifaddr, struct in6_addr ip6, int prefix_len)
{
	int i = 0;
	int count = 0;

	if(!iface || !ip6_ifaddr){
		return 0; 
	}
	
	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){

		if ((!memcmp(&(ip6_ifaddr->dynamic_ifaddr[i].ip6), &ip6, sizeof(struct in6_addr))) 
			&& (ip6_ifaddr->dynamic_ifaddr[i].prefix_len == prefix_len)){
			count++;
		}
	}

	return count;
}

int iface_get_first_ip6_addr(char *iface, IPv6_IFADDR *ip6_ifaddr, struct in6_addr *ip6_src)
{
	int i = 0;

	if(!iface || !ip6_ifaddr || !ip6_src){
		return IH_ERR_NULL_PTR; 
	}

	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
		if (IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->static_ifaddr[i].ip6)){
			continue;
		}

		memcpy(ip6_src, &ip6_ifaddr->static_ifaddr[i].ip6, sizeof(struct in6_addr));
		LOG_IN("Get static src ipv6 address %s from interface %s", inet6_ntoa(*ip6_src), iface);
		return IH_ERR_OK;
	}

	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
		if (IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->dynamic_ifaddr[i].ip6)){
			continue;
		}

		memcpy(ip6_src, &ip6_ifaddr->dynamic_ifaddr[i].ip6, sizeof(struct in6_addr));
		LOG_IN("Get dynamic src ipv6 address %s from interface %s", inet6_ntoa(*ip6_src), iface);
		return IH_ERR_OK;
	}

	return IH_ERR_FAILED;
}

/* Get an ipv6 gateway of interface from it's ipv6 address status.
*	Description: 
*	if didnot specify the dst(dst is null), get the first one address that can be used.
*	if specified the dst, find the first address which prefix equal with dst. if find non, 
*get the first address that can be used
*/
int iface_get_ip6_src_addr(char *iface, IPv6_IFADDR *ip6_ifaddr, struct in6_addr *ip6_dst, struct in6_addr *ip6_src)
{
	int i = 0;

	if(!iface || !ip6_ifaddr || !ip6_src){
		return IH_ERR_NULL_PTR; 
	}
	
	/* no ip6_dst, return the first valid address */
	if (!ip6_dst) {
		return iface_get_first_ip6_addr(iface, ip6_ifaddr, ip6_src);
	}

	/* has ip6_dst, return the matched static address */
	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
		if (IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->static_ifaddr[i].ip6)){
			continue;
		}

		if (!in6_prefix_equal(ip6_ifaddr->static_ifaddr[i].ip6, *ip6_dst, ip6_ifaddr->static_ifaddr[i].prefix_len)) {
			continue;
		}

		memcpy(ip6_src, &ip6_ifaddr->static_ifaddr[i].ip6, sizeof(struct in6_addr));
		LOG_IN("Get static src ipv6 address %s from interface %s", inet6_ntoa(*ip6_src), iface);
		return IH_ERR_OK;
	}

	/* has ip6_dst, return the matched dynamic address */
	for(i = 0; i < MAX_IPV6_ADDR_PER_IF; i++){
		if (IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->dynamic_ifaddr[i].ip6)){
			continue;
		}

		if (!in6_prefix_equal(ip6_ifaddr->dynamic_ifaddr[i].ip6, *ip6_dst, ip6_ifaddr->dynamic_ifaddr[i].prefix_len)) {
			continue;
		}

		memcpy(ip6_src, &ip6_ifaddr->dynamic_ifaddr[i].ip6, sizeof(struct in6_addr));
		LOG_IN("Get dynamic src ipv6 address %s from interface %s", inet6_ntoa(*ip6_src), iface);
		return IH_ERR_OK;
	}

	return iface_get_first_ip6_addr(iface, ip6_ifaddr, ip6_src);
}

int get_ip6_src_addr_from_if_info(IF_INFO if_info, struct in6_addr *ip6_dst, struct in6_addr *ip6_src)
{
	VIF_INFO *vif = NULL;

	vif = get_vif_by_if_info(&if_info);
	if(!vif){
		return IH_ERR_FAILED;
	}

	return iface_get_ip6_src_addr(vif->iface, &vif->ip6_addr, ip6_dst, ip6_src);
}
 
//* Get an ipv6 gateway of interface from it's ipv6 address status. */
int iface_get_ip6_gw(char *iface, IPv6_IFADDR *ip6_ifaddr, struct in6_addr *ip6_gw)
{
	int i = 0;

	if(!iface || !ip6_ifaddr || !ip6_gw){
		return IH_ERR_NULL_PTR; 
	}

	for(i = 0; i < CFG_PER_ADDR; i++){
		/* find the default route */
		if (IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->ra_route[i].prefix)
				&& !IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->ra_route[i].gateway)){
		//if(!IN6_IS_ADDR_UNSPECIFIED (&ip6_ifaddr->ra_route[i].gateway)){
			memcpy(ip6_gw, &ip6_ifaddr->ra_route[i].gateway, sizeof(struct in6_addr));
			LOG_IN("Get gateway %s from interface %s", inet6_ntoa(ip6_ifaddr->ra_route[i].gateway), iface);
			return IH_ERR_OK;
		}
	}

	return IH_ERR_FAILED;
}

static char ipv6_addr_str[][16] = {"global", "site", "link", "host"};
int  flush_ipv6_scope_addr(char *iface, IPV6_ADDR_SCOPE scope)
{
    if (!iface){
		return IH_ERR_NULL_PTR;
	}
	LOG_IN("Flush IPv6 address %s interface %s", ipv6_addr_str[scope], iface);
	eval("ip", "-f", "inet6", "addr", "flush", "scope", ipv6_addr_str[scope], iface);
	return IH_ERR_OK;
}
int  del_ipv6_addr(char *iface, struct in6_addr *ip6_addr)
{
    if (!iface){
		return IH_ERR_NULL_PTR;
	}
	LOG_IN("Del IPv6 address %s interface %s", inet6_ntoa(*ip6_addr), iface);
	eval("ip", "-f", "inet6", "addr", "del", (char*)inet6_ntoa(*ip6_addr), "dev", iface);
	return IH_ERR_OK;
}
/**
 * Start odhcp6c client for specified interface
 * @param	name	interface name
 * @param   pid     daemon pid
 * @param   state   odhcp6c STATEFUL/STATELESS
 * @param   mod_name which script will odhcp6c call
 */
int start_odhcp6c(const char *name, int *pid, ODHCP6C_STATE state, ODHCP6C_CALLER_NUM num)
{
	char pidfile[128];
	char script_call[128];
	int ret;
	
//    	iface_down_up((char *)name);
	sleep(1);
	snprintf(pidfile, sizeof(pidfile), "/var/run/%s_odhcp6c.pid", name);
	snprintf(script_call, sizeof(script_call), "/usr/bin/%s", odhcp6c_caller_name[num]);
	LOG_IN("starting odhcp6c state %d for %s...",state , name);
	if(STATEFUL_PD == state){
		ret = start_daemon(pid, "odhcp6c", "-N", "none", "-P", "0", "-v", "-s", script_call, 
					"-p", pidfile, name, NULL);
	}else if(STATEFUL == state){
		ret = start_daemon(pid, "odhcp6c", "-N", "force", "-P", "0","-v", "-s", script_call, 
					"-p", pidfile, name, NULL);
	}else{
		ret = start_daemon(pid, "odhcp6c", "-N", "none", "-v", "-s", script_call, 
					"-p", pidfile, name, NULL);	
	}

	return ret;
}

/**
 * Start odhcp6c client for specified interface
 * @param	name	interface name
 * @param   pid     daemon pid
 * @param   state   odhcp6c STATEFUL/STATELESS
 * @param   mod_name which script will odhcp6c call
 */
int start_odhcp6c2(const char *name, int *pid, ODHCP6C_STATE state, struct in6_addr iapd_prefix, int prefix_len, ODHCP6C_CALLER_NUM num)
{
	char pidfile[128];
	char script_call[128];
	int ret;
	char *retptr = NULL;
	
	char addr[INET6_ADDRSTRLEN] = {0};
	char length[8] = {0};

	snprintf(pidfile, sizeof(pidfile), "/var/run/%s_odhcp6c.pid", name);
	snprintf(script_call, sizeof(script_call), "/usr/bin/%s", odhcp6c_caller_name[num]);
	LOG_IN("starting odhcp6c state %d for %s...",state , name);
	retptr = (char *) inet_ntop(AF_INET6, &iapd_prefix, addr, INET6_ADDRSTRLEN);
	if(STATEFUL_PD == state){
		if (retptr && prefix_len > 0 && prefix_len <= 64) {
			snprintf(length, sizeof(length), "%d", prefix_len);
			ret = start_daemon(pid, "odhcp6c", "-N", "none", "-P", length, "-a", "-f", "-S30","-v", "-A", addr, "-s", script_call, 
					"-p", pidfile, name, NULL);
		}else {
			ret = start_daemon(pid, "odhcp6c", "-N", "none", "-P", "0","-a", "-f", "-v", "-s", script_call, 
					"-p", pidfile, name, NULL);
		}
	}else if(STATEFUL == state){
		ret = start_daemon(pid, "odhcp6c", "-N", "force", "-P", "0","-v", "-s", script_call, 
					"-p", pidfile, name, NULL);
	}else{
		ret = start_daemon(pid, "odhcp6c", "-N", "none", "-v", "-s", script_call, 
					"-p", pidfile, name, NULL);	
	}

	return ret;
}

void linux_ipv6_route_opt(IPv6_ROUTE* route, BOOL opt, char* ifname)
{
	char gateway[IPV6_ADDR_PREFIX_STR_LEN] = {0};
	char metric[8] = {0};

	inet_ntop(AF_INET6, &route->gateway, gateway,sizeof(gateway));
	snprintf(metric, sizeof(metric), "%d", route->metric);
	LOG_IN("ipv6 route %s prefix %s gateway %s metric %s dev %s", opt?"add":"del",
				in6_addr2str(route->prefix, route->prefix_len), gateway, metric, ifname);

	/* gateway maybe linklocal dev is need */
	if(opt){
		eval("route", "-A", "inet6", "add", in6_addr2str(route->prefix, route->prefix_len),
					"gw", gateway, "metric", metric, "dev", ifname);
	}else{
		eval("route", "-A", "inet6", "del", in6_addr2str(route->prefix, route->prefix_len),
					"gw", gateway, "metric", metric, "dev", ifname);
	}
	return;
}

void linux_ipv6_if_route_opt(char *iface, IPv6_IFADDR *ip6_ifaddr, int num_need_set, BOOL opt_flag)
{
	int i = 0;
	if(NULL == ip6_ifaddr || NULL == iface){
		return;
	}
	/* add/del to kernel*/
	for(i=0;i<num_need_set;i++){
		if(!IN6_IS_ADDR_UNSPECIFIED(&ip6_ifaddr->ra_route[i].gateway)){
			linux_ipv6_route_opt(&ip6_ifaddr->ra_route[i], opt_flag, iface);		
		}
	}
	/* notify to route mode */
}
/**
 * Stop odhcp6c client for specified interface
 * @param	name	interface name
 */
int stop_odhcp6c(char *iface, IPv6_IFADDR *ip6_ifaddr, int* pid)
{
	if(NULL == ip6_ifaddr || NULL == pid){
		return 1;
	}
	if (*pid > 0) {
		LOG_DB("stop odhcp6c, pid %d", *pid);
		stop_or_kill(*pid, 2000);
	}
	*pid = -1;
	memset(ip6_ifaddr->dynamic_ifaddr, 0, sizeof(ip6_ifaddr->dynamic_ifaddr));
	linux_ipv6_if_route_opt(iface, ip6_ifaddr, CFG_PER_ADDR, FALSE );
	memset(ip6_ifaddr->ra_route, 0, sizeof(ip6_ifaddr->ra_route));
	memset(ip6_ifaddr->dhcp_dns, 0, sizeof(ip6_ifaddr->dhcp_dns));
	memset(ip6_ifaddr->ra_dns, 0, sizeof(ip6_ifaddr->ra_dns));
	memset(&ip6_ifaddr->ra_route_df_gw, 0, sizeof(ip6_ifaddr->ra_route_df_gw));
	ip6_ifaddr->odhcp6c_enable = FALSE;
	return 0;
}

/**
*	Release odhcp6c client for specified interface
*/
int release_odhcp6c(pid_t pid)
{
	kill(pid, 102);
	return 0;
}

char *pd_role_str(DHCPV6_PD_ROLE pd_role)
{
	if(PD_SOURCE == pd_role){
		return "dhcp-pd";
	}else if(PD_USER == pd_role){
		return "user";
	}else if(PREFIX_SHARE == pd_role){
		return "prefix-share";
	}else if(PD_NONE == pd_role){
		return "none";
	}	

	return "other";
}

int request_set_pd_label(IF_INFO if_info, DHCPV6_PD_ROLE pd_role, char *pd_label, int subnet_position)
{
	PD_LABEL_REQUEST pd_label_req;
	char iface_name[IF_CMD_NAME_SIZE] = {0};
	int ret = -1;

	memset(&pd_label_req, 0, sizeof(pd_label_req));
	memcpy(&pd_label_req.if_info, &if_info, sizeof(pd_label_req.if_info));
	strlcpy(pd_label_req.pd_label, pd_label, sizeof(pd_label_req.pd_label));
	pd_label_req.pd_role = pd_role;
	pd_label_req.subnet_position = subnet_position;

	get_str_from_if_info(&if_info, iface_name);
	LOG_IN("interface %s request to set role %s of prefix label '%s', position %d", iface_name, pd_role_str(pd_role), pd_label, subnet_position);

	ret = ih_ipc_send_no_ack(IH_SVC_AGENT, IPC_MSG_AGENT_SET_PD_LABEL, (char *)&pd_label_req, sizeof(pd_label_req));
	if(ret) {
		LOG_IN("send set pd label request to service %s(%d) error: %d.", get_name_by_svcid(IH_SVC_AGENT), IH_SVC_AGENT, ret);
		return ret;
	}

	return 0;
}

static int npd6_build_config(char *prefix, char *wan)
{
	FILE *fp;

	if(!prefix || !wan) {
		LOG_ER("bad parameter!");
		return -1;
	}

	fp = fopen(NPD6_CONF_PATH, "w+");
	if (NULL == fp) {
		LOG_ER("cannot open the npd6 configuration file!, errno:%d", errno);
		return -1;
	}
		
	fprintf(fp, "prefix=%s\n", prefix);
	fprintf(fp, "interface = %s\n", wan);
	fprintf(fp, "ralogging = off\n");
	fprintf(fp, "listtype = none\n");
	fprintf(fp, "listlogging = off\n");
	fprintf(fp, "collectTargets = 100\n");
	fprintf(fp, "linkOption = false\n");
	fprintf(fp, "ignoreLocal = true\n");
	fprintf(fp, "routerNA = true\n");
	fprintf(fp, "maxHops = 255\n");
	fprintf(fp, "pollErrorLimit = 20\n");

	fclose(fp);
	return 0;
}

int start_npd6(int *pid, char *prefix, char *wan, BOOL debug)
{	
	int ret;

	if (*pid > 0) {
		stop_npd6(pid);
	}

	ret = npd6_build_config(prefix, wan);

	if (ret) {
		LOG_ER("cannot build configuration file!");
		return ret;
	}

	if(debug) {
		ret = start_daemon(pid, "npd6", "-D", NULL);
	}else {
		ret = start_daemon(pid, "npd6", NULL);
	}
	
	return ret;
}

int stop_npd6(int* pid)
{
	if (*pid > 0) {
		LOG_DB("stop npd6, pid %d", *pid);
		stop_or_kill(*pid, 2000);
	}

	*pid = -1;
	return 0;
}

