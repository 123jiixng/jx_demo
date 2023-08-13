/*
 * $Id$ --
 *
 *   Nvram validation
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
#include <syslog.h>
#include <ctype.h>

#ifdef WIN32
#include <WINSOCK2.H>
#else//WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <netinet/tcp.h>
#ifndef __MUSL__
#include <error.h>
#endif
#endif//WIN32

#include <time.h>

#include "shared.h"
#include "validate.h"
#include "console.h"

#include "if_common.h"
//added by baodn
#define MIN_ROUTERNAME_LENGTH   1 
#define MAX_ROUTERNAME_LENGTH   31    
#define MIN_ACLNAME_LENGTH   1 
#define MAX_ACLNAME_LENGTH   31    

extern BOOL mac_str_valid(char *mac_str);
extern BOOL etype_str_valid(char *etype_str);
extern BOOL fw_mac_str_valid(char *mac_str);
extern int is_valid_ip(const char *host);

/**
 *	Check text length
 *	@param	min		minimal length
 *	@param	max		maximum length
 *	@param	args	not used
 *	@param	value	the text to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_text(int min, int max, char* args, char* value, int flags)
{
	char *dec_value = NULL;
	int len;

	if (!value) {
		return 0;
	}

	dec_value = decrypt_passwd(value);
	if (!dec_value) {
		return 0;
	}

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	len = strlen(dec_value);
	if(len<min || len>max) return 0;

	return 1;
}

/**
 *	Check integer value
 *	@param	min		minimal value
 *	@param	max		maximum value
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_range(int min, int max, char* args, char* value, int flags)
{
	char *dec_value = NULL;
	int n;

	if (!value) {
		return 0;
	}

	dec_value = decrypt_passwd(value);
	if (!dec_value) {
		return 0;
	}

	n = atoi(dec_value);

	if (*dec_value=='-') dec_value++; //skip minus
	
	if (flags & MATCH_ALLOW_PARTIAL) {
		return isdigit(dec_value[0]);
	}

	while(*dec_value) {
		if (*dec_value<'0' || *dec_value>'9') return 0;
		dec_value++;
	}

	if(n<min || n>max) return 0;
	
	return 1;
}

/**
 *	Check whether text is in the list
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	valid data list, seperated with ','
 *	@param	value	the text to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for not in list, 1 for in list
 */
int validate_select(int min, int max, char* args, char* value, int flags)
{
	int len = strlen(value);
	char *p = strstr(args,value);

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if(!p) return 0;
	if(*(p+len)!=',' && *(p+len)!='\0') return 0;

	if(p!=args && *(p-1)!=',') return 0;

	return 1;
}

/**
 *	Check IP address
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_ip(int min, int max, char* args, char* value, int flags)
{
#if 0		//modify by zhengyb,2012/03/22
    unsigned long  n,i;
    unsigned long  ip_pool[][3] = {
            /* class A: 1.0.0.0   - 127.255.255.255,  net mask:255.0.0.0 */
            {0x01000000,0x7FFFFFFF,0xFF000000},
            /* class B: 128.0.0.0 - 191.255.255.255,  net mask:255.255.0.0 */
            {0x80000000,0xBFFFFFFF,0xFFFF0000},
            /* class C: 192.0.0.0 - 223.255.255.255,  net mask:255.255.255.0 */
            {0xC0000000,0xDFFFFFFF,0xFFFFFF00}
    };

    if (!value) return 0;

    n = inet_addr(value);
    n = ntohl(n);
    for (i=0;i<3;i++) {
        if (n >= ip_pool[i][0] &&
            n <= ip_pool[i][1] &&
            (n&~ip_pool[i][2]) != 0 &&
            (n&~ip_pool[i][2]) != ~ip_pool[i][2]) return 1;
    }
    return 0;
#else
	unsigned int p[4];
	int ret;
	
	if (!value) return 0;

	if (flags & MATCH_ALLOW_PARTIAL) {
		ret = isdigit(value[0]);
		return ret;
	}

	ret = inet_addr(value);
	if(ret==-1) return 0;

	ret = sscanf(value, "%u.%u.%u.%u", p, p+1, p+2, p+3);	
	if(ret<4) return 0;
	//filter multicast & broadcast
	//if(*p>=224) return 0;

	return 1;
#endif
}

int validate_ip_mask(char* ip, char* mask)
{
	unsigned int p[4];
	int ip_value;
	int mask_value;
	int host = 0;
	int ret = -1;
	
	if (!ip || !mask) return 0;

	ip_value = inet_addr(ip);
	if(ip_value ==-1) return 0;

	mask_value = inet_addr(mask);
	if(mask_value ==-1) return 0;

	ret = sscanf(ip, "%u.%u.%u.%u", p, p+1, p+2, p+3);	
	if(ret < 4) return 0;
	//filter multicast & broadcast
	if(*p>=224) return 0;
	
	host = ip_value & (~mask_value);
	if (host == 0 || host == (~mask_value)) {
		return 0;
	}

	return 1;
}

/**
 *	Check netmask
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_netmask(int min, int max, char* args, char* value, int flags)
{
	uns32 x = 0xffffffff;
	//unsigned long n = inet_addr(value);
	uns32 n;
	struct in_addr in;	
	int i, ret;
	
	if (flags & MATCH_ALLOW_PARTIAL) {
		ret = isdigit(value[0]);
		return ret;
	}

	ret = inet_aton(value, &in);
	if(ret==0) return 0;
	n = in.s_addr;

//	if (n == x || n == 0) return 0;//modify by zhengyb, ALL-ONE or ALL-ZERO is a valid netmask
	if (n == x || n == 0) return 1;
	
	n = ntohl(n);
	for (i=0; i<32; i++) {
		if (n == x) return 1;
		x = x << 1;
	}

	return 0;
}

/**
 *	Check date value
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_date(int min, int max, char* args, char* value, int flags)
{
    time_t tm;
    struct tm tm_time;

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	time(&tm);
    memcpy(&tm_time, localtime(&tm), sizeof(tm_time));

#if 0 
	if (sscanf(value, "%d:%d:%d", &tm_time.tm_hour, &tm_time.tm_min,
                                     &tm_time.tm_sec) == 3) {
    	/* no adjustments needed */
    } else if (sscanf(value, "%d.%d-%d:%d:%d", &tm_time.tm_mon,
                                            &tm_time.tm_mday, &tm_time.tm_hour,
                                            &tm_time.tm_min, &tm_time.tm_sec) == 5) {
        /* Adjust dates from 1-12 to 0-11 */
		if (tm_time.tm_mon < 1 || tm_time.tm_mday < 1) return 0;
        tm_time.tm_mon -= 1;
    } else  if (sscanf(value, "%d.%d.%d", &tm_time.tm_year,
                                          &tm_time.tm_mon, &tm_time.tm_mday) == 3) {
		if (tm_time.tm_year < 1900 ||
			tm_time.tm_mon < 1 || 
			tm_time.tm_mday < 1) return 0;                                          
        tm_time.tm_year -= 1900;        /* Adjust years */
        tm_time.tm_mon -= 1;    /* Adjust dates from 1-12 to 0-11 */
    } else 
