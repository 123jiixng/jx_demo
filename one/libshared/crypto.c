/*
 * $Id$ --
 *
 *   Crypto routines
 *
 * Copyright (c) 2001-2010 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 06/04/2010
 * Author: Jianliang Zhang
 *
 */

#include <wait.h>
#include <errno.h>
#include <syslog.h>

#include "shared.h"
#include "ih_errno.h"

#define AES_BLOCK_SIZE  16
#define NVRAM_CRYPTO_KEY_SIZE        128     /* bits */
#define NVRAM_CRYPTO_KEY	"nvram_key@inhand"

#ifdef INHAND_IDTU9
#define TMP_SECURITY_KEY_FILE		"/tmp/security.bin"
#define TMP_SECURITY_KEY_FILE_ENC	"/tmp/security_enc.bin"
#define TMP_SECURITY_KEY_FILE_DEC	"/tmp/security_dec.bin"
#define TMP_SECURITY_PRIVKEY		"/tmp/security_priv.bin"
#define TMP_SECURITY_PUBKEY			"/tmp/security_pub.bin"
#define	SECURITY_KEY_MAX			2
#define	SECURITY_PRIVKEY_LEN		32		//privkey 32, pubkey 64
#define	SECURITY_PUBKEY_LEN			64	//privkey 32, pubkey 64
#define	SECURITY_KEY_LEN			(SECURITY_PRIVKEY_LEN + SECURITY_PUBKEY_LEN)	//privkey 32, pubkey 64
#endif

/*                              "1234567890123456" */

/**
 * make a copy for data
 * @param	buf	data for copy
 * @param	len	data length
 * @return	a copy for data or null for failure
 */
void * crypto_dup(const char *buf, int ilen, int olen)
{
	void *p;

	p = malloc(olen);
	if(!p){
		syslog(LOG_ERR, "out of memory!");
		return NULL;
	}

	memset(p, 0, olen);
	memcpy(p, buf, ilen);

	return p;
}



/**
 * encrypt a string and convert the output to a hex string
 * you should make sure the output buffer is larger than the input one
 * @param	ibuf	input
 * @param	ilen	input length
 * @param	obuf	output
 * @param	olen	output length
 * @return	0 for success, <0 for failure
 */
int
aes_encrypt_str (const char *ibuf, int ilen, char *obuf, int olen)
{
	unsigned char IV[AES_BLOCK_SIZE]="\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0";
	aes_context ac;
	void *p = NULL;
	void *p2 = NULL;
	int ret;
	int len = (ilen+AES_BLOCK_SIZE-1)&(~(AES_BLOCK_SIZE-1));

	p = crypto_dup(ibuf, ilen, len);
	if(!p) return -1;

	p2 = malloc(len);
	if(!p2){
		free(p);
		syslog(LOG_ERR, "out of memory!");
		return -1;
	}

	AES_set_key(&ac, (unsigned char*)NVRAM_CRYPTO_KEY, NVRAM_CRYPTO_KEY_SIZE);

	ret = IH_AES_cbc_encrypt(&ac, p, p2, len, IV, 1);
	bin2str(p2, len, obuf);
	free(p);
	free(p2);

	return ret;
}

/**
 * decrypt a hex string to a string (maybe binary!)
 * @param	ibuf	input
 * @param	ilen	input length
 * @param	obuf	output
 * @param	olen	output length
 * @return	> 0 for success, <0 for failure
 */
int
aes_decrypt_str (const char *ibuf, int ilen, char *obuf, int olen)
{
	unsigned char IV[AES_BLOCK_SIZE]="\0\0\0\0\0\0\0\0" "\0\0\0\0\0\0\0\0";
	aes_context ac;
	void *p = NULL;
	int ret;
	int len;

	if(ilen%(2*AES_BLOCK_SIZE)){
		syslog(LOG_ERR, "bad AES string!");
		return -1;
	}

	len = ilen / 2;
	p = malloc(len);
	if(!p){
		syslog(LOG_ERR, "out of memory!");
		return -1;
	}

	str2bin(ibuf, len, p);

	AES_set_key(&ac, (unsigned char *)NVRAM_CRYPTO_KEY, NVRAM_CRYPTO_KEY_SIZE);

//	memset(obuf, '\0', olen);//add by zly, important
	ret = IH_AES_cbc_encrypt(&ac, p, (unsigned char*)obuf, len, IV, 0);
	free(p);

	return ret;
}

