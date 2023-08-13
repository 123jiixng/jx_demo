/*
 * $Id$ --
 *
 *   Generic routines for watchdog
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
#include <fcntl.h>
#include <stdlib.h>

#ifdef WIN32
	#include <io.h>
#else
	#include <unistd.h>
	#include <syslog.h>
#endif

#include "shared.h"

int open_watchdog(void)
{
	int fd;

	if ((fd=open("/dev/misc/wdt", O_WRONLY)) == -1){
		syslog(LOG_ERR, "cannot open watchdog!");
		return -1;
	}

	syslog(LOG_INFO, "watchdog started");
	write(fd, "\0", 1);

	return fd;
}

