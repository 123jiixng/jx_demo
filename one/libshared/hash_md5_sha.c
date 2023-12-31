/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 2010 Denys Vlasenko
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */

#include "hash_md5_sha.h"

/* gcc 4.2.1 optimizes rotr64 better with inline than with macro
 * (for rotX32, there is no difference). Why? My guess is that
 * macro requires clever common subexpression elimination heuristics
 * in gcc, while inline basically forces it to happen.
 */
//#define rotl32(x,n) (((x) << (n)) | ((x) >> (32 - (n))))
static inline uint32_t rotl32(uint32_t x, unsigned n)
{
	return (x << n) | (x >> (32 - n));
}
//#define rotr32(x,n) (((x) >> (n)) | ((x) << (32 - (n))))
static inline uint32_t rotr32(uint32_t x, unsigned n)
{
	return (x >> n) | (x << (32 - n));
}
/* rotr64 in needed for sha512 only: */
//#define rotr64(x,n) (((x) >> (n)) | ((x) << (64 - (n))))
static inline uint64_t rotr64(uint64_t x, unsigned n)
{
	return (x >> n) | (x << (64 - n));
}


/* Feed data through a temporary buffer.
 * The internal buffer remembers previous data until it has 64
 * bytes worth to pass on.
 */
static void common64_hash(md5_ctx_t *ctx, const void *buffer, size_t len)
{
	unsigned bufpos = ctx->total64 & 63;

	ctx->total64 += len;

	while (1) {
		unsigned remaining = 64 - bufpos;
		if (remaining > len)
			remaining = len;
		/* Copy data into aligned buffer */
		memcpy(ctx->wbuffer + bufpos, buffer, remaining);
		len -= remaining;
		buffer = (const char *)buffer + remaining;
		bufpos += remaining;
		/* clever way to do "if (bufpos != 64) break; ... ; bufpos = 0;" */
		bufpos -= 64;
		if (bufpos != 0)
			break;
		/* Buffer is filled up, process it */
		ctx->process_block(ctx);
		/*bufpos = 0; - already is */
	}
}

/* Process the remaining bytes in the buffer */
static void common64_end(md5_ctx_t *ctx, int swap_needed)
{
	unsigned bufpos = ctx->total64 & 63;
	uint64_t **p;

	/* Pad the buffer to the next 64-byte boundary with 0x80,0,0,0... */
	ctx->wbuffer[bufpos++] = 0x80;

	/* This loop iterates either once or twice, no more, no less */
	while (1) {
		unsigned remaining = 64 - bufpos;
		memset(ctx->wbuffer + bufpos, 0, remaining);
		/* Do we have enough space for the length count? */
		if (remaining >= 8) {
			/* Store the 64-bit counter of bits in the buffer */
			uint64_t t = ctx->total64 << 3;
			if (swap_needed)
				t = bb_bswap_64(t);
			/* wbuffer is suitably aligned for this */
#if 1
			*p = (uint64_t *)&ctx->wbuffer[64 - 8];
			**p = t;
#else
			*(uint64_t *) (&ctx->wbuffer[64 - 8]) = t;
#endif
		}
		ctx->process_block(ctx);
		if (remaining >= 8)
			break;
		bufpos = 0;
	}
}


/*
 * Compute MD5 checksum of strings according to the
 * definition of MD5 in RFC 1321 from April 1992.
 *
 * Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.
 *
 * Copyright (C) 1995-1999 Free Software Foundation, Inc.
 * Copyright (C) 2001 Manuel Novoa III
 * Copyright (C) 2003 Glenn L. McGrath
 * Copyright (C) 2003 Erik Andersen
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */

/* 0: fastest, 3: smallest */
#if CONFIG_MD5_SIZE_VS_SPEED < 0
# define MD5_SIZE_VS_SPEED 0
#elif CONFIG_MD5_SIZE_VS_SPEED > 3
# define MD5_SIZE_VS_SPEED 3
#else
# define MD5_SIZE_VS_SPEED CONFIG_MD5_SIZE_VS_SPEED
#endif

/* These are the four functions used in the four steps of the MD5 algorithm
 * and defined in the RFC 1321.  The first function is a little bit optimized
 * (as found in Colin Plumbs public domain implementation).
 * #define FF(b, c, d) ((b & c) | (~b & d))
 */
#undef FF
#undef FG
#undef FH
#undef FI
#define FF(b, c, d) (d ^ (b & (c ^ d)))
#define FG(b, c, d) FF(d, b, c)
#define FH(b, c, d) (b ^ c ^ d)
#define FI(b, c, d) (c ^ (b | ~d))