/** @brief encrypt/decrypt a file
 *   Note: file is encrypt/decrypt block by block, using cbc in each block,
 *         thus please NERVER change block size (i.e. buffer length)!!!
 *
 * @param filein	input file name
 * @param fileout	output file name
 * @param enc		encrypt or decrypt
 *
 * @return <0 for error, >=0 for output file length
 */
int crypt_file(const char *filein, const char *fileout, int enc)
{
	FILE *fpin, *fpout;
	char buf[AES_BLOCK_SIZE*32];
	char obuf[2*AES_BLOCK_SIZE*32]; //double size of inbuf
	int flen = 0, ilen, olen, total;
	char *mark = "$AES$";
	int dec = 0;

	fpin = fopen(filein, "rb");
	if (!fpin) {
		return -1;
	}

	fpout = fopen(fileout, "wb");
	if (!fpout) {
		fclose(fpin);
		return -2;
	}

	if (enc) {
		fseek(fpin, 0, SEEK_END);
		flen = ftell(fpin);
		fseek(fpin, 0, SEEK_SET);

		//set a head
		olen = sprintf(obuf, "%s len=%d\n", mark, flen);
		fwrite(obuf, 1, olen, fpout);
	}else{
		//check crypto mark
		if (fgets(buf, sizeof(buf), fpin)==NULL){
			ilen = 0;
		}else{
			ilen = strlen(buf);
		}

		olen = strlen(mark);
		if (ilen < olen || strncmp(buf, mark, olen)!=0) {
			fseek(fpin, 0, SEEK_SET); //reset to head
		}else{
			sscanf(buf + olen + 1, "len=%d", &flen);
			if (flen>=0) dec = 1; //need to decrypt
			else fseek(fpin, 0, SEEK_SET); //reset to head
		}
	}

	total = 0;
	while (flen>0 && !feof(fpin)) {
		if (enc) {
			ilen = fread(buf, 1, sizeof(buf), fpin);
		}else{
			ilen = fread(obuf, 1, sizeof(obuf), fpin);
		}
		if (ilen<=0) {
			if (errno==EINTR) continue;
			break;
		}

		if (enc) {
			olen = aes_encrypt_str(buf, ilen, obuf, sizeof(obuf));
			if (olen<=0) {
				break;
			}else{
				olen *= 2;
			}
		}else if (dec){
			olen = aes_decrypt_str(obuf, ilen, buf, sizeof(buf));
			if (olen<=0) break;
			memcpy(obuf, buf, olen);
			if (flen<olen) olen = flen;
			flen -= olen;
		}else{
			olen = ilen;
		}

		fwrite(obuf, 1, olen, fpout);
		total += olen;
	}

	fclose(fpin);
	fclose(fpout);

	return total;
}

