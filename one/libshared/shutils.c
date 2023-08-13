/*
 * $Id$ --
 *
 *   Generic routines for shell utilities
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
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#include <process.h>
#else
#include <syslog.h>
#ifndef __MUSL__
#include <error.h>
#endif
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/ethernet.h>
#endif//WIN32

#include "shared.h"
#include "ih_cmd.h"

#ifdef WIN32
#define validate_char(x) (((x)>='0' && (x)<='9') || (x)=='_' || (x)==':' || (x)=='-' || (x)=='/' || (x)=='\\' || (x)=='\'' || (x)=='\"' || (x)=='.' || (x)==',' || ((x)>='a' && (x)<='z') || ((x)>='A' && (x)<='Z'))

int validate_string(char* s)
{
	while(*s){
		if(!validate_char(*s)) return 0;
		s++;
	}

	return 1;
}

int eval(char* cmd, ...)
{
	va_list list;
//	va_list start;
	char *argv[32];
	int argc = 0;
	char *p;

	va_start(list, cmd);

	argv[argc++] = cmd;	
	do{
		p = va_arg(list, char *);
		if(!p || p==(char*)0xcccccccc || !validate_string(p)) break; //FIXME: we cannot get end :(
		argv[argc++] = p;
	}while(1);

	argv[argc++] = NULL;
	va_end(list);

	return _eval(argv, NULL, 0, NULL);
}

#endif//WIN32

static char g_args[MAX_ARGS][MAX_ARG_LEN];
static char* g_argv[MAX_ARGS];

#ifndef WIN32
/* Set terminal settings to reasonable defaults */
static void set_term(int fd)
{
        struct termios tty;

	tcgetattr(fd, &tty);
	/* set control chars */
    tty.c_cc[VINTR]  = 3;   /* C-c */
    tty.c_cc[VQUIT]  = 28;  /* C-\ */
    tty.c_cc[VERASE] = 127; /* C-? */
    tty.c_cc[VKILL]  = 21;  /* C-u */
    tty.c_cc[VEOF]   = 4;   /* C-d */
    tty.c_cc[VSTART] = 17;  /* C-q */
    tty.c_cc[VSTOP]  = 19;  /* C-s */
    tty.c_cc[VSUSP]  = 26;  /* C-z */
    /* use line dicipline 0 */
    tty.c_line = 0;
    /* Make it be sane */
	tty.c_cflag &= CBAUD|CBAUDEX|CSIZE|CSTOPB|PARENB|PARODD;
	tty.c_cflag |= CREAD|HUPCL|CLOCAL;
        /* input modes */
	tty.c_iflag = ICRNL | IXON | IXOFF;
	/* output modes */
	tty.c_oflag = OPOST | ONLCR;
        /* local modes */
        tty.c_lflag =
                ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE | IEXTEN;
        tcsetattr(fd, TCSANOW, &tty);
}
#endif//!WIN32

/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * // @param	path	NULL, ">output", or ">>output"
 * @param	path	NULL, "path". redirect stdin,stdout,stderr to <path>
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @param	ppid	NULL to wait for child termination or pointer to pid
 * @return	return value of executed command or errno
 */