/* Hash a single block, 64 bytes long and 4-byte aligned */
static void md5_process_block64(md5_ctx_t *ctx)
{
#if MD5_SIZE_VS_SPEED > 0
	/* Before we start, one word to the strange constants.
	   They are defined in RFC 1321 as
	   T[i] = (int)(4294967296.0 * fabs(sin(i))), i=1..64
	 */
	static const uint32_t C_array[] = {
		/* round 1 */
		0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
		0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
		0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
		0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
		/* round 2 */
		0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
		0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
		0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
		0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
		/* round 3 */
		0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
		0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
		0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x4881d05,
		0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
		/* round 4 */
		0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
		0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
		0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
		0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
	};
	static const char P_array[] ALIGN1 = {
# if MD5_SIZE_VS_SPEED > 1
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, /* 1 */
# endif
		1, 6, 11, 0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, /* 2 */
		5, 8, 11, 14, 1, 4, 7, 10, 13, 0, 3, 6, 9, 12, 15, 2, /* 3 */
		0, 7, 14, 5, 12, 3, 10, 1, 8, 15, 6, 13, 4, 11, 2, 9  /* 4 */
	};
#endif
	uint32_t *words = (void*) ctx->wbuffer;
	uint32_t A = ctx->hash[0];
	uint32_t B = ctx->hash[1];
	uint32_t C = ctx->hash[2];
	uint32_t D = ctx->hash[3];

#if MD5_SIZE_VS_SPEED >= 2  /* 2 or 3 */

	static const char S_array[] ALIGN1 = {
		7, 12, 17, 22,
		5, 9, 14, 20,
		4, 11, 16, 23,
		6, 10, 15, 21
	};
	const uint32_t *pc;
	const char *pp;
	const char *ps;
	int i;
	uint32_t temp;

# if BB_BIG_ENDIAN
	for (i = 0; i < 16; i++)
		words[i] = SWAP_LE32(words[i]);
# endif

# if MD5_SIZE_VS_SPEED == 3
	pc = C_array;
	pp = P_array;
	ps = S_array - 4;

	for (i = 0; i < 64; i++) {
		if ((i & 0x0f) == 0)
			ps += 4;
		temp = A;
		switch (i >> 4) {
		case 0:
			temp += FF(B, C, D);
			break;
		case 1:
			temp += FG(B, C, D);
			break;
		case 2:
			temp += FH(B, C, D);
			break;
		case 3:
			temp += FI(B, C, D);
		}
		temp += words[(int) (*pp++)] + *pc++;
		temp = rotl32(temp, ps[i & 3]);
		temp += B;
		A = D;
		D = C;
		C = B;
		B = temp;
	}
# else  /* MD5_SIZE_VS_SPEED == 2 */
	pc = C_array;
	pp = P_array;
	ps = S_array;

	for (i = 0; i < 16; i++) {
		temp = A + FF(B, C, D) + words[(int) (*pp++)] + *pc++;
		temp = rotl32(temp, ps[i & 3]);
		temp += B;
		A = D;
		D = C;
		C = B;
		B = temp;
	}
	ps += 4;
	for (i = 0; i < 16; i++) {
		temp = A + FG(B, C, D) + words[(int) (*pp++)] + *pc++;
		temp = rotl32(temp, ps[i & 3]);
		temp += B;
		A = D;
		D = C;
		C = B;
		B = temp;
	}
	ps += 4;
	for (i = 0; i < 16; i++) {
		temp = A + FH(B, C, D) + words[(int) (*pp++)] + *pc++;
		temp = rotl32(temp, ps[i & 3]);
		temp += B;
		A = D;
		D = C;
		C = B;
		B = temp;
	}
	ps += 4;
	for (i = 0; i < 16; i++) {
		temp = A + FI(B, C, D) + words[(int) (*pp++)] + *pc++;
		temp = rotl32(temp, ps[i & 3]);
		temp += B;
		A = D;
		D = C;
		C = B;
		B = temp;
	}
# endif
	/* Add checksum to the starting values */
	ctx->hash[0] += A;
	ctx->hash[1] += B;
	ctx->hash[2] += C;
	ctx->hash[3] += D;

#else  /* MD5_SIZE_VS_SPEED == 0 or 1 */

	uint32_t A_save = A;
	uint32_t B_save = B;
	uint32_t C_save = C;
	uint32_t D_save = D;
# if MD5_SIZE_VS_SPEED == 1
	const uint32_t *pc;
	const char *pp;
	int i;
# endif

	/* First round: using the given function, the context and a constant
	   the next context is computed.  Because the algorithm's processing
	   unit is a 32-bit word and it is determined to work on words in
	   little endian byte order we perhaps have to change the byte order
	   before the computation.  To reduce the work for the next steps
	   we save swapped words in WORDS array.  */
# undef OP
# define OP(a, b, c, d, s, T) \
	do { \
		a += FF(b, c, d) + (*words IF_BIG_ENDIAN(= SWAP_LE32(*words))) + T; \
		words++; \
		a = rotl32(a, s); \
		a += b; \
	} while (0)

	/* Round 1 */
# if MD5_SIZE_VS_SPEED == 1
	pc = C_array;
	for (i = 0; i < 4; i++) {
		OP(A, B, C, D, 7, *pc++);
		OP(D, A, B, C, 12, *pc++);
		OP(C, D, A, B, 17, *pc++);
		OP(B, C, D, A, 22, *pc++);
	}
# else
	OP(A, B, C, D, 7, 0xd76aa478);
	OP(D, A, B, C, 12, 0xe8c7b756);
	OP(C, D, A, B, 17, 0x242070db);
	OP(B, C, D, A, 22, 0xc1bdceee);
	OP(A, B, C, D, 7, 0xf57c0faf);
	OP(D, A, B, C, 12, 0x4787c62a);
	OP(C, D, A, B, 17, 0xa8304613);
	OP(B, C, D, A, 22, 0xfd469501);
	OP(A, B, C, D, 7, 0x698098d8);
	OP(D, A, B, C, 12, 0x8b44f7af);
	OP(C, D, A, B, 17, 0xffff5bb1);
	OP(B, C, D, A, 22, 0x895cd7be);
	OP(A, B, C, D, 7, 0x6b901122);
	OP(D, A, B, C, 12, 0xfd987193);
	OP(C, D, A, B, 17, 0xa679438e);
	OP(B, C, D, A, 22, 0x49b40821);
# endif
	words -= 16;

	/* For the second to fourth round we have the possibly swapped words
	   in WORDS.  Redefine the macro to take an additional first
	   argument specifying the function to use.  */
# undef OP
# define OP(f, a, b, c, d, k, s, T) \
	do { \
		a += f(b, c, d) + words[k] + T; \
		a = rotl32(a, s); \
		a += b; \
	} while (0)

	/* Round 2 */
# if MD5_SIZE_VS_SPEED == 1
	pp = P_array;
	for (i = 0; i < 4; i++) {
		OP(FG, A, B, C, D, (int) (*pp++), 5, *pc++);
		OP(FG, D, A, B, C, (int) (*pp++), 9, *pc++);
		OP(FG, C, D, A, B, (int) (*pp++), 14, *pc++);
		OP(FG, B, C, D, A, (int) (*pp++), 20, *pc++);
	}
# else
	OP(FG, A, B, C, D, 1, 5, 0xf61e2562);
	OP(FG, D, A, B, C, 6, 9, 0xc040b340);
	OP(FG, C, D, A, B, 11, 14, 0x265e5a51);
	OP(FG, B, C, D, A, 0, 20, 0xe9b6c7aa);
	OP(FG, A, B, C, D, 5, 5, 0xd62f105d);
	OP(FG, D, A, B, C, 10, 9, 0x02441453);
	OP(FG, C, D, A, B, 15, 14, 0xd8a1e681);
	OP(FG, B, C, D, A, 4, 20, 0xe7d3fbc8);
	OP(FG, A, B, C, D, 9, 5, 0x21e1cde6);
	OP(FG, D, A, B, C, 14, 9, 0xc33707d6);
	OP(FG, C, D, A, B, 3, 14, 0xf4d50d87);
	OP(FG, B, C, D, A, 8, 20, 0x455a14ed);
	OP(FG, A, B, C, D, 13, 5, 0xa9e3e905);
	OP(FG, D, A, B, C, 2, 9, 0xfcefa3f8);
	OP(FG, C, D, A, B, 7, 14, 0x676f02d9);
	OP(FG, B, C, D, A, 12, 20, 0x8d2a4c8a);
# endif

	/* Round 3 */
# if MD5_SIZE_VS_SPEED == 1
	for (i = 0; i < 4; i++) {
		OP(FH, A, B, C, D, (int) (*pp++), 4, *pc++);
		OP(FH, D, A, B, C, (int) (*pp++), 11, *pc++);
		OP(FH, C, D, A, B, (int) (*pp++), 16, *pc++);
		OP(FH, B, C, D, A, (int) (*pp++), 23, *pc++);
	}
# else
	OP(FH, A, B, C, D, 5, 4, 0xfffa3942);
	OP(FH, D, A, B, C, 8, 11, 0x8771f681);
	OP(FH, C, D, A, B, 11, 16, 0x6d9d6122);
	OP(FH, B, C, D, A, 14, 23, 0xfde5380c);
	OP(FH, A, B, C, D, 1, 4, 0xa4beea44);
	OP(FH, D, A, B, C, 4, 11, 0x4bdecfa9);
	OP(FH, C, D, A, B, 7, 16, 0xf6bb4b60);
	OP(FH, B, C, D, A, 10, 23, 0xbebfbc70);
	OP(FH, A, B, C, D, 13, 4, 0x289b7ec6);
	OP(FH, D, A, B, C, 0, 11, 0xeaa127fa);
	OP(FH, C, D, A, B, 3, 16, 0xd4ef3085);
	OP(FH, B, C, D, A, 6, 23, 0x04881d05);
	OP(FH, A, B, C, D, 9, 4, 0xd9d4d039);
	OP(FH, D, A, B, C, 12, 11, 0xe6db99e5);
	OP(FH, C, D, A, B, 15, 16, 0x1fa27cf8);
	OP(FH, B, C, D, A, 2, 23, 0xc4ac5665);
# endif

	/* Round 4 */
# if MD5_SIZE_VS_SPEED == 1
	for (i = 0; i < 4; i++) {
		OP(FI, A, B, C, D, (int) (*pp++), 6, *pc++);
		OP(FI, D, A, B, C, (int) (*pp++), 10, *pc++);
		OP(FI, C, D, A, B, (int) (*pp++), 15, *pc++);
		OP(FI, B, C, D, A, (int) (*pp++), 21, *pc++);
	}
# else
	OP(FI, A, B, C, D, 0, 6, 0xf4292244);
	OP(FI, D, A, B, C, 7, 10, 0x432aff97);
	OP(FI, C, D, A, B, 14, 15, 0xab9423a7);
	OP(FI, B, C, D, A, 5, 21, 0xfc93a039);
	OP(FI, A, B, C, D, 12, 6, 0x655b59c3);
	OP(FI, D, A, B, C, 3, 10, 0x8f0ccc92);
	OP(FI, C, D, A, B, 10, 15, 0xffeff47d);
	OP(FI, B, C, D, A, 1, 21, 0x85845dd1);
	OP(FI, A, B, C, D, 8, 6, 0x6fa87e4f);
	OP(FI, D, A, B, C, 15, 10, 0xfe2ce6e0);
	OP(FI, C, D, A, B, 6, 15, 0xa3014314);
	OP(FI, B, C, D, A, 13, 21, 0x4e0811a1);
	OP(FI, A, B, C, D, 4, 6, 0xf7537e82);
	OP(FI, D, A, B, C, 11, 10, 0xbd3af235);
	OP(FI, C, D, A, B, 2, 15, 0x2ad7d2bb);
	OP(FI, B, C, D, A, 9, 21, 0xeb86d391);
# undef OP
# endif
	/* Add checksum to the starting values */
	ctx->hash[0] = A_save + A;
	ctx->hash[1] = B_save + B;
	ctx->hash[2] = C_save + C;
	ctx->hash[3] = D_save + D;
#endif
}
#undef FF
#undef FG
#undef FH
#undef FI