int encode_file(FILE *fp, char *buf, int enc)
{
	unsigned char c;
	int n;

	if(enc){
		void *p;

		fseek(fp, 0, SEEK_END);
		n = ftell(fp);

		n = (n+AES_BLOCK_SIZE-1) & (~(AES_BLOCK_SIZE-1));

		if(!buf) return 2*n + 5; //5 is AES head size ($AES$)

		fseek(fp, 0, SEEK_SET);

		p = malloc(n);
		if(!p){
			syslog(LOG_ERR, "out of memory when encoding file!");
			return -1;
		}

		memset(p, 0, n);
		fread(p, 1, n, fp);
		strcpy(buf, "$AES$");
		buf += 5;
		aes_encrypt_str(p, n, buf, 2*n);
		free(p);

		return 2*n + 5;
	}

	n = 0;
	while(fread(&c, 1, 1, fp)==1){
		if (c >= 32 && c <= 126 && c != '~') {
			n++;
			if(buf) *(buf++) = (char)c;
//		} else if (c == 13)	{
//			n++;
//			if(buf) *(buf++) = (char)c;
		} else if (c == 0) {
			n++;
			if(buf) *(buf++) = '~';
//		} else if (c == 10) {
//			n++;
//			if(buf) *(buf++) = (char)c;
		} else {
			n+=3;
			if(buf){
				*(buf++) = '\\';
				sprintf (buf, "%02X", c);
				buf += 2;
			}
		}
	}

	//n++;
	//if(buf) *(buf++) = '\0';

	return n;
}

int decode_file(FILE *fp, char *buf)
{
	int i, n;
	unsigned char c;
	unsigned int x;
	char tmp;

	for (n=0, i=0; buf[i]; n++) {
		if (buf[i] == '\\')  {
			i++;
			tmp = buf[i+2];
			buf[i+2] = 0;
			sscanf(buf + i, "%02X", &x);
			c = (unsigned char) x;
			buf[i+2] = tmp;
			i += 2;
		} else if (buf[i] == '~'){
			c = (unsigned char) buf[i];
			i++;
		} else {
			c = (unsigned char) buf[i];
			i++;
		}

		fwrite (&c, 1, 1, fp);
    }

	return n;
}

#ifdef INHAND_IDTU9

char *hsec_get_security_chip_version(void)
{
	char *str = NULL;
	static char ver[16];
	int status;

	memset(ver, 0, sizeof(ver));

	status = system(INIT_CMD1);
	if(status == -1 || !WIFEXITED(status) || (WEXITSTATUS(status) != 0)){
		syslog(LOG_ERR, "get security chip version error, status[%d]!", status);
		return NULL;
	}

	str = file2str(CHIP_TEST_FILE);
	if (!str) {
		syslog(LOG_ERR, "get security chip version error %d [%s]!", errno, strerror(errno));
		free(str);
		return NULL;
	}

	strlcpy(ver, str, sizeof(ver));
	free(str);
	str = NULL;

    return ver;
}

void hsec_reset_security_chip(void)
{
    int value;

    //reset security chip
    value = 0;
    gpio(GPIO_WRITE,GPIO_SECURITY_CHIP_RESET,&value);
    usleep(100);
    value = 1;
    gpio(GPIO_WRITE,GPIO_SECURITY_CHIP_RESET,&value);

    return;
}

