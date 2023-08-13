/*
 * $Id$ --
 *
 *   Generic routines for string
 *
 * Copyright (c) 2001-2010 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 06/12/2010
 * Author: Jianliang Zhang
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <syslog.h>

#include "strings.h"

const char *find_word(const char *buffer, const char *word)
{
	const char *p, *q;
	int n;
	
	n = strlen(word);
	p = buffer;
	while ((p = strstr(p, word)) != NULL) {
		if ((p == buffer) || (*(p - 1) == ' ') || (*(p - 1) == ',')) {
			q = p + n;
			if ((*q == ' ') || (*q == ',') || (*q == 0)) {
				return p;
			}
		}
		++p;
	}	
	return NULL;
}

/*
static void add_word(char *buffer, const char *word, int max)
{
	if ((*buffer != 0) && (buffer[strlen(buffer) - 1] != ' '))
		strlcat(buffer, " ", max);
	strlcat(buffer, word, max);
}
*/

/**
 * strtrim - Delete white chars from the right side
 * @str: string to be trimed
 */
char* trim_str(char *str)
{
	char *p;
	int len = strlen(str);

	if (len==0) return str;

	p = str + len - 1;

	while (p>=str) {
		if (isspace(*p)) {
			*p = '\0';
		}else{
			break;
		}
		p--;
	}

	return str;
}

/**
 * trim string from both side
 * @s: tring need to be trimed
 */
char *trim_word(char *s)                                                                                                                                                                                    
{                                                                                                                                                                                                     
    char *start;                                                                                                                                                                                      
    char *end;                                                                                                                                                                                        
    int len = strlen(s);                                                                                                                                                                              
                                                                                                                                                                                                      
    start = s;                                                                                                                                                                                        
    end = s + len - 1;                                                                                                                                                                                
                                                                                                                                                                                                      
    while (1)                                                                                                                                                                                         
    {                                                                                                                                                                                                 
        char c = *start;                                                                                                                                                                              
        if (!isspace(c))                                                                                                                                                                              
            break;                                                                                                                                                                                    
                                                                                                                                                                                                      
        start++;                                                                                                                                                                                      
        if (start > end)                                                                                                                                                                              
        {                                                                                                                                                                                             
            s[0] = '\0';                                                                                                                                                                              
            return s;                                                                                                                                                                                   
        }                                                                                                                                                                                             
    }                                                                                                                                                                                                 
                                                                                                                                                                                                      
                                                                                                                                                                                                      
    while (1)                                                                                                                                                                                         
    {                                                                                                                                                                                                 
        char c = *end;                                                                                                                                                                                
        if (!isspace(c))                                                                                                                                                                              
            break;                                                                                                                                                                                    
                                                                                                                                                                                                      
        end--;                                                                                                                                                                                        
        if (start > end)                                                                                                                                                                              
        {                                                                                                                                                                                             
            s[0] = '\0';                                                                                                                                                                              
            return s;                                                                                                                                                                                   
        }                                                                                                                                                                                             
    }                                                                                                                                                                                                 
                                                                                                                                                                                                      
    memmove(s, start, end - start + 1);                                                                                                                                                               
    s[end - start + 1] = '\0';                                                                                                                                                                        
	return s;
}  


/**
 * strlcpy - Copy a %NUL terminated string into a sized buffer
 * @dest: Where to copy the string to
 * @src: Where to copy the string from
 * @size: size of destination buffer
 *
 * Compatible with *BSD: the result is always a valid
 * NUL-terminated string that fits in the buffer (unless,
 * of course, the buffer size is zero). It does not pad
 * out the result like strncpy() does.
 */
size_t strlcpy(char *dest, const char *src, size_t size)
{
	size_t ret = strlen(src);

	if (size) {
		size_t len = (ret >= size) ? size-1 : ret;
		memcpy(dest, src, len);
		dest[len] = '\0';
	}
	return ret;
}

/**
 * strtrimcpy - Copy a string into a buffer and remove the end space
 * @dest: Where to copy the string to
 * @src: Where to copy the string from
 *
 */
size_t strtrimcpy(char *dest, const char *src)
{
	size_t ret = strlen(src);

	strlcpy(dest, src, ret+1);
	trim_str(dest);

	return ret;
}

/**
 * strlcat - Append a length-limited, %NUL-terminated string to another
 * @dest: The string to be appended to
 * @src: The string to append to it
 * @count: The size of the destination buffer.
 */
