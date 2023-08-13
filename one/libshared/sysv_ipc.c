/*
 * $Id$ --
 *
 *   sysV IPC routines
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
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#ifdef WIN32
typedef int	key_t;
#define	SHM_RDONLY	0
#else
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
//#include <linux/sem.h>
#endif//!WIN32

/* arg for semctl system calls. */
union semun {
	int val;                        /* value for SETVAL */
	struct semid_ds *buf;           /* buffer for IPC_STAT & IPC_SET */
	unsigned short *array;          /* array for GETALL & SETALL */
	struct seminfo *__buf;          /* buffer for IPC_INFO */
	void *__pad;
};

#include "shared.h"

/**
 * Create a shared memory
 * @param	name	memory name, can be a pathname
 * @param	nsize	memory size
 * @return	the memory id. -1 for failure
 */
int 
shm_new(char* name, size_t nsize)
{
#ifdef WIN32
	return (int)nsize;
#else
	int shm_id;
	key_t key = ftok(name, 'a');
	
//	syslog(LOG_DEBUG, "try to create the shared memory %s with key 0x%x", name, key);

	shm_id = shmget(key, nsize, IPC_CREAT|IPC_EXCL);	
	if (shm_id<0){
//		syslog(LOG_INFO, "somebody has created the shared memory %s", name);
		shm_id = shmget(key, nsize, IPC_CREAT);
		if(shm_id<0){
			syslog(LOG_ERR, "failed to create shared memory %s", name);
		}
	}else{
		void* p = shm_attach(shm_id, NULL, 0);

		if (p){
			memset(p, 0, nsize);
			shm_detach(p);
		}else{
			syslog(LOG_ERR, "failed to initialize shared memory %s", name);
		}
	}

	return shm_id;
#endif//!WIN32
}

/**
 * Attach to a shared memory
 * @param	shm_id		memory id
 * @param	shm_addr	memory address to attach to
 * @param	flags		attaching flags
 * @return	the attached address. NULL for failure
 */
void *
shm_attach(int shm_id, const void* shm_addr, int flags)
{
#ifdef WIN32
	char *p;

	if(shm_id == 0) return NULL;

	p = malloc(shm_id);
	if(p) memset(p, 0, shm_id);

	return p;
#else
	return shmat(shm_id, shm_addr, flags);
#endif//WIN32
}

/**
 * Detach a shared memory
 * @param	shm_addr	memory address to detach
 * @return	>=0 for sucess, <0 for failure
 */
int
shm_detach(void* shm_addr)
{
#ifdef WIN32
	if(!shm_addr) return -1;
	
	free(shm_addr);

	return 0;
#else
	return shmdt(shm_addr);
#endif//WIN32
}

/**
 * Delete a shared memory
 * @param	key		protect key
 * @param	nsize	memory size
 * @return	the memory id. -1 for failure
 */
int
shm_delete(int shm_id)
{
#ifdef WIN32
	if(shm_id==0) return -1;

	return 0;
#else
	return shmctl(shm_id, IPC_RMID, NULL);	
#endif//!WIN32
}


/**
 * Create an semaphore set
 * @param	name	semaphore set name, can be a pathname
 * @param	nsize	max num of semaphores
 * @return	the semaphore set id. -1 for failure
 */
