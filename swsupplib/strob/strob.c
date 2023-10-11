/* strob.c : unlimited length Null terminated string object
 */

/* 
 Copyright (C) 1995,1996,1997,1998,2000,2001,2005,2014  James H. Lowe, Jr.
 All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "swuser_config.h"
#include "strob.h"

#define o__inline__

#define S_MEM_CACHE 1   /* define to use MEM_CACHE */
			/* MEM_CACHE defers free() of strob objects until the
			   program exits, it maintains a list of active and unactive
			   objects.  It appears to result in *no* performance gains */
#undef S_MEM_CACHE     /* default */

static
int 
sb_close(STROB * strb)
{
	swbis_free(strb->str_);
	swbis_free(strb);
	return 0;
}

static
STROB *
sb_open(size_t initial_size)
{
	STROB *strb;

	strb = (STROB *) malloc(sizeof(STROB));
	if (strb == (STROB *)(NULL)) {
		fprintf(stderr, "strob_open: out of memory.\n");
		exit(22);
		return (STROB *)(NULL);
	}

	if (initial_size <= 0)
		initial_size = STROB_INITLENGTH;

	strb->extra_ = STROB_LENGTHINCR;
	strb->length_ = 0;
	strb->str_ = (unsigned char*)NULL;
	strb->in_use_ = 1;
	strb->fill_charM = '\0';
	strob_reopen(strb, initial_size + 1);
	return strb;
}

#ifdef S_MEM_CACHE
#include "cplob.h"

static CPLOB * blM;
static int last_indexM;

static
void
bl_create(void)
{
	blM = cplob_open(100);
	cplob_add_nta(blM, (char*)(NULL));
}

static
void
bl_add(STROB * s)
{
	if (blM == NULL) bl_create();
	cplob_add_nta(blM, (char*)s);
}

static
STROB *
bl_find_at(int last)
{
	STROB * s;
	int i;

	if (blM == NULL)
		bl_create();
	i = last;
	while((s=(STROB*)cplob_val(blM, i)) != NULL)
	{
		if (s->in_use_ == 0) {
			last_indexM = i;
			/* fprintf(stderr, "reusing strob at %d\n", i); */
			break;
		}
		i++;
	}
	return s;
}

static
STROB *
bl_find(void)
{
	STROB * s;
	s = bl_find_at(last_indexM);
	if (s == NULL)
		s = bl_find_at(0);
	return s;
}

static
STROB *
bl_open(size_t initial_size)
{
	STROB * s;

	s = bl_find();
	if (s == NULL) {
		s = sb_open(initial_size);
		if (s) bl_add(s);
	} else {
		s->in_use_ = 1;
		strob_reopen(s, initial_size + 1);
	}
	return s;
}

static
int
bl_close(STROB * s)
{
	s->in_use_ = 0;
	memset(s->str_, (int)'\0', s->reserve_);
	return 0;
}

#endif

/* =============================================================== */

static
STROB *
strob_reopen_if_fill_with(STROB * strb, size_t reqd_length, int fill_char)
{
	if ((int)reqd_length > strb->reserve_){
		return strob_reopen_fill_with(strb, reqd_length + strb->extra_, fill_char);
	}
	return strb;
}

static
STROB *
strob_reopen_if(STROB * strb, size_t reqd_length)
{
	if ((int)reqd_length > strb->reserve_){
		return strob_reopen(strb, reqd_length + strb->extra_);
	}
	return strb;
}

static
void *
strob_set_up(STROB *strb, int n)
{
	if (!strob_reopen_if(strb, n+1))
		return (char *)(NULL);

	if (n > strb->length_) {
		strb->length_ = n;
		strb->str_[n]='\0';
	}
	return strb;
}

int 
strob_close(STROB * strb)
{
#ifdef S_MEM_CACHE
	return bl_close(strb);
#else
	return sb_close(strb);
#endif
}


STROB *
strob_open(size_t initial_size)
{
#ifdef S_MEM_CACHE
	return bl_open(initial_size);
#else
	return sb_open(initial_size);
#endif
}

char *
strob_release(STROB * strb)
{
	char *x = (char*)(strb->str_);
#ifdef S_MEM_CACHE
	strb->extra_ = STROB_LENGTHINCR;
	strb->length_ = 0;
	strb->str_ = (unsigned char*)NULL;
	strb->in_use_ = 1;
	strob_reopen(strb, STROB_INITLENGTH + 1);
	strob_close(strb);
#else
	swbis_free(strb);
#endif
	return x;
}

o__inline__
void
strob_set_reserve(STROB * strb, int res)
{
	strb->extra_ = res;
}

