/*
 inotify
*/
#include <stdio.h>
#include <sys/inotify.h>
#include <errno.h>
#include "ih_ipc.h"
#include "sw_ipc.h"
#include "shared.h"
#include "inhand_inotify.h"

#define MAX_EVENTS 256
#define BUFFER_SIZE (MAX_EVENTS*sizeof(struct inotify_event))

int register_watchpoint(int fd, char *dir, unsigned int mask)
{
	int err = -1;

	err = inotify_add_watch(fd, dir, mask);

	return err;
}

int watch(int fd, void (*event_handle)(struct inotify_event *ievent))
{
	char ev_buffer[BUFFER_SIZE] = {0};
	struct inotify_event *ievent = NULL;
	int len = 0;
	int i = 0;

read:
	len = read(fd, ev_buffer, sizeof(ev_buffer));
	if(len < 0){
		if(errno == EINTR){
			goto read;
		}
	}

	i = 0;
	while(i < len){
		ievent = (struct inotify_event *)&ev_buffer[i];
		(*event_handle)(ievent);
		i += sizeof(struct inotify_event) + ievent->len;
	}

	return 0;
}

int init_inotify(int *fd, char *dir, unsigned int mask)
{
	int ifd = -1;
	int err = -1;

	ifd = inotify_init();
	if(ifd < 0){
		printf("can't init inotify errno: %d(%s)\n", errno, strerror(errno));
		return -1;
	}

	err = register_watchpoint(ifd, dir, mask);
	if(err < 0){
		printf("can't add inotify watch point errno: %d(%s)\n", errno, strerror(errno));
		return -1;
	}

	if(fd){
		*fd = ifd;
	}

	return 0;
}

