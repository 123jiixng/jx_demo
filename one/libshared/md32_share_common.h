#ifndef	_MD32_COMMON_H_
#define _MD32_COMMON_H_

#if !defined(DATA_ORDER_IS_BIG_ENDIAN) && !defined(DATA_ORDER_IS_LITTLE_ENDIAN)
#error "DATA_ORDER must be defined!"
#endif

#ifndef HASH_CBLOCK
#error "HASH_CBLOCK must be defined!"
#endif
#ifndef HASH_LONG
#error "HASH_LONG must be defined!"
#endif
#ifndef HASH_CTX
#error "HASH_CTX must be defined!"
#endif

#ifndef HASH_UPDATE
#error "HASH_UPDATE must be defined!"
#endif
#ifndef HASH_TRANSFORM
#error "HASH_TRANSFORM must be defined!"
#endif
#ifndef HASH_FINAL
#error "HASH_FINAL must be defined!"
#endif

#ifndef HASH_BLOCK_HOST_ORDER
#error "HASH_BLOCK_HOST_ORDER must be defined!"
#endif

#if 0
/*
 * Moved below as it's required only if HASH_BLOCK_DATA_ORDER_ALIGNED
 * isn't defined.
 */
#ifndef HASH_BLOCK_DATA_ORDER
#error "HASH_BLOCK_DATA_ORDER must be defined!"
#endif
#endif

#ifndef HASH_LBLOCK
#define HASH_LBLOCK	(HASH_CBLOCK/4)
#endif

#ifndef HASH_LONG_LOG2
#define HASH_LONG_LOG2	2
#endif

/*
 * Engage compiler specific rotate intrinsic function if available.
 */
#undef ROTATE
#ifndef PEDANTIC
# if defined(_MSC_VER)
#  define ROTATE(a,n)	_lrotl(a,n)
# elif defined(__MWERKS__)
#  if defined(__POWERPC__)
#   define ROTATE(a,n)	__rlwinm(a,n,0,31)
#  elif defined(__MC68K__)
    /* Motorola specific tweak. <appro@fy.chalmers.se> */
#   define ROTATE(a,n)	( n<24 ? __rol(a,n) : __ror(a,32-n) )
#  else
#   define ROTATE(a,n)	__rol(a,n)
#  endif
# elif defined(__GNUC__) && __GNUC__>=2 && !defined(NO_ASM) && !defined(NO_INLINE_ASM)
  /*
   * Some GNU C inline assembler templates. Note that these are
   * rotates by *constant* number of bits! But that's exactly
   * what we need here...
   *
   * 					<appro@fy.chalmers.se>
   */
#  if defined(__i386)
#   define ROTATE(a,n)	({ register unsigned int ret;	\
				asm (			\
				"roll %1,%0"		\
				: "=r"(ret)		\
				: "I"(n), "0"(a)	\
				: "cc");		\
			   ret;				\
			})
#  elif defined(__powerpc) || defined(__ppc)
#   define ROTATE(a,n)	({ register unsigned int ret;	\
				asm (			\
				"rlwinm %0,%1,%2,0,31"	\
				: "=r"(ret)		\
				: "r"(a), "I"(n));	\
			   ret;				\
			})
#  endif
# endif

/*
 * Engage compiler specific "fetch in reverse byte order"
 * intrinsic function if available.
 */
# if defined(__GNUC__) && __GNUC__>=2 && !defined(NO_ASM) && !defined(NO_INLINE_ASM)
  /* some GNU C inline assembler templates by <appro@fy.chalmers.se> */
#  if defined(__i386) && !defined(I386_ONLY)
#   define BE_FETCH32(a)	({ register unsigned int l=(a);\
				asm (			\
				"bswapl %0"		\
				: "=r"(l) : "0"(l));	\
			  l;				\
			})
#  elif defined(__powerpc)
#   define LE_FETCH32(a)	({ register unsigned int l;	\
				asm (			\
				"lwbrx %0,0,%1"		\
				: "=r"(l)		\
				: "r"(a));		\
			   l;				\
			})

#  elif defined(__sparc) && defined(ULTRASPARC)
#  define LE_FETCH32(a)	({ register unsigned int l;		\
				asm (				\
				"lda [%1]#ASI_PRIMARY_LITTLE,%0"\
				: "=r"(l)			\
				: "r"(a));			\
			   l;					\
			})
#  endif
# endif
#endif /* PEDANTIC */

