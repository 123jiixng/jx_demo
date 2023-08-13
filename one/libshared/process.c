/*
 * $Id$ --
 *
 *   Generic routines for process
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

#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#ifdef WIN32
#include <WINSOCK2.H>
#else
#include <unistd.h>
#include <dirent.h>
#endif//WIN32

#include <stdarg.h>
#include <syslog.h>
#include "shared.h"

#define PPID_MARK "PPid:" 

//# cat /proc/1/stat
//1 (init) S 0 0 0 0 -1 256 287 10043 109 21377 7 110 473 1270 9 0 0 0 27 1810432 126 2147483647 4194304 4369680 2147450688 2147449688 717374852 0 0 0 514751 2147536844 0 0 0 0

char *psname(int pid, char *buffer, int maxlen)
{
	char buf[512];
	char path[64];
	char *p;
	
	if (maxlen <= 0) return NULL;
	*buffer = 0;
	sprintf(path, "/proc/%d/stat", pid);
	if ((f_read_string(path, buf, sizeof(buf)) > 4) && ((p = strrchr(buf, ')')) != NULL)) {
		*p = 0;
		if (((p = strchr(buf, '(')) != NULL) && (atoi(buf) == pid)) {
			strlcpy(buffer, p + 1, maxlen);
		}
	}
	return buffer;
}

static int _pidof(const char *name, pid_t** pids)
{
	const char *p;
	int count;
#ifndef WIN32
	char *e;
	pid_t i;
	char buf[256];
	DIR *dir;
	struct dirent *de;
#endif//WIN32
	
	count = 0;
	*pids = NULL;
	if ((p = strchr(name, '/')) != NULL) name = p + 1;
#ifndef WIN32
	if ((dir = opendir("/proc")) != NULL) {
		while ((de = readdir(dir)) != NULL) {
			i = strtol(de->d_name, &e, 10);
			if (*e != 0) continue;
			if (strcmp(name, psname(i, buf, sizeof(buf))) == 0) {
				if ((*pids = realloc(*pids, sizeof(pid_t) * (count + 1))) == NULL) {
					closedir(dir);
					return -1;
				}
				(*pids)[count++] = i;
			}
		}
		closedir(dir);        
	}
#endif
	
	return count;
}

int pidof(const char *name)
{
	pid_t *pids;
	pid_t p;
	
	if (_pidof(name, &pids) > 0) {
		p = *pids;
		free(pids);
		return p;
	}
	return -1;
}

int killall(const char *name, int sig)
{
	pid_t *pids;
	int i;
	int r;
	
	if ((i = _pidof(name, &pids)) > 0) {
		r = 0;
		do {
			r |= kill(pids[--i], sig);
		} while (i > 0);
		free(pids);
		return r;
	}
	return -2;
}

int daemon(int nochdir, int noclose)
{
	int fd;

	switch (fork()) {
	case -1:
		return (-1);
	case 0:
		break;
	default:
		_exit(0);
	}

	if (setsid() == -1)
		return -1;

	if (!nochdir)
		(void)chdir("/");

	if (!noclose && (fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
		(void)dup2(fd, STDIN_FILENO);
		(void)dup2(fd, STDOUT_FILENO);
		(void)dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO)
			(void)close(fd);
	}

	return getpid();
}

int start_program(pid_t *ppid, const char *cmd, ...)
{
	va_list ap;
	char *argv[MAX_PROG_ARGS];
	int argc = 0, sig, status=0;
	char *p;
	pid_t pid;
	
	argv[argc++] = (char *)cmd;
	
	va_start(ap, cmd);
	do{
		p = va_arg(ap, char *);
		argv[argc++] = p;
		if(!p) break;
		//syslog(LOG_DEBUG, "  argv[%d] = %s", argc-1, argv[argc-1]);
	} while(argc<MAX_PROG_ARGS);
	
	if (argc>=MAX_PROG_ARGS) {
		syslog(LOG_ERR, "too many parameters (%d) when executing %s", argc, cmd);
		argv[MAX_PROG_ARGS-1] = NULL;
	}
	
	va_end(ap);
	
	pid = fork();
	if (pid < 0) {
		syslog(LOG_ERR, "fail to fork child.");
		return -1;
	} else if(pid != 0) {
		//parent
		if (ppid) {
			*ppid = pid;
		}
		
		waitpid(pid, &status, 0);
		return status;
	}

	//child
	// reset signal handlers
	for (sig = 0; sig < (_NSIG - 1); sig++)
		 signal(sig, SIG_DFL);
	
	execvp(argv[0], argv);

	return 0;
}

pid_t ppid_of(pid_t pid)
{
	char path[64] = {0};
	char buf[MAX_BUFF_LEN] = {0};
	FILE *pf = NULL;
	pid_t ppid = -1;
	char *p = NULL;

	if(pid < 0){
		return ppid; 
	}

	sprintf(path, "/proc/%d/status", pid);
	if ((pf = fopen(path,"r")) == NULL){
		return ppid;
	}

	while (fgets(buf,sizeof(buf),pf) != NULL){
		if ((p = strstr(buf,PPID_MARK)) == NULL){
			continue;
		}

		ppid = atoi(p+strlen(PPID_MARK));

		fclose(pf);
		return ppid;
	}

	fclose(pf);
	return ppid;
}

void log_caller_process(void)
{
	pid_t ppid = -1;
	char path[64] = {0};
	char caller[512] = {0};
	char *process_name = NULL;
	int len = 0;
	int total_len = 0;

	total_len = sizeof(caller);

	ppid = getpid();
	while(ppid > 0 ){

		snprintf(path, sizeof(path), "/proc/%d/comm", ppid);
		process_name = file2str(path);

		len += snprintf(caller + len, total_len-len, "<-%s:%d", trim_str(process_name), ppid);
		if(process_name){
			free(process_name);
		}

		ppid = ppid_of(ppid);
	}

	syslog(LOG_NOTICE,"%s", caller);

	return;
}

/*
int main(int argc, char **argv)
{
	int p;
	char buf[64];
	
	if (argc != 2) return 0;
	p = pidof(argv[1]);
	printf("pidof = %d\n", p);
	if (p > 1) {
		printf("psname = %s\n", psname(p, buf, sizeof(buf)));
		killall(argv[1], SIGTERM);
	}
}
*/