STROB *
strob_reopen_fill_with(STROB * strb, size_t new_length, int fill_char)
{
	unsigned char *tmpstr;

	if (new_length <= 1) new_length = 2;
	if (strb->str_ == (unsigned char*)(NULL))
			strb->str_= (unsigned char *)malloc(2);
		
	if (strb->str_ == (unsigned char*)(NULL)) {
		fprintf(stderr, "strob_reopen(loc=1): out of memory.\n");
		exit(22);
	}

	tmpstr = (unsigned char *)SWBIS_REALLOC(strb->str_,
					(size_t)(new_length), strb->reserve_);
	if (!tmpstr) {
		fprintf(stderr, "strob_reopen(loc=2): out of memory.\n");
		exit(22);
		return(STROB *)(NULL);
	}
	strb->str_ = tmpstr;
	strb->reserve_ = new_length;
	if (strb->reserve_ > strb->length_) {
		memset(strb->str_ + strb->length_,
				(int)fill_char, strb->reserve_ - strb->length_);
	}
	strb->str_[new_length - 1]='\0';
	return strb;
}

STROB *
strob_reopen(STROB * strb, size_t new_length)
{
	return strob_reopen_fill_with(strb, new_length, (int)'\0');
}

o__inline__
char *
strob_get_str(STROB * strb)
{
	return (char*)(strb->str_);
}

o__inline__
int
strob_get_reserve(STROB * strb)
{
	return strb->reserve_;
}

o__inline__
int
strob_get_length(STROB * strb)
{
	return strb->length_;
}

o__inline__
STROB *
strob_trunc(STROB * strb)
{
	return strob_reopen(strb, STROB_INITLENGTH + 1);
}

void
strob_set_length(STROB * strb, int len)
{
	strob_set_memlength(strb, len + 1);
	strb->str_[len]='\0';
}

char *
strob_strcpy_at_offset(STROB * strb, int offset, char *str)
{
	STROB *strb_ret;
	strb_ret = strob_reopen_if(strb, strlen(str) + offset + 1);
	if (!strb_ret)
		return (char *)(NULL);

	strb->length_ = offset + strlen(str);	
	memmove(strb->str_ + offset, str, strlen(str) + 1);
	return (char*)(strb->str_ + offset);
}

void
strob_chr_index(STROB * strb, int index, int ch)
{
	strob_reopen_if_fill_with(strb, index + 2, strb->fill_charM);

	if (index >= strb->length_) {
		memset(strb->str_ + strb->length_, strb->fill_charM, index -  strb->length_ + 1);
		*(strb->str_ + index + 1) = '\0';
	}

	if (index > strb->length_-1) {
		strb->length_=index+1;
		*(strb->str_ + strb->length_) = '\0';
	}
	*(strb->str_ + index) = (unsigned char) (ch);
}

int
strob_get_char(STROB * strb, int index)
{
	if (index < 0) return -1;
	if (index >= strb->length_) return -1;
	return (int) strb->str_[index];
}

char *
strob_strcat_at_offset(STROB * strb, int offset, char *str)
{
	STROB *strb_ret;
	strb_ret = strob_reopen_if(strb, strlen(str) + offset + 1 + strlen((char*)(strb->str_ + offset)));
	if (!strb_ret)
		return (char *)(NULL);

	strb->length_ = offset + strlen(str);	
	memmove(strb->str_ + offset + strlen((char*)(strb->str_ + offset)), str, strlen(str) + 1);
	return (char*)(strb->str_ + offset);
}

/* -------------------------------------------------------------*/

o__inline__
STROB *
strob_cpy(STROB * s, STROB * ct)
{
	strob_strcpy_at_offset(s, 0, (char*)(ct->str_));
	return s;
}

STROB *
strob_cat(STROB * s, STROB * ct)
{
	strob_catstr(s, (char*)(ct->str_));
	return s;
}

o__inline__
int
strob_cmp(STROB * cs, STROB * ct)
{
	return strob_strcmp(cs, (char*)(ct->str_));
}

/* --- NULL Terminated String Interface ------------------------*/

char *
strob_chomp(STROB * strb)
{
	char *p;
	char *s = strob_str(strb);
	if ((p = strchr(s, '\n'))) *p = '\0';
	if ((p = strchr(s, '\r'))) *p = '\0';
	return s;
}

char *
strob_strncat(STROB * strb, char *str, size_t len)
{
	char * cret;
	STROB *strb_ret;
	int ilen;

	ilen = (int)len;
	strb_ret = strob_reopen_if(strb, strlen((char*)(strb->str_)) + ilen + 1);
	if (!strb_ret) {
		return (char *)(NULL);
	}
	cret = strncat((char*)(strb->str_), str, ilen);
	strb->length_ = strlen((char*)(strb->str_));
	(strb->str_)[strb->length_] = '\0';
	return cret;
}


