#include <errno.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "ih_errno.h"
#include "shared.h"
#include "mtd-abi.h"
#include "mtd-user.h"
#include "ih_logtrace.h"
#include "bootenv.h"

#ifdef USE_EMMC
typedef unsigned int sector_t;
typedef unsigned long uoff_t;

sector_t erase_sector_size = 64*1024;
#if !defined(BLKGETSIZE)
#define BLKGETSIZE _IO(0x12,96)	/* return device size /512 (long *arg) */
#endif

#if !defined(BLKGETSIZE64)
#define BLKGETSIZE64 _IOR(0x12,114,size_t)	/* return device size in bytes (u64 *arg) */
#endif

#define EMMC_SECTOR_SIZE (512)
int get_emmc_partition_bloksize(char *emmc_part_name);
sector_t get_emmc_sectors(int fd);
#endif

static int mtd_getinfo(const char *mtdname, int *part, int *size)
{
	FILE *f;
	char s[256];
	char t[256];
	int r;

	r = 0;
	if ((strlen(mtdname) < 128) && (strcmp(mtdname, "pmon") != 0)) {
		sprintf(t, "\"%s\"", mtdname);
		if ((f = fopen("/proc/mtd", "r")) != NULL) {
			while (fgets(s, sizeof(s), f) != NULL) {
				if ((sscanf(s, "mtd%d: %x", part, size) == 2) && (strstr(s, t) != NULL)) {
					// don't accidentally mess with bl (0)
					if (*part >= 0) r = 1;
					break;
				}
			}
			fclose(f);
		}
	}
	
	if (!r) {
		*size = 0;
		*part = -1;
	}
	return r;
}

/** @brief open a mtd device 
 *
 * @param mtdname	name of mtd device,must be on of /proc/mtd
 * @return <0 for error, >=0 for file description
 */
int mtd_open(const char *mtdname)
{
	char path[32];
	int part;
	int size;

	if (mtd_getinfo(mtdname, &part, &size)) {
		snprintf(path,sizeof(path),"/dev/mtd%d", part);
		//printf("open mtd dev:%s\n",path);
		return open(path, O_RDWR|O_SYNC);
	}

	return -1;
}

#ifdef USE_EMMC
int emmc_open(const char *partname) 
{
	char path[32];
	int fd = -1;

	if(partname == NULL){
		return -1;
	}

	snprintf(path,sizeof(path),"%s", partname);
	fd = open(path, O_RDWR|O_SYNC);
	if(fd < 0){
		syslog(LOG_ERR, "open %s failed! %s(%d)", partname, strerror(errno), errno);	
	}

	return fd; 
}
#endif

int part_open(const char *partname)
{
#ifdef USE_EMMC
	return emmc_open(partname);
#else
	return mtd_open(partname);
#endif

}

#ifdef USE_EMMC
int sync_fd_close(int fd)
{
	FILE *pf = NULL;

	if(-1 == fd){
		return -1;
	}

	pf = fdopen(fd, "a+");
	if(pf){
		fflush(pf);
		fsync(fd);
		fclose(pf);
	}else{
		close(fd);
	}

	return 0;
}
#endif

/** @brief close a mtd device 
 *
 * @param fd	file description
 */
void part_close(int fd)
{
#ifdef USE_EMMC
		sync_fd_close(fd);
#else
	close(fd);
#endif
}

/** @brief erase mtd block
 *
 * @param fd			file description
 * @param start			start offset of this mtd
 * @param count			erase count (-1 means all blocks)
 * @param unlock		if need unlock
 * @param quiet			if need print process information
 *
 * @return <0 for error, 0 success , >0 number of blocks whose was failed to erase
 */
