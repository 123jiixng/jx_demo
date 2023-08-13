/*
 * $Id$ --
 *
 *   Generic routines for file operations
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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#include <stdarg.h>
#endif//WIN32

#include "shared.h"


int f_exists(const char *path)	// note: anything but a directory
{
	struct stat st;
#ifdef WIN32
	return (stat(path, &st) == 0);
#else
	return (stat(path, &st) == 0) && (!S_ISDIR(st.st_mode));
#endif
}

int f_copy(const char *src_path,const char *dst_path)
{
	FILE *spf,*dpf;
	uns8 buf[2048];
	size_t total,i;

	spf = fopen(src_path,"rb");
	if (spf == NULL) return -1;

	dpf = fopen(dst_path,"wb");
	if (dpf == NULL) {
		fclose(spf);
		return -1;
	}

	total = 0;
	while ((i = fread(buf,1,sizeof(buf),spf)) > 0){
		fwrite(buf,i,1,dpf);
		total += i;
	} 

	fclose(spf);
	fclose(dpf);
	
	return (int)total;
}

long f_size(const char *path)	// 4GB-1	-1 = error
{
	struct stat st;
	if (stat(path, &st) == 0) return st.st_size;
	return -1;
}

int f_read(const char *path, void *buffer, int max)
{
	int f;
	int n;
	
	if ((f = open(path, O_RDONLY)) < 0) return -1;
	n = read(f, buffer, max);
	close(f);
	return n;
}

int f_write(const char *path, const void *buffer, int len, unsigned flags, unsigned cmode)
{
	static const char nl = '\n';
	int f;
	int r = -1;

#ifndef WIN32
	mode_t m;

	m = umask(0);
#endif	

	if (cmode == 0) cmode = 0666;
	if ((f = open(path, (flags & FW_APPEND) ? (O_WRONLY|O_CREAT|O_APPEND) : (O_WRONLY|O_CREAT|O_TRUNC), cmode)) >= 0) {
		if ((buffer == NULL) || ((r = write(f, buffer, len)) == len)) {
			if (flags & FW_NEWLINE) {
				if (write(f, &nl, 1) == 1) ++r;
			}
		}
		close(f);
	}

#ifndef WIN32
	umask(m);
#endif	

	return r;
}

int f_read_string(const char *path, char *buffer, int max)
{
	int n;

	if (max <= 0) return -1;
	n = f_read(path, buffer, max - 1);
	buffer[(n > 0) ? n : 0] = 0;
	return n;
}

int f_write_string(const char *path, const char *buffer, unsigned flags, unsigned cmode)
{
	return f_write(path, buffer, strlen(buffer), flags, cmode);
}

static int _f_read_alloc(const char *path, char **buffer, int max, int z)
{
	int n;

	*buffer = NULL;
	if (max >= 0) {
		if ((n = f_size(path)) != (unsigned long)-1) {
			if (n < max) max = n;
			if ((!z) && (max == 0)) return 0;
			if ((*buffer = malloc(max + z)) != NULL) {
				if ((max = f_read(path, *buffer, max)) >= 0) {
					if (z) *(*buffer + max) = 0;
					return max;
				}
				free(buffer);
			}
		}
	}
	return -1;
}

int f_read_alloc(const char *path, char **buffer, int max)
{
	return _f_read_alloc(path, buffer, max, 0);
}

int f_read_alloc_string(const char *path, char **buffer, int max)
{
	return _f_read_alloc(path, buffer, max, 1);
}


/*
 * Reads file and returns contents
 * @param	fd	file descriptor
 * @return	contents of file or NULL if an error occurred
 */
char *fd2str(int fd)
{
	char *buf = NULL;
	char *buf1 = NULL;
	size_t count = 0;
	ssize_t n = 0;

	do {
		buf1 = realloc(buf, count + 512);
		if (!buf1) {
			if (buf) {
				free(buf);
				buf = NULL;
			}
			return NULL;
		}

		buf =  buf1;
		n = read(fd, buf + count, 512);
		if (n < 0) {
			free(buf);
			buf = NULL;
			//FIXME don't return? the count will be polluted
		}
		count += n;
	} while (n == 512);

	close(fd);
	if (buf)
		buf[count] = '\0';
	return buf;
}

/*
 * Reads file and returns contents
 * @param	path	path to file
 * @return	contents of file or NULL if an error occurred
 */
char *file2str(const char *path)
{
	int fd;

	if ((fd = open(path, O_RDONLY)) == -1) {
		//perror(path);
		return NULL;
	}

	return fd2str(fd);
}

/*
 * Waits for a file descriptor to change status or unblocked signal
 * @param	fd	file descriptor
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @return	1 if descriptor changed status or 0 if timed out or -1 on error
 */

int waitfor(int fd, int timeout)
{
	fd_set rfds;
	struct timeval tv = { timeout, 0 };

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	return select(fd + 1, &rfds, NULL, NULL, (timeout > 0) ? &tv : NULL);
}


/*
 * fread() with automatic retry on syscall interrupt
 * @param	ptr	location to store to
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully read
 */
int safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;
	do {
		clearerr(stream);
		ret += fread((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);
	return ret;
}

/*
 * fwrite() with automatic retry on syscall interrupt
 * @param	ptr	location to read from
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully written
 */
int safe_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;
	do {
		clearerr(stream);
		ret += fwrite((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}

int sync_file_write(char *path)
{
	int fd = -1;
	FILE *pf = NULL;

	fd = open(path, O_RDWR);
	if(-1 == fd){
		return -1;
	}

	pf = fdopen(fd, "w+");
	fflush(pf);
	fsync(fd);
	fclose(pf);

	return 0;
} 

int f_splice(const char *src_path,const char *dst_path)
{
	FILE *spf,*dpf;
	uns8 buf[2048];
	size_t total,i;

	spf = fopen(src_path,"rb");
	if (spf == NULL) return -1;

	dpf = fopen(dst_path,"a+b");
	if (dpf == NULL) {
		fclose(spf);
		return -1;
	}

	total = 0;
	while ((i = fread(buf,1,sizeof(buf),spf)) > 0){
		fwrite(buf,i,1,dpf);
		total += i;
	} 

	fclose(spf);
	fclose(dpf);
	
	return (int)total;
}
