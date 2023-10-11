/* config_remains.h: define inttypes if-not-yet-defined with checks  */


#ifndef CONFIG_REMAINS_H_2007
#define CONFIG_REMAINS_H_2007

#undef swbis_bad_uintmax_t
#undef swbis_bad_intmax_t

#ifndef HAVE_STDINT_H_WITH_UINTMAX
	#ifndef HAVE_INTTYPES_H_WITH_UINTMAX
		#define uintmax_t unsigned long
	#endif
#endif

#ifndef HAVE_STDINT_H_WITH_UINTMAX 
#ifndef HAVE_INTTYPES_H_WITH_UINTMAX 
	#define uintmax_t unsigned long
	#ifdef HAVE_UNSIGNED_LONG_LONG_INT
		#define swbis_bad_uintmax_t 
	#endif
#endif
#endif

#ifndef HAVE_INTMAX_T
	#define intmax_t long
	#ifdef HAVE_LONG_LONG_INT
 		#define swbis_bad_intmax_t 
	#endif
#endif

#ifdef HAVE_UNSIGNED_LONG_LONG_INT
	#ifdef swbis_bad_uintmax_t
		#define ULLONG_MAX	Bad_Value_for_ULLONG_MAX_this_should_not_compile
	#else
		#ifndef ULLONG_MAX
			#define ULLONG_MAX 18446744073709551615ULL
		#endif
	#endif
#else
	#ifndef ULLONG_MAX
		#define ULLONG_MAX ULONG_MAX
	#endif
#endif

#ifdef HAVE_LONG_LONG_INT
	#ifdef swbis_bad_intmax_t
		#define LLONG_MAX	Bad_Value_for_LLONG_MAX_this_should_not_compile
	#else
		#ifndef LLONG_MAX
			#define LLONG_MAX 9223372036854775807LL
		#endif
	#endif
#else
	#ifndef LLONG_MAX
		#define LLONG_MAX LONG_MAX
	#endif
#endif

#endif
