#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include "shared.h"
#include "ih_errno.h"
#include "bootenv.h"
#include "ih_logtrace.h"

/* boot env map
 * crc
 * name1=value1
 * name2=value2
 * \0
 */
struct bootenv_t {
	unsigned long	crc; /* CRC32 over data bytes	*/
	char data[BOOT_ENV_DATA_SIZE]; /* Environment data		*/
};

/****************************About CRC******************************/
/*use this crc instead of libz's crc32. refer mtd.c and uboot*/
static unsigned long *crc_table = NULL;
static void crc_done(void)
{
	free(crc_table);
	crc_table = NULL;
}
static int crc_init(void)
{
	unsigned long c, poly;
//	int i, j;
	int n, k;
	static const char p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};


	if(crc_table == NULL) {
		if((crc_table = malloc(sizeof(unsigned long)*256)) == NULL)
			return 0;
		/* make exclusive-or pattern from polynomial (0xedb88320L) */
		poly = 0L;
		for (n = 0; n < sizeof(p)/sizeof(char); n++)
			poly |= 1L << (31 - p[n]);

		for (n = 0; n < 256; n++) {
			c = (unsigned long)n;
			for (k = 0; k < 8; k++)
				c = c & 1 ? poly ^ (c >> 1) : c >> 1;
			crc_table[n] = c;
		}

/*		for(i=255; i>=0; --i) {
			c = i;
			for(j=8; j>0; --j) {
				if(c & 1) c=(c>>1)^0xEDB88320L;
				else c>>=1;				
			}
			crc_table[i] = c;
		}
*/
	}
	return 1;
}
/* ========================================================================= */
#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);
/* ========================================================================= */
static unsigned long crc_calc(unsigned long crc, char *buf, int len)
{
/*	while (len-- > 0) {
		crc = crc_table[(crc ^ *((char *)buf)) & 0xFF] ^ (crc >> 8);
		(char *)buf++;
	}
	return crc;
*/
  	crc = crc ^ 0xffffffffL;
	while (len >= 8)
	{
	      DO8(buf);
	      len -= 8;
	}
	if (len) do {
		DO1(buf);
	} while (--len);
	return crc ^ 0xffffffffL;
}

unsigned long crc32(unsigned long crc, char* buf, unsigned long len)
{
	unsigned long newcrc;

	crc_init();
	newcrc = crc_calc(crc, buf, len);
	crc_done();
	return newcrc;
}

unsigned int uboot_crc32(char *data,unsigned int len)
{
	return 	crc32(0,data,len);
}

/*************************END CRC**************************************/

int bootenv_set(char *name,char *value)
{
	struct bootenv_t *bootenv;
	FILE *fp;
	char *ps_data,*pd_data;
	unsigned int len,i;

	if (!name||!*name||!value||!*value) return ERR_INVAL;

	fp = fopen(BOOTLOADER_PART, "r");
	if (fp == NULL) {
		printf("can not open %s, %s(%d)\n", BOOTLOADER_PART, strerror(errno), errno);
		return ERR_NOENT;
	}

	bootenv = (struct bootenv_t *)malloc(BOOT_ENV_SIZE);
	if (bootenv == NULL) {
		printf(" %s(%d): can not malloc!\n", strerror(errno), errno);
		fclose(fp);
		return ERR_NOMEM;
	}

	fseek(fp, CFG_ENV_OFFSET, SEEK_SET);
	fread(bootenv, 1, BOOT_ENV_SIZE, fp);
	fclose(fp);

	//printf("bootenv set %s %s\n",name,value);

	/* 计算空间 */
	ps_data = pd_data = bootenv->data;
	while (*ps_data) {
		if (strncmp(ps_data,name,strlen(name)) == 0) {
			ps_data += strlen(ps_data) + 1;
		} else {
			len = strlen(ps_data);
			
			if (ps_data != pd_data) {
				for (i=0;i<len;i++) {
					*pd_data++ = *ps_data++;
				}
				
				*pd_data++ = '\0';
				ps_data++;
			} else {
				pd_data += len + 1;
				ps_data += len + 1;
			}
		}
	}

	if (BOOT_ENV_DATA_SIZE - (pd_data - bootenv->data) 
		< strlen(name) + strlen(value) + 3) {
		free(bootenv);
		printf("no memory\n");
		return ERR_NOMEM;
	}
	
	strcpy(pd_data,name);
	pd_data += strlen(name);
	*pd_data++ = '=';
	strcpy(pd_data,value);
	pd_data += strlen(value);
	memset(pd_data,0,BOOT_ENV_DATA_SIZE - (pd_data - bootenv->data)); 

	bootenv->crc = crc32(0,bootenv->data,BOOT_ENV_DATA_SIZE);

#ifdef USE_EMMC
	/* boot env size is 64kB*2 */
	part_erase2(BOOTLOADER_PART, CFG_ENV_OFFSET,BOOT_ENV_SIZE/EMMC_WRITE_BUFF_SIZE,0,1);
#else
	part_erase2(BOOTLOADER_PART_NAME,CFG_ENV_OFFSET,1,0,1);
#endif	

	fp = fopen(BOOTLOADER_PART, "w");
	if (fp == NULL) {
		printf("无法打开设备文件（/dev/mtd0），配置失败！\n");
		free(bootenv);
		return ERR_NOENT;
	}
	
	fseek(fp, CFG_ENV_OFFSET, SEEK_SET);
	fwrite(bootenv,1,BOOT_ENV_SIZE, fp);
	fclose(fp);

	free(bootenv);
	return 0;
}

