#ifndef SWUSER_ASSERT_CONFIGG1_H
#define SWUSER_ASSERT_CONFIGG1_H

/* FIXME
#ifndef __FUNCTION__
#define __FUNCTION__ "unknown"
#endif
*/

#ifndef SWLIB_NO_ASSERT
#define SWLIB_ASSERT(arg)  swlib_assertion_fatal(arg, "", (char*)__FILE__, __LINE__, (char*)__FUNCTION__);
#else
#define SWLIB_ASSERT(arg)
#endif

#define SWLIB_ALLOC_ASSERT(arg)  swlib_assertion_fatal(arg, "out of memory", (char*)__FILE__, __LINE__, (char*)__FUNCTION__);

#endif
