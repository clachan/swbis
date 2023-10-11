/* strtoint.h -- convert printable strings to integers */

#ifndef STRTOINT_H_2007
#define STRTOINT_H_2007

#include <stdint.h>
#include <sys/types.h>

#include "swuser_config.h"
#include "intprops.h"

intmax_t strtoimax (char const *ptr, char **endptr, int base);
uintmax_t strtoumax (char const *ptr, char **endptr, int base);

#endif