int mtd_erase(int fd,int start,int count,int unlock,int quiet)
{
	int err = -1;
	mtd_info_t meminfo;
	erase_info_t erase;
	loff_t offs;

	if (ioctl(fd,MEMGETINFO,&meminfo)) return -1;
	
	erase.start = start;
	erase.length = meminfo.erasesize;

	if (count <= 0) {
		count = meminfo.size/meminfo.erasesize;
	}

	err = 0;
	for (; count > 0; count--) {
		offs = (erase.start & (~(meminfo.erasesize-1)));
		
		if (meminfo.type == MTD_NANDFLASH) {
			err = ioctl(fd, MEMGETBADBLOCK, &offs);
			if (err > 0) {
				if (!quiet) printf("Skipping bad block at 0x%08x\n",(unsigned int)offs);
				erase.start += meminfo.erasesize;
				continue;
			} else if (err < 0) {
				if (!quiet) printf("ioctl(MEMGETBADBLOCK) at 0x%08x\n",(unsigned int)offs);
				return err;
			}
		}
		
		if(unlock != 0) {
			if((err = ioctl(fd, MEMUNLOCK, &erase)) != 0) {
				if (!quiet) printf("ioctl(MEMUNLOCK)");
				return err;
			}
		}

		if (!quiet) {
			printf("Performing Flash Erase of length %u at offset 0x%x\n",
					erase.length,(unsigned int)offs);
		}

		if ((err = ioctl(fd,MEMERASE,&erase)) != 0) {
			if (!quiet) printf("ioctl(MEMERASE)");
			return err;
		}

		erase.start += meminfo.erasesize;
		if (erase.start >= meminfo.size) break;
	}
	
	return err;
}

#ifdef USE_EMMC
int emmc_erase(int fd,int start,int count,int unlock,int quiet)
{
	int err;
	char *ptmp = NULL;
	unsigned int size = 0;
	unsigned int sectors = 0;
	unsigned int total_size = 0;
	unsigned int erase_total_size = 0;
	off_t cur_pos = -1;
	int ret = -1;

	if(fd < 0){
		printf("fd param is invalid!\n");	
		return ERR_INVAL; 
	}

	cur_pos = lseek(fd, 0, SEEK_CUR);

	sectors = get_emmc_sectors(fd);
	total_size = sectors * EMMC_SECTOR_SIZE;
	if(start >= total_size){
		printf("The start position is overrload\n");
		goto erase_exit;
	}

	erase_total_size = total_size - start;
	if((count > 0) && (count*erase_sector_size <= erase_total_size)){
		erase_total_size = count*erase_sector_size;
	}

	lseek(fd, start, SEEK_SET);
	if (!quiet) {
		printf("erase %d Bytes from %#x \n", erase_total_size,start);
	}

	ptmp = malloc(erase_sector_size);
	if(!ptmp){
		printf("malloc emmc sector unit failed! %s(%d)\n", strerror(errno), errno);
		goto erase_exit;
	}

	memset(ptmp, 0x00, erase_sector_size);
	while (erase_total_size > 0){
		size=MIN(erase_sector_size, erase_total_size);
		err = write(fd, ptmp, size);
		if (err <= 0){
			if (errno == EINTR){
				continue;
			}

			if (!quiet) printf("write err:%s(%d)\n", strerror(errno), errno);
			goto erase_exit;
		}else if (err){
			erase_total_size -= err;
		}

		syncfs(fd);
	}		

	ret = 0;
erase_exit:
	if(ptmp){
		free(ptmp);
		ptmp = NULL;
  }

	if(cur_pos >= 0){
		lseek(fd, cur_pos, SEEK_SET);
	}

	return ret;
}
#endif

int part_erase(int fd,int start,int count,int unlock,int quiet)
{
#ifdef USE_EMMC
	return emmc_erase(fd, start, count, unlock, quiet);
#else
	return mtd_erase(fd, start, count, unlock, quiet);
#endif
}

int part_erase2(char *mtdname,int start,int count,int unlock,int quiet)
{
	int mf;
	int err = -1;

	if ((mf = part_open(mtdname)) >= 0) {
		err = part_erase(mf,start,count,unlock,quiet);
		close(mf);
	}

	return err;
}