char *bootenv_get(char *name,char *value,unsigned short value_len)
{
	struct bootenv_t *bootenv;
	FILE *fp;
	char *pdata;
	char *p;

	if (!name||!*name||!value) return NULL;

	fp = fopen(BOOTLOADER_PART, "r");
	if(fp == NULL){
		printf("can not open %s. %s(%d)\n", BOOTLOADER_PART, strerror(errno), errno);
		return NULL;
	}

	bootenv = (struct bootenv_t *)malloc(BOOT_ENV_SIZE);
	if (bootenv == NULL) {
		fclose(fp);
		printf("can not malloc\n");
		return NULL;
	}

	fseek(fp, CFG_ENV_OFFSET, SEEK_SET);
	fread(bootenv, 1, BOOT_ENV_SIZE, fp);
	fclose(fp);

	/* 计算空间 */
	pdata = bootenv->data;
	while (*pdata) {
		p = strsep(&pdata, "=");
		if (!p) {
			free(bootenv);
			return NULL;
		}		

		if (strcmp(p, name) == 0) {
			strlcpy(value,pdata,value_len);
			free(bootenv);
			return value;
		}

		pdata += strlen(pdata) + 1;
	}

	free(bootenv);
	return NULL;		
}

int bootenv_show(char *buf,unsigned int buf_len)
{
	FILE *fp;

	if (buf_len < BOOT_ENV_SIZE) return ERR_NOMEM;
	
	fp = fopen(BOOTLOADER_PART, "r");
	if(fp == NULL){
		printf("can not open %s. %s(%d)\n", BOOTLOADER_PART, strerror(errno), errno);
		return ERR_NOENT;
	}

	fseek(fp, CFG_ENV_OFFSET, SEEK_SET);
	fread(buf, 1, BOOT_ENV_SIZE, fp);
	fclose(fp);

	return BOOT_ENV_SIZE;
}

int set_bootflag(unsigned int flag)
{
	int fd;
	int ret = -1;

	fd = part_open(BOOTFLAG_PART);
	if (fd < 0) {
		LOG_ER("unable to open Boot Flag");
		return -1;
	}

	if (part_erase(fd,0,-1,0,1) < 0) {
		LOG_ER("unable to erase Boot Flag");
		part_close(fd);
		return -1;
	}

	ret = part_write(fd, 0, (unsigned char *)&flag, sizeof(flag), 1);
	if(ret < 0){
		LOG_ER("unable to write Boot Flag");
		part_close(fd);
		return ret;
	}

	part_close(fd);
	
	return 0;
}

/*
 * mark boot flag to be ok
 */
void clear_bootflag_cnt(void)
{
	unsigned int flag;
	int fd;

	fd = part_open(BOOTFLAG_PART);
	if (fd < 0) {
		LOG_ER("open Boot Flag fail.");
		return;
	}

	read(fd, (char *)&flag, sizeof(flag));

	LOG_IN("bootflag is 0x%X", flag);

	/*
	 *   31~8      7~0
	 *  Reserved   CNT
	 */
	/*boot successfully, clear cnt*/
	if((flag&0xff)!=0) {
		LOG_IN("new firmware has been upgraded successfully!");
		lseek(fd, 0, SEEK_SET);
		if (part_erase(fd,0,-1,0,1) < 0) {
			LOG_ER("unable to erase Boot Flag");
			part_close(fd);
			return;
		}
#ifdef USE_EMMC
	/*if using second now, erase first, ELse erase sencond
	 *    31      30~8      7~0
	 *  Index    Reserved   CNT
	 */
	/*boot successfully, clear cnt*/
		/*rewrite flag*/
		flag &= 0xffffff00;
#elif defined INHAND_ER6 || defined INHAND_WR3
		flag &= 0xffffff00;
#elif defined INHAND_ER3
		//FIXME ER3 how
#else
		/*rewrite flag*/
		flag = 0;
#endif
		part_write(fd, 0, (unsigned char *)&flag, sizeof(flag), 1);
	}
	part_close(fd);
}

void reverse_bootflag(void)
{
	unsigned int flag;
	int fd;
	unsigned char bootindex = 0;
	int ret = -1;

	fd = part_open(BOOTFLAG_PART);
	if (fd < 0) {
		LOG_ER("open Boot Flag fail.");
		return;
	}

	read(fd, (char *)&flag, sizeof(flag));
	if (part_erase(fd,0,-1,0,1) < 0) {
		LOG_ER("unable to erase Boot Flag");
		part_close(fd);
		return;
	}
	/*
	 *   31~8      7~0
	 *  Reserved   CNT
	*/
	bootindex = ~(flag>>31);

	/*update boot flag*/
	flag = ((bootindex<<31) | (0x7f<<24) | (0xffff<<8) | (flag&0xff));
	LOG_IN("bootflag is 0x%X", flag);
	ret = part_write(fd, 0, (unsigned char *)&flag, sizeof(flag), 1);
	if(ret < 0){
		LOG_ER("unable to write Boot Flag");
		part_close(fd);
		return;
	}

	part_close(fd);
}

