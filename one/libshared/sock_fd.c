/*
 * $Id$ --
 *
 *   Generic routines for base64
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
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#ifdef WIN32
#include <io.h>
#include <WINSOCK2.H>
#include <process.h>
#include <direct.h>
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
#include <unistd.h>
#endif//WIN32

#include "shared.h"

int SockInit(void)
{
#ifdef WIN32
	//windows need additional socket initialization
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD( 2, 2 );

	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return -1;
	}

	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */

	if ( LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 2 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		WSACleanup();
		return -1;
	}
#endif//WIN32

	/* The WinSock DLL is acceptable. Proceed. */
	return 0;
}

void SockCleanup(void)
{
#ifdef WIN32
	WSACleanup();
#endif//WIN32
}

#ifdef WIN32

#define MAXBUFSIZE 1024

SOCK_FILEP sock_fdopen(int fd, char* mode)
{
	return fd;
}

int sock_fprintf(SOCK_FILEP sock, const char *fmt, ...)  
{  
	va_list args;  
	char temp[MAXBUFSIZE];  
	int ret;  
	va_start(args, fmt);  
	ret = vsprintf(temp, fmt, args);  
	va_end(args);  
	if(send(sock, temp, strlen(temp), 0) == SOCKET_ERROR)  
		return -1;  
	return ret;  
}  

char* sock_fgets(char *buf, int maxlen, SOCK_FILEP sock)  
{  
	int ret, count = 0;  
	char *tempbuf = buf;  
	while(1)  
	{  
		++count;  
		if(count > maxlen)  
			return buf;  
		ret = recv(sock, tempbuf, 1, 0);  
		if((ret == SOCKET_ERROR) || (ret <= 0))  
			return NULL;  
		if(*tempbuf == '\n')  
		{  
			++tempbuf;  
			*tempbuf = 0;  
			return buf;  
		}  
		++tempbuf;  
	}  
}  

int sock_fputs(SOCK_FILEP sock, const char *buf)  
{  
	int len = strlen(buf);

	if(send(sock, buf, len, 0) == SOCKET_ERROR)  
		return -1;  

	return len;  
}

char sock_fgetc(SOCK_FILEP sock)  
{  
	char c;
	int ret;

	ret = recv(sock, &c, 1, 0);

	if(ret<=0) return EOF;
	
	return c;
}
  

int sock_fputc(SOCK_FILEP sock, const char c)  
{  
	
	if(send(sock, &c, 1, 0) == SOCKET_ERROR)  
		return -1;  
	
	return 1;  
}  

int sock_fread(char *buf, int size, int n, SOCK_FILEP sock)  
{  	
	int ret;
	int i;

	for(i=0; i<n; i++){
		ret = recv(sock, buf, size, 0);
		if((ret == SOCKET_ERROR) || (ret <= 0)) {
			if (i > 0) {
				return i;
			} else {
				return ret;
			}
		}

		buf += ret;
	}

	if(i==0) return EOF;
	
	return i;  
}

int sock_fwrite(const char *buf, int size, int n, SOCK_FILEP sock)  
{  	
	int r = 0;
	r = send(sock, buf, size*n, 0); 
	if(r < 0)  
		return -1;  
	
	return r;
} 

int sock_fclose(SOCK_FILEP sock)  
{  
	return closesocket(sock);  
}  

int sock_fflush(SOCK_FILEP sock)  
{  
	return 0;
}

#endif//WIN32