int _eval(char *const argv[], const char *path, int timeout, int *ppid)
{
	pid_t pid;
	int status = 0;
	int i, n;
//	const char *p;
	//char tz[16];
	//char s[256];
#ifndef WIN32
	int fd;
	int flags;
	int sig;
	sigset_t block_set;
#endif

	//get a global copy to avoid a libc bug (triggered by syslog)
	//TRACE("eval:");
    for(i=0; i < MAX_ARGS && argv[i]; i++){
		strcpy(g_args[i], argv[i]);
		//TRACE(" argv[%d]: %s", i, g_args[i]);
		g_argv[i] = g_args[i];
	}
	if(i==MAX_ARGS){
		syslog(LOG_ERR, "too many args!");
		return -1;
	}

	g_argv[i] = NULL;

	pid = fork();
	if (pid == -1) {
		syslog(LOG_ERR,"failed to fork child for %s!", argv[0]);
		//perror("fork");
		return errno;
	}
	
	if (pid != 0) {
		// parent
		if (ppid) {
			*ppid = pid;
			return 0;
		}
		
		waitpid(pid, &status, 0);
#ifndef WIN32
		if (WIFEXITED(status)) return WEXITSTATUS(status);
#endif//!WIN32
		return status;
	}
	
	// child
#ifndef WIN32
	// reset signal handlers
	for (sig = 0; sig < (_NSIG - 1); sig++)
		signal(sig, SIG_DFL);

	sigfillset(&block_set);//add all signal
	sigprocmask(SIG_UNBLOCK, &block_set, NULL);//block signal

	setsid();

	/*when path is "std", means donot direct to null*/
	if(!path || strcmp(path, "std")) {
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	
		open("/dev/null", O_RDONLY);
		open("/dev/null", O_WRONLY);
		open("/dev/null", O_WRONLY);
	}

#endif//WIN32

#if 0//disabled by shandy
//	if (nvram_match("debug_logeval", "1")) {
	if(1){
		char s[128];

		pid = getpid();

		//TRACE("_eval %d ", pid);
		//for (n = 0; argv[n]; ++n) TRACE("%s ", argv[n]);

#if 0//ndef WIN32		
		if ((fd = open("/dev/ttyAM0", O_RDWR)) >= 0) {
		//if ((fd = open("/dev/console", O_RDWR)) >= 0) {
			dup2(fd, STDIN_FILENO);
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
		}
		else {
			sprintf(s, "/tmp/eval.%d", pid);
			if ((fd = open(s, O_CREAT|O_RDWR, 0600)) >= 0) {
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
			}
		}
		if (fd > STDERR_FILENO) close(fd);
#endif//!WIN32
	}
#endif//1

#if 0 //commented by zly
	// Redirect stdout to <path>
	if (path) {
		flags = O_WRONLY | O_CREAT;
		if (*path == '>') {
			++path;
			if (*path == '>') {
				++path;
				// >>path, append
				flags |= O_APPEND;
			}
			else {
				// >path, overwrite
				flags |= O_TRUNC;
			}
		}
		
		if ((fd = open(path, flags, 0644)) < 0) {
			perror(path);
		}
		else {
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
	}
#endif	
	//Redirect stdin,stdout,stderr to <path>
#ifndef WIN32
	if(path && *path && strcmp(path, "std")) {
		if (*path == '>') {
			flags = O_WRONLY | O_CREAT;
			++path;
			if (*path == '>') {
				++path;
				// >>path, append
				flags |= O_APPEND;
			}
			else {
				// >path, overwrite
				flags |= O_TRUNC;
			}
			if ((fd = open(path, flags, 0644)) < 0) {
				syslog(LOG_ERR, "can not open %s\n", path);
			}
			else {
				dup2(fd, STDOUT_FILENO);
				close(fd);
			}
		}
		else{
			/* Clean up */
	        ioctl(0, TIOCNOTTY, 0);
	        close(0);
	        close(1);
	        close(2);
	        setsid();
	
			if ((fd = open(path, O_RDWR, 0644)) <0 ) {
			/* Avoid debug messages is redirected to socket packet if no exist a UART chip, added by honor, 2003-12-04 */
				open("/dev/null", O_RDONLY);
				open("/dev/null", O_WRONLY);
				open("/dev/null", O_WRONLY);
				syslog(LOG_ERR, "can not open %s\n", path);
				return -1;
			} else {
				dup2(fd, STDIN_FILENO);
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				/*Because above close 0,1,2, so fd is 0. If close fd, 
				 * stdin will be closed.*/
				//	close(fd);//must not close fd, or login startup fail.
			}
			ioctl(0, TIOCSCTTY, 1);
			tcsetpgrp(0, getpgrp());
			set_term(0);
		}
	}
#endif//!WIN32

#if 0
	// execute command
	p = "/sbin:/bin:/usr/sbin:/usr/bin:.";
	setenv("PATH", p, 1);
	setenv("TZ",g_sysinfo.timezone,1);
#endif	

#ifdef WIN32
	if (1) {
		char cmd[MAX_SYS_CMD_LEN] = "";

		for(i=0; i < MAX_ARGS && argv[i]; i++){
			strcat(cmd, argv[i]);
			strcat(cmd, " ");
		}

		//TRACE("run cmd: %s", cmd);
		system(cmd);
	}

	return 0;
#endif	

	//alarm(timeout);
	//TRACE("call %s...", g_argv[0]);
	n = execvp(g_argv[0], g_argv);

	//openlog("service", LOG_PID, LOG_DAEMON);
	syslog(LOG_ERR, "failed to execute %s! ret: %d, err: %d, %s", argv[0], n, errno, strerror(errno));

	//perror(argv[0]);

#ifndef WIN32
	exit(errno);
#endif

	return 0;
}

int _eval_wait(char *const argv[], const char *path, int timeout)
{
	pid_t pid,wpid;
	int status;
	time_t start;

	pid = _eval(argv,path,timeout,NULL);
	if (pid < 0) return pid;

	start = get_uptime();
	do {
#if (!(defined INHAND_IP812))
		wpid = waitpid(pid, &status,WNOHANG|WUNTRACED|WCONTINUED);
#else
		wpid = waitpid(pid, &status,WNOHANG|WUNTRACED);
#endif
		if (wpid < 0)		return wpid;
		else if(pid==wpid)	return 0;

		if(timeout>0&&
		   get_uptime()-start>timeout) break;

		sleep(1);
	} while(timeout != 0);

	return -1;
}


#if 0
/*
 * Search a string backwards for a set of characters
 * This is the reverse version of strspn()
 *
 * @param	s	string to search backwards
 * @param	accept	set of chars for which to search
 * @return	number of characters in the trailing segment of s
 *		which consist only of characters from accept.
 */
static size_t
sh_strrspn(const char *s, const char *accept)
{
	const char *p;
	size_t accept_len = strlen(accept);
	int i;


	if (s[0] == '\0')
		return 0;

	p = s + strlen(s);
	i = 0;

	do {
		p--;
		if (memchr(accept, *p, accept_len) == NULL)
			break;
		i++;
	} while (p != s);

	return i;
}
#endif

// -----------------------------------------------------------------------------

/*
 * Kills process whose PID is stored in plaintext in pidfile
 * @param	pidfile	PID file
 * @param	sig		signal to be sent
 * @return	0 on success and errno on failure
 */
int kill_pidfile(const char *pidfile, int sig)
{
	FILE *fp;
	char buf[16];

	if ((fp = fopen(pidfile, "r")) != NULL) {
		if (fgets(buf, sizeof(buf), fp)) {
			pid_t pid = strtoul(buf, NULL, 0);
			fclose(fp);
			if(pid!=0 && pid!=(pid_t)-1) return kill(pid, sig);
			else return -1;
		}

		fclose(fp);
  	}

	return errno;
}


/*
 * write pid to pidfile
 * @param	pidfile	PID file
 * @param	pid		pid to be written
 * @return	0 on success and errno on failure
 */
int write_pidfile(const char *pidfile, pid_t pid)
{
	FILE *fp;
	char buf[16];

	sprintf(buf, "%u\n", pid);

	if ((fp = fopen(pidfile, "w")) != NULL) {
		fputs(buf, fp);
		fclose(fp);
		return 0;
  	}

	return errno;
}

/*
 * Read PID from pidfile
 * @param	pidfile	PID file
 * @return	>0 for pid, or <=0 for error
 */
pid_t read_pidfile(const char *pidfile)
{
	FILE *fp;
	char buf[16];

	if ((fp = fopen(pidfile, "r")) != NULL) {
		if (fgets(buf, sizeof(buf), fp)) {
			pid_t pid = strtoul(buf, NULL, 0);
			fclose(fp);
			return pid;
		}

		fclose(fp);
  	}

	return -1;
}


//the last arg should be NULL
int start_daemon(pid_t *ppid, const char *cmd, ...)
{
	va_list ap;
	char *argv[MAX_ARGS];
	int argc = 0;
	char *p;

	//TRACE("start daemon %s", cmd);

	argv[argc++] = (char *)cmd;

	va_start(ap, cmd);
	do{
		p = va_arg(ap, char *);
		argv[argc++] = p;
		if(!p) break;
		//TRACE("  argv[%d] = %s", argc-1, argv[argc-1]);
	}while(argc<MAX_ARGS);

	if(argc>=MAX_ARGS){
		syslog(LOG_ERR, "too many parameters (%d) when executing %s", argc, cmd);
		argv[MAX_ARGS-1] = NULL;
	}

	va_end(ap);

	return _eval(argv, NULL, 0, ppid);
}

int _xstart( char* const args[])
{
	pid_t pid;

	return _eval(args, NULL, 0, &pid);
}

int cli_pipecmd(const char *cmd)
{
	FILE *f = NULL;
	char buf[MAX_BUFF_LEN] = {0};
	int nr = 0;

	if ((f = popen(cmd, "r")) != NULL) {
		while ((nr = fread(buf, 1, sizeof(buf) - 1, f)) > 0) {
			buf[nr] = 0;
			ih_cmd_rsp_print("%s", buf);
		}
		pclose(f);
		return 1;
	}

	return 0;
}

