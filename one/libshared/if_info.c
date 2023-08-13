/*
 * $Id$ --
 *
 *   IF_INFO functions: struct IF_INFO describes all interfaces
 *		 including physical and virtual interface.
 *
 * Copyright (c) 2001-2012 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 11/16/2012
 * Author: Zhengyb
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "if_common.h"

const TYPE_NAME_PAIR g_if_type_name[] = {
        {IF_TYPE_FE, "fastethernet"},
        {IF_TYPE_GE, "gigabitethernet"},
        {IF_TYPE_CELLULAR, "cellular"},
        {IF_TYPE_TUNNEL, "tunnel"},
        {IF_TYPE_VXLAN, "vxlan"},
//      {IF_TYPE_VT, "virtual-template"},
        {IF_TYPE_SVI, "vlan"},
        {IF_TYPE_LO, "loopback"},
        {IF_TYPE_DIALER, "dialer"},
        {IF_TYPE_SUB_FE, "fastethernet"},
        {IF_TYPE_SUB_GE, "gigabitethernet"},
        {IF_TYPE_SUB_CELLULAR, "cellular"},
        {IF_TYPE_VP, "virtual-ppp"},
        {IF_TYPE_OPENVPN, "openvpn"},
        {IF_TYPE_DOT11, "dot11radio"},
        {IF_TYPE_SUB_DOT11, "dot11radio"},
        {IF_TYPE_BRIDGE, "bridge"},
        {IF_TYPE_CAN, "can"},
        {IF_TYPE_VE, "virtual-eth"},
		{IF_TYPE_STA_DOT11, "wlan-sta"}
};

const TYPE_NAME_PAIR g_if_type_name_abbr[] = {
        {IF_TYPE_FE, "FE"},
        {IF_TYPE_GE, "GE"},
        {IF_TYPE_SUB_FE, "FE"},
        {IF_TYPE_SUB_GE, "GE"}
};

const IF_TYPE_MIRROR if_type_mirror[] = {
    {IF_TYPE_LAN,        "lan",      "iana-if-type:ethernetCsmacd"},
    {IF_TYPE_WAN,        "wan",      "iana-if-type:l3ipvlan"},
    {IF_TYPE_CELLULAR,   "cellular", "iana-if-type:modem"},
    {IF_TYPE_TUNNEL,     "tunnel",   "iana-if-type:tunnel"},
    // {IF_TYPE_VLAN,       "vlan",     "iana-if-type:l2vlan"},
    {IF_TYPE_SVI,        "vlan",     "iana-if-type:l3ipvlan"},
    {IF_TYPE_LO,         "loopback", "iana-if-type:softwareLoopback"},
    {IF_TYPE_DIALER,     "dialer",   "iana-if-type:adsl"},
    {IF_TYPE_VP,         "l2tp",     "iana-if-type:ppp"},
    {IF_TYPE_OPENVPN,     "openvpn",    "iana-if-type:ovpn"},
    {IF_TYPE_STA_DOT11,  "wlan-sta",     "iana-if-type:ieee80211"},
    {IF_TYPE_DOT11,      "wlan",     "iana-if-type:ieee80211"},
    {IF_TYPE_SUB_DOT11,  "wlan",     "iana-if-type:ieee80211"},
    {IF_TYPE_BRIDGE,     "bridge",   "iana-if-type:bridge"},
    {IF_TYPE_VXLAN,      "vxlan",    "iana-if-type:vxlan"},
    //{IF_TYPE_VE,         "l2tp-eth", ""}
};

char *get_yang_if_type_by_if_info(IF_INFO *if_info)
{
    IF_TYPE type;
    const IF_TYPE_MIRROR *p = NULL;

    if (!if_info) {
        return NULL;
    }

    type = if_info->type;

    if (type <= IF_TYPE_NONE || type >= IF_TYPE_INVALID) {
        return NULL;
    }

    if (type == IF_TYPE_SVI && (IF_WAN_SVI_ID_BASE <= if_info->port && if_info->port <= IF_WAN_SVI_ID_MAX)) {
        type = IF_TYPE_WAN;
    }

    if((type == IF_TYPE_FE) || (type == IF_TYPE_GE)){
		if (ih_license_support(IH_FEATURE_ER3)) {
			if (if_info->port == IF_WAN_ETH_ID) {
				type = IF_TYPE_WAN;
			} else {
				type = IF_TYPE_LAN;
			}
		} else {
			type = IF_TYPE_LAN;
		}
    }

    for (p = if_type_mirror; p->ih_if_type < IF_TYPE_INVALID; ++p) {
        if (p->ih_if_type == type) {
            return p->yang_if_type;
        }
    }

    return NULL;   

}

/*
 * return interface type by panel name
 * FixME: a string may be map to servel types, like "wan"
 *
 * @param str       interface type name
 *
 * @return interface type
 */