#endif
	if (sscanf(value, "%d.%d.%d-%d:%d:%d", &tm_time.tm_year,
                                            &tm_time.tm_mon, &tm_time.tm_mday,
                                            &tm_time.tm_hour, &tm_time.tm_min,
                                            &tm_time.tm_sec) == 6) {
        if (tm_time.tm_year < 2000 ||
			tm_time.tm_mon < 1 || 
			tm_time.tm_mday < 1) return 0;
        tm_time.tm_year -= 1900;        /* Adjust years */
        tm_time.tm_mon -= 1;    /* Adjust dates from 1-12 to 0-11 */
	} else {
        return 0;
    }

	/* Correct any day of week and day of year etc. fields */
    tm_time.tm_isdst = -1;  /* Be sure to recheck dst. */
    tm = mktime(&tm_time);
    if (tm < 0) {
    	return 0;
    }
	
	return 1;
}

/**
 *	Check email addr
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_email(int min, int max, char* args, char* value, int flags)
{
	char *email;
	char ch;
	int atpos = 0,pos = 0,dotpos = 0;

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if (!value) return 0;
	email = value;
	
	while ((ch = *(email + pos)) != '\0') {
		if (!isprint(ch) || isspace(ch)) return 0;
		if (ch == '@') {
			atpos = pos;
		} else if (ch == '.') {
			dotpos = pos;
		}

		pos ++;
	}

	if (pos > 253 || atpos == 0 || dotpos == 0 || pos - dotpos > 64 
		|| pos - dotpos < 3 || dotpos - 2 < atpos)
		return 0;

	return 1;
}

/**
 *	Check email addr
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_domain_name(int min, int max, char* args, char* value, int flags)
{
	int dotpos = 0,pos = 0;
	char ch;
	int len = strlen(value);
	struct in6_addr ip6_addr;
	int prefix_len;

        if(len >= MAX_DOMAIN_NAME) return 0;//added by baodn to limit ip/domain length

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if(str_to_ip6addr_prefixlen(value, &ip6_addr, &prefix_len)){
		return 1;
	}

	if (validate_ip(0,0,0,value, flags)) {
		return 1;	//modify by ganjx to check surplus
		#if 0	
		unsigned n;
		
		n = inet_addr(value);
		n &= 0xFF000000;
		if (n==0 || n==0xFF000000) return 0;
		#endif
	}

	while ((ch = *(value + pos)) != '\0') {
		if (!isprint(ch) || isspace(ch)) return 0;
		
		if (ch == '.') {
			dotpos = pos;
		}

		pos ++;
	}

	if (dotpos == 0 || pos - dotpos < 2) return 0;

	return 1;
}

/**
 *	Check router name 
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_router_name(int min, int max, char* args, char* value, int flags)
{
	int pos = 0;
	char ch;
	int len = strlen(value);

    if(len < MIN_ROUTERNAME_LENGTH || len > MAX_ROUTERNAME_LENGTH) 
		return 0;

	if (flags & MATCH_ALLOW_PARTIAL) 
		return 1;

	while ((ch = *(value + pos)) != '\0') {
		if (!isalnum(ch) && strncmp(&ch, "-", 1) && strncmp(&ch, "_", 1)) 
			return 0;
		pos++;
	}

	return 1;
}

/**
 *	Check acl name 
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_acl_name(int min, int max, char* args, char* value, int flags)
{
	int pos = 0;
	char ch;

	int len = strlen(value);

	if(len < MIN_ACLNAME_LENGTH || len > MAX_ACLNAME_LENGTH) 
		return 0;

	if (flags & MATCH_ALLOW_PARTIAL) 
		return 1;

	if (!isalpha(*value)) 
		return 0;

	while ((ch = *(value + pos)) != '\0') {
		if (!isalnum(ch) && strncmp(&ch, "-", 1) && strncmp(&ch, "_", 1) && strncmp(&ch, ":", 1)) 
			return 0;
		pos++;
	}

	return 1;
}

/**
 *	Check temperature limit
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_temperature(int min, int max, char* args, char* value, int flags)
{
	char *p,*chr;
	int v,symbol,got,v2;

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	/*Format: lower_limit,upper_limit */
	p = value;

	chr = strchr(p,',');
	if (!chr) return 0;

	SKIP_WHITE(p);

	got = 0;
	v = v2 = 0;
	symbol = 1;
	if (chr != p) { /*has lower-limit*/
		if (*p == '-') {
			p++;
			symbol = -1;
		}
		
		while (p < chr) {
			if(*p == ' ') {
				break;
			} else if (isdigit(*p)) {
				if (got == 0) {
					v = 0;
					got++;
				}
				
				v = v*10 + (*p - '0');
				if (v > 100) return 0;
			} else {
				return 0;
			}

			p++;
		}

		if (symbol==-1 && !got) return 0; /*\BD\F6\D3\D0һ\B8\F6\B8\BA\BA\C5*/
		if (got) {
			v *= symbol;
			if (v < -40 || v > 85) return 0;
		}

		SKIP_WHITE(p);
		if (*p != ',') return 0;
	}
	
	p = chr + 1;
	SKIP_WHITE(p);
	if (*p != '\0') {
		int got2 = 0;
		
		symbol = 1;
		if (*p == '-') {
			p++;
			symbol = -1;
		}
		
		while (*p) {
			if(*p == ' ') {
				break;
			} else if (isdigit(*p)) {
				if (got2 == 0) {
					v2 = 0;
					got2 = 1;
				}
				
				v2 = v2*10 + (*p - '0');
				if (v2 > 100) return 0;
			}  else {
				return 0;
			}

			p++;
		}

		if (symbol==-1 && !got2) return 0; /*\BD\F6\D3\D0һ\B8\F6\B8\BA\BA\C5*/
		if (got2) {
			got++;
			v2 *= symbol;
			if (v2 < -40 || v2 > 85) return 0;
		}

		SKIP_WHITE(p);
		if (*p != '\0') return 0;
	}

	if (got == 2 &&v2 < v) return 0;

	if (!got) return 0;

	return 1;
}

