#include "swuser_config.h"

#ifdef USE_WITH_SELF_RPM
#include "self_rpmlib.h"
#elif USE_WITH_RPM
#include <rpm/rpmlib.h>
#else
#include "self_rpmlib.h"
#endif