#if HASH_LONG_LOG2==2	/* Engage only if sizeof(HASH_LONG)== 4 */
/* A nice byte order reversal from Wei Dai <weidai@eskimo.com> */
#ifdef ROTATE
/* 5 instructions with rotate instruction, else 9 */
#define REVERSE_FETCH32(a,l)	(					\
		l=*(const HASH_LONG *)(a),				\
		((ROTATE(l,8)&0x00FF00FF)|(ROTATE((l&0x00FF00FF),24)))	\
				)
#else
/* 6 instructions with rotate instruction, else 8 */
#define REVERSE_FETCH32(a,l)	(				\
		l=*(const HASH_LONG *)(a),			\
		l=(((l>>8)&0x00FF00FF)|((l&0x00FF00FF)<<8)),	\
		ROTATE(l,16)					\
				)
/*
 * Originally the middle line started with l=(((l&0xFF00FF00)>>8)|...
 * It's rewritten as above for two reasons:
 *	- RISCs aren't good at long constants and have to explicitely
 *	  compose 'em with several (well, usually 2) instructions in a
 *	  register before performing the actual operation and (as you
 *	  already realized:-) having same constant should inspire the
 *	  compiler to permanently allocate the only register for it;
 *	- most modern CPUs have two ALUs, but usually only one has
 *	  circuitry for shifts:-( this minor tweak inspires compiler
 *	  to schedule shift instructions in a better way...
 *
 *				<appro@fy.chalmers.se>
 */
#endif
#endif

#ifndef ROTATE
#define ROTATE(a,n)     (((a)<<(n))|(((a)&0xffffffff)>>(32-(n))))
#endif

/*
 * Make some obvious choices. E.g., HASH_BLOCK_DATA_ORDER_ALIGNED
 * and HASH_BLOCK_HOST_ORDER ought to be the same if input data
 * and host are of the same "endianess". It's possible to mask
 * this with blank #define HASH_BLOCK_DATA_ORDER though...
 *
 *				<appro@fy.chalmers.se>
 */
#if defined(B_ENDIAN)
#  if defined(DATA_ORDER_IS_BIG_ENDIAN)
#    if !defined(HASH_BLOCK_DATA_ORDER_ALIGNED) && HASH_LONG_LOG2==2
#      define HASH_BLOCK_DATA_ORDER_ALIGNED	HASH_BLOCK_HOST_ORDER
#    endif
#  elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)
#    ifndef HOST_FETCH32
#      ifdef LE_FETCH32
#        define HOST_FETCH32(p,l)	LE_FETCH32(p)
#      elif defined(REVERSE_FETCH32)
#        define HOST_FETCH32(p,l)	REVERSE_FETCH32(p,l)
#      endif
#    endif
#  endif
#elif defined(L_ENDIAN)
#  if defined(DATA_ORDER_IS_LITTLE_ENDIAN)
#    if !defined(HASH_BLOCK_DATA_ORDER_ALIGNED) && HASH_LONG_LOG2==2
#      define HASH_BLOCK_DATA_ORDER_ALIGNED	HASH_BLOCK_HOST_ORDER
#    endif
#  elif defined(DATA_ORDER_IS_BIG_ENDIAN)
#    ifndef HOST_FETCH32
#      ifdef BE_FETCH32
#        define HOST_FETCH32(p,l)	BE_FETCH32(p)
#      elif defined(REVERSE_FETCH32)
#        define HOST_FETCH32(p,l)	REVERSE_FETCH32(p,l)
#      endif
#    endif
#  endif
#endif

#if !defined(HASH_BLOCK_DATA_ORDER_ALIGNED)
#ifndef HASH_BLOCK_DATA_ORDER
#error "HASH_BLOCK_DATA_ORDER must be defined!"
#endif
#endif

#if defined(DATA_ORDER_IS_BIG_ENDIAN)

#define HOST_c2l(c,l)	(l =(((unsigned long)(*((c)++)))<<24),		\
			 l|=(((unsigned long)(*((c)++)))<<16),		\
			 l|=(((unsigned long)(*((c)++)))<< 8),		\
			 l|=(((unsigned long)(*((c)++)))    ),		\
			 l)
#define HOST_p_c2l(c,l,n)	{					\
			switch (n) {					\
			case 0: l =((unsigned long)(*((c)++)))<<24;	\
			case 1: l|=((unsigned long)(*((c)++)))<<16;	\
			case 2: l|=((unsigned long)(*((c)++)))<< 8;	\
			case 3: l|=((unsigned long)(*((c)++)));		\
				} }
#define HOST_p_c2l_p(c,l,sc,len) {					\
			switch (sc) {					\
			case 0: l =((unsigned long)(*((c)++)))<<24;	\
				if (--len == 0) break;			\
			case 1: l|=((unsigned long)(*((c)++)))<<16;	\
				if (--len == 0) break;			\
			case 2: l|=((unsigned long)(*((c)++)))<< 8;	\
				} }
