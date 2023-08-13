#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include <linux/rtc.h>
#include <sys/ioctl.h>
#include "strings.h"

#define RTC_GET_TEMPERATURE     _IO('p', 0xa0)  /* Get current temperature */
#define RTC_GET_RESETSTATUS     _IO('p', 0xb0)  /* Get reset status */

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

int get_reset_status(unsigned short *status,char *str,unsigned short str_size)
{
	FILE *pf;
	char buf[64];

	pf = fopen("/proc/reset_status","r");
	if (pf == NULL)	return -1;

	if (fgets(buf,sizeof(buf),pf) == NULL) {
		fclose(pf);
		return -1;
	}

	if (strstr(buf,"Unkown")) {
		if (status)  *status = 0;
		if (str) strlcpy(str,"General",str_size);
	} else {
		if (status)  *status = (unsigned short)(buf[0] - '0');
		if (str) {
			char *chr;

			chr = strtok(&buf[2],"\r\n");
			strlcpy(str,chr,str_size);
		}
	}

	fclose(pf);
	return 0;
}