IF_TYPE get_if_type_by_panel_name(char *str, char **type_name)
{
    uns32 i = 0;
    char * p, *q;
    IF_TYPE type = IF_TYPE_NONE;

    if(!str)    return IF_TYPE_NONE;

    p = str;
    SKIP_WHITE(p);
    q = strchr(p, ' ');
    if (q)  *q = '\0';

    for(i = 0; i < ARRAY_COUNT(if_type_mirror); i++){
        /* cmd string may be not a complete interface type name */
        if(!strncmp(p, if_type_mirror[i].panel_name, strlen(if_type_mirror[i].panel_name))){
            type = if_type_mirror[i].ih_if_type;
            if (type_name) {
                *type_name = if_type_mirror[i].panel_name;
            }
            break;
        }
    }

    if (q)  *q = ' ';

    return type;
}

/*
 * return interface type name
 *
 * @param type      interface type
 *
 * @return interface type name
 */
char *get_panel_type_name_by_type(IF_TYPE type)
{
    uns32 i = 0;

    if((type == IF_TYPE_FE) || (type == IF_TYPE_GE)){
	type = IF_TYPE_LAN;
    }

    for(i = 0; i < ARRAY_COUNT(if_type_mirror); i++){
        if(type == if_type_mirror[i].ih_if_type){
            return if_type_mirror[i].panel_name;
        }
    }

    return NULL;
}

/*
 * get interface information from interface string
 * Note: input string will not be changed.
 *
 * @param str       interface string(such as 'wan1, cellular1, vlan1, bridge1')
 *  note: there may be no space between type name and interface id 
 * @param if_info       (out)IF_INFO structure
 *
 * @return IH_ERR_OK for ok, others for error
 */
