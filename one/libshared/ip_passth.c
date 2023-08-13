/************************************************************
Copyright (C), 2020, Wh, All Rights Reserved.
FileName: ip_passth.c
Description: for ip passthrough public function
Author: Wh
Version: 1.0
Date: 
Function:

History:
<author>    <time>  <version>   <description>
 Wh
************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "ih_cmd.h"
#include "sw_ipc.h"
#include "vif_shared.h"
#include "ippassth_ipc.h"

#define USED_IP(ip)	(((ip)->s_addr)?(TRUE):(FALSE))

extern int netmask_aton(const char* netmask);

int get_ip_passthrough_lan_ip(IP_PASSADDRINFO *addrinfo, struct in_addr *ip, struct in_addr *netmask)
{
	struct in_addr in_ip, in_netmask;
	unsigned int lan_ip, lan_netmask, gw_ip, wan_ip;

	gw_ip = ntohl(addrinfo->gateway.s_addr);
	wan_ip = ntohl(addrinfo->ip.s_addr);

	lan_netmask = 0xFFFF0000;
    in_netmask.s_addr = htonl(lan_netmask);
    lan_ip = ntohl(addrinfo->ip.s_addr & in_netmask.s_addr) + 1;

    while (lan_ip == gw_ip || lan_ip == wan_ip) {
		lan_ip++;
    }

    in_ip.s_addr = htonl(lan_ip);
 
 	memcpy(ip, &(in_ip), sizeof(struct in_addr));
 	memcpy(netmask, &(in_netmask), sizeof(struct in_addr));  	

    return 0;
}

//The addrinfo must be wan current info
int change_ip_passthrough_wan_defroute(IP_PASSADDRINFO *addrinfo, char *iface)
{
	char gateway[64] = {0};
	char netmask[64] = {0};
	char buf[64] = {0};
	struct in_addr in_ip, in_netmask, subnet;
	int ippassth_mask;

	//del default route
	eval("ip", "route", "del", "default");

	snprintf(gateway, sizeof(gateway), "%s", inet_ntoa(addrinfo->gateway));

	if(strncmp(iface, "ppp", 3)){
		//add static route to the gw, add default route via the gw
		eval("ip", "route", "add", gateway, "dev", iface);
	}

	/*
	* default lan mask is 255.255.0.0
	* split the lan subnet to 2 route of 255.255.128.0 via wan
	* such as 10.122.0/17 and 10.122.128.0/17
	* */
	get_ip_passthrough_lan_ip(addrinfo, &in_ip, &in_netmask);

	subnet.s_addr = in_ip.s_addr&in_netmask.s_addr;
	strlcpy(netmask, inet_ntoa(in_netmask), sizeof(netmask));
	ippassth_mask = netmask_aton(netmask);

	snprintf(buf, sizeof(buf), "%s/%d",inet_ntoa(subnet), ippassth_mask+1);
	LOG_IN("ip passthrough subnet %s", buf);
	eval("ip", "route", "add", buf, "via", gateway, "dev", iface);

	subnet.s_addr = htonl(ntohl(subnet.s_addr) | (0x1 << (32 - (ippassth_mask+1))));
	snprintf(buf, sizeof(buf), "%s/%d",inet_ntoa(subnet), ippassth_mask+1);
	LOG_IN("ip passthrough subnet %s", buf);
	eval("ip", "route", "add", buf, "via", gateway, "dev", iface);

	//add default route
	eval("ip", "route", "add", "default", "dev", iface, "via", gateway);

	return 0;
}