/* Initialize structure containing state of computation.
 * (RFC 1321, 3.3: Step 3)
 */
void md5_begin(md5_ctx_t *ctx)
{
	ctx->hash[0] = 0x67452301;
	ctx->hash[1] = 0xefcdab89;
	ctx->hash[2] = 0x98badcfe;
	ctx->hash[3] = 0x10325476;
	ctx->total64 = 0;
	ctx->process_block = md5_process_block64;
}

/* Used also for sha1 and sha256 */
void md5_hash(md5_ctx_t *ctx, const void *buffer, size_t len)
{
	common64_hash(ctx, buffer, len);
}

/* Process the remaining bytes in the buffer and put result from CTX
 * in first 16 bytes following RESBUF.  The result is always in little
 * endian byte order, so that a byte-wise output yields to the wanted
 * ASCII representation of the message digest.
 */
void md5_end(md5_ctx_t *ctx, void *resbuf)
{
	/* MD5 stores total in LE, need to swap on BE arches: */
	common64_end(ctx, /*swap_needed:*/ BB_BIG_ENDIAN);

	/* The MD5 result is in little endian byte order */
#if BB_BIG_ENDIAN
	ctx->hash[0] = SWAP_LE32(ctx->hash[0]);
	ctx->hash[1] = SWAP_LE32(ctx->hash[1]);
	ctx->hash[2] = SWAP_LE32(ctx->hash[2]);
	ctx->hash[3] = SWAP_LE32(ctx->hash[3]);
#endif
	memcpy(resbuf, ctx->hash, sizeof(ctx->hash[0]) * 4);
}


