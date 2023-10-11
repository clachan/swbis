
#include "swuser_config.h"

#ifdef USE_WITH_SELF_RPM
#include "self_header.h"
#elif USE_WITH_RPM
#include <rpm/header.h>
#else
#include "self_header.h"
#endif