/* NOTE the pointer is not incremented at the end of this */
#define HOST_c2l_p(c,l,n)	{					\
			l=0; (c)+=n;					\
			switch (n) {					\
			case 3: l =((unsigned long)(*(--(c))))<< 8;	\
			case 2: l|=((unsigned long)(*(--(c))))<<16;	\
			case 1: l|=((unsigned long)(*(--(c))))<<24;	\
				} }
#define HOST_l2c(l,c)	(*((c)++)=(unsigned char)(((l)>>24)&0xff),	\
			 *((c)++)=(unsigned char)(((l)>>16)&0xff),	\
			 *((c)++)=(unsigned char)(((l)>> 8)&0xff),	\
			 *((c)++)=(unsigned char)(((l)    )&0xff),	\
			 l)

#elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)

#define HOST_c2l(c,l)	(l =(((unsigned long)(*((c)++)))    ),		\
			 l|=(((unsigned long)(*((c)++)))<< 8),		\
			 l|=(((unsigned long)(*((c)++)))<<16),		\
			 l|=(((unsigned long)(*((c)++)))<<24),		\
			 l)
#define HOST_p_c2l(c,l,n)	{					\
			switch (n) {					\
			case 0: l =((unsigned long)(*((c)++)));		\
			case 1: l|=((unsigned long)(*((c)++)))<< 8;	\
			case 2: l|=((unsigned long)(*((c)++)))<<16;	\
			case 3: l|=((unsigned long)(*((c)++)))<<24;	\
				} }
#define HOST_p_c2l_p(c,l,sc,len) {					\
			switch (sc) {					\
			case 0: l =((unsigned long)(*((c)++)));		\
				if (--len == 0) break;			\
			case 1: l|=((unsigned long)(*((c)++)))<< 8;	\
				if (--len == 0) break;			\
			case 2: l|=((unsigned long)(*((c)++)))<<16;	\
				} }
/* NOTE the pointer is not incremented at the end of this */
#define HOST_c2l_p(c,l,n)	{					\
			l=0; (c)+=n;					\
			switch (n) {					\
			case 3: l =((unsigned long)(*(--(c))))<<16;	\
			case 2: l|=((unsigned long)(*(--(c))))<< 8;	\
			case 1: l|=((unsigned long)(*(--(c))));		\
				} }
#define HOST_l2c(l,c)	(*((c)++)=(unsigned char)(((l)    )&0xff),	\
			 *((c)++)=(unsigned char)(((l)>> 8)&0xff),	\
			 *((c)++)=(unsigned char)(((l)>>16)&0xff),	\
			 *((c)++)=(unsigned char)(((l)>>24)&0xff),	\
			 l)

#endif

/*
 * Time for some action:-)
 */

void HASH_UPDATE (HASH_CTX *c, const void *data_, unsigned long len)
	{
	const unsigned char *data=data_;
	register HASH_LONG * p;
	register unsigned long l;
	int sw,sc,ew,ec;

	if (len==0) return;

	l=(c->Nl+(len<<3))&0xffffffffL;
	/* 95-05-24 eay Fixed a bug with the overflow handling, thanks to
	 * Wei Dai <weidai@eskimo.com> for pointing it out. */
	if (l < c->Nl) /* overflow */
		c->Nh++;
	c->Nh+=(len>>29);
	c->Nl=l;

	if (c->num != 0)
		{
		p=c->data;
		sw=c->num>>2;
		sc=c->num&0x03;

		if ((c->num+len) >= HASH_CBLOCK)
			{
			l=p[sw]; HOST_p_c2l(data,l,sc); p[sw++]=l;
			for (; sw<HASH_LBLOCK; sw++)
				{
				HOST_c2l(data,l); p[sw]=l;
				}
			HASH_BLOCK_HOST_ORDER (c,p,1);
			len-=(HASH_CBLOCK-c->num);
			c->num=0;
			/* drop through and do the rest */
			}
		else
			{
			c->num+=len;
			if ((sc+len) < 4) /* ugly, add char's to a word */
				{
				l=p[sw]; HOST_p_c2l_p(data,l,sc,len); p[sw]=l;
				}
			else
				{
				ew=(c->num>>2);
				ec=(c->num&0x03);
				l=p[sw]; HOST_p_c2l(data,l,sc); p[sw++]=l;
				for (; sw < ew; sw++)
					{
					HOST_c2l(data,l); p[sw]=l;
					}
				if (ec)
					{
					HOST_c2l_p(data,l,ec); p[sw]=l;
					}
				}
			return;
			}
		}

	sw=len/HASH_CBLOCK;
	if (sw > 0)
		{
#if defined(HASH_BLOCK_DATA_ORDER_ALIGNED)
		/*
		 * Note that HASH_BLOCK_DATA_ORDER_ALIGNED gets defined
		 * only if sizeof(HASH_LONG)==4.
		 */
		if ((((unsigned long)data)%4) == 0)
			{
			/* data is properly aligned so that we can cast it: */
			HASH_BLOCK_DATA_ORDER_ALIGNED (c,(HASH_LONG *)data,sw);
			sw*=HASH_CBLOCK;
			data+=sw;
			len-=sw;
			}
		else
#if !defined(HASH_BLOCK_DATA_ORDER)
			while (sw--)
				{
				memcpy (p=c->data,data,HASH_CBLOCK);
				HASH_BLOCK_DATA_ORDER_ALIGNED(c,p,1);
				data+=HASH_CBLOCK;
				len-=HASH_CBLOCK;
				}
#endif
#endif
#if defined(HASH_BLOCK_DATA_ORDER)
			{
			HASH_BLOCK_DATA_ORDER(c,data,sw);
			sw*=HASH_CBLOCK;
			data+=sw;
			len-=sw;
			}
#endif
		}

	if (len!=0)
		{
		p = c->data;
		c->num = len;
		ew=len>>2;	/* words to copy */
		ec=len&0x03;
		for (; ew; ew--,p++)
			{
			HOST_c2l(data,l); *p=l;
			}
		HOST_c2l_p(data,l,ec);
		*p=l;
		}
	}