//int32 get_interface_info_from_str(char * str, IF_INFO *if_info)
int32 get_if_info_from_panel_name(const char * str, IF_INFO *if_info)
{
    char *p, *q, *s, *b;
    char *type_name = NULL;
	char iface[64] = {0};

    if(!str || !if_info){
        return IH_ERR_NULL_PTR;
    }

    memset(if_info, 0, sizeof(IF_INFO));

	strlcpy(iface, str, sizeof(iface));

    if_info->type = get_if_type_by_panel_name(iface, &type_name);
    if(!if_info->type){
        LOG_ER("invalid interface type");
        return IH_ERR_INVALID;
    }
    //LOG_DB("interface type is [%d:%s]", if_info->type, type_name);
    q = iface + strlen(type_name);
    //q = strchr(str, ' ');
    if (q[0] == ' ') {
        SKIP_WHITE(q);
    }
    if (q[0] == '-') {
		q++;
        SKIP_WHITE(q);
    }
    b = strchr(q, ' ');
    if(b)	*b = '\0';
    p = strchr(iface, '/');
    s = strchr(iface, '.');

    switch(if_info->type) {
        case IF_TYPE_CELLULAR:
			if (ih_license_support(IH_FEATURE_MODEM_NONE)) {
				return IH_ERR_INVALID;
			}

            if_info->slot = 0;
            if_info->port = atoi(q);

			if (if_info->port <=0) {
				return IH_ERR_INVALID;
			}

            if (s){
                if_info->sid = atoi(s+1);
                if_info->type = IF_TYPE_SUB_CELLULAR;
            }
            break;
        case IF_TYPE_STA_DOT11:
            if (!ih_license_support(IH_FEATURE_WLAN_AP) && !ih_license_support(IH_FEATURE_WLAN_STA)) {
                return IH_ERR_INVALID;
			}
            if_info->slot = 0;
            if_info->port = VIF_DOT11_STA_PORT;
            break;
        case IF_TYPE_DOT11:
			if (!ih_license_support(IH_FEATURE_WLAN_AP) && !ih_license_support(IH_FEATURE_WLAN_STA)) {
				return IH_ERR_INVALID;
			}

        case IF_TYPE_TUNNEL:
        case IF_TYPE_VXLAN:
        case IF_TYPE_SVI:
        case IF_TYPE_LO:
        case IF_TYPE_VT:
        case IF_TYPE_DIALER:
        case IF_TYPE_VP:
        case IF_TYPE_VE:
        case IF_TYPE_OPENVPN:
            if_info->slot = 0;
            if_info->port = atoi(q);
            if (s){
                if_info->sid = atoi(s+1);
                if_info->type = IF_TYPE_SUB_DOT11;
            }
        case IF_TYPE_BRIDGE:
            if_info->slot = 0;
            if(if_info->type == IF_TYPE_OPENVPN && strcmp(q, "server") == 0){
                if_info->port = OPENVPN_SERVER_PORT; //openvpn server
			} else if(if_info->type == IF_TYPE_OPENVPN && strcmp(q, "cloud") == 0){
                if_info->port = OPENVPN_CLOUD_PORT; //openvpn could interface
            } else {
                if_info->port = atoi(q);
            }
            break;
        case IF_TYPE_FE:
        case IF_TYPE_GE:
        //case IF_TYPE_LAN:
            if_info->slot = atoi(q);
            if_info->port = atoi(p+1);
            if (s){
                if_info->sid = atoi(s+1);
                if_info->type = (if_info->type == IF_TYPE_FE)?IF_TYPE_SUB_FE:IF_TYPE_SUB_GE;
            }
            break;
        case IF_TYPE_CAN:
            if_info->slot = 0;
            if_info->port = atoi(q);
            break;
        case IF_TYPE_LAN:
            if_info->type = IF_TYPE_GE;
            if_info->port = atoi(q);
			if (ih_license_support(IH_FEATURE_ER3)) {
				if_info->port = IF_LAN_ETH_ID;
			} else {
				if_info->slot = 1;
			}
            break;
        case IF_TYPE_WAN:
			if (ih_license_support(IH_FEATURE_ER3)) {
				if (atoi(q) != 1) {	//ER3 only support wan1
					return IH_ERR_INVALID;
				}

				if_info->type = IF_TYPE_GE;
				if_info->port = IF_WAN_ETH_ID;
				if_info->slot = 0;
			} else {
            if_info->type = IF_TYPE_SVI;
            if_info->port = IF_WAN_SVI_ID_BASE + atoi(q) - 1;
            if_info->slot = 0;
			}
            break;
        default:
            return IH_ERR_INVALID;
            break;
    }

    if(b)	*b = ' ';
    return IH_ERR_OK;
}

/*
 * get interface string  from IF_INFO *
 *
 * @param if_info       [in]            IF_INFO structure
 * @param str           [out]           interface string(such as 'wan1', 'vlan1'...)
 *
 * @return IH_ERR_OK for ok, others for error
 */
