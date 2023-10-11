/* swinttypes.h:  */

#ifndef SWINTTYPES_H
#define SWINTTYPES_H
#include "swuser_config.h"

#if HAVE_INTTYPES_H_WITH_UINTMAX
# include <inttypes.h>
#endif

#ifdef HAVE_STDINT_H_WITH_UINTMAX
# include <stdint.h>
#endif

#ifdef HAVE_INTMAX_T
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#endif

#endif
