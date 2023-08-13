#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <stdarg.h>
#include <syslog.h>
#include <net/if.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#include "mtd-user.h"
#endif//!WIN32

#include "ih_logtrace.h"
#include "product.h"

#if (defined INHAND_VG9 || defined INHAND_IR8 || defined INHAND_ER3)
#define WIFI0_DATA_START 0x1000
#define WIFI1_DATA_START 0x5000
#define MAC_ADDR_OFFSET  6
#define CHECKSUM_OFFSET  2
#define ART_PARTITION_SIZE  256*1024

#define QC98XX_EEPROM_SIZE_LARGEST_AR900B   12064

uint32_t QC98XX_EEPROM_SIZE_LARGEST = QC98XX_EEPROM_SIZE_LARGEST_AR900B;

static uint16_t le16_to_cpu(uint16_t val)
{
    return val; //wucl: note that we are little endian.
}

static uint16_t qc98xx_calc_checksum(void *eeprom)
{
    uint16_t *p_half;
    uint16_t sum = 0;  
    int i;

    //printf("%s flash checksum 0x%x\n", __func__, le16_to_cpu(*((uint16_t *)eeprom + 1)));

    *((uint16_t *)eeprom + 1) = 0;

    p_half = (uint16_t *)eeprom;
    for (i = 0; i < QC98XX_EEPROM_SIZE_LARGEST / 2; i++) {
        sum ^= le16_to_cpu(*p_half++);
    }   

    sum ^= 0xFFFF;
    //printf("%s: calculated checksum: 0x%4x\n", __func__, sum);

    return sum;
}

int ft_set_wifi_mac(char *mac_2g_str, char *mac_5g_str)
{
	int datalen = 0;
	int imageFd = -1;
	off_t offset = 0;
	char mac_2g[6] = {0};
	char mac_5g[6] = {0};
	uint16_t csum = 0;
	unsigned char read_buff[QC98XX_EEPROM_SIZE_LARGEST_AR900B] = {0};
    char wifi_factory_part[64];

	if(!mac_2g_str || !mac_5g_str){
		LOG_DB("invalid mac_2g or mac_5g\n");
		return -1;
	}

#ifdef INHAND_ER8
    snprintf(wifi_factory_part, sizeof(wifi_factory_part), "%s", WIFI_ART_PART);
#elif defined INHAND_ER6
    snprintf(wifi_factory_part, sizeof(wifi_factory_part), "%s", WIFI_FACTORY_PART);
#endif

	/* open source file*/
	imageFd = open(wifi_factory_part, O_RDWR);
    if(imageFd<0) {
		LOG_DB("open %s failed! %s(%d)", wifi_factory_part, strerror(errno), errno);
		return -1;
	}

	sscanf(mac_2g_str, "%02x:%02x:%02x:%02x:%02x:%02x",
		 mac_2g, mac_2g+1, mac_2g+2, mac_2g+3, mac_2g+4, mac_2g+5);

	sscanf(mac_5g_str, "%02x:%02x:%02x:%02x:%02x:%02x",
		 mac_5g, mac_5g+1, mac_5g+2, mac_5g+3, mac_5g+4, mac_5g+5);

	offset = WIFI0_DATA_START + MAC_ADDR_OFFSET;
	lseek(imageFd, offset, SEEK_SET);
	write(imageFd, mac_2g, sizeof(mac_2g));

	offset = WIFI1_DATA_START + MAC_ADDR_OFFSET;
	lseek(imageFd, offset, SEEK_SET);
	write(imageFd, mac_5g, sizeof(mac_5g));

	offset = WIFI0_DATA_START;
	lseek(imageFd, offset, SEEK_SET);
	memset(read_buff, 0, sizeof(read_buff));
	datalen = read(imageFd, read_buff, sizeof(read_buff));
	if(datalen < 0){
		LOG_DB("read wifi0 data from %s failed! %s(%d)", wifi_factory_part, strerror(errno), errno);
		goto got_error_1;
	}

	//printf("wifi0 cal data read: %dbytes\n", datalen);
	csum = qc98xx_calc_checksum(read_buff);
	//printf("wifi0 data caculated checksum: %04x\n", csum);

	offset = WIFI0_DATA_START + CHECKSUM_OFFSET;
	lseek(imageFd, offset, SEEK_SET);
	datalen = write(imageFd, &csum, sizeof(csum));
	if(datalen < 0){
		LOG_DB("update wifi0 checksum to %s failed! %s(%d)", wifi_factory_part, strerror(errno), errno);
		goto got_error_1;
	}


	offset = WIFI1_DATA_START;
	lseek(imageFd, offset, SEEK_SET);
	memset(read_buff, 0, sizeof(read_buff));
	datalen = read(imageFd, read_buff, sizeof(read_buff));
	if(datalen < 0){
		LOG_DB("read wifi1 data from %s failed! %s(%d)", wifi_factory_part, strerror(errno), errno);
		goto got_error_1;
	}

	//printf("wifi1 cal data read: %dbytes\n", datalen);
	csum = qc98xx_calc_checksum(read_buff);
	//printf("wifi1 data caculated checksum: %04x\n", csum);

	offset = WIFI1_DATA_START + CHECKSUM_OFFSET;
	lseek(imageFd, offset, SEEK_SET);
	datalen = write(imageFd, &csum, sizeof(csum));
	if(datalen < 0){
		LOG_DB("update wifi1 checksum to %s failed! %s(%d)", wifi_factory_part, strerror(errno), errno);
		goto got_error_1;
	}

	close(imageFd);

	return 0;

got_error_1:
	close(imageFd);

	return -1;
}
#elif defined INHAND_ER6
#define MT7603_EEPROM_FILE	"/etc/wireless/mt7603/MT7603_EEPROM.BIN"
#define MT7663_EEPROM_FILE	"/etc/wireless/mt7663/MT7663_EEPROM.BIN"
#define MT7603_FACTORY_OFFSET	0x0
#define MT7663_FACTORY_OFFSET	0x8000

