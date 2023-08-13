/*
 * $Id$ --
 *
 *   pysdk image routines
 * 1. pysdk1 is the main image partition, pysdk2 is the backup image.
 * 2. pysdk flag indicates the upgrading process(flag &= 0x07).
 * 			0x07: S0, flag is erased or in the factory
 * 			0x06: S1, main img erasing is interrupted.
 * 			0x04: S2, new main img is writen, need to check the python work status
 *          0x00: S3, python world works well, no upgrading event happens.
 *          else: error

 *       
 * Copyright (c) 2001-2019 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 09/09/2019
 * Author: Zhengyb
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include "shared.h"
#include "python_ipc.h"
#include "ih_logtrace.h"
#include "mobius_pysdk_img.h"

#define PYSDK_IMG_FLAG_MASK 0x07

#define PYSDK_IMG_FLAG_PART_NAME "PySDK Flag"

void setup_python_enviroments()
{		
	setenv("LD_LIBRARY_PATH", "/var/pycore/lib:"PYUSER_LIB_PATH, 1);
	setenv("PYTHONHOME", "/var/pycore", 1);
	setenv("PYTHON_EGG_CACHE", "/tmp/.python-eggs", 1);
	setenv("PYTHONUSERBASE", PYUSER_PATH, 1); // TODO: PYTHONUSERBASE may be changed 
	setenv("PYTHONPATH", PYUSER_LIB_PATH":"PYUSER_BIN_PATH":/var/pycore/lib/python27_std:/var/pycore/lib/python37_std:/var/pycore/lib/stdlib:/var/pycore/lib:/var/pycore/libinpy:/var/pycore/bin", 1);
}

int set_pysdk_img_flag(unsigned int flag)
{
    int fd;

	fd = part_open(PYSDK_IMG_FLAG_PART_NAME);
	if (fd < 0) {
		LOG_ER("unable to open pysdk Flag");
		return -1;
	}

	write(fd, (char *)&flag, sizeof(flag));
	part_close(fd);
	
	return 0;
}

int erase_pysdk_img_flag()
{
    int fd;

	fd = part_open(PYSDK_IMG_FLAG_PART_NAME);
	if (fd < 0) {
		LOG_ER("unable to open pysdk Flag");
		return -1;
	}

	if (part_erase(fd,0,-1,0,1) < 0) {
		LOG_ER("unable to erase pysdk Flag");
		part_close(fd);
		return -1;
	}
	part_close(fd);
	
	return 0;
}

int erase_set_pysdk_img_flag(unsigned int flag)
{
    int fd;

	fd = part_open(PYSDK_IMG_FLAG_PART_NAME);
	if (fd < 0) {
		LOG_ER("unable to open pysdk Flag");
		return -1;
	}

	if (part_erase(fd,0,-1,0,1) < 0) {
		LOG_ER("unable to erase pysdk Flag");
		part_close(fd);
		return -1;
	}

	write(fd, (char *)&flag, sizeof(flag));
	part_close(fd);
	
	return 0;
}

unsigned int pysdk_img_flag()
{
	int fd;
	unsigned int flag;

	fd = part_open(PYSDK_IMG_FLAG_PART_NAME);
	if (fd < 0) {
		LOG_ER("unable to open pysdk Flag");
		return 0xFFFFFFFF;
	}

	read(fd, (char *)&flag, sizeof(flag));
	flag &= PYSDK_IMG_FLAG_MASK;
	part_close(fd);

	return flag;
}

/* Return Code:
 * 		0: fail
 *		1: success
 */
int verify_pysdk_img_sign(const char *img_path)
{
	DIR *d = NULL;
	struct dirent *dir_entry = NULL;
	char pysdk[128] = {0};
	char file[64] = {0};
	char pysdk_sign_file[64] = {0};
	
	if(NULL == img_path){
		return 0;
	}	

	d = opendir(img_path);
	if(!d){
		LOG_ER("open dir %s failed(%d:%s)", img_path, errno, strerror(errno));
		return 0;
	}

	while((dir_entry = readdir(d))){
		 if(strstr(dir_entry->d_name, ".tar.gz")){
			strlcpy(file, dir_entry->d_name, sizeof(file));
			break;
		 }
	}
	closedir(d);

	if(!file[0]){
		//LOG_ER("not found pysdk");
		return 0;
	}

	snprintf(pysdk_sign_file, sizeof(pysdk_sign_file), "%s/pysdk.sign", img_path);	
	snprintf(pysdk, sizeof(pysdk), "%s/%s", img_path, file);
	if(_pysdk_verify(pysdk, pysdk_sign_file)){
		LOG_DB("PySDK %s verify failed\n", file);
		return 0;
	}
	
	LOG_DB("PySDK %s verify OK", file);
	return 1;
}

/* Return Code:
 * 		0: backup error
 * 		1: backup successfully
 */
int backup_pysdk_img()
{
	char cmd[128];

	/*Erase 2nd img partiton*/
	snprintf(cmd, sizeof(cmd), "rm -rf %s/*;sync", BACKUP_PYSDK_IMG_PATH);
	system(cmd);

	/*Copy from pysdk1 to pysdk2*/	
	snprintf(cmd, sizeof(cmd), "cp %s/* %s/;sync", MAIN_PYSDK_IMG_PATH, BACKUP_PYSDK_IMG_PATH);
	system(cmd);

	/*Verify*/
	return verify_pysdk_img_sign(BACKUP_PYSDK_IMG_PATH);
}

/* Return Code:
 * 		0: recover error
 * 		1: recover successfully
 */
int recover_pysdk_img()
{
	char cmd[128];

	erase_pysdk_img_flag();//S0
	/*Erase 1st img partiton*/
	snprintf(cmd, sizeof(cmd), "rm -rf %s/*;sync", MAIN_PYSDK_IMG_PATH);
	system(cmd);
	set_pysdk_img_flag(PYSDK_IMG_FLAG_S1);
	/*Copy from pysdk2 to pysdk1*/
	snprintf(cmd, sizeof(cmd), "cp %s/* %s/;sync", BACKUP_PYSDK_IMG_PATH, MAIN_PYSDK_IMG_PATH);
	system(cmd);
	set_pysdk_img_flag(PYSDK_IMG_FLAG_S2);
	/*Verify*/
	return verify_pysdk_img_sign(MAIN_PYSDK_IMG_PATH);
}