/*
 * SHA1 part is:
 * Copyright 2007 Rob Landley <rob@landley.net>
 *
 * Based on the public domain SHA-1 in C by Steve Reid <steve@edmweb.com>
 * from http://www.mirrors.wiretapped.net/security/cryptography/hashes/sha1/
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 *
 * ---------------------------------------------------------------------------
 *
 * SHA256 and SHA512 parts are:
 * Released into the Public Domain by Ulrich Drepper <drepper@redhat.com>.
 * Shrank by Denys Vlasenko.
 *
 * ---------------------------------------------------------------------------
 *
 * The best way to test random blocksizes is to go to coreutils/md5_sha1_sum.c
 * and replace "4096" with something like "2000 + time(NULL) % 2097",
 * then rebuild and compare "shaNNNsum bigfile" results.
 */

static void sha1_process_block64(sha1_ctx_t *ctx)
{
	static const uint32_t rconsts[] = {
		0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
	};
	int i, j;
	int cnt;
	uint32_t W[16+16];
	uint32_t a, b, c, d, e;

	/* On-stack work buffer frees up one register in the main loop
	 * which otherwise will be needed to hold ctx pointer */
	for (i = 0; i < 16; i++)
		W[i] = W[i+16] = SWAP_BE32(((uint32_t*)ctx->wbuffer)[i]);

	a = ctx->hash[0];
	b = ctx->hash[1];
	c = ctx->hash[2];
	d = ctx->hash[3];
	e = ctx->hash[4];

	/* 4 rounds of 20 operations each */
	cnt = 0;
	for (i = 0; i < 4; i++) {
		j = 19;
		do {
			uint32_t work;

			work = c ^ d;
			if (i == 0) {
				work = (work & b) ^ d;
				if (j <= 3)
					goto ge16;
				/* Used to do SWAP_BE32 here, but this
				 * requires ctx (see comment above) */
				work += W[cnt];
			} else {
				if (i == 2)
					work = ((b | c) & d) | (b & c);
				else /* i = 1 or 3 */
					work ^= b;
 ge16:
				W[cnt] = W[cnt+16] = rotl32(W[cnt+13] ^ W[cnt+8] ^ W[cnt+2] ^ W[cnt], 1);
				work += W[cnt];
			}
			work += e + rotl32(a, 5) + rconsts[i];

			/* Rotate by one for next time */
			e = d;
			d = c;
			c = /* b = */ rotl32(b, 30);
			b = a;
			a = work;
			cnt = (cnt + 1) & 15;
		} while (--j >= 0);
	}

	ctx->hash[0] += a;
	ctx->hash[1] += b;
	ctx->hash[2] += c;
	ctx->hash[3] += d;
	ctx->hash[4] += e;
}

