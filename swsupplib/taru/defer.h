/* defer.h - keep track of the defered hard-link files.
 */

#ifndef dfe_defer_h_FEB
#define dfe_defer_h_FEB

#include "swuser_config.h"

struct deferment
{
    struct new_cpio_header * headerP;
    struct deferment *nextP;
};

typedef struct {
	int formatM;
	struct deferment * deferoutsM;
	void * taruM;
} DEFER;

DEFER *  defer_open(int format);
void     defer_close(DEFER * def);
void     defer_set_taru(DEFER * def, void * taru);
void     defer_set_format(DEFER * def, int format);
int      defer_is_last_link(DEFER * def, struct new_cpio_header *file_hdr);
void     defer_add_link_defer(DEFER *def, struct new_cpio_header *file_hdr);
int      defer_writeout_zero_length_defers(DEFER *def, struct new_cpio_header *file_hdr, int outfd);
int      defer_writeout_final_defers(DEFER *def, int out_des);
#endif