int ft_set_wifi_mac(char *mac_2g_str, char *mac_5g_str)
{
	int factory_fd, wifi_2g_fd, wifi_5g_fd;
	uint8_t wifi_2g_buf[512] = {0};
	uint8_t wifi_5g_buf[1536] = {0};
	char mac_2g[6] = {0};
	char mac_5g[6] = {0};
	char cmd[128];

	if(!mac_2g_str || !mac_5g_str){
		return -1;
	}

	factory_fd = part_open(WIFI_FACTORY_PART);
	if (factory_fd < 0) {
		printf("unable to open Factory\n");
		return -1;
	}

	lseek(factory_fd, MT7603_FACTORY_OFFSET, SEEK_SET);
	read(factory_fd, wifi_2g_buf, sizeof(wifi_2g_buf));

	/*no valid 2.4g eeprom*/
	if (wifi_2g_buf[0] == 0xff) {
		wifi_2g_fd = open(MT7603_EEPROM_FILE, O_RDONLY);
		if (wifi_2g_fd < 0) {
			printf("Cannot open %s factory eeprom file\n", MT7603_EEPROM_FILE);
			part_close(factory_fd);
			return -1;
		}
		read(wifi_2g_fd, wifi_2g_buf, sizeof(wifi_2g_buf));
		close(wifi_2g_fd);
	}

	lseek(factory_fd, MT7663_FACTORY_OFFSET, SEEK_SET);
	read(factory_fd, wifi_5g_buf, sizeof(wifi_5g_buf));

	/*no valid 5g eeprom*/
	if (wifi_5g_buf[0] == 0xff) {
		wifi_5g_fd = open(MT7663_EEPROM_FILE, O_RDONLY);
		if (wifi_5g_fd < 0) {
			printf("Cannot open %s factory eeprom file\n", MT7663_EEPROM_FILE);
			part_close(factory_fd);
			return -1;
		}
		read(wifi_5g_fd, wifi_5g_buf, sizeof(wifi_5g_buf));
		close(wifi_5g_fd);
	}
	part_close(factory_fd);

	/*erase factory mtd*/
	snprintf(cmd, sizeof(cmd), "mtd-erase -m \"%s\" -s %d -n %d", WIFI_FACTORY_PART, 0, 1);
	system(cmd);

	sscanf(mac_2g_str, "%02x:%02x:%02x:%02x:%02x:%02x",
		mac_2g, mac_2g+1, mac_2g+2, mac_2g+3, mac_2g+4, mac_2g+5);

	sscanf(mac_5g_str, "%02x:%02x:%02x:%02x:%02x:%02x",
		mac_5g, mac_5g+1, mac_5g+2, mac_5g+3, mac_5g+4, mac_5g+5);

	factory_fd = part_open(WIFI_FACTORY_PART);
	if (factory_fd < 0) {
		printf("unable to open Factory\n");
		return -1;
	}

	// set 2.4g wifi mac address
	wifi_2g_buf[4] = mac_2g[0];
	wifi_2g_buf[5] = mac_2g[1];
	wifi_2g_buf[6] = mac_2g[2];
	wifi_2g_buf[7] = mac_2g[3];
	wifi_2g_buf[8] = mac_2g[4];
	wifi_2g_buf[9] = mac_2g[5];
	lseek(factory_fd, MT7603_FACTORY_OFFSET, SEEK_SET);
	if(write(factory_fd, wifi_2g_buf, sizeof(wifi_2g_buf)) < 0){
		printf("write 2.4g eeprom fail\n");
		goto EEPROM_ERR;
	}

	// set 5g wifi mac address
	wifi_5g_buf[4] = mac_5g[0];
	wifi_5g_buf[5] = mac_5g[1];
	wifi_5g_buf[6] = mac_5g[2];
	wifi_5g_buf[7] = mac_5g[3];
	wifi_5g_buf[8] = mac_5g[4];
	wifi_5g_buf[9] = mac_5g[5];
	lseek(factory_fd, MT7663_FACTORY_OFFSET, SEEK_SET);
	if(write(factory_fd, wifi_5g_buf, sizeof(wifi_5g_buf)) < 0){
		printf("write 5g eeprom fail\n");
		goto EEPROM_ERR;
	}

	part_close(factory_fd);

	return 0;
EEPROM_ERR:
	part_close(factory_fd);

	return -1;
}
#endif