#ifndef TZNAME_MAX 
#define TZNAME_MAX 6
#endif /* TZNAME_MAX */

#ifndef __isdigit_char
#define __isdigit_char(c) ((unsigned char)((c) - '0') <= 9)
#endif 

static char vals[] = {
	'T', 'Z', 0,				/* 3 */
	'U', 'T', 'C', 0,			/* 4 */
	25, 60, 60, 1,				/* 4 */
	'.', 1,						/* M */
	5, '.', 1,
	6,  0,  0,					/* Note: overloaded for non-M non-J case... */
	0, 1, 0,					/* J */
	',', 'M',      '4', '.', '1', '.', '0',
	',', 'M', '1', '0', '.', '5', '.', '0', 0,
	',', 'M',      '3', '.', '2', '.', '0',
	',', 'M', '1', '1', '.', '1', '.', '0', 0
};

#define TZ    vals
#define UTC   (vals + 3)
#define RANGES (vals + 7)
#define RULE  (vals + 11 - 1)
#define DEFAULT_RULES (vals + 22)
#define DEFAULT_2007_RULES (vals + 38)

static char *getoffset(char *e, long *pn)
{
	char *s = (char *)(RANGES - 1);
	long n;
	int f;

	n = 0;
	f = -1;
	do {
		++s;
		if (__isdigit_char(*e)) {
			f = *e++ - '0';
		}
		if (__isdigit_char(*e)) {
			f = 10 * f + (*e++ - '0');
		}
		if (((unsigned int)f) >= *s) {
			return NULL;
		}
		n = (*s) * n + f;
		f = 0;
		if (*e == ':') {
			++e;
			--f;
		}
	} while (*s > 1);

	*pn = n;
	return e;
}

static char *getnumber(char *e, int *pn)
{
	int n, f;

	n = 3;
	f = 0;
	while (n && __isdigit_char(*e)) {
		f = 10 * f + (*e++ - '0');
		--n;
	}

	*pn = f;
	return (n == 3) ? NULL : e;
}

/**
 *	Check time format value:"12:00"
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_time(int min, int max, char *args, char *value, int flags)
{
	char *p = NULL;

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if (!value || !*value) return 0;

	if (strlen(value) != 5) return 0;

	//0-9 ASCII:48-57
	if ((int)(value[0]) < 48 || (int)(value[0]) > 57 || (int)(value[1]) < 48 || (int)(value[1]) > 57) {
		return 0;
	}

	if (value[2] != ':') {
		return 0;
	}

	if ((int)(value[3]) < 48 || (int)(value[3]) > 57 || (int)(value[4]) < 48 || (int)(value[4]) > 57) {
		return 0;
	}

	return 1;
}

/**
 * Check timezone format: stdoffset[dst[offset][,start[/time],end[/time]]]
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 */
int validate_timezone(int min, int max, char* args, char* value, int flags)
{
	int n, count, f;
	char c;
	char *e;
	char *s;
	long off = 0;

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if (!value || !*value) return 0;

	e = value;
	count = 0;
LOOP:	
	/* check std or dst name. */
	c = 0;
	if (*e == '<') {
		++e;
		c = '>';
	}

	n = 0;
	while (*e
		   && isascii(*e)		/* SUSv3 requires char in portable char set. */
		   && (isalpha(*e)
			   || (c && (isalnum(*e) || (*e == '+') || (*e == '-'))))
		   ) {
		if (++n > TZNAME_MAX) {
			goto ILLEGAL;
		}
		e++;
	}

	if ((n < 3)					/* Check for minimum length. */
		|| (c && (*e++ != c))	/* Match any quoting '<'. */
		) {
		goto ILLEGAL;
	}

	/* Get offset */
	s = (char *) e;
	if ((*e != '-') && (*e != '+')) {
		if (count && !__isdigit_char(*e)) {
			off -= 3600;		/* Default to 1 hour ahead of std. */
			goto SKIP_OFFSET;
		}
		--e;
	}

	++e;
	if (!(e = getoffset(e, &off))) {
		goto ILLEGAL;
	}

	if (*s == '-') {
		off = -off;				/* Save off in case needed for dst default. */
	}
SKIP_OFFSET:
	if (!count) {
		if (*e) {
			++count;
			goto LOOP;
		}
	} else {   /* OK, we have dst, so get some rules. */
		count = 0;
		
		if (!*e) {
			e = DEFAULT_2007_RULES;
		}

		do {
			if (*e++ != ',') {
				goto ILLEGAL;
			}

			n = 365;
			s = (char *) RULE;
			if ((c = *e++) == 'M') {
				n = 12;
			} else if (c == 'J') {
				s += 8;
			} else {
				--e;
				c = 0;
				s += 6;
			}

			do {
				++s;
				if (!(e = getnumber(e, &f))
					|| (((unsigned int)(f - s[1])) > n)
					|| (*s && (*e++ != *s))
					) {
					goto ILLEGAL;
				}
			} while ((n = *(s += 2)) > 0);

			off = 2 * 60 * 60;	/* Default to 2:00:00 */
			if (*e == '/') {
				++e;
				if (!(e = getoffset(e, &off))) {
					goto ILLEGAL;
				}
			}
			
		} while (++count < 2);

		if (*e) {
			goto ILLEGAL;
		}
	}

	return 1;
ILLEGAL:
	return 0;
}