//The addrinfo must be wan current info
int get_ip_passthrough_gateway_arpinfo(IP_PASSADDRINFO *addrinfo, char *iface, char *arpinfo_ip, char *arpinfo_mac)
{
	char gateway[64] = {0};
	char buf[256] = {0};
	FILE *f = NULL;
	char ip[64] = {0};
    char mac[64] = {0};
    char dev[64] = {0};
    unsigned int flags;
    int ret = -1;
	char cmdbuff[64] = {0};
	snprintf(gateway, sizeof(gateway), "%s", inet_ntoa(addrinfo->gateway));

	if (strncmp(iface, "ppp", 3)) {
		sprintf(cmdbuff,"echo 1 > /proc/sys/net/ipv4/conf/%s/arp_accept",iface);
		system(cmdbuff);
	 	eval("arping", "-q", "-c", "3", "-I", iface, gateway);
		 /*get arp info of the gw
		 *         cat /proc/net/arp
		 *         IP address       HW type     Flags       HW address            Mask     Device
		 *         192.168.0.1      0x1         0x2         00:01:02:03:04:05     *        vlan1
		 */
		if ((f = fopen("/proc/net/arp", "r")) != NULL) {
		    while (fgets(buf, sizeof(buf), f)) {
			if (sscanf(buf, "%15s %*s 0x%X %17s %*s %16s", ip, &flags, mac, dev) != 4) continue;
			if ((strlen(mac) != 17) || (strcmp(mac, "00:00:00:00:00:00") == 0)) continue;
			if (flags == 0) continue;
			if (strcmp(ip, gateway)) {
			    continue;
			}

			LOG_IN("get gateway mac %s --- %s", gateway, mac);
			strlcpy(arpinfo_ip, gateway, sizeof(gateway));
			strlcpy(arpinfo_mac, mac, sizeof(mac));
			ret = 0;
			break;
		    }

			fclose(f);
		}
		memset(cmdbuff, 0x0, sizeof(cmdbuff));	
		sprintf(cmdbuff,"echo 0 > /proc/sys/net/ipv4/conf/%s/arp_accept",iface);
		system(cmdbuff);
	}
	return ret;
}

//The addrinfo must be wan current info
int set_ip_passthrough_wan_arpproxy(char *iface, char *ip, char *mac)
{
	char buf[256] = {0};
	
	snprintf(buf, sizeof(buf), "echo 1 > /proc/sys/net/ipv4/conf/%s/proxy_arp", iface);
	system(buf);

	//set static arp of gateway
	eval("arp", "-d", ip);
	eval("arp", "-i", iface, "-s", ip, mac);

	return 0;
}

int set_ip_passthrough_lan_arpproxy(char *iface)
{
	char buf[256] = {0};

	//proxy the arp request on lan interface, for the new lan subnet
    snprintf(buf, sizeof(buf), "echo 1 > /proc/sys/net/ipv4/conf/%s/proxy_arp", iface);
    system(buf);

	return 0;
}

/*****************************************************************************
 函 数 名  : check_ipaddr_legal
 功能描述  : check the ip address value
 输入参数  : 
 			ipaddr : Pointer of 'struct in_addr'  
 输出参数  : 
 返 回 值  : return TRUR is legal value, FALSE is illegal value
 
 修改历史      :
  1.日    期   : 2020-7-21 11:30
    作    者   : wh
    修改内容   : 新生成函数

*****************************************************************************/
BOOL check_ipaddr_legal(struct in_addr *ipaddr)
{
	if(USED_IP(ipaddr))
		return TRUE;
	return FALSE;
}

/*****************************************************************************
 函 数 名  : set_ip_passthrough_fw
 功能描述  : set the iptables rules for ip passthrough
 输入参数  : 
 			iface: Pointer of 'wan' interface
 			wanip: Pointer of 'wan' ip address
 			handle: Pointer of rules setting, add or del  
 输出参数  : 
 返 回 值  : 0
 
 修改历史      :
  1.日    期   :2020-7-22 16:30
    作    者   : wh
    修改内容   : 新生成函数

*****************************************************************************/
int set_ip_passthrough_fw(char *iface, char *wanip, char *handle)
{
	if(handle == NULL){
		LOG_IN("invalid handle for setting ip passthrough firewall");
	}

	/*IPPT should block ICMP unreachable packets
	 * cause:
	 * 		ICMP unreachable packets can't be NATed by IPPT SNAT policy, so these packets was sent out without source ip changed.
	 * 		Verizon have heavy security check, dialing would be kicked after received a few packets with wrong source ip.
	 */
	LOG_IN("ip passthrough %s icmp unreachable packets", handle);
	eval("iptables", "-t", "filter", handle, "OUTPUT-IPPASSTH", "-o", iface, "-p", "icmp", "--icmp-type", "0", "-j", "ACCEPT");
	eval("iptables", "-t", "filter", handle, "OUTPUT-IPPASSTH", "-o", iface, "-p", "icmp", "--icmp-type", "8", "-j", "ACCEPT");
	eval("iptables", "-t", "filter", handle, "OUTPUT-IPPASSTH", "-o", iface, "-p", "icmp", "-j", "DROP");

	if(iface != NULL && wanip != NULL){
		eval("iptables", "-t", "nat", "-F", "POST-IPPASSTH");
		LOG_IN("ip passthrough set wan firewall");
		eval("iptables", "-t", "nat", handle, "POST-IPPASSTH", "-o", iface, "-j", "SNAT", "--to", wanip);
	}

	return 0;
}

