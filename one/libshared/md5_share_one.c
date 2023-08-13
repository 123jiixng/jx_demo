#include <stdio.h>
#include <string.h>
#include "md5_share.h"

#ifdef CHARSET_EBCDIC
#include <openssl/ebcdic.h>
#endif

unsigned char *_DoMD5(const unsigned char *d, unsigned long n, unsigned char *md)
	{
	ST_MD5_CTX c;
	static unsigned char m[MD5_DIGEST_LENGTH];

	if (md == NULL) md=m;
	Md5Init(&c);
#ifndef CHARSET_EBCDIC
	Md5Update(&c,d,n);
#else
	{
		char temp[1024];
		unsigned long chunk;

		while (n > 0)
		{
			chunk = (n > sizeof(temp)) ? sizeof(temp) : n;
			ebcdic2ascii(temp, d, chunk);
			Md5Update(&c,temp,chunk);
			n -= chunk;
			d += chunk;
		}
	}
#endif
	Md5Final(md,&c);
	memset(&c,0,sizeof(c)); /* security consideration */
	return(md);
	}
/*
static char *MD5print(unsigned char *md)
	{
	int i;
	static char buf[80];

	for (i=0; i<MD5_DIGEST_LENGTH; i++)
		sprintf(&(buf[i*2]),"%02x",md[i]);
	return(buf);
	}
*/
//add by shandy

unsigned char *DoMD5(const unsigned char *d, unsigned long n, unsigned char *md)
{
	unsigned char*	pResult;
	static char buf[80];
	int i;

	pResult = _DoMD5(d,n,NULL);
	if (md == NULL) md = (unsigned char *)buf;

	for (i=0; i<MD5_DIGEST_LENGTH; i++)
		sprintf((char *)&(md[i*2]), "%02X", pResult[i]);

	return md;
}