/**
 *	Check MAC address
 *	@param	min		flags, 1 means '00:00:00:00:00:00' is valid, 0 means invalid
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_mac(int min, int max, char* args, char* value, int flags)
{
	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	return 1;
}

/**
 *	Check IP address range, valid range is: ip1-ip2 or ip/netmask
 *	Notic:In order not to destroy generality,don't add the value 'any' 
 *	    check in this function
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_iprange(int min, int max, char* args, char* value, int flags)
{	
    int mask = 0;
    char ips[32] = {0};
    char ipe[32] = {0};
    unsigned int startip = 0;
    unsigned int endip = 0;

    if (flags & MATCH_ALLOW_PARTIAL) return 1;
    if(!value) return 0;	
    
    if(strchr(value, '-')){
        //maybe value format is ip1-ip2
        if(sscanf(value, "%[^-]-%s", ips, ipe)){
            if(!is_valid_ip(ips) && !is_valid_ip(ipe)){
                startip = ntohl(inet_addr(ips));
                endip = ntohl(inet_addr(ipe));
                if(startip < endip){
                    return 1;
                }
            }
        }
    }else if(strchr(value, '/')){
        //maybe value format is ip/mask
        if(sscanf(value, "%[^/]/%d", ips, &mask)){
            if(!is_valid_ip(ips) && 0 < mask && mask <= 32){
                return 1;
            }
        }     
    }else{
        //maybe value format is ip
        if(!is_valid_ip(value)){
            return 1;
        }
    }
    
    return 0;
}

/**
 *	Check port range, valid range is 1-65535 and value's format like: 
 *	    1)."50"
 *	    2)."50-69"
 *	    3)."3,50-69,57,9000,45-78"
 *	Notic:In order not to destroy generality,don't add the value 'any' 
 *	    check in this function 
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_portrange(int min, int max, char* args, char* value, int flags)
{	
    char *tmp_value = NULL;
    char *port = NULL;
    int startport = 0;
    int endport = 0;
    int ret = 1;

    if (flags & MATCH_ALLOW_PARTIAL) return 1;
    if(!value) return 0;	
    
    tmp_value = strdup(value);
    if(tmp_value){
        while ((port = strsep(&tmp_value, ","))){
            if(strchr(port, '-')){
                if(sscanf(port, "%d-%d", &startport, &endport)){
                    if(startport > endport || startport < 1 || 
                       startport > 65535 || endport < 1 || endport > 65535){
                        ret = 0;
                        break;
                    }
                }
            }else{
                if(atoi(port) < 1 || atoi(port) > 65535){
                    ret = 0;
                    break;
                }
            }
        }
        free(tmp_value);
    }

    return ret;
}

/**
 *	Check IPv6 address and prefixlen 
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_ip6addr_prefixlen(int min, int max, char* args, char* value, int flags)
{	
	struct in6_addr ip6_addr;
	int prefix_len = 0;

	if(flags & MATCH_ALLOW_PARTIAL){
        if(strlen(value) == 0) {
            return 1;
        }
		return 0;
	}

	if(!strchr(value, '/')){
		return 0;
	}

	if(!str_to_ip6addr_prefixlen(value, &ip6_addr, &prefix_len)){
		return 0;
	}

	return 1;
}

/**
 *	Check IPv6 address without prefixlen 
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_ip6addr(int min, int max, char* args, char* value, int flags)
{	
	struct in6_addr ip6_addr;
	int prefix_len = 0;

	if(flags & MATCH_ALLOW_PARTIAL){
        if(strlen(value) == 0) {
            return 1;
        }
		return 0;
	}

	if(strchr(value, '/')){
		return 0;
	}

	if(!str_to_ip6addr_prefixlen(value, &ip6_addr, &prefix_len)){
		return 0;
	}

	return 1;
}

/**
 *	Check slot/port range, valid range is: slot/port
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_slotportvlan(int min, int max, char* args, char* value, int flags)
{
	char *p = NULL;
	char *pchr = NULL;
	int slot, port,vid;
	IF_TYPE type;
	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	p = strchr(value, '.');
	if (!p || (p == value)){
		return FALSE;
	}
	
	p = strchr(value, '/');
	if (!p || (p == value)){
		return FALSE;
	}
	
	/* check slot */
	pchr = value;

	if(!*pchr){
		return FALSE;
	}

	while(*pchr && (pchr < p)){
		if(!isdigit(*pchr)){
			return FALSE;
		}
		pchr++;
	}

	/* check port */
	pchr = p+1;
	if(!*pchr){
		return FALSE;
	}

	p = strchr(value, '.');
	while(*pchr && (pchr < p)){
		if(!isdigit(*pchr)){
			return FALSE;
		}
		pchr++;
	}
	/* check vlan id*/
	pchr = p+1;
	if(!*pchr){
		return FALSE;
	}
	while(*pchr){
		if(!isdigit(*pchr)){
			return FALSE;
		}
		pchr++;
	}	
	
	slot = atoi(value);
	p = strchr(value, '/');
	port = atoi(p+1);
	p = strchr(value, '.');
	vid = atoi(p+1);
	if (vid < 1 || vid > 4094)
		return FALSE;
	/* FixMe: the interface service must do the check depented on special model */
	/* search back to find port type */
	pchr = value;
	do{pchr--;}while(*pchr == ' ');
	do{pchr--;}while(*pchr != ' ');
	pchr++;
	if (*pchr == 'g')
		type = IF_TYPE_GE;
	else
		type = IF_TYPE_FE;
	
	if (ih_license_support(IH_FEATURE_ETH5_OMAP_KSZ)) {
			if (type == IF_TYPE_GE) goto unsupported;
			if (slot > 1) goto unsupported;
			if (slot == 0 && port != 1) goto unsupported;
			if (slot == 1 && (port < 1 || port > 4)) goto unsupported;
	} else if (ih_license_support(IH_FEATURE_ETH8_MV)) {
			if (type == IF_TYPE_GE) goto unsupported;
			if (slot != 1) goto unsupported;
			if (port < 1 || port > 8) goto unsupported;
	} else if (ih_license_support(IH_FEATURE_WLAN_MTK) && ih_license_support(IH_FEATURE_IR9)) {
			if (type == IF_TYPE_GE) goto unsupported;
			if (slot > 1) goto unsupported;
			if (slot == 0 && (port < 1 || port > 2)) goto unsupported;
			if (slot == 1 && (port < 1 || port > 4)) goto unsupported;
	} else {
			if (type == IF_TYPE_GE) goto unsupported;
			if (slot > 0) goto unsupported;
			if (port < 1 || port > 2) goto unsupported;
	}

	return TRUE;
unsupported:
	//printf("\nPort is not supported\n");
	return FALSE;
}