void HASH_TRANSFORM (HASH_CTX *c, const unsigned char *data)
	{
#if defined(HASH_BLOCK_DATA_ORDER_ALIGNED)
	if ((((unsigned long)data)%4) == 0)
		/* data is properly aligned so that we can cast it: */
		HASH_BLOCK_DATA_ORDER_ALIGNED (c,(HASH_LONG *)data,1);
	else
#if !defined(HASH_BLOCK_DATA_ORDER)
		{
		memcpy (c->data,data,HASH_CBLOCK);
		HASH_BLOCK_DATA_ORDER_ALIGNED (c,c->data,1);
		}
#endif
#endif
#if defined(HASH_BLOCK_DATA_ORDER)
	HASH_BLOCK_DATA_ORDER (c,data,1);
#endif
	}


void HASH_FINAL (unsigned char *md, HASH_CTX *c)
	{
	register HASH_LONG *p;
	register unsigned long l;
	register int i,j;
	static const unsigned char end[4]={0x80,0x00,0x00,0x00};
	const unsigned char *cp=end;

	/* c->num should definitly have room for at least one more byte. */
	p=c->data;
	i=c->num>>2;
	j=c->num&0x03;

#if 0
	/* purify often complains about the following line as an
	 * Uninitialized Memory Read.  While this can be true, the
	 * following p_c2l macro will reset l when that case is true.
	 * This is because j&0x03 contains the number of 'valid' bytes
	 * already in p[i].  If and only if j&0x03 == 0, the UMR will
	 * occur but this is also the only time p_c2l will do
	 * l= *(cp++) instead of l|= *(cp++)
	 * Many thanks to Alex Tang <altitude@cic.net> for pickup this
	 * 'potential bug' */
#ifdef PURIFY
	if (j==0) p[i]=0; /* Yeah, but that's not the way to fix it:-) */
#endif
	l=p[i];
#else
	l = (j==0) ? 0 : p[i];
#endif
	HOST_p_c2l(cp,l,j); p[i++]=l; /* i is the next 'undefined word' */

	if (i>(HASH_LBLOCK-2)) /* save room for Nl and Nh */
		{
		if (i<HASH_LBLOCK) p[i]=0;
		HASH_BLOCK_HOST_ORDER (c,p,1);
		i=0;
		}
	for (; i<(HASH_LBLOCK-2); i++)
		p[i]=0;

#if   defined(DATA_ORDER_IS_BIG_ENDIAN)
	p[HASH_LBLOCK-2]=c->Nh;
	p[HASH_LBLOCK-1]=c->Nl;
#elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)
	p[HASH_LBLOCK-2]=c->Nl;
	p[HASH_LBLOCK-1]=c->Nh;
#endif
	HASH_BLOCK_HOST_ORDER (c,p,1);

#ifndef HASH_MAKE_STRING
#error "HASH_MAKE_STRING must be defined!"
#else
	HASH_MAKE_STRING(c,md);
#endif

	c->num=0;
	/* clear stuff, HASH_BLOCK may be leaving some stuff on the stack
	 * but I'm not worried :-)
	memset((void *)c,0,sizeof(HASH_CTX));
	 */
	}
#endif
