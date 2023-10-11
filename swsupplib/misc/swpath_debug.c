#include "swuser_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "swlib.h"
#include "xformat.h"
#include "swpath.h"

static STROB * buf = NULL;

void
swpath_debug_dump(SWPATH * sp, FILE * fp)
{
	unsigned long name_sum;
	char * name = swpath_get_pkgpathname(sp);
	char * prepath = sp->swpath_p_prepath_;
	char * dfiles = sp->swpath_p_dfiles_;
	char * fileset = sp->swpath_p_fileset_dir_;
	char * pfiles = sp->swpath_p_pfiles_;

	name_sum = swlib_bsd_sum_from_mem(name, strlen(name));

	prepath = prepath ? prepath   : "<null>";
	dfiles  = dfiles  ? dfiles    : "<null>";
	fileset = fileset ? fileset   : "<null>";
	pfiles  = pfiles  ? pfiles    : "<null>";

	fprintf (fp, "swpath%05lu   name                 = \"%s\"\n", name_sum, name);
	fprintf (fp, "swpath%05lu   pkgpathname          = \"%s\"\n", name_sum, swpath_get_pkgpathname(sp));
	fprintf (fp, "swpath%05lu   prepath              = \"%s\"\n", name_sum, swpath_get_prepath(sp));
	fprintf (fp, "swpath%05lu   dfiles               = \"%s\"\n", name_sum, swpath_get_dfiles(sp));
	fprintf (fp, "swpath%05lu   pfiles               = \"%s\"\n", name_sum, swpath_get_pfiles(sp));
	fprintf (fp, "swpath%05lu   product_control_dir  = \"%s\"\n", name_sum, swpath_get_product_control_dir(sp));
	fprintf (fp, "swpath%05lu   fileset_dir          = \"%s\"\n", name_sum, swpath_get_fileset_control_dir(sp));
	fprintf (fp, "swpath%05lu   pathname             = \"%s\"\n", name_sum, swpath_get_pathname(sp));
	fprintf (fp, "swpath%05lu   filename             = \"%s\"\n", name_sum, swpath_get_basename(sp));
	fprintf (fp, "swpath%05lu   is catalog           = \"%d\"\n", name_sum, swpath_get_is_catalog(sp));
	fprintf (fp, "swpath%05lu   p_prepath            = \"%s\"\n", name_sum, prepath);
	fprintf (fp, "swpath%05lu   p_dfiles             = \"%s\"\n", name_sum, dfiles);
	fprintf (fp, "swpath%05lu   p_fileset            = \"%s\"\n", name_sum, fileset);
	fprintf (fp, "swpath%05lu   p_pfiles             = \"%s\"\n", name_sum, pfiles);

}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