int32 get_panel_name_from_if_info(IF_INFO *if_info, char * str)
{
    char *type;

    if(!str || !if_info){
        return IH_ERR_NULL_PTR;
    }

    type = get_panel_type_name_by_type(if_info->type);

	if (ih_license_support(IH_FEATURE_ER3)) {
		if (if_info->type == IF_TYPE_GE && if_info->port == IF_WAN_ETH_ID) {
			type = get_panel_type_name_by_type(IF_TYPE_WAN);
		}
	}

    if (!type) return IH_ERR_FAILED;

    switch(if_info->type) {
        case IF_TYPE_CELLULAR:
        case IF_TYPE_TUNNEL:
        case IF_TYPE_VXLAN:
        case IF_TYPE_SVI:
        case IF_TYPE_VT:
        case IF_TYPE_DIALER:
        case IF_TYPE_VP:
        case IF_TYPE_VE:
        case IF_TYPE_OPENVPN:
        case IF_TYPE_LO:
            snprintf(str, IF_CMD_NAME_SIZE, "%s%d", type, if_info->port);

            if (if_info->type == IF_TYPE_SVI && (IF_WAN_SVI_ID_BASE <= if_info->port && if_info->port <= IF_WAN_SVI_ID_MAX)) {
                snprintf(str, IF_CMD_NAME_SIZE, "wan%d", (if_info->port - IF_WAN_SVI_ID_BASE + 1));
            }
            if(if_info->type == IF_TYPE_OPENVPN && if_info->port == OPENVPN_CLOUD_PORT){	//Note: the connector's openvpn client config always use id 11 
                snprintf(str, IF_CMD_NAME_SIZE, "%s-%s", type, "cloud");
            }
            if(if_info->type == IF_TYPE_OPENVPN && if_info->port > OPENVPN_CLOUD_PORT){
                snprintf(str, IF_CMD_NAME_SIZE, "%s-%s", type, "server");
            }
            if(if_info->type == IF_TYPE_VP && if_info->port > 20){
                snprintf(str, IF_CMD_NAME_SIZE, "%s-%s", type, "server");
            }
            break;
        case IF_TYPE_FE:
        case IF_TYPE_GE:
			if (ih_license_support(IH_FEATURE_ER3)) {
				if (if_info->port == IF_WAN_ETH_ID) {
					snprintf(str, IF_CMD_NAME_SIZE, "%s%d", type, if_info->port - 1);
				} else {
					snprintf(str, IF_CMD_NAME_SIZE, "%s%d", type, if_info->port);
				}
			} else {
				snprintf(str, IF_CMD_NAME_SIZE, "%s%d", type, if_info->port);
			}
            break;
        case IF_TYPE_SUB_FE:
        case IF_TYPE_SUB_GE:
            snprintf(str, IF_CMD_NAME_SIZE, "%s%d/%d.%d", type, if_info->slot, if_info->port, if_info->sid);
            break;
        case IF_TYPE_STA_DOT11:
            snprintf(str, IF_CMD_NAME_SIZE, "%s", type);
            break;
        case IF_TYPE_SUB_CELLULAR:
        case IF_TYPE_SUB_DOT11:
            snprintf(str, IF_CMD_NAME_SIZE, "%s%d.%d", type, if_info->port, if_info->sid);
            break;
        default:
            snprintf(str, IF_CMD_NAME_SIZE, "%s%d", type, if_info->port);
            break;
    }
    LOG_DB("get_panel_name_from_if_info ifinfo(%s-%d), str:%s", type, if_info->port, str);
    return IH_ERR_OK;
}

/*
 * return #port# interface type abbr name
 *
 * @param type      interface type
 *
 * @return interface type name
 */
char *get_if_name_abbr_by_type(IF_TYPE type)
{
        uns32 i = 0;

        for(i = 0; i < ARRAY_COUNT(g_if_type_name_abbr); i++){
        if(type == g_if_type_name_abbr[i].type){
                return g_if_type_name_abbr[i].name;
        }
        }

        return NULL;
}

/*
 * return interface type name
 *
 * @param type      interface type
 *
 * @return interface type name
 */
char *get_if_name_by_type(IF_TYPE type)
{
        uns32 i = 0;

        for(i = 0; i < ARRAY_COUNT(g_if_type_name); i++){
        if(type == g_if_type_name[i].type){
                return g_if_type_name[i].name;
        }
        }

        return NULL;
}


/*
 * return interface type by cmd string
 * FixME: a string may be map to servel types, like "fastethernet"
 *
 * @param str       interface type name
 *
 * @return interface type
 */
IF_TYPE get_if_type_by_name(char *str)
{
        uns32 i = 0;
        char * p, *q;
        IF_TYPE type = IF_TYPE_NONE;

        if(!str)    return IF_TYPE_NONE;

        p = str;
        SKIP_WHITE(p);
        q = strchr(p, ' ');
        if (q)  *q = '\0';

        for(i = 0; i < ARRAY_COUNT(g_if_type_name); i++){
		/* cmd string may be not a complete interface type name */
                if(!strncmp(p, g_if_type_name[i].name,
                                (strlen(p) < strlen(g_if_type_name[i].name))
                                        ?(strlen(p)):(strlen(g_if_type_name[i].name)))){
                        type = g_if_type_name[i].type;
                        break;
                }
        }

        if (q)  *q = ' ';

        return type;
}