int hsec_import_security_key(char *file)
{
    int i;
	int status;
	long file_len;
	char cmd[128];

//		openssl2 enc -in test.enc -out test.dec -pass pass:123456 -d -a -aes-256-cbc
	if(!file || !f_exists(file)){
		syslog(LOG_DEBUG, "security key file doesn't exist");
		return ERR_FAILED;
	}

	sprintf(cmd, "openssl2 enc -in %s -out %s -pass pass:%s -d -a -aes-256-cbc", 
			TMP_SECURITY_KEY_FILE, TMP_SECURITY_KEY_FILE_DEC, NVRAM_CRYPTO_KEY);
	status = system(cmd);
	if(status == -1 || !WIFEXITED(status) || (WEXITSTATUS(status) != 0)){
		syslog(LOG_ERR, " decrypt security key error, status[%d]!", status);
		return ERR_FAILED;
	}

	file_len=f_size(TMP_SECURITY_KEY_FILE_DEC);

	if (file_len%SECURITY_KEY_LEN != 0) {
		syslog(LOG_ERR, "import security key error!");
		return ERR_FAILED;
	}

	for (i =0; i < file_len/SECURITY_KEY_LEN; i++) {
		snprintf(cmd, sizeof(cmd), "dd if=%s of=%s bs=1 skip=%d count=%d",
				TMP_SECURITY_KEY_FILE_DEC, TMP_SECURITY_PRIVKEY, i*SECURITY_KEY_LEN, SECURITY_PRIVKEY_LEN);
		status = system(cmd);
		if(status == -1 || !WIFEXITED(status) || (WEXITSTATUS(status) != 0)){
			syslog(LOG_ERR, " split key, status[%d]!", status);
			return ERR_FAILED;
		}

		snprintf(cmd, sizeof(cmd), "dd if=%s of=%s bs=1 skip=%d count=%d",
				TMP_SECURITY_KEY_FILE_DEC, TMP_SECURITY_PUBKEY, i*SECURITY_KEY_LEN + SECURITY_PRIVKEY_LEN, SECURITY_PUBKEY_LEN);
		status = system(cmd);
		if(status == -1 || !WIFEXITED(status) || (WEXITSTATUS(status) != 0)){
			syslog(LOG_ERR, " split key, status[%d]!", status);
			return ERR_FAILED;
		}

		sprintf(cmd, "pki -e -o write -t privkey -k %d -i %s", i + 1, TMP_SECURITY_PRIVKEY);
		status = system(cmd);
		if(status == -1 || !WIFEXITED(status) || (WEXITSTATUS(status) != 0)){
			syslog(LOG_ERR, " write private key error, status[%d]!", status);
			return ERR_FAILED;
		}

		sprintf(cmd, "pki -e -o write -t pubkey -k %d -i %s", i + 1, TMP_SECURITY_PUBKEY);
		status = system(cmd);
		if(status == -1 || !WIFEXITED(status) || (WEXITSTATUS(status) != 0)){
			syslog(LOG_ERR, " write public key error, status[%d]!", status);
			return ERR_FAILED;
		}
	}

	unlink(TMP_SECURITY_KEY_FILE);
	unlink(TMP_SECURITY_KEY_FILE_DEC);
	unlink(TMP_SECURITY_PUBKEY);
	unlink(TMP_SECURITY_PRIVKEY);

    return ERR_OK;
}

int hsec_get_security_key(char *file)
{
    int i;
	int status;
	long file_len;
	char cmd[128];

	if(!file){
		syslog(LOG_DEBUG, "security key file doesn't exist");
		return ERR_FAILED;
	}

	unlink(TMP_SECURITY_KEY_FILE_DEC);
	for (i =1; i <=SECURITY_KEY_MAX; i++) {
		sprintf(cmd, "pki -e -o read -t privkey -k %d >> %s", i, TMP_SECURITY_KEY_FILE_DEC);
		status = system(cmd);
		if(status == -1 || !WIFEXITED(status) || (WEXITSTATUS(status) != 0)){
			syslog(LOG_ERR, " read private key error, status[%d]!", status);
			return ERR_FAILED;
		}

		sprintf(cmd, "pki -e -o read -t pubkey -k %d >> %s", i, TMP_SECURITY_KEY_FILE_DEC);
		status = system(cmd);
		if(status == -1 || !WIFEXITED(status) || (WEXITSTATUS(status) != 0)){
			syslog(LOG_ERR, " read public key error, status[%d]!", status);
			return ERR_FAILED;
		}
	}

	file_len=f_size(TMP_SECURITY_KEY_FILE_DEC);

	if (file_len != SECURITY_KEY_MAX * SECURITY_KEY_LEN) {
		syslog(LOG_ERR, "get security key error!");
		return ERR_FAILED;
	}

	sprintf(cmd, "openssl2 enc -in %s -out %s -pass pass:%s -e -a -salt -aes-256-cbc", 
			TMP_SECURITY_KEY_FILE_DEC, file, NVRAM_CRYPTO_KEY);
	status = system(cmd);
	if(status == -1 || !WIFEXITED(status) || (WEXITSTATUS(status) != 0)){
		syslog(LOG_ERR, " encrypt security key error, status[%d]!", status);
		return ERR_FAILED;
	}

	unlink(TMP_SECURITY_KEY_FILE_DEC);
    return ERR_OK;
}
#endif
