/**/
#ifndef SWUSER_CONFIGG1_H
#define SWUSER_CONFIGG1_H
#include "config.h"
#include "swprog_versions.h"

#define OPT_inline__
#define o__inline__

#define SW_YYPARSE yyparse

/* ================================================================================== */
/* If you have problems with your realloc this *may* help.
*/

/* FIXME
#ifndef __FUNCTION__
#define __FUNCTION__ "unknown"
#endif
*/


#define SWBIS_BUSTED_REALLOC  1
#undef SWBIS_BUSTED_REALLOC  
	/* Note: swlib_realloc()  is untested. */
#ifdef SWBIS_BUSTED_REALLOC 
#define SWBIS_REALLOC(ptr, newsize, oldsize)  swlib_realloc(ptr, newsize, oldsize)
#else
#define SWBIS_REALLOC(ptr, newsize, oldsize)  realloc(ptr, newsize)
#endif

#define SWBISMALLOCDEBUG 1
#undef SWBISMALLOCDEBUG

#ifdef SWBISMALLOCDEBUG
void * malloc_fprintf(int sz, const char *format, ...);

#define swbis_free(arg) { \
	\
        fprintf(stderr, "FREE    %18s:%-5d , %s : arg=%p\n", \
		                __FILE__, __LINE__, __FUNCTION__, (void*)(arg)); \
	free(arg); \
	}

#define malloc(arg) \
	malloc_fprintf(arg, "MALLOC  %18s:%-5d , %s : ", \
		__FILE__, __LINE__, __FUNCTION__)

#define realloc(arg, arg1) \
	realloc_fprintf(arg, arg1, "REALLOC %18s:%-5d , %s : ", \
		__FILE__, __LINE__, __FUNCTION__)
#else
#define swbis_free free
#endif

#define swbis_devnull_open uxfio_devnull_open	/* cache opens on "/dev/null" */
#define swbis_devnull_close uxfio_devnull_close

/*
* =====================
*/
/*
#ifndef NOASSERT
#define SWLIB_ASSERT(arg)  swlib_assertion_fatal(arg, "", __FILE__, __LINE__, __FUNCTION__);
#else
#define SWLIB_ASSERT(arg)
#endif

#define SWLIB_ALLOC_ASSERT(arg)  swlib_assertion_fatal(arg, "out of memory", __FILE__, __LINE__, __FUNCTION__);
*/
/*
* =====================
*/
/* #define SWBISNEEDDEBUG 1 */
#undef SWBISNEEDDEBUG

#undef DEBUG_WITH_PID
#define DEBUG_WITH_PID 1

#ifdef DEBUG_WITH_PID
#define SWBISERROR3(class, format, arg1, arg2) { \
	\
	fprintf(stderr, class "[%d]: %18s:%-5d , %s : " format "\n", (int)0,\
		__FILE__, __LINE__, __FUNCTION__,  arg1, arg2); \
	}
#define SWBISERROR2(class, format, arg) { \
	\
	fprintf(stderr, class "[%d]: %18s:%-5d , %s : " format "\n", (int)0,\
		__FILE__, __LINE__, __FUNCTION__, arg); \
	}
#define SWBISERROR(class, format) { \
	\
	fprintf(stderr, class "[%d]: %18s:%-5d , %s : " format "\n", (int)0,\
		__FILE__, __LINE__, __FUNCTION__); \
	}
#else


#define SWBISERROR3(class, format, arg1, arg2) { \
	\
	fprintf(stderr, class "[%d]: %18s:%-5d , %s : " format "\n", (int)getpid(),\
		__FILE__, __LINE__, __FUNCTION__,  arg1, arg2); \
	}
#define SWBISERROR2(class, format, arg) { \
	\
	fprintf(stderr, class "[%d]: %18s:%-5d , %s : " format "\n", (int)getpid(),\
		__FILE__, __LINE__, __FUNCTION__, arg); \
	}
#define SWBISERROR(class, format) { \
	\
	fprintf(stderr, class "[%d]: %18s:%-5d , %s : " format "\n", (int)getpid(),\
		__FILE__, __LINE__, __FUNCTION__); \
	}
#endif





#define SWBISNEEDFAIL 1	/* Should be on even for production use. */

/*
 * Controlled outside this file
 * #define FILENEEDDEBUG
 * #undef FILENEEDDEBUG
 */
#ifdef FILENEEDDEBUG
#define E_DEBUG(format) SWBISERROR("DEBUG", format)
#define E_DEBUG2(format, arg) SWBISERROR2("DEBUG", format, arg)
#define E_DEBUG3(format, arg, arg1) \
			SWBISERROR3("DEBUG", format, arg, arg1)
#else
#define E_DEBUG(arg)
#define E_DEBUG2(arg, arg1)
#define E_DEBUG3(arg, arg1, arg2)
#endif

#ifdef SWBISNEEDFAIL
#define SWBIS_E_FAIL(format) SWBISERROR("INTERNAL ERROR: ", format)
#define SWBIS_E_FAIL2(format, arg) SWBISERROR2("INTERNAL ERROR: ", format, arg)
#define SWBIS_E_FAIL3(format, arg, arg1) SWBISERROR3("INTERNAL ERROR: ", format, arg, arg1)
#else
#define SWBIS_E_FAIL(arg)
#define SWBIS_E_FAIL2(arg, arg1)
#define SWBIS_E_FAIL3(arg, arg1, arg2)
#endif /* SWBISNEEDFAIL */

#ifdef SWBISNEEDDEBUG
#define SWBIS_E_DEBUG(format) SWBISERROR("DEBUG: ", format)
#define SWBIS_E_DEBUG2(format, arg) SWBISERROR2("DEBUG: ", format, arg)
#define SWBIS_E_DEBUG3(format, arg, arg1) SWBISERROR3("DEBUG: ", format, arg, arg1)
#else
#define SWBIS_E_DEBUG(arg)
#define SWBIS_E_DEBUG2(arg, arg1)
#define SWBIS_E_DEBUG3(arg, arg1, arg2)
#endif /* SWBISNEEDDEBUG */

/*
* =====================
*/

#define swbis_free free

#include "swinttypes.h"


#endif