/*
 * get interface information from interface string
 * Note: input string will not be changed.
 *
 * @param str       interface string(such as 'fastethernet 1/8')
 * @param if_info       (out)IF_INFO structure
 *
 * @return IH_ERR_OK for ok, others for error
 */
//int32 get_interface_info_from_str(char * str, IF_INFO *if_info)
int32 get_if_info_from_str2(char * str, IF_INFO *if_info)
{
	char *p, *q, *s, *b;

	if(!str || !if_info){
			return IH_ERR_NULL_PTR;
	}
	
	memset(if_info, 0, sizeof(IF_INFO));

	if_info->type = get_if_type_by_name(str);
	if(!if_info->type){
			//LOG_ER("invalid interface type");
			return IH_ERR_INVALID;
	}
	q = strchr(str, ' ');
	SKIP_WHITE(q);
	b = strchr(q, ' ');
	if(b)	*b = '\0';
	p = strchr(str, '/');
	s = strchr(str, '.');

	switch(if_info->type) {
		case IF_TYPE_CELLULAR:
			if_info->slot = 0;
			if_info->port = atoi(q);
			if (s){
				if_info->sid = atoi(s+1);
				if_info->type = IF_TYPE_SUB_CELLULAR;
			}
			break;
		case IF_TYPE_TUNNEL:
        case IF_TYPE_VXLAN:
		case IF_TYPE_SVI:
		case IF_TYPE_LO:
		case IF_TYPE_VT:
		case IF_TYPE_DIALER:
		case IF_TYPE_VP:
		case IF_TYPE_VE:
		case IF_TYPE_OPENVPN:
		case IF_TYPE_DOT11:
			if_info->slot = 0;
			if_info->port = atoi(q);
			if (s){
				if_info->sid = atoi(s+1);
				if_info->type = IF_TYPE_SUB_DOT11;
			}
		case IF_TYPE_BRIDGE:
			if_info->slot = 0;
            if(if_info->type == IF_TYPE_OPENVPN && strcmp(q, "server") == 0){
                if_info->port = OPENVPN_SERVER_PORT; //openvpn server
            } else {
                if_info->port = atoi(q);
            }
			break;
		case IF_TYPE_FE:
		case IF_TYPE_GE:
			if_info->slot = atoi(q);
			if_info->port = atoi(p+1);
			if (s){
				if_info->sid = atoi(s+1);
				if_info->type = (if_info->type == IF_TYPE_FE)?IF_TYPE_SUB_FE:IF_TYPE_SUB_GE;
			}
			break;
		case IF_TYPE_CAN:
			if_info->slot = 0;
			if_info->port = atoi(q);
			break;
		default:
			return IH_ERR_INVALID;
			break;
		}

#if ((defined INHAND_IR9 && !(defined INHAND_VG9)) || defined INHAND_IG9)
	if (if_info->type == IF_TYPE_DOT11 && if_info->port == 2){
		if_info->port = 1;
		if_info->sid = 1;
		if_info->type = IF_TYPE_SUB_DOT11;
	}
#endif

	if(b)	*b = ' ';
      return IH_ERR_OK;
}

/*
 * Get IF_INFO information from CMD string. This function is called by services except CLI.
 * This func will not check the value of ARGs, so validating of ARGs must be done in CLI.
 *
 *  Note: string modification will be done(by calling cmdsep).
 *  interface string:
 *      <type> [<slot/>]<port>[.<sub-id>] 
 *
 * @param pcmd      part of CMD string(such as 'vlan 1 xxx zzz')
 * @param if_info       (out)IF_INFO structure
 *
 * @return IH_ERR_OK for ok, others for error
 */