unsigned int mtd_size(int fd)
{
	mtd_info_t mi;

 	if (ioctl(fd, MEMGETINFO, &mi)) return 0;
	return mi.size;
}

/** @brief get mtd device page size
 *
 */
unsigned int mtd_page_size(int fd)
{
	mtd_info_t mi;

 	if (ioctl(fd, MEMGETINFO, &mi)) return 0;
	return mi.oobblock;
}

/* TODO: must be test on NandFlash*/
/** @brief write data to mtd device
 *  Note: 1, this function will skip bad block;
 * 	  2, this function will fill unused space with 0xFF for NandFlash
 *
 * @param fd			file description
 * @param start			the start position,must be page align
 * @param data			data buf
 * @param size			data size
 *
 * @return <0 for error, >=0 for next writable position
 */
int mtd_write(int fd,
				unsigned int start,
				const unsigned char *data,
				unsigned int size,
				int quiet)
{
	mtd_info_t mi;
	int err;
	loff_t off;
	char *pdata = (char *)data;

	if (!data||!size) return ERR_INVAL;
	
	if ((err = ioctl(fd, MEMGETINFO, &mi))) {
		return err;
	}

	if (mi.type == MTD_NANDFLASH) {
		if (start % mi.oobblock) return ERR_INVAL; /* need page align */
	}
	
	if (start >= mi.size) return ERR_INVAL;
	if (((long long)start + size) > (long long)mi.size) { /* cut overflow data */
		unsigned int osize = size;
		
		size = mi.size - start;
		if (!quiet) printf("cut size from %d to %d\n",osize,size);
	}
	
	lseek(fd,start,SEEK_SET);

	if (!quiet) printf("write %d to %d (oob_size:%d)\n",size,start,mi.oobblock);

	while (size > 0) {
		if (mi.type == MTD_NANDFLASH
			&&start%mi.erasesize == 0) { /*check bad block*/
			off = (loff_t)start;
			if ((err = ioctl(fd, MEMGETBADBLOCK, &off)) < 0) {
				if (!quiet) printf("ioctl(MEMGETBADBLOCK) (%d:%s)\n",err,strerror(err));
				return err;
			}

			if (err) {//skip bad block
				if (!quiet) printf("skip bad block:%d\n",start);
				start += mi.erasesize;
				lseek(fd,start,SEEK_SET);
				continue;
			}
		}

		if (mi.type == MTD_NANDFLASH){
			if (size < mi.oobblock){
				/* Fill the remain space with 0xFF. */
				char *tmp = malloc(mi.oobblock);
				if (!tmp) return -ERR_NOMEM;
				memcpy(tmp,pdata,size);
				memset(tmp + size,0xFF,mi.oobblock-size);
				char *p = tmp;
				size = mi.oobblock;
				while (size > 0){
					err = write(fd, p, size);
					if (err <= 0){
						if (!quiet) printf("write err:%d\n",err);
						free(tmp);
						return -1;
					}else if (err){
						p += err;
						size -= err;
					}
				}
				free(tmp);
				start += mi.oobblock;
				continue;
			}
		}

		err = write(fd,pdata, (mi.type == MTD_NANDFLASH)?(mi.oobblock):(size));
		if (err <= 0){
			if (!quiet) printf("write err:%d\n",err);
			return -1;
		}
		pdata += err;
		size -= err;
		start += err;
	}

	return start;
}

#ifdef USE_EMMC
int emmc_write(int fd, unsigned int start, const unsigned char *data, unsigned int size, int quiet)
{
	int err;
	char *pdata = (char *)data;
	unsigned int sectors = 0;
	unsigned int total_size = 0;

	if(fd < 0){
		printf("fd param is invalid!\n");	
		return ERR_INVAL; 
	}

	if (!data||!size){
		return ERR_INVAL;
	}

	sectors = get_emmc_sectors(fd);
	//printf("get emmc sectors:%d, %#x\n", sectors, sectors);
	total_size = sectors * EMMC_SECTOR_SIZE;
	if(start >= total_size){
		printf("The start position is overrload\n");
		return ERR_INVAL; 
	}

	if(start + size > total_size){
		size = total_size - start;
	}
	
	lseek(fd,start,SEEK_SET);
	if (!quiet){
		printf("write %d Bytes to position %#x\n",size,start);
	}

	while (size > 0){
		err = write(fd, pdata, size);
		if (err <= 0){
			if (errno == EINTR){
				continue;
			}

			if (!quiet){
				printf("write err:%s(%d)\n", strerror(errno), errno);
			}

			return -1;
		}

		pdata += err;
		size -= err;
		start += err;
	}	

	syncfs(fd);

	return start;
}
#endif

