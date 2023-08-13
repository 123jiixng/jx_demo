#include <stdio.h>
#include <string.h>

#include "ih_types.h"
#include "ih_logtrace.h"
#include "ih_errno.h"
#include "files.h"
#include <openssl/evp.h>
#include <openssl/opensslv.h>

#if (OPENSSL_VERSION_NUMBER < 0x10100000L)	//version < 1.1.0
#define OPENSSL_V1_0_2
#endif

int32 digest(char *type, char *message, int len, unsigned char *hash, size_t hash_len)
{
	EVP_MD_CTX *mdctx;
	const EVP_MD *md;
	unsigned char md_value[EVP_MAX_MD_SIZE];
	unsigned int md_len;

	if(!type || !message || len <= 0 || !hash || hash_len <= 0) {
		LOG_IN("message for digest is invalid");
		return ERR_INVAL;
	}

#ifdef OPENSSL_V1_0_2
	OpenSSL_add_all_digests();
#endif

	md = EVP_get_digestbyname(type);
	if(!md) {
		LOG_IN("Unknown message digest %s\n", type);
		return ERR_INVAL;
	}

#ifdef OPENSSL_V1_0_2
	mdctx = EVP_MD_CTX_create();
#else
	mdctx = EVP_MD_CTX_new();
#endif
	EVP_DigestInit_ex(mdctx, md, NULL);
	EVP_DigestUpdate(mdctx, message, len);
	EVP_DigestFinal_ex(mdctx, md_value, &md_len);
#ifdef OPENSSL_V1_0_2
	EVP_MD_CTX_destroy(mdctx);
#else
	EVP_MD_CTX_free(mdctx);
#endif

	if (hash_len < md_len) {
		return ERR_INVAL;
	}

	memset(hash, 0, hash_len);
	memcpy(hash, md_value, md_len);

	return ERR_OK;
}


int32 digest_str(char *type, char *message, int len, char *hash, int hash_len)
{
	int i = 0, idx = 0, len_min = 0;
	unsigned char hash_tmp[128];

	if(!type || !message || len <= 0 || !hash || hash_len <= 0) {
		LOG_IN("message for digest is invalid");
		return ERR_INVAL;
	}

	memset(hash, 0, hash_len);

	if (digest(type, message, len, hash_tmp, sizeof(hash_tmp)) == ERR_OK) {
		len_min = hash_len > sizeof(hash_tmp) ? sizeof(hash_tmp) : hash_len;
		while(idx < len_min){
			idx += snprintf(hash+idx, hash_len-idx, "%02x", hash_tmp[i++] & 0xff);
		}
		LOG_DB("hash %s of key [%s] is [%s]", type, message, hash);
		return ERR_OK;
	}

	LOG_ER("hash %s of key [%s] failed", type, message);

	return ERR_FAILED;
}

int indigest_main(int argc, char *argv[])
{
	char type[16], *message = NULL;
	unsigned char hash[64];
	char filename[64];
	int ret;

	if (argc < 3) {
		printf("help: digest {md5|sha1|sha256|sha384|sha512} [-in <file> | <string>] [-out file]");
		return -1;
	}

	memset(type,0, sizeof(type));
	memset(hash,0, sizeof(hash));
	memset(filename,0, sizeof(filename));

	memcpy(type, argv[1], sizeof(type));
	
	if (strcmp(argv[2], "-in") == 0) {
		if (argc < 4) {
			printf("help: digest {md5|sha1|sha256|sha384|sha512} [-in <file> | <string>] [-out file]");
			return -1;
		}

		memcpy(filename, argv[3], sizeof(filename));
		message =  file2str(filename);
	} else {
		message = strdup2(argv[2]);
	}


	//FIXME -out
	ret = digest(type, message, strlen(message), hash, sizeof(hash));
	if (message) {
		free(message);
	}

	return ret;
}