char *
strob_strcat(STROB * strb, char *str)
{
	STROB *strb_ret;
	strb_ret = strob_reopen_if(strb,
			strlen((char*)(strb->str_)) + strlen(str) + 1);
	if (!strb_ret)
		return (char *)(NULL);
	strb->length_=strlen((char*)(strb->str_)) + strlen(str);
	return strcat((char*)(strb->str_), str);
}

o__inline__
char * 
strob_strcpy (STROB * strb, char * str) {
	return strob_strcpy_at_offset(strb, 0, str);
}

char *
strob_charcat(STROB * strb, int ch)
{
	char c[2];
	c[0] = (char)ch;
	c[1] = '\0';
	if (ch) {
		return strob_strcat(strb, c);
	} else {
		char * s;
		strob_strcat(strb, "X");
		s = strob_str(strb);
		s[strlen(s) - 1] = '\0';
	}
	return (char*)NULL;	
}

char *
strob_strncpy(STROB * strb, char *str, size_t n)
{
	char * s;
	strob_reopen_if(strb, n + 1);
	s = strncpy((char*)(strb->str_), str, n);
	strb->str_[n] = '\0';
	strb->length_=strlen((char*)(strb->str_));
	return s;
}

o__inline__
int
strob_strcmp(STROB * strb, char *str)
{
	return strcmp((char*)(strb->str_), str);
}

o__inline__
size_t
strob_strlen(STROB * strb)
{
	return strlen((char*)(strb->str_));
}

char *
strob_strchar(STROB * strb, int index)
{
	strob_reopen_if(strb, index + 1);
	return (char*)(strb->str_ + index);
}

o__inline__
char *
strob_strrchr(STROB * strb, int c)
{
	return strrchr((char*)(strb->str_), c);
}

o__inline__
char *
strob_strstr(STROB * strb, char *str)
{
	return strstr((char*)(strb->str_), str);
}


o__inline__
void
strob_chr(STROB * strb, int ch)
{
	strob_chr_index(strb, strlen((char*)(strb->str_)), ch);
}

char *
strob_strtok(STROB * buf, char *s, const char * delim)
{
	char * retval;	
	char * start;	
	char * p;	
	char * m;	
	char * end;	
	if (s) {
		if (s != (char*)(buf->str_))
			strob_strcpy(buf, s);
		buf->tok_ = strob_str(buf);
	}
	start = buf->tok_;
	if (!strlen(start))
		return NULL;	

	p = NULL;
	do {
		if (p == start) start++;
		p = strpbrk(start, delim);
	} while (p && p == start);
	
	if (p) {
		*p = '\0';
		end = p + 1 + strlen(p+1);
		if (strlen(start))
			retval = start;
		else
			retval = NULL;
	} else {
		p = start + strlen(start);	
		end = p;
		retval = start;
	}
	buf->tok_ = end;	
	if (p < end) {
		m = p;	
		m++;
		while(*m && strchr(delim, (int)*m)) {
			m++;
		}
		buf->tok_ = m;	
	}
	if (!strlen(retval)) retval = NULL;
	return retval;
}

char *
strob_strstrtok(STROB * buf, char *s, const char * delim)
{
	char * retval;	
	char * start;	
	char * p;	
	char * m;	
	char * end;	
	int dlen = strlen(delim);

	if (s) {
		strob_strcpy(buf, s);
		buf->tok_ = strob_str(buf);
	}
	start = buf->tok_;
	if (!strlen(start))
		return NULL;	

	p = NULL;
	do {
		if (p == start) start+=dlen;
		p = strstr(start, delim);
	} while (p && p == start);
	
	if (p) {
		*p = '\0';
		end = p + dlen + strlen(p+dlen);
		if (strlen(start))
			retval = start;
		else
			retval = NULL;
	} else {
		p = start + strlen(start);	
		end = p;
		retval = start;
	}
	buf->tok_ = end;	
	if (p < end) {
		m = p;	
		m+=dlen;
		while(*m && strstr(m, delim) == m) {
			m+=dlen;
		}
		buf->tok_ = m;	
	}
	if (!strlen(retval)) retval = NULL;
	return retval;
}

void
strob_set_fill_char(STROB * strb, int ch)
{
	strb->fill_charM = ch;
}

int
strob_get_fill_char(STROB * strb)
{
	return strb->fill_charM;
}

/* --- Unrestricted binary string interface ---- NOT WELL TESTED ----------*/


void *
strob_memcpy(STROB *strb, void * ct, size_t n)
{
	if (!strob_set_up(strb, (int)n))
		return (void*)(NULL);
	return memcpy(strb->str_, ct, n);
}

