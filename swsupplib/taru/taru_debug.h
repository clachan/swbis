
#ifndef TARU_DEBUG_H_INC_20020505jhl
#define TARU_DEBUG_H_INC_20020505jhl

#include "swuser_config.h"

#include "debug_config.h"

#ifdef TARUNEEDFAIL
#define TARU_E_FAIL(format) SWBISERROR("TARU INTERNAL ERROR: ", format)
#define TARU_E_FAIL2(format, arg) SWBISERROR2("TARU INTERNAL ERROR: ", format, arg)
#define TARU_E_FAIL3(format, arg, arg1) SWBISERROR3("TARU INTERNAL ERROR: ", format, arg, arg1)
#else
#define TARU_E_FAIL(arg)
#define TARU_E_FAIL2(arg, arg1)
#define TARU_E_FAIL3(arg, arg1, arg2)
#endif

#ifdef TARUNEEDDEBUG
#define TARU_E_DEBUG(format) SWBISERROR("TARU DEBUG: ", format)
#define TARU_E_DEBUG2(format, arg) SWBISERROR2("TARU DEBUG ", format, arg)
#define TARU_E_DEBUG3(format, arg, arg1) SWBISERROR3("TARU DEBUG: ", format, arg, arg1)
#else
#define TARU_E_DEBUG(arg)
#define TARU_E_DEBUG2(arg, arg1)
#define TARU_E_DEBUG3(arg, arg1, arg2)
#endif /* TARUNEEDDEBUG */


#endif
