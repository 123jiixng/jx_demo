#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <linux/rtc.h>
#include <sys/ioctl.h>

#define RTC_GET_TEMPERATURE     _IO('p', 0xa0)  /* Get current temperature */

int get_temperature(float *value)
{
	unsigned char temperature[4];
	int fd,err;

	if ((fd = open("/dev/rtc0",O_RDWR)) < 0) {
		return -1;
	}

	err = ioctl(fd, RTC_GET_TEMPERATURE,(unsigned long *)temperature);
	close(fd);

	if (temperature[0]) {
		*value = -(temperature[1] + temperature[2]/10);
	} else {
		*value = temperature[1] + temperature[2]/10;
	}
	
	return err;
}

int get_reset_status(unsigned int *status,char *status_str)
{
	
}