int part_write(int fd, unsigned int start, const unsigned char *data, unsigned int size, int quiet)
{
#ifdef USE_EMMC
	return emmc_write(fd, start, data, size, quiet);
#else
	return mtd_write(fd, start, data, size, quiet);
#endif
}


/** @brief read data from mtd device
 *  Note will skip bad block
 *
 * @param fd			file description
 * @param start			the start position,must be page align
 * @param data			data buf
 * @param size			data size
 *
 * @return <0 for error, >=0 for next readble position
 */
int mtd_read(int fd,
				unsigned int start,
				const unsigned char *data,
				unsigned int size,
				int quiet)
{
	mtd_info_t mi;
	int err;
	unsigned int rrel;
	loff_t off;
	char *pdata = (char *)data;

	if (!data||!size) return ERR_INVAL;
	
	if ((err = ioctl(fd, MEMGETINFO, &mi))) {
		if (!quiet) printf("ioctl(MEMGETINFO) %d\n",err);
		return err;
	}

	if (mi.type == MTD_NANDFLASH) {
		if (start % mi.oobblock) return ERR_INVAL; /* need page align */
	}
	
	if (start >= mi.size) return ERR_INVAL;
	if (((long long)start + size) > (long long)mi.size) { /* cut overflow data */
		unsigned int relsize = size;
		
		size = mi.size - start;
		if (!quiet) printf("cut size from %d to %d\n",relsize,size);
	}

	lseek(fd,start,SEEK_SET);

	if (!quiet) printf("read %d from %d\n",size,start);
	
	while (size > 0) {
		if (mi.type == MTD_NANDFLASH) {
			if(start%mi.erasesize == 0) { /*check bad block*/
				off = (loff_t)start;
				if ((err = ioctl(fd, MEMGETBADBLOCK, &off)) < 0) {
					if (!quiet) printf("ioctl(MEMGETBADBLOCK) (%d:%s)\n",err,strerror(err));
					return err;
				}

				if (err) {
					if (!quiet) printf("bad block:%d\n",start);
					start += mi.erasesize;
					lseek(fd,start,SEEK_SET);
					continue;
				}
			}

			rrel = min(mi.oobblock,size);
		} else {
			rrel = size;
		}

		err = read(fd,pdata,rrel);
		if (err != rrel) {
			if (!quiet) printf("read err:%d\n",err);
		}

		pdata += rrel;
		start += rrel;
		size -= rrel;
	}

	return start;
}

#ifdef USE_EMMC
int emmc_read(int fd, unsigned int start, const unsigned char *data, unsigned int size, int quiet)
{
	int err;
	char *pdata = (char *)data;
	unsigned int sectors = 0;
	unsigned int total_size = 0;

	if(fd < 0){
		printf("fd param is invalid!\n");	
		return ERR_INVAL; 
	}

	if (!data||!size) return ERR_INVAL;

	sectors = get_emmc_sectors(fd);
	//printf("get emmc sectors:%d, %#x\n", sectors, sectors);
	total_size = sectors * EMMC_SECTOR_SIZE;
	if(start >= total_size){
		printf("The start position is overrload\n");
		return ERR_INVAL; 
	}

	if(start + size > total_size){
		size = total_size - start;
	}

	lseek(fd,start,SEEK_SET);
	if (!quiet) printf("read %d from position:%#x\n",size,start);
	
	while (size > 0) {
		err = read(fd,pdata, size);
		if (err <= 0) {
			if (errno == EINTR){
				continue;
			}

			if (!quiet && (err < 0)) {
				printf("read err:%s(%d)\n",strerror(errno), errno);
			}
			break;
		}

		pdata += err;
		start += err;
		size -= err;
	}

	return start;
}
#endif