size_t strlcat(char *dest, const char *src, size_t count)
{
	size_t dsize = strlen(dest);
	size_t len = strlen(src);
	size_t res = dsize + len;

	/* This would be a bug */
//	BUG_ON(dsize >= count);

	dest += dsize;
	count -= dsize;
	if (len >= count)
		len = count-1;
	memcpy(dest, src, len);
	dest[len] = 0;
	return res;
}



#ifdef WIN32 


/**
 * strnicmp - Case insensitive, length-limited string comparison
 * @s1: One string
 * @s2: The other string
 * @len: the maximum number of characters to compare
 */
int strnicmp(const char *s1, const char *s2, size_t len)
{
	/* Yes, Virginia, it had better be unsigned */
	unsigned char c1, c2;

	c1 = 0;	c2 = 0;
	if (len) {
		do {
			c1 = *s1; c2 = *s2;
			s1++; s2++;
			if (!c1)
				break;
			if (!c2)
				break;
			if (c1 == c2)
				continue;
			c1 = tolower(c1);
			c2 = tolower(c2);
			if (c1 != c2)
				break;
		} while (--len);
	}
	return (int)c1 - (int)c2;
}

/**
 * strnchr - Find a character in a length limited string
 * @s: The string to be searched
 * @count: The number of characters to be searched
 * @c: The character to search for
 */
char *strnchr(const char *s, size_t count, int c)
{
	for (; count-- && *s != '\0'; ++s)
		if (*s == (char) c)
			return (char *) s;
	return NULL;
}

/**
 * strnlen - Find the length of a length-limited string
 * @s: The string to be sized
 * @count: The maximum number of bytes to search
 */
