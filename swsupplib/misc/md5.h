#ifndef MD5_H_sw1999
#define MD5_H_sw1999

#include "config.h"

#if (SIZEOF_UNSIGNED_LONG == 4)
typedef unsigned long uint32;
#elif (SIZEOF_UNSIGNED_INT == 4)
typedef unsigned int uint32;
#else
typedef unsigned noway-BUSTED_HERE uint32  Stop . Stop . Stop.
#endif

struct MD5Context {
	uint32 buf[4];
	uint32 bits[2];
	unsigned char in[64];
	int doByteReverse;
};

void MD5Init(struct MD5Context *context, int brokenEndian);
void MD5Update(struct MD5Context *context, unsigned char const *buf,
	       unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Transform(uint32 buf[4], uint32 const in[16]);

/* These assume a little endian machine and return incorrect results! 
   They are here for compatibility with old (broken) versions of RPM */
int mdfileBroken(char *fn, unsigned char *digest);
int mdbinfileBroken(char *fn, unsigned char *bindigest);

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;

#endif /* !MD5_H */
