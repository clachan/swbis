/* sha.h - Declaration of functions and datatypes for SHA1 sum computing
   library functions.

   Copyright (C) 1999, Scott G. Miller
*/

#ifndef _SHA_H
# define _SHA_H 1

# include <stdio.h>

/* 
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Start of Code that obviates need to include "new_md5_for_sha.h" 
 * The new_md5_for_sha.h file was the #include "md5.h" file in sha_orig.c
 */

#ifdef _LIBC
# include <sys/types.h>
typedef u_int32_t md5_uint32;
#else
# if defined __STDC__ && __STDC__
#  define UINT_MAX_32_BITS 4294967295U
# else
#  define UINT_MAX_32_BITS 0xFFFFFFFF
# endif
/* If UINT_MAX isn't defined, assume it's a 32-bit type.
   This should be valid for all systems GNU cares about because
   that doesn't include 16-bit systems, and only modern systems
   (that certainly have <limits.h>) have 64+-bit integral types.  */

# ifndef UINT_MAX
#  define UINT_MAX UINT_MAX_32_BITS
# endif

# if UINT_MAX == UINT_MAX_32_BITS
   typedef unsigned int md5_uint32;
# else
#  if USHRT_MAX == UINT_MAX_32_BITS
    typedef unsigned short md5_uint32;
#  else
#   if ULONG_MAX == UINT_MAX_32_BITS
     typedef unsigned long md5_uint32;
#   else
     /* The following line is intended to evoke an error.
        Using #error is not portable enough.  */
     "Cannot determine unsigned 32-bit data type."
#   endif
#  endif
# endif
#endif

/* The following is from gnupg-1.0.2's cipher/bithelp.h.  */
/* Rotate a 32 bit integer by n bytes */
#undef __un_i386__me__
#if defined __un_i386__me__ && __GNUC__ && defined __i386__
     	: 
	: Stop
	: phooey.. this is slower than the portable version.
	: Stop
	:
static md5_uint32
rol(md5_uint32 x, int n)
{
  __asm__("roll %%cl,%0"
          :"=r" (x)
          :"0" (x),"c" (n));
  return x;
}
#else
# define rol(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )
#endif

/* 
 * End of Code that obviates need to include "new_md5_for_sha.h" 
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

/* Structure to save state of computation between the single steps.  */
struct sha_ctx
{
  md5_uint32 A;
  md5_uint32 B;
  md5_uint32 C;
  md5_uint32 D;
  md5_uint32 E;

  md5_uint32 total[2];
  md5_uint32 buflen;
  char buffer[128];
};


/* Starting with the result of former calls of this function (or the
   initialization function update the context for the next LEN bytes
   starting at BUFFER.
   It is necessary that LEN is a multiple of 64!!! */
extern void sha_process_block __P ((const void *buffer, size_t len,
                            struct sha_ctx *ctx));

/* Starting with the result of former calls of this function (or the
   initialization function update the context for the next LEN bytes
   starting at BUFFER.
   It is NOT required that LEN is a multiple of 64.  */
extern void sha_process_bytes __P((const void *buffer, size_t len,
				    struct sha_ctx *ctx));

/* Initialize structure containing state of computation. */
extern void sha_init_ctx __P ((struct sha_ctx *ctx));

/* Process the remaining bytes in the buffer and put result from CTX
   in first 16 bytes following RESBUF.  The result is always in little
   endian byte order, so that a byte-wise output yields to the wanted
   ASCII representation of the message digest.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 32 bits value.  */
extern void *sha_finish_ctx __P ((struct sha_ctx *ctx, void *resbuf));


/* Put result from CTX in first 16 bytes following RESBUF.  The result is
   always in little endian byte order, so that a byte-wise output yields
   to the wanted ASCII representation of the message digest.

   IMPORTANT: On some systems it is required that RESBUF is correctly
   aligned for a 32 bits value.  */
extern void *sha_read_ctx __P ((const struct sha_ctx *ctx, void *resbuf));


/* Compute MD5 message digest for bytes read from STREAM.  The
   resulting message digest number will be written into the 16 bytes
   beginning at RESBLOCK.  */
/* extern int sha_stream __P ((FILE *stream, void *resblock)); */
int sha_stream (int ifd, void *resblock);
int sha_block(void * thisisa, void *resblock, unsigned char * iblock, int icount);

/* Compute MD5 message digest for LEN bytes beginning at BUFFER.  The
   result is always in little endian byte order, so that a byte-wise
   output yields to the wanted ASCII representation of the message
   digest.  */
/* extern void *sha_buffer __P ((const char *buffer, size_t len, void *resblock)); */

#endif
