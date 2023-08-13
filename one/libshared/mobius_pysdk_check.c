#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include "shared.h"
#include "python_ipc.h"
#include "ih_logtrace.h"
#include "mobius_pysdk_img.h"

int _pysdk_verify(const char *pysdkfile, const char *signfile)
{
	FILE *fp = NULL;
	char cmd[512] = {0};
	char buf[64] = {0};
	char *certs = "/tmp/rsa_pysdk_public.key";
	
	if(!pysdkfile || !signfile){
		return -1;
	}

	if(access(pysdkfile, F_OK) || access(signfile, F_OK)){
		LOG_IN("pysdk or sign file not exists");
		return -1;
	}

	snprintf(cmd, sizeof(cmd), "openssl2 enc -d -aes256 -pass pass:%s -in %s -out %s", PYSDK_PW, PYSDK_RSA_PUB_KEY, certs);
	system(cmd);
	
	// TODO: check return string or code?
	snprintf(cmd, sizeof(cmd), "openssl2 dgst -verify %s -sha1 -signature %s %s", certs, signfile, pysdkfile);
	fp = popen(cmd, "r");
	if(!fp){
		syslog(LOG_ERR, "popen cmd:%s failed(%s)", cmd, strerror(errno));
		unlink(certs); // delete public key
		return -1;
	}

	fgets(buf, sizeof(buf), fp);
	pclose(fp);

	trim_str(buf);
	unlink(certs);

	if(strstr(buf, "OK")){
		return 0;
	}

	return -1;
}