/**
 *	Check slot/port range, valid range is: slot/port
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_slotport(int min, int max, char* args, char* value, int flags)
{
	char *p = NULL;
	char *pchr = NULL;
	int slot, port;
	IF_TYPE type;

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	p = strchr(value, '/');
	if (!p || (p == value)){
		return FALSE;
	}

	/* check slot */
	pchr = value;

	if(!*pchr){
		return FALSE;
	}

	while(*pchr && (pchr < p)){
		if(!isdigit(*pchr)){
			return FALSE;
		}
		pchr++;
	}

	/* check port */
	pchr = p+1;
	if(!*pchr){
		return FALSE;
	}

	while(*pchr){
		if(!isdigit(*pchr)){
			return FALSE;
		}
		pchr++;
	}

	slot = atoi(value);
	port = atoi(p+1);
	/*FixMe: search back can not find the desired stream*/
	/* search back to find port type */
	pchr = value;
	do{pchr--;}while(*pchr == ' ');
	do{pchr--;}while(*pchr != ' ');
	pchr++;
	if (*pchr == 'g')
		type = IF_TYPE_GE;
	else
		type = IF_TYPE_FE;

	if (ih_license_support(IH_FEATURE_ETH2_MTK)) {
		if (slot != 1) goto unsupported;
		if (port < 1 || port > 2) goto unsupported;
	}else if (ih_license_support(IH_FEATURE_ETH4_MTK)) {
#ifdef INHAND_ER6
  		if (slot == 0) {
            if (port != 1) goto unsupported;
        } else if (slot == 1)  {
            if (port < 1 || port > 4) goto unsupported;
        } else {
            goto unsupported;
        }
#elif defined INHAND_ER3
  		if (slot == 0) {
            if (port != 1) goto unsupported;
        } else if (slot == 1)  {
            if (port != 1) goto unsupported;
        } else {
            goto unsupported;
        }
#else
		if (slot != 1) goto unsupported;
		if (port < 1 || port > 4) goto unsupported;
#endif
	}else if (ih_license_support(IH_FEATURE_ETH4_IPQ40XX)) {
#ifdef INHAND_IR8
        if (slot == 0) {
            if (port != 1) goto unsupported;
        } else if (slot == 1)  {
            if (port < 1 || port > 4) goto unsupported;
        } else {
            goto unsupported;
        }

#elif defined INHAND_VG9
		if (slot != 1) goto unsupported;
		if (port < 1 || port > 4) goto unsupported;
#endif
	}else if (ih_license_support(IH_FEATURE_ETH5_OMAP_KSZ)) {
			//if (type == IF_TYPE_GE) goto unsupported;
			if (slot > 1) goto unsupported;
			if (slot == 0 && port != 1) goto unsupported;
			if (slot == 1 && (port < 1 || port > 4)) goto unsupported;
	} else if (ih_license_support(IH_FEATURE_ETH8_MV)) {
			//if (type == IF_TYPE_GE) goto unsupported;
			if (slot != 1) goto unsupported;
			if (port < 1 || port > 8) goto unsupported;
	} else if (ih_license_support(IH_FEATURE_WLAN_MTK) && ih_license_support(IH_FEATURE_IR9)) {
			//if (type == IF_TYPE_GE) goto unsupported;
			if (slot > 1) goto unsupported;
			if (slot == 0 && (port < 1 || port > 2)) goto unsupported;
			if (slot == 1 && (port < 1 || port > 4)) goto unsupported;
	} else {
			//if (type == IF_TYPE_GE) goto unsupported;
			if (slot > 0) goto unsupported;
			if (port < 1 || port > 2) goto unsupported;
	}

	return TRUE;
unsupported:
	//printf("\nPort is not supported\n");
	return FALSE;
}

/**
 *	Check MAC address string(such as '0000.0000.0001')
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_macaddr(int min, int max, char* args, char* value, int flags)
{
	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if(!mac_str_valid(value)){
		return FALSE;
	}

	return TRUE;
}

/**
 *	Check HH:MM/HOUR:MINUTE string(such as '12:00')
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_hour_minute(int min, int max, char* args, char* value, int flags)
{
	char *p = NULL;
	char *pchr = NULL;
	int minute = 0;
	int hour = 0;

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if(!value){
		return FALSE;
	}

	p = strchr(value, ':');
	if (!p || (p == value)){
		return FALSE;
	}
	
	pchr = value;
	while(*pchr && (pchr < p)){
		if(!isdigit(*pchr)){
			return FALSE;
		}
		pchr++;
	}

	if((p - value) >  2){
		return FALSE;
	}

	hour = atoi(value);
	if(hour < 0 || hour > 23){
		return FALSE;
	}

	pchr = p + 1;

	p = value + strlen(value) - 1;
	if(pchr > p || ((p - pchr) > 1)){
		return FALSE;
	}
	
	minute = atoi(pchr);
	if(minute < 0 || minute > 59){
		return FALSE;
	}

	while(*pchr && (pchr <= p)){
		if(!isdigit(*pchr)){
			return FALSE;
		}
		pchr++;
	}

	return TRUE;
}

/**
 *	Check MAC address string(such as '00:00:00:00:00:01')
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int fw_validate_macaddr(int min, int max, char* args, char* value, int flags)
{
	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if(!fw_mac_str_valid(value)){
		return FALSE;
	}

	return TRUE;
}

int ip6_validate_interface_id(int min, int max, char* args, char* value, int flags)
{
	if (flags & MATCH_ALLOW_PARTIAL) return 1;
	if(!ip6_if_id_str_valid(value)){
		return FALSE;
	}

	return TRUE;
}

/**
 *	Check ether type string(such as '9100' (HEX))
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_etype(int min, int max, char* args, char* value, int flags)
{
	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if(!etype_str_valid(value)){
		return FALSE;
	}

	return TRUE;
}


/**
 *	Check IEC61850 MAC string(last 3 bytes)(such as '000' (HEX), valid range: <000-1ff>)
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_mac_last3_byte(int min, int max, char* args, char* value, int flags)
{
	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if(!mac_last3_byte_str_valid(value)) return 0;

	return 1;
}

int validate_hex_byte(int min, int max, char* args, char* value, int flags)
{
	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if(!hex_str_valid(value, 1, 2)) return 0;

	return 1;
}

/**
 *	Check slot/port range
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_slotportrange(int min, int max, char* args, char* value, int flags)
{
	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	return 1;
}

/**
 *	Check filename
 *	@param	min		0: ignore, 1: must exist, 2: must not exist
 *	@param	max		not used
 *	@param	args	directory
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_filename(int min, int max, char* args, char* value, int flags)
{
	int exist;
	char path[MAX_PATH];

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if (!*value) return 0;

	snprintf(path, sizeof(path), "%s/%s", args, value);

	exist = f_exists(path);

	if (min==1) return exist;
	if (min==2) return !exist;

	return 1;
}

/**
 *	Check oid, valid range is: .1.3.6.1.
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_varoid(int min, int max, char* args, char* value, int flags)
{
	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	return 1;
}

/**
 *	check serial number
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_sn(int min, int max, char* args, char* value, int flags)
{
	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if (strlen(value) != 15) return 0;
		
	return 1;
}


//////////////////////////////////////////////////////////////////
// validate for InRouter firewall CLI args
/**
 *	check access-list-number
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args		not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_acl_num(int min, int max, char* args, char* value, int flags)
{
	int n = atoi(value);

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	while(*value) {
		if (*value<'0' || *value>'9') return 0;
		value++;
	}

	if (n >= 1 && n <= 99)	return 1;
	if (n >= 1300 && n <= 1999)	return 1;//for standard ip acl

	if (n >= 100 && n <= 199)	return 1;
	if (n >= 2000 && n <= 2699)	return 1;//for static externed ip acl
	
	return 0;
}

int validate_wildcard(int min, int max, char* args, char* value, int flags)
{
	uns32 x = 0xffffffff;
	uns32 n = 0;
	int i;
	int ret;
	struct in_addr in;
		
	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	ret = inet_aton(value, &in);
	if (!ret) return 0;
	n = ntohl(in.s_addr);
	n = ~n;
	
	if(n == 0) return 1;

	for (i=0; i<32; i++) {
		if (n == x) return 1;
		x = x << 1;
	}

	return 0;
}


/**
 *	check vlan list string, lik "1,3,5-100,30"
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args		not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_vlan_list(int min, int max, char* args, char* value, int flags)
{
	char *p, *q;
	char tmp;
	int vid;
	int last_vid = 0;

	
	/* 0): string must start with a number*/
	if (*value < '0' || *value > '9'){
		return 0;
	}

	if (flags & MATCH_ALLOW_PARTIAL) return 1;
		
	/* 1): only allow '0'~'9', ',' and '-' */
	p = value;
	while(*p != '\0'){
		if ((*p >= '0' && *p <= '9') || (*p == ',') || (*p == '-') || (*p == ' ')){
			p++;
			continue;
		}
		return 0;
	}
	
	/* 2): 1 <= vid <= 4094 */
	q = p = value;
	while(*q != '\0'){
		for (; (*q >= '0' && *q <= '9'); q++);
		tmp = *q;
		*q = '\0';
		vid = atoi(p);
		*q = tmp;
				
		if (vid < 1 || vid > 4094)
			return 0;
		if (vid <= last_vid)
			return 0;
	
		for( ; ((*q < '0' || *q > '9') && *q != '\0'); q++); //find number char
		if (*q == '\0') {
			break;
		}
			
		p = q;
		q++;
			last_vid = vid;
	}

	/* 3): last char. must be a number */
	for (p = value; *p != '\0'; p++);
	p--;
	if (*p < '0' || *p > '9') return 0;
	return 1;
}

	
/**
 *	check pin code
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args		not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_pincode(int min, int max, char* args, char* value, int flags)
{
	char *pin = NULL;

	if (!value) {
		return 0;
	}

	pin = decrypt_passwd(value);
	if (!pin) {
		return 0;
	}

	if (flags & MATCH_ALLOW_PARTIAL) {
		return isdigit(pin[0]);
	}
    

	if(strlen(pin)<4 || strlen(pin)>8) return 0;

	while(*pin) {
		if (*pin<'0' || *pin>'9') return 0;
		pin++;
	}

	return 1;
}

/**
 *	Check cell port.vid, valid range is: 1.1-2
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_cell_portvid(int min, int max, char* args, char* value, int flags)
{
	char *p = NULL;
	char *pchr = NULL;
	int port,vid;

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	p = strchr(value, '.');
	if (!p || (p == value)){
		return FALSE;
	}
	
	/* check port */
	pchr = value;

	if(!*pchr){
		return FALSE;
	}

	while(*pchr && (pchr < p)){
		if(!isdigit(*pchr)){
			return FALSE;
		}
		pchr++;
	}

	/* check virtual id*/
	pchr = p+1;
	if(!*pchr){
		return FALSE;
	}
	while(*pchr){
		if(!isdigit(*pchr)){
			return FALSE;
		}
		pchr++;
	}	
	
	port = atoi(value);
	p = strchr(value, '.');
	vid = atoi(p+1);
	if (vid < 1 || vid > 2)
		return FALSE;

	return TRUE;
}