/*****************************************************************************
 函 数 名  : handle_ip_passthrough_wanset
 功能描述  : send the current 'wan' info to 'lan' and get the current 'lan' 
 info set for 'wan' 
 输入参数  : 
          waninfo:Poniter of current 'wan' info, must be fill before call this function
          iface:Poniter of 'wan' interface  
 输出参数  : 
 返 回 值  : return '1' is successed, other vaules are failed
 
 修改历史      :
  1.日    期   : 2020-7-27 9:20
    作    者   : wh
    修改内容   : 新生成函数

*****************************************************************************/
int handle_ip_passthrough_wanset(IP_PASSADDRINFO *waninfo, char *iface)
{
	IPC_MSG *rsp = NULL;
    int ret = -1;
    char wan_ip[64] = {0};
    char arpip[64] = {0};
    char arpmac[64] = {0};
    char ip[64] = {0};
    char netmask[64] = {0};
    IP_PASSADDRINFO addrif;
    IP_PASSADDRINFO *laninfo = &addrif;

    LOG_IN("Handle the IP Passthrough WAN Setting...........");

   /*inet_ntoa non reentrant，so ip and netmask、gateway print in three times*/
    LOG_IN("g_waninfo: ip-%s", inet_ntoa(waninfo->ip) );
    LOG_IN("g_waninfo: netmask-%s", inet_ntoa(waninfo->netmask) );
    LOG_IN("g_waninfo: gateway-%s", inet_ntoa(waninfo->gateway) );

    /*set firewall rule*/
    memset(wan_ip, 0x0, sizeof(wan_ip));
    snprintf(wan_ip, sizeof(wan_ip), "%s", inet_ntoa(waninfo->ip));
    set_ip_passthrough_fw(iface, wan_ip, "-A");
    clear_conntrack();

   /*send the current waninfo to lan service before set ip and wait reply*/
    ret = ih_ipc_send_wait_rsp(IH_SVC_IF, 5000, IPC_MSG_IP_PASSTHROUGH_WANINFO, (char *)waninfo, sizeof(IP_PASSADDRINFO), &rsp );
    if(0 == ret){
        //dump_msg_hdr(&(rsp->hdr));
        memset(laninfo, 0x0, sizeof(IP_PASSADDRINFO));
        memcpy(laninfo, (IP_PASSADDRINFO *)(rsp->body), sizeof(IP_PASSADDRINFO));

        if(check_ipaddr_legal(&(laninfo->ip))){
            LOG_IN("Get the lan addrinfo from service %d ", rsp->hdr.svc_id);
          
            /*get arp info for gateway before wan addr changed*/
            get_ip_passthrough_gateway_arpinfo(waninfo, iface, arpip, arpmac);
          	/*set the wan ip addr*/
            flush_ip_addr(iface);
            snprintf(ip, sizeof(ip), "%s", inet_ntoa(laninfo->ip));
            snprintf(netmask, sizeof(netmask), "%s", "255.255.255.255");// wan 32bit,lan 24 bit route, in order to avoid redial disconnect;
            add_ip_addr(iface, ip, netmask);
            LOG_IN("Change WAN ip to  %s/%s ", ip, netmask);

          	/*change route for wan*/
            change_ip_passthrough_wan_defroute(waninfo, iface);

          	/*set arp proxy for wan*/
            set_ip_passthrough_wan_arpproxy(iface, arpip, arpmac);

          	/*send the current waninfo to dhcp server*/
            ih_ipc_send_no_ack(IH_SVC_AGENT, IPC_MSG_IP_PASSTHROUGH_WANINFO, (char *)waninfo, sizeof(IP_PASSADDRINFO));

            ret = 1;
            ih_ipc_free_msg(rsp);
		    rsp = NULL;
        }
    }

	return ret;
}