int
sem_new(char* name, size_t nsize)
{
#ifdef WIN32
	return (int)nsize;
#else
	int sem_id;
	struct semid_ds sem_info;
//	struct seminfo sem_info2;
	union semun arg; 
	int i, init_ok = 0;
	key_t key;
	
	key = ftok(name, 'a');

#define MAX_RETRIES 3

	sem_id = semget(key, nsize, IPC_CREAT|IPC_EXCL|00666);

	if (sem_id<0){
		if (errno==EEXIST){
			sem_id = semget(key, nsize, IPC_CREAT|00666);

			//flag2 只包含了IPC_CREAT标志, 参数nsems(这里为1)必须与原来的信号灯数目一致
			arg.buf = &sem_info;
			for (i=0; i<MAX_RETRIES; i++)
			{
				if (semctl(sem_id, 0, IPC_STAT, arg) == -1){
					syslog(LOG_ERR, "failed to call semctl for %s", name);
					break;
				}else{ 
					if (arg.buf->sem_otime != 0){
						init_ok = 1;
						break;
					}else{
						//syslog(LOG_ERR, "wait for init ok...%d", i+1);
						//sleep(1);	
						init_ok = 1;
						break; //FIXME: shandy, todo
					}
				}
			}

			// do some initializing, here we assume that the first process that creates the sem will 
			// finish initialize the sem and run semop in max_tries*1 seconds. else it will not run 
			// semop any more.
			if (!init_ok){
				syslog(LOG_ERR, "failed to init semaphore set for %s", name);
				arg.val = 1;
				if (semctl(sem_id, 0, SETVAL, arg) == -1){
					syslog(LOG_ERR, "failed to call semctl for %s", name);
				}
			}
		} else {
			syslog(LOG_ERR, "failed to call semget for %s", name);
		}
	}else{
		//do some initializing 	
		arg.val = 1;
		if (semctl(sem_id, 0, SETVAL, arg) == -1)
			syslog(LOG_ERR, "failed to call semctl for %s", name);
	}

	return sem_id;
#endif//!WIN32
}

/**
 * Delete an semaphore set
 * @param	sem_id		semaphore set id
 * @return	0 for success, -1 for failure
 */
int
sem_delete(int sem_id)
{
#ifdef WIN32
	return 0;
#else
	return semctl(sem_id, 0, IPC_RMID);
#endif//!WIN32
}


/**
 * Operate an semaphore
 * @param	sem_id		semaphore set id
 * @param	nid			operate the nid-th semaphore
 * @param	op			operation number (-1 for lock, 1 for unlock)
 * @param	timeout		wait timeout in 100*ms, 0 for no waiting, <0 for no timeout
 * @return	0 for success, -1 for failure, 1 for timeout
 */
int
sem_op(int sem_id, int nid, int op, int timeout)
{
#ifdef WIN32
	return 0;
#else
	struct sembuf sop;
	int ret;
	
	sop.sem_num = nid;
	sop.sem_op = op;
	sop.sem_flg = SEM_UNDO | (timeout<0 ? 0 : IPC_NOWAIT);

/*
	//no semtimedop in arm linux?
	if (timeout>=0){
		to.tv_sec = timeout/1000;
		to.tv_nsec = (timeout%1000)*1000;
	}

	ret = semtimedop(sem_id, &sop, 1, timeout>0 ? &to : NULL);
*/

	if(timeout<0) return semop(sem_id, &sop, 1);

	do{
		ret = semop(sem_id, &sop, 1);
		if(ret==0) return 0;
		if(ret<0 && errno!=EAGAIN) return -1;
		usleep(100000);
	}while(timeout>0);

	if (timeout>=0 && errno==EAGAIN) return 1; 

	return -1;
#endif//WIN32
}

/**
 * Lock an semaphore
 * @param	sem_id		semaphore set id
 * @param	nid			operate the nid-th semaphore
 * @param	timeout		wait timeout in millisecond, 0 for no timeout
 * @return	0 for success, -1 for failure, 1 for timeout
 */
//#define sem_lock(sem_id, nid, op, timeout) sem_op((sem_id), (nid), -1, (timeout))


/**
 * Unlock an semaphore
 * @param	sem_id		semaphore set id
 * @param	nid			operate the nid-th semaphore
 * @param	timeout		wait timeout in millisecond, 0 for no timeout
 * @return	0 for success, -1 for failure, 1 for timeout
 */
//#define sem_unlock(sem_id, nid, op, timeout) sem_op((sem_id), (nid), 1, (timeout))