/* Constants for SHA512 from FIPS 180-2:4.2.3.
 * SHA256 constants from FIPS 180-2:4.2.2
 * are the most significant half of first 64 elements
 * of the same array.
 */
static const uint64_t sha_K[80] = {
	0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
	0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
	0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
	0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
	0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
	0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
	0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
	0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
	0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
	0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
	0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
	0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
	0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
	0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
	0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
	0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
	0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
	0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
	0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
	0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
	0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
	0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
	0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
	0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
	0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
	0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
	0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
	0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
	0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
	0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
	0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
	0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
	0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, /* [64]+ are used for sha512 only */
	0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
	0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
	0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
	0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
	0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
	0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
	0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

#undef Ch
#undef Maj
#undef S0
#undef S1
#undef R0
#undef R1

static void sha256_process_block64(sha256_ctx_t *ctx)
{
	unsigned t;
	uint32_t W[64], a, b, c, d, e, f, g, h;
	const uint32_t *words = (uint32_t*) ctx->wbuffer;

	/* Operators defined in FIPS 180-2:4.1.2.  */
#define Ch(x, y, z) ((x & y) ^ (~x & z))
#define Maj(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define S0(x) (rotr32(x, 2) ^ rotr32(x, 13) ^ rotr32(x, 22))
#define S1(x) (rotr32(x, 6) ^ rotr32(x, 11) ^ rotr32(x, 25))
#define R0(x) (rotr32(x, 7) ^ rotr32(x, 18) ^ (x >> 3))
#define R1(x) (rotr32(x, 17) ^ rotr32(x, 19) ^ (x >> 10))

	/* Compute the message schedule according to FIPS 180-2:6.2.2 step 2.  */
	for (t = 0; t < 16; ++t)
		W[t] = SWAP_BE32(words[t]);
	for (/*t = 16*/; t < 64; ++t)
		W[t] = R1(W[t - 2]) + W[t - 7] + R0(W[t - 15]) + W[t - 16];

	a = ctx->hash[0];
	b = ctx->hash[1];
	c = ctx->hash[2];
	d = ctx->hash[3];
	e = ctx->hash[4];
	f = ctx->hash[5];
	g = ctx->hash[6];
	h = ctx->hash[7];

	/* The actual computation according to FIPS 180-2:6.2.2 step 3.  */
	for (t = 0; t < 64; ++t) {
		/* Need to fetch upper half of sha_K[t]
		 * (I hope compiler is clever enough to just fetch
		 * upper half)
		 */
		uint32_t K_t = sha_K[t] >> 32;
		uint32_t T1 = h + S1(e) + Ch(e, f, g) + K_t + W[t];
		uint32_t T2 = S0(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;
	}
#undef Ch
#undef Maj
#undef S0
#undef S1
#undef R0
#undef R1
	/* Add the starting values of the context according to FIPS 180-2:6.2.2
	   step 4.  */
	ctx->hash[0] += a;
	ctx->hash[1] += b;
	ctx->hash[2] += c;
	ctx->hash[3] += d;
	ctx->hash[4] += e;
	ctx->hash[5] += f;
	ctx->hash[6] += g;
	ctx->hash[7] += h;
}

static void sha512_process_block128(sha512_ctx_t *ctx)
{
	unsigned t;
	uint64_t W[80];
	/* On i386, having assignments here (not later as sha256 does)
	 * produces 99 bytes smaller code with gcc 4.3.1
	 */
	uint64_t a = ctx->hash[0];
	uint64_t b = ctx->hash[1];
	uint64_t c = ctx->hash[2];
	uint64_t d = ctx->hash[3];
	uint64_t e = ctx->hash[4];
	uint64_t f = ctx->hash[5];
	uint64_t g = ctx->hash[6];
	uint64_t h = ctx->hash[7];
	const uint64_t *words = (uint64_t*) ctx->wbuffer;

	/* Operators defined in FIPS 180-2:4.1.2.  */
#define Ch(x, y, z) ((x & y) ^ (~x & z))
#define Maj(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define S0(x) (rotr64(x, 28) ^ rotr64(x, 34) ^ rotr64(x, 39))
#define S1(x) (rotr64(x, 14) ^ rotr64(x, 18) ^ rotr64(x, 41))
#define R0(x) (rotr64(x, 1) ^ rotr64(x, 8) ^ (x >> 7))
#define R1(x) (rotr64(x, 19) ^ rotr64(x, 61) ^ (x >> 6))

	/* Compute the message schedule according to FIPS 180-2:6.3.2 step 2.  */
	for (t = 0; t < 16; ++t)
		W[t] = SWAP_BE64(words[t]);
	for (/*t = 16*/; t < 80; ++t)
		W[t] = R1(W[t - 2]) + W[t - 7] + R0(W[t - 15]) + W[t - 16];

	/* The actual computation according to FIPS 180-2:6.3.2 step 3.  */
	for (t = 0; t < 80; ++t) {
		uint64_t T1 = h + S1(e) + Ch(e, f, g) + sha_K[t] + W[t];
		uint64_t T2 = S0(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;
	}
#undef Ch
#undef Maj
#undef S0
#undef S1
#undef R0
#undef R1
	/* Add the starting values of the context according to FIPS 180-2:6.3.2
	   step 4.  */
	ctx->hash[0] += a;
	ctx->hash[1] += b;
	ctx->hash[2] += c;
	ctx->hash[3] += d;
	ctx->hash[4] += e;
	ctx->hash[5] += f;
	ctx->hash[6] += g;
	ctx->hash[7] += h;
}


void sha1_begin(sha1_ctx_t *ctx)
{
	ctx->hash[0] = 0x67452301;
	ctx->hash[1] = 0xefcdab89;
	ctx->hash[2] = 0x98badcfe;
	ctx->hash[3] = 0x10325476;
	ctx->hash[4] = 0xc3d2e1f0;
	ctx->total64 = 0;
	ctx->process_block = sha1_process_block64;
}

static const uint32_t init256[] = {
	0,
	0,
	0x6a09e667,
	0xbb67ae85,
	0x3c6ef372,
	0xa54ff53a,
	0x510e527f,
	0x9b05688c,
	0x1f83d9ab,
	0x5be0cd19,
};
static const uint32_t init512_lo[] = {
	0,
	0,
	0xf3bcc908,
	0x84caa73b,
	0xfe94f82b,
	0x5f1d36f1,
	0xade682d1,
	0x2b3e6c1f,
	0xfb41bd6b,
	0x137e2179,
};

/* Initialize structure containing state of computation.
   (FIPS 180-2:5.3.2)  */
void sha256_begin(sha256_ctx_t *ctx)
{
	memcpy(&ctx->total64, init256, sizeof(init256));
	/*ctx->total64 = 0; - done by prepending two 32-bit zeros to init256 */
	ctx->process_block = sha256_process_block64;
}

/* Initialize structure containing state of computation.
   (FIPS 180-2:5.3.3)  */
void sha512_begin(sha512_ctx_t *ctx)
{
	int i;
	/* Two extra iterations zero out ctx->total64[2] */
	uint64_t *tp = ctx->total64;
	for (i = 0; i < 2+8; i++)
		tp[i] = ((uint64_t)(init256[i]) << 32) + init512_lo[i];
	/*ctx->total64[0] = ctx->total64[1] = 0; - already done */
}

void sha512_hash(sha512_ctx_t *ctx, const void *buffer, size_t len)
{
	unsigned bufpos = ctx->total64[0] & 127;
	unsigned remaining;

	/* First increment the byte count.  FIPS 180-2 specifies the possible
	   length of the file up to 2^128 _bits_.
	   We compute the number of _bytes_ and convert to bits later.  */
	ctx->total64[0] += len;
	if (ctx->total64[0] < len)
		ctx->total64[1]++;
#if 0
	remaining = 128 - bufpos;

	/* Hash whole blocks */
	while (len >= remaining) {
		memcpy(ctx->wbuffer + bufpos, buffer, remaining);
		buffer = (const char *)buffer + remaining;
		len -= remaining;
		remaining = 128;
		bufpos = 0;
		sha512_process_block128(ctx);
	}

	/* Save last, partial blosk */
	memcpy(ctx->wbuffer + bufpos, buffer, len);
#else
	while (1) {
		remaining = 128 - bufpos;
		if (remaining > len)
			remaining = len;
		/* Copy data into aligned buffer */
		memcpy(ctx->wbuffer + bufpos, buffer, remaining);
		len -= remaining;
		buffer = (const char *)buffer + remaining;
		bufpos += remaining;
		/* clever way to do "if (bufpos != 128) break; ... ; bufpos = 0;" */
		bufpos -= 128;
		if (bufpos != 0)
			break;
		/* Buffer is filled up, process it */
		sha512_process_block128(ctx);
		/*bufpos = 0; - already is */
	}
#endif
}

/* Used also for sha256 */
void sha1_end(sha1_ctx_t *ctx, void *resbuf)
{
	unsigned hash_size;

	/* SHA stores total in BE, need to swap on LE arches: */
	common64_end(ctx, /*swap_needed:*/ BB_LITTLE_ENDIAN);

	hash_size = (ctx->process_block == sha1_process_block64) ? 5 : 8;
	/* This way we do not impose alignment constraints on resbuf: */
	if (BB_LITTLE_ENDIAN) {
		unsigned i;
		for (i = 0; i < hash_size; ++i)
			ctx->hash[i] = SWAP_BE32(ctx->hash[i]);
	}
	memcpy(resbuf, ctx->hash, sizeof(ctx->hash[0]) * hash_size);
}

void sha512_end(sha512_ctx_t *ctx, void *resbuf)
{
	unsigned bufpos = ctx->total64[0] & 127;

	/* Pad the buffer to the next 128-byte boundary with 0x80,0,0,0... */
	ctx->wbuffer[bufpos++] = 0x80;

	while (1) {
		unsigned remaining = 128 - bufpos;
		memset(ctx->wbuffer + bufpos, 0, remaining);
		if (remaining >= 16) {
			/* Store the 128-bit counter of bits in the buffer in BE format */
			uint64_t t;
			uint64_t **p;

			t = ctx->total64[0] << 3;
			t = SWAP_BE64(t);
#if 1
			*p = (uint64_t *)&ctx->wbuffer[128 - 8];
			**p = t;
#else
			*(uint64_t *) (&ctx->wbuffer[128 - 8]) = t;
#endif
			t = (ctx->total64[1] << 3) | (ctx->total64[0] >> 61);
			t = SWAP_BE64(t);
#if 1
			*p = (uint64_t *)&ctx->wbuffer[128 - 16];
			**p = t;
#else
			*(uint64_t *) (&ctx->wbuffer[128 - 16]) = t;
#endif
		}
		sha512_process_block128(ctx);
		if (remaining >= 16)
			break;
		bufpos = 0;
	}

	if (BB_LITTLE_ENDIAN) {
		unsigned i;
		for (i = 0; i < ARRAY_SIZE(ctx->hash); ++i)
			ctx->hash[i] = SWAP_BE64(ctx->hash[i]);
	}
	memcpy(resbuf, ctx->hash, sizeof(ctx->hash));
}

int i64c(int i)
{
	i &= 0x3f;
	if (i == 0)
		return '.';
	if (i == 1)
		return '/';
	if (i < 12)
		return ('0' - 2 + i);
	if (i < 38)
		return ('A' - 12 + i);
	return ('a' - 38 + i);
}

char*
to64(char *s, unsigned v, int n)
{
	while (--n >= 0) {
			/* *s++ = ascii64[v & 0x3f]; */
			*s++ = i64c(v);
			v >>= 6;
		}
	return s;
}

char *
md5_crypt(char *result, const unsigned char *pw, const unsigned char *salt)
{
	char *p;
	unsigned char final[17]; /* final[16] exists only to aid in looping */
	int sl, pl, i, pw_len;
	md5_ctx_t ctx, ctx1;

	/* NB: in busybox, "$1$" in salt is always present */

	/* Refine the Salt first */

	/* Get the length of the salt including "$1$" */
	sl = 3;
	while (salt[sl] && salt[sl] != '$' && sl < (3 + 8))
		sl++;

	/* Hash. the password first, since that is what is most unknown */
	md5_begin(&ctx);
	pw_len = strlen((char*)pw);
	md5_hash(&ctx, pw, pw_len);

	/* Then the salt including "$1$" */
	md5_hash(&ctx, salt, sl);

	/* Copy salt to result; skip "$1$" */
	memcpy(result, salt, sl);
	result[sl] = '$';
	salt += 3;
	sl -= 3;

	/* Then just as many characters of the MD5(pw, salt, pw) */
	md5_begin(&ctx1);
	md5_hash(&ctx1, pw, pw_len);
	md5_hash(&ctx1, salt, sl);
	md5_hash(&ctx1, pw, pw_len);
	md5_end(&ctx1, final);
	for (pl = pw_len; pl > 0; pl -= 16)
		md5_hash(&ctx, final, pl > 16 ? 16 : pl);

	/* Then something really weird... */
	memset(final, 0, sizeof(final));
	for (i = pw_len; i; i >>= 1) {
			md5_hash(&ctx, ((i & 1) ? final : (const unsigned char *) pw), 1);
		}
	md5_end(&ctx, final);

	/* And now, just to make sure things don't run too fast.
	 *	 * On a 60 Mhz Pentium this takes 34 msec, so you would
	 *		 * need 30 seconds to build a 1000 entry dictionary...
	 *			 */
	for (i = 0; i < 1000; i++) {
			md5_begin(&ctx1);
			if (i & 1)
				md5_hash(&ctx1, pw, pw_len);
			else
				md5_hash(&ctx1, final, 16);
	
			if (i % 3)
				md5_hash(&ctx1, salt, sl);
	
			if (i % 7)
				md5_hash(&ctx1, pw, pw_len);
	
			if (i & 1)
				md5_hash(&ctx1, final, 16);
			else
				md5_hash(&ctx1, pw, pw_len);
			md5_end(&ctx1, final);
		}

	p = result + sl + 4; /* 12 bytes max (sl is up to 8 bytes) */

	/* Add 5*4+2 = 22 bytes of hash, + NUL byte. */
	final[16] = final[5];
	for (i = 0; i < 5; i++) {
			unsigned l = (final[i] << 16) | (final[i+6] << 8) | final[i+12];
			p = to64(p, l, 4);
		}
	p = to64(p, final[11], 2);
	*p = '\0';

	/* Don't leave anything around in vm they could use. */
	memset(final, 0, sizeof(final));

	return result;
}
