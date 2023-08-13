/*
 * $Id$ --
 *
 *   RTC routines
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <unistd.h>
#include <linux/rtc.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include "build_info.h"

int saveRtc(void)
{
	struct tm timenow;
	time_t now;
	int fd,err;

#ifdef INHAND_ER3
	if ((fd = open("/dev/rtc1",O_RDWR)) >= 0) {
	} else
#endif
	if ((fd = open("/dev/rtc0",O_RDWR)) < 0) {
		perror("unable to open rtc");
		return -1;
	}

	now = time(NULL);
	gmtime_r(&now, &timenow);

#if 0
	printf("current time: %d-%d-%d %d:%d:%d\n",
		timenow.tm_year+1900,
		timenow.tm_mon+1,
		timenow.tm_mday,
		timenow.tm_hour,
		timenow.tm_min,
		timenow.tm_sec);
#endif

	err = ioctl(fd, RTC_SET_TIME, &timenow);
	if (err) {
		perror("unable to set rtc");
		close(fd);
		return err;
	}
	
	close(fd);
	return 0;
}

int getRtc(time_t *now)
{
	struct tm timenow;
	int fd,err;

#ifdef INHAND_ER3
	if ((fd = open("/dev/rtc1",O_RDWR)) >= 0) {
	} else
#endif
	if ((fd = open("/dev/rtc0",O_RDWR)) < 0) {
		perror("unable to open rtc");
		return -1;
	}

	err = ioctl(fd, RTC_RD_TIME, &timenow);
	if (err) {
		perror("unable to get rtc");
		close(fd);
		return err;
	}
	close(fd);

#if 0
	printf("current time: %d-%d-%d %d:%d:%d\n",
			timenow.tm_year+1900,
			timenow.tm_mon+1,
			timenow.tm_mday,
			timenow.tm_hour,
			timenow.tm_min,
			timenow.tm_sec);
#endif	

	if (now) *now = mktime(&timenow);
	return 0;
}