//int32 get_iface_info_from_str(char **pcmd, IF_INFO *if_info)
int32 get_if_info_from_str1(char **pcmd, IF_INFO *if_info)
{
        char *p, *s;
        char *ptmp = NULL;
        char *value_str = NULL;

        if(!pcmd || !(*pcmd)|| !if_info){
                return IH_ERR_NULL_PTR;
        }

	memset(if_info, 0, sizeof(IF_INFO));

        value_str = cmdsep(pcmd); /* interface type */

        if_info->type = get_if_type_by_name(value_str);
        if(!if_info->type)
                return IH_ERR_NOT_FOUND;

        value_str = cmdsep(pcmd); /* [<slot>/]<port>[.<sub-id>] */
        if (!value_str || !*value_str)
                return IH_ERR_NOT_FOUND;

        ptmp = value_str;
        s = strchr(value_str, '.');
        p = strchr(value_str, '/');
        if (!p){
                if_info->slot = 0;
        }else{
                *p = '\0';
                if_info->slot = atoi(ptmp);
                *p = '/';
                ptmp = p + 1;;
        }
        if (s)
                *s = '\0';
        if(if_info->type == IF_TYPE_OPENVPN && strcmp(ptmp, "server") == 0){
            if_info->port = 11; //openvpn server
        } else {
            if_info->port = atoi(ptmp);
        }
	if (ih_license_support(IH_FEATURE_ETH2_OMAP)){
		if(ih_license_support(IH_FEATURE_WLAN_MTK)){
			if(if_info->type == IF_TYPE_FE || if_info->type == IF_TYPE_GE 
                        || if_info->type == IF_TYPE_TE || if_info->type == IF_TYPE_SUB_FE 
                        || if_info->type == IF_TYPE_SUB_GE) { 
                if(!if_info->slot && if_info->port>1) 
                    return IH_ERR_NOT_FOUND;
            }
		}else if (ih_license_support(IH_FEATURE_SINGLE_ETH)){
                if(if_info->port>1) {
                    return IH_ERR_NOT_FOUND;
				}
		}
	}
        if (s){
                *s = '.';
                ptmp = s + 1;
                if_info->sid = atoi(ptmp);
                switch(if_info->type){
                case IF_TYPE_FE:
                        if_info->type = IF_TYPE_SUB_FE;
                        break;
                case IF_TYPE_GE:
                        if_info->type = IF_TYPE_SUB_GE;
                        break;
				case IF_TYPE_DOT11:
                        if_info->type = IF_TYPE_SUB_DOT11;
                        break;
                case IF_TYPE_CELLULAR:
                        if_info->type = IF_TYPE_SUB_CELLULAR;
                        break;
                /*TODO: add new sub interface*/
                default:
                        break;
                }
        }else{
                if_info->sid = 0;
        }

#if ((defined INHAND_IR9 && !(defined INHAND_VG9)) || defined INHAND_IG9)
	if (if_info->type == IF_TYPE_DOT11 && if_info->port == 2){
		if_info->port = 1;
		if_info->sid = 1;
		if_info->type = IF_TYPE_SUB_DOT11;
	}
#endif

        return IH_ERR_OK;
}



/*
 * get interface string  from IF_INFO *
 *
 * @param if_info       [in]            IF_INFO structure
 * @param str           [out]           interface string(such as 'fastethernet 1/8', 'vlan 1')
 *
 * @return IH_ERR_OK for ok, others for error
 */