char * 
swpath_dump_string_s(SWPATH * swpath, char * prefix)
{
	char *s;
	char prebuf[400];
	if (buf == (STROB*)NULL) buf = strob_open(100);

	strob_sprintf(buf, 0, "%s%p (SWPATH*)\n", prefix,  (void*)swpath);
	strob_sprintf(buf, 1, "%s%p->swpath_is_catalog_      = [%d]\n",  prefix, (void*)swpath, swpath->swpath_is_catalog_);
	strob_sprintf(buf, 1, "%s%p->errorM                  = [%d]\n",  prefix, (void*)swpath, swpath->errorM);
	strob_sprintf(buf, 1, "%s%p->control_path_nominal_depth_ = [%d]\n",  prefix, 
							(void*)swpath, swpath->control_path_nominal_depth_);
	
	strob_sprintf(buf, 1, "%s%p->product_control_dir_    = [%s]\n", prefix, (void*)swpath, strob_str(swpath->product_control_dir_));
	strob_sprintf(buf, 1, "%s%p->fileset_control_dir_    = [%s]\n", prefix, (void*)swpath, strob_str(swpath->fileset_control_dir_));
	strob_sprintf(buf, 1, "%s%p->dfiles_                 = [%s]\n", prefix, (void*)swpath, strob_str(swpath->dfiles_));
	strob_sprintf(buf, 1, "%s%p->pfiles_                 = [%s]\n", prefix, (void*)swpath, strob_str(swpath->pfiles_));
    
	strob_sprintf(buf, 1, "%s%p->swpackage_pathname_     = [%s]\n", prefix, (void*)swpath, strob_str(swpath->swpackage_pathname_));
	strob_sprintf(buf, 1, "%s%p->pathname_               = [%s]\n", prefix, (void*)swpath, strob_str(swpath->pathname_));
	strob_sprintf(buf, 1, "%s%p->buffer_                 = [%s]\n", prefix, (void*)swpath, strob_str(swpath->buffer_));
	strob_sprintf(buf, 1, "%s%p->basename_               = [%s]\n", prefix, (void*)swpath, strob_str(swpath->basename_));
	
	strob_sprintf(buf, 1, "%s%p->prepath_                = [%s]\n", prefix, (void*)swpath, strob_str(swpath->prepath_));
	strob_sprintf(buf, 1, "%s%p->p_pfiles_               = [%s]\n", prefix, (void*)swpath, strob_str(swpath->p_pfiles_));
	strob_sprintf(buf, 1, "%s%p->p_dfiles_               = [%s]\n", prefix, (void*)swpath, strob_str(swpath->p_dfiles_));
    
	strob_sprintf(buf, 1, "%s%p->swpath_p_prepath_       = [%s]\n", prefix, (void*)swpath, (s=swpath->swpath_p_prepath_)?s:"");
	strob_sprintf(buf, 1, "%s%p->swpath_p_dfiles_        = [%s]\n", prefix, (void*)swpath, (s=swpath->swpath_p_dfiles_)?s:"");
	strob_sprintf(buf, 1, "%s%p->swpath_p_pfiles_        = [%s]\n", prefix, (void*)swpath, (s=swpath->swpath_p_pfiles_)?s:"");
	strob_sprintf(buf, 1, "%s%p->swpath_p_fileset_dir_   = [%s]\n", prefix, (void*)swpath, (s=swpath->swpath_p_fileset_dir_)?s:"");
	strob_sprintf(buf, 1, "%s%p->pathlist_               = [%p]\n", prefix, (void*)swpath, (void*)(swpath->pathlist_));

	strob_sprintf(buf, 1, "...\n");
	strob_sprintf(buf, 1, "   pkgpathname          = \"%s\"\n", swpath_get_pkgpathname(swpath));
	strob_sprintf(buf, 1, "   prepath              = \"%s\"\n", swpath_get_prepath(swpath));
	strob_sprintf(buf, 1, "   dfiles               = \"%s\"\n", swpath_get_dfiles(swpath));
	strob_sprintf(buf, 1, "   pfiles               = \"%s\"\n", swpath_get_pfiles(swpath));
	strob_sprintf(buf, 1, "   product_control_dir  = \"%s\"\n", swpath_get_product_control_dir(swpath));
	strob_sprintf(buf, 1, "   fileset_dir          = \"%s\"\n", swpath_get_fileset_control_dir(swpath));
	strob_sprintf(buf, 1, "   pathname             = \"%s\"\n", swpath_get_pathname(swpath));
	strob_sprintf(buf, 1, "   filename             = \"%s\"\n", swpath_get_basename(swpath));


	/*
	if (swpath->pathlist_) {
		snprintf(prebuf, sizeof(prebuf)-1, "%s%p->pathlist_ ", prefix, (void*)(swpath));
		strob_sprintf(buf, 1, "%s", strar_dump_string_s(swpath->pathlist_, prebuf));
	}
	*/
	return strob_str(buf);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

char * 
swpath_dump_string_ex_s(SWPATH_EX * swpath, char * prefix)
{
	char *s;
	char prebuf[800];
	if (buf == (STROB*)NULL) buf = strob_open(100);
	
	strob_sprintf(buf, 0, "%s%p (SWPATH_EX*)\n", prefix,  (void*)swpath);
	if (swpath) {
	strob_sprintf(buf, 1, "%s%p->is_catalog      = [%d]\n",  prefix, (void*)swpath, swpath->is_catalog);
	strob_sprintf(buf, 1, "%s%p->ctl_depth       = [%d]\n",  prefix, (void*)swpath, swpath->ctl_depth);
	strob_sprintf(buf, 1, "%s%p->pkgpathname     = [%s]\n", prefix, (void*)swpath, swpath->pkgpathname);
	strob_sprintf(buf, 1, "%s%p->prepath         = [%s]\n", prefix, (void*)swpath, swpath->prepath);
	strob_sprintf(buf, 1, "%s%p->dfiles          = [%s]\n", prefix, (void*)swpath, swpath->dfiles);
	strob_sprintf(buf, 1, "%s%p->pfiles          = [%s]\n", prefix, (void*)swpath, swpath->pfiles);
	strob_sprintf(buf, 1, "%s%p->product_control_dir = [%s]\n", prefix, (void*)swpath, swpath->product_control_dir);
	strob_sprintf(buf, 1, "%s%p->fileset_control_dir = [%s]\n", prefix, (void*)swpath, swpath->fileset_control_dir);
	strob_sprintf(buf, 1, "%s%p->pathname            = [%s]\n", prefix, (void*)swpath, swpath->pathname);
	strob_sprintf(buf, 1, "%s%p->basename            = [%s]\n", prefix, (void*)swpath, swpath->basename);
	}
	return strob_str(buf);
}