void
strob_append_hidden_null(STROB *strb)
{
	strob_set_up(strb, strb->length_+1);
	memcpy(strb->str_+strb->length_, "\0", 1);
}

void *
strob_memcpy_at(STROB *strb, size_t offset, void * ct, size_t n)
{
	if (!strob_set_up(strb, (int)n + (int)offset))
		return (void*)(NULL);
	return memcpy(strb->str_+offset, ct, n);
}

void *
strob_memmove(STROB *strb, void * ct, size_t n)
{
	return strob_memmove_to(strb, 0, ct, n);
}

void *
strob_memmove_to(STROB *strb, size_t dst_offset, void * ct, size_t n)
{
	if (!strob_set_up(strb, (int)(dst_offset + n)))
		return (void*)(NULL);
	return memmove(strb->str_ + dst_offset, ct, n);
}

void *
strob_memcat(STROB *strb, void * ct, size_t n)
{
	return strob_memmove_to(strb, strb->length_, ct, n);
}

void *
strob_memset(STROB *strb, int c, size_t n)
{
	if (!strob_set_up(strb, (int)(n)))
		return (void*)(NULL);
	return memset(strb->str_, c, n);
}

void
strob_set_memlength(STROB * strb, int len)
{
	strob_reopen_if(strb, len);
	strb->length_=len;
}


/*-------------------- Depricated Names ------------------------------------ */

o__inline__
int
strob_setlen(STROB * strb, int len)
{
	strob_set_length(strb, len);
	return len;
}

o__inline__
int
strob_length(STROB * strb)
{
	return strob_get_length(strb);
}


o__inline__
char *
strob_str(STROB * strb)
{
	return strob_get_str(strb);
}

o__inline__
char *
strob_catstr(STROB * strb, char *str)
{
	return strob_strcat(strb, str);
}

int
strob_vsprintf_at(STROB * sb, int at_offset, char * format, va_list ap)
{
	int added_amount=0; 
	int up_incr = 128; 
	char * start;
	va_list aq;
	int ret;
	int len;
	char * oldend = NULL;

	if (at_offset > strob_get_reserve(sb)) {
		strob_set_memlength(sb, at_offset + up_incr);
	}

	do {
		if (oldend)
			*oldend = '\0';

		strob_set_memlength(sb, strob_get_reserve(sb) + added_amount);

		start = strob_str(sb) + at_offset; 
		len = strob_get_reserve(sb) - at_offset;

		oldend = start;
		added_amount += up_incr;
#if defined va_copy
                va_copy(aq, ap);
#elif defined __va_copy
                __va_copy(aq, ap);
#else
                memcpy(aq, ap, sizeof(va_list));
#endif
		ret=vsnprintf(start, len, format, aq);
		va_end(aq);
	} while (ret < 0 || ret >= len);
	return ret;
}

int
strob_vsprintf(STROB * sb, int do_append, char * format, va_list ap)
{
	int added_amount=0; 
	int up_incr = 128; 
	char * start;
	va_list aq;
	int ret;
	int len;
	char * oldend = NULL;

	do {
		if (oldend)
			*oldend = '\0';
		strob_set_memlength(sb, strob_get_reserve(sb) + added_amount);
		if (do_append) {
			start = strob_str(sb) + strlen(strob_str(sb)); 
			len = strob_get_reserve(sb) - strlen(strob_str(sb));
		} else {
			start = strob_str(sb);
			len = strob_get_reserve(sb);
		}		
		oldend = start;
		added_amount += up_incr;
#if defined va_copy
                va_copy(aq, ap);
#elif defined __va_copy
                __va_copy(aq, ap);
#else
                memcpy(aq, ap, sizeof(va_list));
#endif
		ret=vsnprintf(start, len, format, aq);
		va_end(aq);
	} while (ret < 0 || ret >= len);
	return ret;
}

int
strob_snprintf(STROB * sb, int do_append, size_t len, char * format, ...)
{
	int ret;
	va_list ap;
	
	int old_len;
	char * s;

	if (len <= 0) return len;

	if (do_append)
		old_len = strob_strlen(sb);
	else
		old_len = 0;

	va_start(ap, format);
	ret = strob_vsprintf(sb, do_append, format, ap); 
	va_end(ap);

	strob_chr_index(sb, old_len + len - 1, 0 /*i.e. NUL, '\x0' */);
	return ret;
}

int
strob_sprintf(STROB * sb, int do_append, char * format, ...)
{
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = strob_vsprintf(sb, do_append, format, ap); 
	va_end(ap);
	return ret;
}

int
strob_sprintf_at(STROB * sb, int at_offset, char * format, ...)
{
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = strob_vsprintf_at(sb, at_offset, format, ap); 
	va_end(ap);
	return ret;
}