//int32 get_str_from_interface_info(IF_INFO *if_info, char * str)
int32 get_str_from_if_info(IF_INFO *if_info, char * str)
{
        char *type;
        
	if(!str || !if_info){
                return IH_ERR_NULL_PTR;
        }

        type = get_if_name_by_type(if_info->type);

        if (!type) return IH_ERR_FAILED;

        switch(if_info->type) {
        case IF_TYPE_CELLULAR:
        case IF_TYPE_TUNNEL:
        case IF_TYPE_VXLAN:
        case IF_TYPE_SVI:
        case IF_TYPE_VT:
        case IF_TYPE_DIALER:
        case IF_TYPE_VP:
        case IF_TYPE_VE:
        case IF_TYPE_OPENVPN:
        case IF_TYPE_LO:
                snprintf(str, IF_CMD_NAME_SIZE, "%s %d", type, if_info->port);
                if(if_info->type == IF_TYPE_SVI 
                && if_info->port == DOT11_SVI_PORT){
                    snprintf(str, IF_CMD_NAME_SIZE, "dot11radio 1");
                }
                if(if_info->type == IF_TYPE_OPENVPN && if_info->port > 10){
                    snprintf(str, IF_CMD_NAME_SIZE, "%s %s", type, "server");
                }
                break;
        case IF_TYPE_FE:
        case IF_TYPE_GE:
                snprintf(str, IF_CMD_NAME_SIZE, "%s %d/%d", type, if_info->slot, if_info->port);
                break;
        case IF_TYPE_SUB_FE:
        case IF_TYPE_SUB_GE:
                snprintf(str, IF_CMD_NAME_SIZE, "%s %d/%d.%d", type, if_info->slot, if_info->port, if_info->sid);
                break;
        case IF_TYPE_SUB_CELLULAR:
        case IF_TYPE_SUB_DOT11:
                snprintf(str, IF_CMD_NAME_SIZE, "%s %d.%d", type, if_info->port, if_info->sid);
                break;
		case IF_TYPE_STA_DOT11:
				snprintf(str, IF_CMD_NAME_SIZE, "%s", type);
				break;
        default:
                snprintf(str, IF_CMD_NAME_SIZE, "%s %d", type, if_info->port);
                break;
        }

        return IH_ERR_OK;
}


/* Map ship between IF_INFO and logical port. These APIs relie to license APIs */
int ir900_if_info_to_logical(IF_INFO * if_info)
{
	if (if_info->type == 0)
		return 0;
#if (defined INHAND_IR9 || defined INHAND_IG5 || defined INHAND_IR8 || defined INHAND_ER6)
	if (if_info->slot == 0)
		return if_info->port;
	if (ih_license_support(IH_FEATURE_ETH5_OMAP_KSZ)) 
		return if_info->port + 1;
	return if_info->port;
#elif (defined INHAND_IG9)
	if (ih_license_support(IH_FEATURE_ETH2_KSZ9893)){ 
		if (if_info->slot == 0)
			return if_info->port;
	}
#elif defined INHAND_ER3
	//FIXME ER3 how
	return if_info->port;
#else
	if (if_info->type == IF_TYPE_FE)
		return if_info->port;
	if (if_info->type == IF_TYPE_GE)
	return (if_info->port) + 4;
#endif
}

IF_INFO * ir900_logical_to_if_info(int logical)
{
	static IF_INFO if_info;

	if (logical <= 0)
		return NULL;

	if (ih_license_support(IH_FEATURE_ETH2_MTK)) {
		if (logical > 2)
			return NULL;
		if_info.type = IF_TYPE_FE;
		if_info.slot = 1;
		if_info.port = logical;
	}else if (ih_license_support(IH_FEATURE_ETH4_MTK)) {
		if (logical > 4)
			return NULL;
		if_info.type = IF_TYPE_FE;
		if_info.slot = 1;
		if_info.port = logical;
	}else if (ih_license_support(IH_FEATURE_ETH5_OMAP_KSZ) 
	|| (ih_license_support(IH_FEATURE_WLAN_MTK) && ih_license_support(IH_FEATURE_IR9))) {		
		if (logical > 5)
			return NULL;
		if_info.type = IF_TYPE_FE;
		if_info.slot = (logical != 1);
		if (logical > 1)
			if_info.port = logical - 1;
		else
			if_info.port = 1;
	} else if (ih_license_support(IH_FEATURE_ETH8_MV)) {
		if (logical > 8)
			return NULL;
		if_info.type = IF_TYPE_FE;
		if_info.slot = 1;
		if_info.port = logical;
	//} else if (ih_license_support(IH_FEATURE_ETH2_OMAP)) {
	} else if(ih_license_support(IH_FEATURE_ETH2_KSZ9893)){
		if (logical > 2)
			return NULL;
		if_info.type = IF_TYPE_GE;
		if_info.slot = 0;
		if_info.port = logical;
	} else {
		if (logical > 2)
			return NULL;
		if_info.type = IF_TYPE_FE;
		if_info.slot = 0;
		if_info.port = logical;
	}

	return &if_info;
}