/**
 *	Check port.sid, valid range is: <1.1-2.2>
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_portsid(int min, int max, char* args, char* value, int flags)
{
	char *p = NULL;
	char *pchr = NULL;
	int port,sid;

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	p = strchr(value, '.');
	if (!p || (p == value)){
		return FALSE;
	}
	
	/* check port */
	pchr = value;

	if(!*pchr){
		return FALSE;
	}

	while(*pchr && (pchr < p)){
		if(!isdigit(*pchr)){
			return FALSE;
		}
		pchr++;
	}

	/* check sid*/
	pchr = p+1;
	if(!*pchr){
		return FALSE;
	}
	while(*pchr){
		if(!isdigit(*pchr)){
			return FALSE;
		}
		pchr++;
	}	
	
	port = atoi(value);
	p = strchr(value, '.');
	sid = atoi(p+1);
	if (sid < 1 || sid > 2)
		return FALSE;

	return TRUE;
}

/**
 *	Check shell script file name: name.sh
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_shell_script(int min, int max, char* args, char* value, int flags)
{
	int len;
	char *p;

	if (!validate_text(min, max, args, value, flags)) return FALSE;
	
	len = strlen(value);
	if (len < 3) return FALSE;

	p = value + len - 3;
	if (strcmp(p, ".sh") == 0)
		return TRUE;
	return FALSE;
}

/**
 *	Check shell script file name: name.sh
 *	@param	min		not used
 *	@param	max		not used
 *	@param	args	not used
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_rsync_task(int min, int max, char* args, char* value, int flags)
{
	int len;
	char *p;

	if (!validate_text(min, max, args, value, flags)) return FALSE;
	
	len = strlen(value);
	if (len < 3) return FALSE;

	p = value + len - 6;
	if (strcmp(p, ".rsync") == 0)
		return TRUE;
	return FALSE;
}

/**
 *	Check interface name 
 *	@param	min		0: ignore, 1: must exist, 2: must not exist
 *	@param	max		not used
 *	@param	args	directory
 *	@param	value	to be checked
 *	@param	flags	match flags (MATCH_EXACT|MATCH_ALLOW_PARTIAL)
 *	@return	0 for invalid, 1 for valid
 */
int validate_ifname(int min, int max, char* args, char* value, int flags)
{
	IF_INFO if_info;

	if (flags & MATCH_ALLOW_PARTIAL) return 1;

	if (!*value) return 0;

	if(!strcmp(value, "any")){
		return 1;
	}

	if(get_if_info_from_panel_name(value, &if_info)){
		LOG_DB("get if_info by interface %s failed, maybe not support this interface", value);
		return 0;
	}

	return 1;
}

//////////////////////////////////////////////////////////////////////



// validating
#define MIN_INT	0x80000000
#define MAX_INT 0x7FFFFFFF
#define MAX_VALUE_LEN	128