#ifndef WIN32
static int region_erase(int Fd, int start, int count, int unlock, int regcount)
{
    int i, j;
    region_info_t * reginfo;

    reginfo = calloc(regcount, sizeof(region_info_t));
	if(!reginfo){
		LOG_ER("%s out of memory.", __func__);
        return 8;
	}

    for(i = 0; i < regcount; i++)
    {
        reginfo[i].regionindex = i;
        if(ioctl(Fd,MEMGETREGIONINFO,&(reginfo[i])) != 0){
		    free(reginfo);
		    return 8;
		} else{
		    LOG_DB("Region %d is at %d of %d sector and with sector "
					"size %x\n", i, reginfo[i].offset, reginfo[i].numblocks,
					reginfo[i].erasesize);
		}
    }

    // We have all the information about the chip we need.

    for(i = 0; i < regcount; i++)
    { //Loop through the regions
        region_info_t * r = &(reginfo[i]);

        if((start >= reginfo[i].offset) && (start < (r->offset + r->numblocks*r->erasesize)))
            break;
    }

    if(i >= regcount)
    {
        LOG_DB("Starting offset %x not within chip.\n", start);
        free(reginfo);
        return 8;
    }

    //We are now positioned within region i of the chip, so start erasing
    //count sectors from there.

    for(j = 0; (j < count)&&(i < regcount); j++)
    {
        erase_info_t erase;
        region_info_t * r = &(reginfo[i]);

        erase.start = start;
        erase.length = r->erasesize;

        if(unlock != 0)
        { //Unlock the sector first.
            if(ioctl(Fd, MEMUNLOCK, &erase) != 0)
            {
                perror("\nMTD Unlock failure");
				free(reginfo);
                return 8;
            }
        }
        LOG_DB("\rPerforming Flash Erase of length %u at offset 0x%x",
                erase.length, erase.start);
        //fflush(stdout);
        if(ioctl(Fd, MEMERASE, &erase) != 0)
        {
            perror("\nMTD Erase failure");
			free(reginfo);
            return 8;
        }


        start += erase.length;
        if(start >= (r->offset + r->numblocks*r->erasesize))
        { //We finished region i so move to region i+1
            LOG_DB("\nMoving to region %d\n", i+1);
            i++;
        }
    }

    LOG_DB(" done\n");
	free(reginfo);

    return 0;
}

static int non_region_erase(int Fd, int start, int count, int unlock)
{
    mtd_info_t meminfo;

    if (ioctl(Fd,MEMGETINFO,&meminfo) == 0)
    {
        erase_info_t erase;

        erase.start = start;

        erase.length = meminfo.erasesize;

        for (; count > 0; count--) {
            LOG_DB("\rPerforming Flash Erase of length %u at offset 0x%x",
                                        erase.length, erase.start);
            //fflush(stdout);

            if(unlock != 0)
            {
                //Unlock the sector first.
                LOG_DB("\rPerforming Flash unlock at offset 0x%x",erase.start);
                if(ioctl(Fd, MEMUNLOCK, &erase) != 0)
                {
                    perror("\nMTD Unlock failure");
                    return 8;
                }
            }

            if (ioctl(Fd,MEMERASE,&erase) != 0)
            {
                perror("\nMTD Erase failure");
                return 8;
            }
                erase.start += meminfo.erasesize;
        }
        LOG_DB(" done\n");
    }
    return 0;
}

int ft_flash_erase(char *devname,int start,int count)
{
    int regcount;
    int Fd;
    int unlock =0 ;
    int res = 0;

    // Open and size the device
    if ((Fd = open(devname,O_RDWR)) < 0)
    {
        LOG_DB("File open error");
        return 8;
    }
    LOG_DB("Erase Total %d Units\n", count);

    if (ioctl(Fd,MEMGETREGIONCOUNT,&regcount) == 0)
    {
        if(regcount == 0)
        {
            res = non_region_erase(Fd, start, count, unlock);
        }
        else
        {
            res = region_erase(Fd, start, count, unlock, regcount);
        }
    }

	close(Fd);

    return res;
}

int ft_flash_eraseall(char *devname)
{
	mtd_info_t mi;
    int fd;
    int count=1;

    // Open and size the device
    if ((fd = open(devname,O_RDWR)) < 0){
	    LOG_DB("File open error");
		return -1;
    }
	if(ioctl(fd, MEMGETINFO, &mi) == 0) {
		count = mi.size/mi.erasesize; 
	}
	close(fd);

	return ft_flash_erase(devname, 0, count);
}
#endif//!WIN32