size_t strnlen(const char * s, size_t count)
{
	const char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

/**
 * strspn - Calculate the length of the initial substring of @s which only
 * 	contain letters in @accept
 * @s: The string to be searched
 * @accept: The string to search for
 */
size_t strspn(const char *s, const char *accept)
{
	const char *p;
	const char *a;
	size_t count = 0;

	for (p = s; *p != '\0'; ++p) {
		for (a = accept; *a != '\0'; ++a) {
			if (*p == *a)
				break;
		}
		if (*a == '\0')
			return count;
		++count;
	}

	return count;
}

/**
 * strcspn - Calculate the length of the initial substring of @s which does
 * 	not contain letters in @reject
 * @s: The string to be searched
 * @reject: The string to avoid
 */
size_t strcspn(const char *s, const char *reject)
{
	const char *p;
	const char *r;
	size_t count = 0;

	for (p = s; *p != '\0'; ++p) {
		for (r = reject; *r != '\0'; ++r) {
			if (*p == *r)
				return count;
		}
		++count;
	}

	return count;
}	

/**
 * strpbrk - Find the first occurrence of a set of characters
 * @cs: The string to be searched
 * @ct: The characters to search for
 */
char * strpbrk(const char * cs,const char * ct)
{
	const char *sc1,*sc2;

	for( sc1 = cs; *sc1 != '\0'; ++sc1) {
		for( sc2 = ct; *sc2 != '\0'; ++sc2) {
			if (*sc1 == *sc2)
				return (char *) sc1;
		}
	}
	return NULL;
}

/**
 * strsep - Split a string into tokens
 * @s: The string to be searched
 * @ct: The characters to search for
 *
 * strsep() updates @s to point after the token, ready for the next call.
 *
 * It returns empty tokens, too, behaving exactly like the libc function
 * of that name. In fact, it was stolen from glibc2 and de-fancy-fied.
 * Same semantics, slimmer shape. ;)
 */
char * strsep(char **s, const char *ct)
{
	char *sbegin = *s, *end;

	if (sbegin == NULL)
		return NULL;

	end = strpbrk(sbegin, ct);
	if (end)
		*end++ = '\0';
	*s = end;

	return sbegin;
}

/**
 * memset - Fill a region of memory with the given value
 * @s: Pointer to the start of the area.
 * @c: The byte to fill the area with
 * @count: The size of the area.
 *
 * Do not use memset() to access IO space, use memset_io() instead.
 */
void * memset(void * s,int c,size_t count)
{
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}

/**
 * bcopy - Copy one area of memory to another
 * @srcp: Where to copy from
 * @destp: Where to copy to
 * @count: The size of the area.
 *
 * Note that this is the same as memcpy(), with the arguments reversed.
 * memcpy() is the standard, bcopy() is a legacy BSD function.
 *
 * You should not use this function to access IO space, use memcpy_toio()
 * or memcpy_fromio() instead.
 */
void bcopy(const void * srcp, void * destp, size_t count)
{
	const char *src = srcp;
	char *dest = destp;

	while (count--)
		*dest++ = *src++;
}

/**
 * memcpy - Copy one area of memory to another
 * @dest: Where to copy to
 * @src: Where to copy from
 * @count: The size of the area.
 *
 * You should not use this function to access IO space, use memcpy_toio()
 * or memcpy_fromio() instead.
 */
void * memcpy(void * dest,const void *src,size_t count)
{
	char *tmp = (char *) dest, *s = (char *) src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

/**
 * memmove - Copy one area of memory to another
 * @dest: Where to copy to
 * @src: Where to copy from
 * @count: The size of the area.
 *
 * Unlike memcpy(), memmove() copes with overlapping areas.
 */
void * memmove(void * dest,const void *src,size_t count)
{
	char *tmp, *s;

	if (dest <= src) {
		tmp = (char *) dest;
		s = (char *) src;
		while (count--)
			*tmp++ = *s++;
		}
	else {
		tmp = (char *) dest + count;
		s = (char *) src + count;
		while (count--)
			*--tmp = *--s;
		}

	return dest;
}

/**
 * memcmp - Compare two areas of memory
 * @cs: One area of memory
 * @ct: Another area of memory
 * @count: The size of the area.
 */
int memcmp(const void * cs,const void * ct,size_t count)
{
	const unsigned char *su1, *su2;
	int res = 0;

	for( su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}

/**
 * memscan - Find a character in an area of memory.
 * @addr: The memory area
 * @c: The byte to search for
 * @size: The size of the area.
 *
 * returns the address of the first occurrence of @c, or 1 byte past
 * the area if @c is not found
 */
void * memscan(void * addr, int c, size_t size)
{
	unsigned char * p = (unsigned char *) addr;

	while (size) {
		if (*p == c)
			return (void *) p;
		p++;
		size--;
	}
  	return (void *) p;
}


/**
 * memchr - Find a character in an area of memory.
 * @s: The memory area
 * @c: The byte to search for
 * @n: The size of the area.
 *
 * returns the address of the first occurrence of @c, or %NULL
 * if @c is not found
 */
void *memchr(const void *s, int c, size_t n)
{
	const unsigned char *p = s;
	while (n-- != 0) {
        	if ((unsigned char)c == *p++) {
			return (void *)(p-1);
		}
	}
	return NULL;
}
#endif//WIN32

/**
 * strtoupper -translate a string to upper alpha string.
 * @str: The string to be searched
 * @size: the length of string
 */
void strtoupper(char *str, size_t size)
{
    char *p = NULL;

    if(!str){
        return;
    }

    p = str;
    while(*p){
        if(isalpha(*p) && islower(*p)){
            *p = toupper(*p);
        }

        p++;
    }
}

/**
 * memfind - Find a block in an area of memory.
 * @s: The memory area
 * @b: The block to search for
 * @n: The size of the area.
 * @n: The size of the block.
 *
 * returns the address of the first occurrence of @c, or %NULL
 * if @c is not found
 */
void *memfind(const void *s, size_t ns, const void *b, size_t nb)
{
	int n = ns - nb;
	const unsigned char *p = s;

	while(n-->=0){
		if(memcmp(p, b, nb)==0) 
			return (void*)p;
		p++;
	}

	return NULL;
}


int vstrsep(char *buf, const char *sep, ...)
{
	va_list ap;
	char **p;
	int n;

	n = 0;
	va_start(ap, sep);
	while ((p = va_arg(ap, char **)) != NULL) {
		if ((*p = strsep(&buf, sep)) == NULL) break;
		++n;
	}
	va_end(ap);

	return n;
}

int _vstrsep(char *buf, const char *sep, char** const args[])
{
	char **p;
	int n;
	int i = 0;

	//TRACE("vstrsep: %s", buf);

	n = 0;
	while ((p = args[i++]) != NULL) {
		if ((*p = strsep(&buf, sep)) == NULL) break;
		++n;
	}

	//TRACE("vstrsep: cnt=%d", n);

	return n;
}

/** @brief simple string match
 *
 * @param match		regular expression, only '?' and '*' are accepted, and '*' is accepted only once
 * @param str		string to be matched
 *
 * @return 0 for unmatched, !=0 for matched
 */
int strmatch(const char *match, const char *str)
{
	int i, x;

	for (i=0, x=0; match[i]; i++) {
		if (match[i]==str[x] || match[i]=='?') {
			x++;
			continue;
		}

		if (str[x]=='\0') {
			if (match[i]=='*') continue;
			return 0;
		}

		if (match[i]=='*') {
			int n1, n2;

			n1 = strlen(&match[i+1]);
			n2 = strlen(&str[x]);
			if (n1>n2) return 0;
			x += n2 - n1;
			continue;
		}

		return 0;
	}

	return (str[x]=='\0');
}

void str2bin(const char *ibuf, int ilen, char *obuf)
{
	int i;
	unsigned char x;

	for (i=0; i<ilen; i++, obuf++) {
		x = 0;
//		sscanf(ibuf, "%02X", &x);
//		ibuf += 2;
//		*obuf = (char)x;

		if (*ibuf>='0' && *ibuf<='9') x = *ibuf - '0';
		else if (*ibuf>='a' && *ibuf <='f') x = 10 + *ibuf - 'a';
		else if (*ibuf>='A' && *ibuf <='F') x = 10 + *ibuf - 'A';
		else x = 0;

		ibuf++;
		x = x << 4;

		if (*ibuf>='0' && *ibuf<='9') x |= *ibuf - '0';
		else if (*ibuf>='a' && *ibuf <='f') x |= 10 + *ibuf - 'a';
		else if (*ibuf>='A' && *ibuf <='F') x |= 10 + *ibuf - 'A';

		ibuf++;
		*obuf = (char)x;
	}
}

void bin2str(const unsigned char *ibuf, int ilen, char *obuf)
{
	int i;

	for (i=0; i<ilen; i++) {
		sprintf(obuf, "%02X", *(ibuf++));
		obuf += 2;
	}
}

/** @brief separate cmd line, find ' ' or '"'
 *
 * @param cmd	[in|out]	cmd line
 *
 * @return the first piece of the cmd
 */
char *cmdsep(char **cmd)
{
	char *p;
	char *pcmd = *cmd;
	int quote = 0;

	if (!pcmd) return NULL;

	SKIP_WHITE(pcmd);

	if (*pcmd == '\"') {
		p = strchr(pcmd+1, '\"');
		quote = 1;
	}else{
		p = strchr(pcmd, ' ');
	}

	if (!p) {
		*cmd = NULL;
		return pcmd;
	}

	if (quote) {
		p++;
		if (*p) {
			*p = '\0';
			p++;
			SKIP_WHITE(p);
			*cmd = p;
		}else{
			*cmd = NULL;
		}
		return pcmd;
	}

	*p = '\0';
	p++;
	SKIP_WHITE(p);
	*cmd = *p ? p : NULL;

	return pcmd;
}

#define IS_JSON_SPECIAL(x) ((x)=='\\' || (x)=='\"' || (x)=='\'' || (x)=='/' )
#define JSON_STR_BUF_SIZE	512
static char json_str[JSON_STR_BUF_SIZE];
extern char *get_json_str(char *str)
{
	uns16 len = 0;

	while (len < JSON_STR_BUF_SIZE - 1 && *str) {
		if (IS_JSON_SPECIAL(*str)) {
			json_str[len++] = '\\';
			if (len == JSON_STR_BUF_SIZE - 1) {
				json_str[len] = '\0';
				return json_str;
			}
		}

		json_str[len++] = *str++;
	}

	json_str[len] = '\0';
	return json_str;
}

int gen_serial_cmd(char *buf, int len, char *cmd)
{
	int size = 0 ;
	
	if(buf == NULL || cmd == NULL) return -1;
	if(len != SERIAL_CMD_LEN) return -1;
	size += snprintf(buf+size, len-size, SERIAL_CMD_HEAD);
	size += snprintf(buf+size, len-size, "%s", cmd);
	if(size <0 )return size;
	while(size < len){
		size += snprintf(buf+size, len-size, "#");
		if(size <0 )return size;
	}
	buf[SERIAL_CMD_LEN-2] = '&';
	buf[SERIAL_CMD_LEN-1] = '@';
	buf[SERIAL_CMD_LEN] = '\0';
	return 0;
}


char *get_string_by_num(int num, ENUM_STR *strings)
{
	ENUM_STR *p;
	if (num < 0 || !strings) {
		return NULL;
	}
	p = strings;

	while(p->str){
		if(p->num == num) {
			return (char *)p->str;
		}
		p++;
	}

	return NULL;
}

int get_num_by_string(char *str, ENUM_STR *strings)
{
	ENUM_STR *p;
	if (!str || !strings) {
		return -1;
	}
	p = strings;

	while(p->str){
		if(!strcmp(p->str, str)) {
			return p->num;
		}
		p++;
	}

	return -1;
}

/* -, ., blank, 0-9, a-z, A-Z*/
int is_valid_option_str(const char *str, int len)
{
	char *p = NULL;
	int n = 0;

	if (!str || len < 0) {
		syslog(LOG_INFO, "check valid str, invalid params");
		return 0;
	}

	p = str;
	while (n < len && *(p+n) != '\0') {
		if (*(p+n) != '-' && *(p+n) != '.' && !isalnum(*(p+n)) && !isblank(*(p+n))) {
			return 0;
		}

		n++;
	}

	//check ok
	return 1;
}

char *traffic_to_string(uns64 traffic, char *buf, int buf_len)
{
#define KB (1024)
#define MB (1024*1024)
#define GB (1024*1024*1024)
	if(!buf) {
		return NULL;
	}

	if(traffic < KB) {
		snprintf(buf, buf_len, "%7.2f B", (float)traffic);
	} else if(traffic >= KB && traffic < MB) {
		snprintf(buf, buf_len, "%7.2f KB", ((float)traffic)/KB);
	} else if (traffic >= MB && traffic < GB) {
		snprintf(buf, buf_len, "%7.2f MB", ((float)traffic)/MB);
	} else if (traffic >= GB) {
		snprintf(buf, buf_len, "%7.2f GB", ((float)traffic)/GB);
	}

	return buf;
}

size_t utf8_check_single(char byte)
{
    unsigned char u = (unsigned char)byte;

    if(u < 0x80)
        return 1;

    if(0x80 <= u && u <= 0xBF) {
        /* second, third or fourth byte of a multi-byte
           sequence, i.e. a "continuation byte" */
        return 0;
    }
    else if(u == 0xC0 || u == 0xC1) {
        /* overlong encoding of an ASCII byte */
        return 0;
    }
    else if(0xC2 <= u && u <= 0xDF) {
        /* 2-byte sequence */
        return 2;
    }

    else if(0xE0 <= u && u <= 0xEF) {
        /* 3-byte sequence */
        return 3;
    }
    else if(0xF0 <= u && u <= 0xF4) {
        /* 4-byte sequence */
        return 4;
    }
    else { /* u >= 0xF5 */
        /* Restricted (start of 4-, 5- or 6-byte sequence) or invalid
           UTF-8 */
        return 0;
    }
}

static size_t utf8_check_full(const char *buffer, size_t size, int32_t *codepoint)
{
    size_t i;
    int32_t value = 0;
    unsigned char u = (unsigned char)buffer[0];

    if(size == 2)
    {
        value = u & 0x1F;
    }
    else if(size == 3)
    {
        value = u & 0xF;
    }
    else if(size == 4)
    {
        value = u & 0x7;
    }
    else
        return 0;

    for(i = 1; i < size; i++)
    {
        u = (unsigned char)buffer[i];

        if(u < 0x80 || u > 0xBF) {
            /* not a continuation byte */
            return 0;
        }

        value = (value << 6) + (u & 0x3F);
    }

    if(value > 0x10FFFF) {
        /* not in Unicode range */
        return 0;
    }

    else if(0xD800 <= value && value <= 0xDFFF) {
        /* invalid code point (UTF-16 surrogate halves) */
        return 0;
    }

    else if((size == 2 && value < 0x80) ||
            (size == 3 && value < 0x800) ||
            (size == 4 && value < 0x10000)) {
        /* overlong encoding */
        return 0;
    }

    if(codepoint)
        *codepoint = value;

    return 1;
}

int utf8_check_string(const char *string, size_t length)
{
    size_t i;

    for(i = 0; i < length; i++)
    {
        size_t count = utf8_check_single(string[i]);
        if(count == 0)
            return 0;
        else if(count > 1)
        {
            if(count > length - i)
                return 0;

            if(!utf8_check_full(&string[i], count, NULL))
                return 0;

            i += count - 1;
        }
    }

    return 1;
}