const struct opt_validate opt_validates[] = {
	{ "name",		NULL,	OP_LENGTH(1, 64), OPT_FLAG_NONE},
	{ "hostname",	NULL,	OP_LENGTH(1, (MAX_HOST_NAME -1)), OPT_FLAG_NONE},
	{ "routername",	NULL,	OP_ROUTERNAME, OPT_FLAG_NONE},
	{ "aclname",	NULL,	OP_ACLNAME, OPT_FLAG_NONE},
    { "groupname",  NULL,   OP_LENGTH(1, 32), OPT_FLAG_NONE},
    { "username",   NULL,   OP_LENGTH(1, 128), OPT_FLAG_NONE},
    { "port-name",	NULL,	OP_LENGTH(1, MAX_DESCRIPTION-1), OPT_FLAG_NONE},
    { "vlan-name",	NULL,	OP_LENGTH(1, MAX_VLAN_NAME_LEN-1), OPT_FLAG_NONE},
    { "domain-name/ip",NULL,OP_DOMAINNAME, OPT_FLAG_NONE},
	{ "email-addr",	NULL,	OP_EMAIL, OPT_FLAG_NONE},
	{ "string",		NULL,	OP_LENGTH(1, 128), OPT_FLAG_NONE},
	{ "json-string",		NULL,	OP_LENGTH(1, 1024), OPT_FLAG_NONE},
    { "desc",		NULL,	OP_LENGTH(1, 128), OPT_FLAG_NONE},
	{ "script",		NULL,	OP_LENGTH(1, 128), OPT_FLAG_NONE},
	{ "args",		NULL,	OP_LENGTH(1, 128), OPT_FLAG_NONE},
	{ "key",		NULL,	OP_LENGTH(1, 64), OPT_FLAG_NONE},
    { "contactString",              NULL,   OP_LENGTH(1, 64), OPT_FLAG_NONE},
    { "locationString",             NULL,   OP_LENGTH(1, 64), OPT_FLAG_NONE},
	{ "ownerString",		NULL,	OP_LENGTH(1, 32), OPT_FLAG_NONE},
	{ "password",	NULL,	OP_LENGTH(1, 128), OPT_FLAG_NONE},
    { "auth_password",      NULL,   OP_LENGTH(8, 32+5), OPT_FLAG_NONE},
    { "priv_password",      NULL,   OP_LENGTH(8, 32+5), OPT_FLAG_NONE},
    { "trap_community",             NULL,   OP_LENGTH(1, 32), OPT_FLAG_NONE},
	{ "value",		NULL,	OP_LENGTH(1, MAX_VALUE_LEN), OPT_FLAG_NONE},
	{ "timezone",	NULL,	OP_TIMEZONE, OPT_FLAG_NONE},
	{ "ip",			NULL,	OP_IP, OPT_FLAG_NONE},
	{ "time",		NULL,	OP_TIME, OPT_FLAG_NONE},
	{ "netmask",	NULL,	OP_NETMASK, OPT_FLAG_NONE},
	{ "mask_len",	NULL,	OP_RANGE(0, 32), OPT_FLAG_NONE},
	{ "mtu",		NULL,	OP_RANGE(68, 1500), OPT_FLAG_NONE},
	{ "ipz",		NULL,	OP_IPZ, OPT_FLAG_NONE},
	{ "ip/mask",	NULL,	OP_IPRANGE, OPT_FLAG_NONE},
	{ "portrange",	NULL,	OP_PORTRANGE, OPT_FLAG_NONE},
	{ "date",		NULL,	OP_DATE, OPT_FLAG_NONE},
	{ "slot/port",	NULL,	OP_SLOTPORT, OPT_FLAG_NONE},
	{ "slot/port.vlan", NULL, 	OP_SLOTPORTVLAN, OPT_FLAG_NONE},
	{ "slot-port-range",	NULL,	OP_SLOTPORTRANGE, OPT_FLAG_NONE},
	{ "1.1-2", NULL, 	OP_CELL_PORTVID, OPT_FLAG_NONE},
	{ "port.1-2", NULL, 	OP_CELL_PORTVID, OPT_FLAG_NONE},
	//{ "mac-address",		NULL,	OP_LENGTH(1, 64), OPT_FLAG_NONE},
	{ "variable",		NULL,	OP_VAR_OID, OPT_FLAG_NONE},
	{ "HHHH.HHHH.HHHH",		NULL, OP_MAC_ADDR, OPT_FLAG_NONE},
	{ "HH:HH:HH:HH:HH:HH",		NULL, OP_FW_MAC_ADDR, OPT_FLAG_NONE},
	{ "HHHH",		NULL, OP_ETHER_TYPE, OPT_FLAG_NONE},
	{ "HHH",		NULL, OP_MAC_LAST3_BYTE, OPT_FLAG_NONE},
	{ "HH",			NULL, OP_HEX_BYTE, OPT_FLAG_NONE},
	{ "HH:MM",		NULL, OP_HOUR_MINUTE, OPT_FLAG_NONE},

	{ "ipv6-interface-id",		NULL, OP_IP6_IF_ID, OPT_FLAG_NONE},

	{ "bool",		NULL,	OP_BOOL, OPT_FLAG_NONE},

	{ "n",			NULL,	OP_RANGE(MIN_INT, MAX_INT), OPT_FLAG_NONE},
	{ "id",			NULL,	OP_RANGE(MIN_INT, MAX_INT), OPT_FLAG_NONE},
	{ "pdus",		NULL,	OP_RANGE(MIN_INT, MAX_INT), OPT_FLAG_NONE},
	{ "seconds",	NULL,	OP_RANGE(0, MAX_INT), OPT_FLAG_NONE},
	{ "timeout",	NULL,	OP_RANGE(0, MAX_INT), OPT_FLAG_NONE},
	{ "size",		NULL,	OP_RANGE(1, MAX_INT), OPT_FLAG_NONE},
	{ "port",		NULL,	OP_PORT, OPT_FLAG_NONE},
	{ "times",		NULL,	OP_TIMES, OPT_FLAG_NONE},
	{ "vid",		NULL,	OP_VLAN_ID, OPT_FLAG_NONE},

	{ "output-file",NULL,	OP_FILENAME(2, "/tmp/"), OPT_FLAG_NONE},
	{ "input-file",NULL,	OP_FILENAME(1, "/tmp/"), OPT_FLAG_NONE},
	{ "config-file",NULL,	OP_FILENAME(0, "/var/backups/"), OPT_FLAG_NONE},

	{ "community-string",	NULL,	OP_LENGTH(1, 32), OPT_FLAG_NONE},
	{ "trap-type",			NULL,	OP_RANGE(1, MAX_INT), OPT_FLAG_NONE},
    	{ "host-addr",NULL,	OP_DOMAINNAME, OPT_FLAG_NONE},
	
	/* ssh */
	{ "ssh-timeout",		NULL,	OP_RANGE(0, 120), OPT_FLAG_NONE},
	{ "authentication-retries",	NULL,	OP_RANGE(0, 5), OPT_FLAG_NONE},
	{ "temperature-limit",	NULL,OP_TEMPERATURE, OPT_FLAG_NONE},
	/* redial */
	{ "apn",	NULL,	OP_LENGTH(1, 31), OPT_FLAG_NONE},
	{ "dial-number",	NULL,	OP_LENGTH(1, 31), OPT_FLAG_NONE},
	{ "pincode",	NULL,	OP_PINCODE, OPT_FLAG_NONE},
	{ "phone-number",	NULL,	OP_LENGTH(1, 20), OPT_FLAG_NONE},
	/*acl*/
	{ "access-list-number",		NULL,	OP_ACL_NUM, 		OPT_FLAG_NONE},
	{ "line",			NULL,	OP_LENGTH(1, 100), 	OPT_FLAG_NONE},
	{ "source",			NULL,	OP_IP, 			OPT_FLAG_NONE},
	{ "source-wildcard",		NULL,	OP_WILDCARD, 		OPT_FLAG_NONE},
	{ "destination",		NULL,	OP_IP, 			OPT_FLAG_NONE},
	{ "destination-wildcard",	NULL,	OP_WILDCARD, 		OPT_FLAG_NONE},
	/*static route*/
	{ "gateway",		NULL,	OP_IP, 			OPT_FLAG_NONE},
	{ "wildcard",		NULL,	OP_WILDCARD, 		OPT_FLAG_NONE},
        { "access-list-name",	NULL,	OP_LENGTH(1, MAX_DESCRIPTION-1), OPT_FLAG_NONE},
	{ "prefix-list-name",   NULL,	OP_LENGTH(1, MAX_DESCRIPTION-1), OPT_FLAG_NONE},
	{ "ip-masklen",	NULL,	OP_LENGTH(9, 18), OPT_FLAG_NONE},

	/*certmanager*/
	{ "passphrase",	NULL,	OP_LENGTH(1, 64), OPT_FLAG_NONE},
	{ "serialnum",	NULL,	OP_LENGTH(8, 8), OPT_FLAG_NONE},
	{ "tlskey",	NULL,	OP_LENGTH(1, 2048), OPT_FLAG_NONE},
	{ "vlan-list",	NULL,	OP_VLAN_LIST, OPT_FLAG_NONE},
	/*IP QoS*/
	{ "classifier-name",  NULL,   OP_LENGTH(1, 31), OPT_FLAG_NONE},
	{ "behavior-name",  NULL,   OP_LENGTH(1, 31), OPT_FLAG_NONE},
	{ "policy-name",  NULL,   OP_LENGTH(1, 31), OPT_FLAG_NONE},
	/* dot11 */
	{ "ssid",  NULL,   OP_LENGTH(1, 32), OPT_FLAG_NONE},
	{ "country-code",  NULL,   OP_LENGTH(2, 2), OPT_FLAG_NONE},
	{ "wpa-psk-ascii",  NULL,   OP_LENGTH(8, 160), OPT_FLAG_NONE},
	{ "wpa-psk-hex",  NULL,   OP_LENGTH(64, 64), OPT_FLAG_NONE},
	{ "HHHHHHHHHH", NULL, OP_LENGTH(10, 10), OPT_FLAG_NONE},
	{ "HHHHHHHHHHHHHHHHHHHHHHHHHH",	NULL, OP_LENGTH(26, 26), OPT_FLAG_NONE},
	{ "wep104-key-ascii",  NULL,   OP_LENGTH(13, 13), OPT_FLAG_NONE},
	{ "wep40-key-ascii",  NULL,   OP_LENGTH(5, 5), OPT_FLAG_NONE},
	{ "transmit-key",  NULL,   OP_RANGE(1, 4), OPT_FLAG_NONE},
	/* nocatd */
	{ "url",	NULL,	OP_LENGTH(4, (256 -1)), OPT_FLAG_NONE},
	{ "ifname",	NULL,	OP_IFNAME, OPT_FLAG_NONE},
	/* bridge */
	{ "br_index",	NULL,	OP_RANGE(1, 10), OPT_FLAG_NONE},
	/* rsync */
	{"transaction-id", NULL, OP_RANGE(1, 65535), OPT_FLAG_NONE},
	{"var_del", NULL, OP_RANGE(1,1), OPT_FLAG_NONE},
	{ "shell-script",		NULL,	OP_SH_SCR(1, 128), OPT_FLAG_NONE},
	{ "rsync-task",		NULL,	OP_RSYNC_TASK(1, 128), OPT_FLAG_NONE},
	/* banner */
	{ "banner",		NULL,	OP_LENGTH(3, 512), OPT_FLAG_NONE},
	/* sms-content*/
	{ "sms-content",	NULL,	OP_LENGTH(1, 140), OPT_FLAG_NONE},

	/* gre */
	{ "gre-key",		NULL,	OP_RANGE(0, 2147483646), OPT_FLAG_NONE},

	{ "ipv6-address/prefix-length",	NULL,	OP_IP6ADDRPLEN, OPT_FLAG_NONE},
	{ "ipv6-address",	NULL,	OP_IP6ADDR, OPT_FLAG_NONE},
	{ "pd-label",	NULL,	OP_LENGTH(1, 32), OPT_FLAG_NONE},
	{ "prefix-label",	NULL,	OP_LENGTH(1, 32), OPT_FLAG_NONE},

	/*gnss*/
	{ "lon",NULL,OP_RANGE(-181, 181), OPT_FLAG_NONE },
	{ "lat",NULL,OP_RANGE(-91, 91), OPT_FLAG_NONE },

	{NULL, NULL, OP_NONE, OPT_FLAG_NONE }
};

/**
 *	Validate variable
 *	@param	name	variable name
 *	@param	value	variable value
 *	@return	0 for invalid, 1 for valid, -1 for not found
 */
char*
get_default(char* name)
{	
	struct opt_validate *opt = (struct opt_validate *) opt_validates;
	
	while (opt->nv){
		if (strcmp(opt->nv, name)==0) 
			return opt->deflt;
		opt++;
	}

	return NULL;
}

/**
 *	check whether the variable needs encryption
 *	@param	name	variable name
 *	@return	0 for no, 1 for yes
 */
int
need_crypt(const char* name)
{
	struct opt_validate *opt = (struct opt_validate *) opt_validates;
	
	while (opt->nv){
		if ((opt->flags & OPT_FLAG_CRYPT) && strcmp(opt->nv, name)==0)  return 1;
		opt++;
	}
	
	return 0;
}
