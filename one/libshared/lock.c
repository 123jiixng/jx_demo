/*
 * $Id$ --
 *
 *   Head file for MD5 lib
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
#include "ih_logtrace.h"
#include <syslog.h>

#include "shared.h"



int file_lock_by_id(int lock_id, int timeout)
{
	int ret;

	ret = sem_lock(lock_id, 0, timeout);
	if (ret!=0) {
		//tty_unlock(lock_id);
		/*Risk: can not delete it!!!*/
		//sem_delete(lock_id);
		return -1;
	}

	return lock_id;
}

/** @brief lock file by name
 * 
 * @param name    [in] file name
 * @param timeout [in] timeout in 100ms, -1 for infinite
 *
 * @return >0 for lock id, <0 for error. used for unlocking
 */
int file_lock(char *name, int timeout)
{
	int lock_id, ret;

	lock_id = sem_new(name, 1);
	//LOG_IN("LOCK name %s, ID %d", name, lock_id);
	ret = sem_lock(lock_id, 0, timeout);
	if (ret!=0) {
		//tty_unlock(lock_id);
		/*Risk: can not delete it!!!*/
		//sem_delete(lock_id);
		return -1;
	}

	return lock_id;
}

/** @brief unlock file
 * 
 * @param lock_id [in] lock id, returned by tty_lock
 *
 * @return 0
 */
int file_unlock(int lock_id)
{
	if (lock_id<0) return -1;
	
	sem_unlock(lock_id, 0);
	/*Risk: can not delete it!!!*/
	//sem_delete(lock_id);
	
	lock_id = -1;
	
	return 0;
}


void simple_unlock(const char *name)
{
	char fn[256];

#ifdef WIN32
	snprintf(fn, sizeof(fn), "%s.lock", name);
#else
	snprintf(fn, sizeof(fn), "/var/lock/%s.lock", name);
#endif
	f_write(fn, NULL, 0, 0, 0600);
}

void simple_lock(const char *name)
{
	int n;
	char fn[256];

	n = 5 + (getpid() % 10);
#ifdef WIN32
	snprintf(fn, sizeof(fn), "%s.lock", name);
#else
	snprintf(fn, sizeof(fn), "/var/lock/%s.lock", name);
#endif
	while (unlink(fn) != 0) {
		if (--n == 0) {
			syslog(LOG_INFO, "breaking lock %s", fn);
			break;
		}
		sleep(1);
	}
}