int part_read(int fd, unsigned int start, const unsigned char *data, unsigned int size, int quiet)
{
#ifdef USE_EMMC
	return emmc_read(fd, start, data, size, quiet);
#else
	return mtd_read(fd, start, data, size, quiet);
#endif
}

char *mtd_get_part( char *mtd_name,char *part,uns16 part_size)
{
	FILE *f;
	char s[256];

	if (!mtd_name || !*mtd_name || !part) return NULL;

	if ((f = fopen("/proc/mtd", "r")) != NULL) {
		while (fgets(s, sizeof(s), f) != NULL) {
			if (strstr(s,mtd_name)) {
				strtok(s,":");
				snprintf(part,part_size,"/dev/%s",s);
				fclose(f);
				return part;
			}
		}
		
		fclose(f);
	}
	
	return NULL;
}

#ifdef USE_EMMC
char *emmc_get_part( char *part_name, char *part, uns16 part_size)
{
	if (!part_name || !*part_name || !part){
		return NULL;
	}

	snprintf(part, part_size, "%s", part_name);
	return part;
}
#endif

char *get_part( char *part_name,char *part,uns16 part_size)
{
#ifdef USE_EMMC
	return emmc_get_part(part_name, part, part_size);
#else
	return mtd_get_part(part_name, part, part_size);
#endif
}


#ifdef USE_EMMC
int get_emmc_partition_bloksize(char *emmc_part_name)
{
	int ret = -1; 
	FILE *fp = NULL;
	char buf[128];
	char *p = NULL;
	int sectors = 0;
	char *part_name = NULL;

	if(!emmc_part_name || !*emmc_part_name){
		return ret;	
	}

	fp = fopen("/proc/partitions", "r");
	if(!fp){
		printf("open partitions file error!\n");
		return ret; 
	}

	part_name = strrchr(emmc_part_name, '/');
	if(part_name){
		part_name ++;	
	}else{
		part_name = emmc_part_name;	
	}

	while(fgets(buf, sizeof(buf), fp)){
		if (part_name && strstr(buf, part_name)) {
			strtok(buf, " "); 
			strtok(NULL, " ");  
			p = strtok(NULL, " ");  
			ret = 0;  
			break;
		}   
	}   

	fclose(fp); 
	sectors = p?atoi(p): -1;

	return sectors; 
}

sector_t get_emmc_sectors(int fd)
{
	uint64_t v64;
	unsigned long longsectors;

	if (ioctl(fd, BLKGETSIZE64, &v64) == 0) {
		/* Got bytes, convert to 512 byte sectors */
		v64 >>= 9;
		if (v64 != (sector_t)v64) {
ret_trunc:
			/* Not only DOS, but all other partition tables
			 * we support can't record more than 32 bit
			 * sector counts or offsets
			 */
			printf("device has more than 2^32 sectors, can't use all of them.\n");
			v64 = (uint32_t)-1L;
		}
		return v64;
	}

	/* Needs temp of type long */
	if (ioctl(fd, BLKGETSIZE, &longsectors)) {
		/* Perhaps this is a disk image */
		off_t curt_sz = lseek(fd, 0, SEEK_CUR);
		off_t sz = lseek(fd, 0, SEEK_END);

		longsectors = 0;
		if (sz > 0){
			longsectors = (uoff_t)sz / EMMC_SECTOR_SIZE;
		}

		lseek(fd, curt_sz, SEEK_SET);
	}

	if (sizeof(long) > sizeof(sector_t) && longsectors != (sector_t)longsectors ) {
		goto ret_trunc;
	}

	return longsectors;
}
#endif

