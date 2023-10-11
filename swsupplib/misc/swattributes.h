/* swattributes.h: Attribute mapping cross reference.
 */

#include "rpmbis.h"

#define SWMAP_MAX_LENGTH 163  /*  i.e. because the last tag is 1163 */

#define RPMPSF_DEF_DFILES		"rpm_dfiles"     /* FIXME rename these? */
#define RPMPSF_FILESET_SOURCE		"sources"
#define RPMPSF_FILESET_BUILD		"build_control"
#define RPMPSF_FILESET_PATCHES		"patches"
#define RPMPSF_BIN_FILESET_TAG		"binpackage"


typedef struct {
	struct swattributes_map * map_table_;
} SWATTMAP;

#define SWATMAP_NAME_RPMTAG		0	/* rpmtag name RPMTAG */
#define SWATMAP_NAME_SWLIST_ARGS	1	/* args to swlist */
#define SWATMAP_NAME_DEB		2	/* Debian attribute name */
#define SWATMAP_NAME_SWBIS		3	/* swbis attribute name */

#define SWATMAP_NOT_LSB			0
#define SWATMAP_IS_LSB			1

#define SWATMAP_ATT_Header		0       /* Use the package attributes */
#define SWATMAP_ATT_Signature		1       /* Use the signature attributes */

#define SWATTMAP_MAP_NA			"NA"    /* Mapping Not applicable */
#define SWATTMAP_MAP_NONE		"NONE"  /* Maping undefined */

struct swattributes_map
        {
		int rpmtag_number;      /* rpm tag number           */
	        int rpmtag_type;        /* tag type                 */
		int is_lsb;		/* 0: not LSB tag  1: LSB sanctioned tag */
		int lsb_status;         /* 0: optional  1: required  -1: not set, or NA */
		int count;              /* 1 if single attribute, 0 if array */
		char * names[4]; 	/* See SWATMAP_NAME_<*> for definition */
		/*
		 names[0] is rpmtag_name;
		 names[1] is swlist_args;
		 names[2] is debian_attribute;
		 names[3] is swbis_attribute;
		*/
	};



int swatt_get_rpmtype(int table_id, int rpmtag);
int swatt_get_rpmcount(int table_id, int rpmtag);

const struct swattributes_map swdef_sig_maptable[] = {
	/* Last tag of rpm-4.4.1 */
		{
		0 , (int)0, -1, -1, 0,
		{
		(char *)NULL, 
		"", 
		"", 
		""
		}}
};



const struct swattributes_map swdef_pkg_maptable[] = {
	{ 
		1000, RPM_STRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_NAME",
	        "-d -a distributions",
		"",	
		"product.tag"
		}},
	{ 
		1001, RPM_STRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_VERSION",
		"-d -l distribution -a revision",
		"",	
		"product.revision"
		}},
	{ 
		1002, RPM_STRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_RELEASE", 
		"-d -l distribution -a release",
		"",	
		"product.vendor_tag"
		}},
	{ 
		1003, RPM_STRING_TYPE, 0, -1, 1,	   /* Not LSB sanctioned */
		{
		"RPMTAG_SERIAL", 
		"-d -l product -a number",
		"",	
		SWATTMAP_MAP_NONE
		}},
	{ 
		1004, RPM_I18NSTRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_SUMMARY", 
		"-d -l distribution -a title",
		"",	
		"product.title"
		}},
	{ 
		1005, RPM_I18NSTRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_DESCRIPTION", 
		"-d -l distribution -a description",
		"",	
		"product.description"
		}},
	{ 
		1006, RPM_INT32_TYPE, 1, 0, 1,
		{
		"RPMTAG_BUILDTIME", 
		"-d -l distribution -a create_time",
		"",	
		"distribution.create_time"
		}},
	{ 
		1007, RPM_STRING_TYPE, 1, 0, 1,
		{
		"RPMTAG_BUILDHOST", 
		"-d -l product -a build_host",
		"",	
		"product.build_host"
		}},
	{ 
		1008, RPM_INT32_TYPE, 0, -1, 1,
		{
		"RPMTAG_INSTALLTIME", 
		"-d -l product -a mod_time",
		"",	
		"product.mod_time"
		}},
	{ 
		1009, RPM_INT32_TYPE, 1, 1, 1,
		{
		"RPMTAG_SIZE", 
		"-d -l fileset -a size",
		"",	
		"fileset.size"
		}},
	{ 
		1010, RPM_STRING_TYPE, 0, -1,  1,  /* LSB Informational */
		{
		"RPMTAG_DISTRIBUTION", 
		"-d -l distribution -a dfiles", 	   /* swbis */
		"",	
		"distribution.dfiles"
		}},
	{ 
		1011, RPM_STRING_TYPE, 0, -1,  1,  /* Informational */
		{
		"RPMTAG_VENDOR", 
		"-d -l vendor -a title *,vq=seller",       /* swbis */
		"",	
		"vendor.tag"
		}},
	{ 
		1012 , RPM_BIN_TYPE, 0, -1, 1,
		{
		"RPMTAG_GIF", 
		"",
		"",	
		""
		}},
	{ 
		1013, RPM_BIN_TYPE, 0, -1, 1,
		{
		"RPMTAG_XPM", 
		"",
		"",	
		""
		}},
	{ 
		1014, RPM_STRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_LICENSE", 
		"-d -l distribution -a license",
		"",	
		""
		}},
	{
		1015, RPM_STRING_TYPE, -1, -1, 1,
		{
		"RPMTAG_PACKAGER", 
		"-d -l vendor -a title *,vq=packager",      /* swbis */
		"",	
		""
		}},
	{
 	 	 1016, RPM_I18NSTRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_GROUP",  
		"-d -l category -a title",
		"",	
		""
		}},
	{
 	 	 1017, RPM_STRING_TYPE, 0, -1,  1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1018, RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_SOURCE",  
		"-d -l product -a source_package",  		/* swbis */
		"",	
		""
		}},
	{
 	 	1019, RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_PATCH",  
		"-d -l product -a all_patches", 		/* swbis */
		"",	
		""
		}},
	{
 	 	 1020, RPM_STRING_TYPE, 0, -1, 1,
		{
		"RPMTAG_URL",  
		"-d -l product -a url",
		"",	
		""
		}},
	{
 	 	 1021, RPM_STRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_OS",  
		"-d -l host -a os_name",
		"",	
		""
		}},
	{
 	 	 1022, RPM_STRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_ARCH",  
		"-d -l distribution -a architecture",
		"",	
		""
		}},
	{
 	 	 1023, RPM_STRING_TYPE, 1, 0, 1,
		{"RPMTAG_PREIN",  
		"-d -l control_file -a preinstall *", /* source stored in control_file.contents */ 
		"",	
		""
		}},
	{
 	 	 1024, RPM_STRING_TYPE, 1, 0, 1,
		{
		"RPMTAG_POSTIN",  
		"-d -l control_file -a postinstall *", /* source stored in control_file.contents */   
		"",	
		""
		}},
	{
 	 	 1025, RPM_STRING_TYPE, 1, 0, 1,
		{
		"RPMTAG_PREUN",  
		"-d -l control_file -a unpreinstall *",  /* source stored in control_file.contents */   
		"",	
		""
		}},
	{
 	 	 1026, RPM_STRING_TYPE, -1, -1, 1,
		{
		"RPMTAG_POSTUN",  
		"-d -l control_file -a unpostinstall *",  /* source stored in control_file.contents */   
		"",	
		""
		}},
	{
 	 	 1027, RPM_STRING_ARRAY_TYPE, 1, 0, 0,
		{
		"RPMTAG_FILENAMES|RPMTAG_OLDFILENAMES",   /* also called RPMTAG_OLDFILENAMES by the LSB */
		"-d -l file -l control_file -a path",
		"",	
		""
		}},
	{
 	 	 1028, RPM_INT32_TYPE, 1, 1, 0,
		{"RPMTAG_FILESIZES",  
		"-d -l file -l control_file -a size",
		"",	
		""
		}},
	{
 	 	 1029, RPM_INT8_TYPE, -1, -1, 0,
		{"RPMTAG_FILESTATES",  
		"-d -l file -l control_file -a state",
		"",	
		""
		}},
	{
 	 	 1030, RPM_INT16_TYPE, 1, 1, 0,
		{
		"RPMTAG_FILEMODES",  
		"-d -l file -l control_file -a mode",
		"",	
		""
		}},
	{
 	 	 1031, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	 1032, RPM_INT16_TYPE, -1, -1,  1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}}, 
	{
 	 	 1033, RPM_INT16_TYPE, 1, 1, 0,
		{
		"RPMTAG_FILERDEVS",  
		"-d -l file -l control_file -x one_liner='major minor'",  /* stored as minor and major */
		"",	
		""
		}},
	{
 	 	 1034, RPM_INT32_TYPE, 1, 1, 0,
		{
		"RPMTAG_FILEMTIMES",  
		"-d -l file -l control_file -a mtime",
		"",
		""
		}},
	{
 	 	 1035, RPM_STRING_ARRAY_TYPE, 1, 1, 0,
		{"RPMTAG_FILEMD5S",  
		"-d -l file -l control_file -a md5sum",
		"",	
		""
		}},
	{
 	 	 1036, RPM_STRING_ARRAY_TYPE, 1, 1, 0,
		{
		"RPMTAG_FILELINKTOS",  
		"-d -l file -l control_file -a link_source",
		"",	
		""
		}},
	{
 	 	1037, RPM_INT32_TYPE, 1, 1, 0,
		{
		"RPMTAG_FILEFLAGS",  
		"-d -l file -l control_file -a rpm_fileflags", /* swbis */
		"",	
		""
		}},
	{
 	 	1038, RPM_STRING_TYPE, -1, -1, 1, /* not sure */
		{
		"RPMTAG_ROOT",  
		"",
		"",	
		""
		}},
	{
 	 	1039, RPM_STRING_ARRAY_TYPE, 1, 1, 0,
		{"RPMTAG_FILEUSERNAME",  
		"-d -l file -l control_file -a owner",
		"",	
		""
		}},
	{
 	 	1040, RPM_STRING_ARRAY_TYPE, 1, 1, 0,
		{
		"RPMTAG_FILEGROUPNAME",  
		"-d -l file -l control_file -a group",
		"",	
		""
		}},
	{
 	 	1041, RPM_STRING_TYPE, -1, -1,  1, /* depricated in rpm */
		{
		"RPMTAG_EXCLUDE",  
		"",
		"",	
		""
		}},
	{
 	 	1042, RPM_STRING_TYPE, -1, -1,  1, /* depricated in rpm */
		{
		"RPMTAG_EXCLUSIVE",  
		"",
		"",	
		""
		}},
                                              /* unsure about type */	
	{
 	 	1043, RPM_BIN_TYPE, -1, -1,  1, /* not yet assigned */ 
		{
		"RPMTAG_ICON",  
		"",
		"",	
		""
		}},
	{
 	 	1044, RPM_STRING_TYPE, 1, 0, 1,
		{
		"RPMTAG_SOURCERPM",  
		"-d -l product -a sourcerpm",                    /* swbis */
		"",	
		""
		}},
	{
 	 	1045, RPM_INT32_TYPE, 1, 0, 1,
		{
		"RPMTAG_FILEVERIFYFLAGS", 
		"-d -l file -l control_file -a rpm_verifyflags",
		"",	
		""
		}},
	{
 	 	 1046, RPM_INT32_TYPE, 1, 0, 1,
		{"RPMTAG_ARCHIVESIZE",  
		"",
		"",	
		""
		}},
	{
 	 	 1047, RPM_STRING_ARRAY_TYPE, 1, 1, 0,  /* only the first element is valid */
		{
		"RPMTAG_PROVIDENAME",  
		"-d -l fileset -a rpm_provides *," RPMPSF_BIN_FILESET_TAG,  /* swbis */	
		"",	
		""
		}},
	{
 	 	 1048, RPM_INT32_TYPE, 1, 1, 0,  /* stored as ver_id (rpmflag) of rpm_flag attribute */
		{
		"RPMTAG_REQUIREFLAGS",  
		"-d -l fileset -a prerequisites *." RPMPSF_BIN_FILESET_TAG,
		"",	
		""
		}},
	{
 	 	 1049, RPM_STRING_ARRAY_TYPE, 1, 1, 0,
		{
		"RPMTAG_REQUIRENAME",  
		"-d -l fileset -a prerequisites *." RPMPSF_BIN_FILESET_TAG,
		"",	
		""
		}},
	{
 	 	 1050, RPM_STRING_ARRAY_TYPE, 1, 1, 0,
		{"RPMTAG_REQUIREVERSION",  
		"-d -l fileset -a prerequisites *."RPMPSF_BIN_FILESET_TAG,
		"",	
		""
		}},
	{
 	 	 1051, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	 1052, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"",
		"",	
		"" 
		}},
	{
 	 	 1053, RPM_INT32_TYPE, 1, 0, 0,  /* stored as ver_id (rpmflag) of rpm_flag attribute */
		{
		"RPMTAG_CONFLICTFLAGS", 
		"-d -l fileset -a exrequisites *."RPMPSF_BIN_FILESET_TAG, 
		"",
		""
		}},
	{
 	 	1054, RPM_STRING_ARRAY_TYPE, 1, 0, 0,
		{
		"RPMTAG_CONFLICTNAME",  
		"-d -l fileset -a exrequisites *."RPMPSF_BIN_FILESET_TAG,
		"",	
		""
		}},
	{
 	 	1055, RPM_STRING_ARRAY_TYPE, 1, 0, 0,
		{
		"RPMTAG_CONFLICTVERSION",  
		"-d -l fileset -a exrequisites *."RPMPSF_BIN_FILESET_TAG,
		"",	
		""
		}},
	{
 	 	1056, RPM_STRING_TYPE, -1, -1, 1,
 		{
		"RPMTAG_DEFAULTPREFIX",  
		"-d -l distribution -a rpm_default_prefix",	/* swbis */
		"",	
		""
		}},
	{
 	 	1057, RPM_STRING_TYPE, -1, -1, 1,
		{"RPMTAG_BUILDROOT",  
		"-d -l product -a build_root",                  /* swbis */
		"",	
		""
		}},
	{
 	 	1058, RPM_STRING_TYPE, -1, -1, 1,
		{
		"RPMTAG_INSTALLPREFIX",  
		"-d -l product -a directory",
		"",	
		""
		}},
	{
 	 	1059, RPM_STRING_ARRAY_TYPE, -1, -1, 0,
		{
		"RPMTAG_EXCLUDEARCH",  
		"-d -l fileset -a excluded_arch *." RPMPSF_FILESET_BUILD, 		/* swbis */
		"",	
		""
		}},
	{
 	 	1060, RPM_STRING_ARRAY_TYPE, -1, -1, 0,  /* Does not appear to be widely used */ 
		{
		"RPMTAG_EXCLUDEOS",  
		"-d -l fileset -a exclude_os *." RPMPSF_FILESET_BUILD, /* swbis */
		"",	
		""
		}},
	{
 	 	1061, RPM_STRING_ARRAY_TYPE, -1, -1, 0,
		{
		"RPMTAG_EXCLUSIVEARCH",  
		"-d -l fileset -a exclusive_arch *."RPMPSF_FILESET_BUILD, 	/* swbis */
		"",	
		""
		}},
	{
 	 	1062, RPM_STRING_ARRAY_TYPE, -1, -1, 0,
		{
		"RPMTAG_EXCLUSIVEOS",  
		"-d -l fileset -a exclusive_os *."RPMPSF_FILESET_BUILD, 	/* swbis */
		"",	
		""
		}},
	{
 	 	1063, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"",
		"",	
		"" 
		}},
	{
 	 	1064, RPM_STRING_TYPE, 1, 0, 1,
		{
		"RPMTAG_RPMVERSION",  
		"-d -l distribution -a rpmversion",
		"",
		""
		}},
	{
 	 	1065, RPM_INT16_TYPE, -1, -1,  1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1066, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1067, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1068, RPM_INT16_TYPE, -1, -1, 1,
		{"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1069, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1070, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1071, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1072, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1073, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1074, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1075, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1076, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"", 
		"" 
		}},
	{
 	 	1077, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1078, RPM_INT16_TYPE, -1, -1, 1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	 1079, RPM_STRING_TYPE, -1, -1, 1,
		{
		"RPMTAG_VERIFYSCRIPT",  
		"-d -l control_file -a verify *."RPMPSF_FILESET_BUILD,
		"",	
		""
		}},
	{
 	 	1080, RPM_INT32_TYPE, 1, 0, 1,
		{
		"RPMTAG_CHANGELOGTIME",  
		"-d -l fileset -a change_log *."RPMPSF_FILESET_BUILD",*."RPMPSF_BIN_FILESET_TAG,
		"",	
		""
		}},
	{
 	 	1081, RPM_STRING_ARRAY_TYPE, 1, 0, 0,
		{
		"RPMTAG_CHANGELOGNAME",  
		"-d -l fileset -a change_log *."RPMPSF_FILESET_BUILD",*."RPMPSF_BIN_FILESET_TAG,
		"",	
		""
		}},
	{
 	 	1082, RPM_STRING_ARRAY_TYPE, 1, 0, 0,
		{
		"RPMTAG_CHANGELOGTEXT",  
		"-d -l fileset -a change_log *."RPMPSF_FILESET_BUILD",*."RPMPSF_BIN_FILESET_TAG,
		"",	
		""
		}},
	{
 	 	1083, RPM_INT16_TYPE, -1, -1,  1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1084, RPM_INT16_TYPE, -1, -1,  1,
		{
		"NOT_USED",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1085, RPM_STRING_TYPE, 1, 0, 1,
		{
		"RPMTAG_PREINPROG",  
		"-d -l control_file -a interpreter preinstall",
		"",	
		""
		}},
	{
 	 	1086, RPM_STRING_TYPE, 1, 0, 1,
		{
		"RPMTAG_POSTINPROG",  
		"-d -l control_file -a interpreter postinstall",
		"",	
		""
		}},
	{
 	 	1087 , RPM_STRING_TYPE, 1, 0, 1,
		{
		"RPMTAG_PREUNPROG",
		"-d -l control_file -a interpreter unpreinstall",
		"",	
		""
		}},
	{
 	 	 1088, RPM_STRING_TYPE, 1, 0, 1,
		{
		"RPMTAG_POSTUNPROG",
		"-d -l control_file -a interpreter unpostinstall",
		"",	
		""
		}},
	{
 	 	1089, RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_BUILDARCHS",
		"-d -l distribution -a rpm_buildarchs",
		"",	
		""
		}},
	{
 	 	1090, RPM_STRING_ARRAY_TYPE, 1, 0, 0,
		{
		"RPMTAG_OBSOLETENAME",  
		"-d -l distribution -a rpm_obsoletes",
		"",	
		""
		}},
	{
 	 	1091, RPM_STRING_TYPE, -1, -1, 1,
		{
		"RPMTAG_VERIFYSCRIPTPROG",  
		"-d -l control_file -a interpreter verify",
		"",	
		""
		}},
	{
 	 	1092, RPM_STRING_ARRAY_TYPE, -1, -1, 0, /* not apparently used */
		{
		"RPMTAG_TRIGGERSCRIPTPROGS",  
		"",
		"",	
		""
		}},
	{
 	 	1093, RPM_INT16_TYPE, -1, -1,  1,
		{
		"RPMTAG_DOCDIR",  
		"", 
		"",	
		"" 
		}},
	{
 	 	1094, RPM_INT32_TYPE, 1, 0,  1,
		{
		"RPMTAG_COOKIE", 
		"",
		"",	
		""
		}},
	{
 	 	1095, RPM_INT32_TYPE, 1, 1,  0, /* Depricated ???? */
		{
		"RPMTAG_FILEDEVICES",  
		"-d -l file -l control_file -x one_liner='major minor'",
		"",	
		""
		}},
	{
 	 	1096, RPM_INT32_TYPE, 1, 1, 0, /* does not appear widely used */
		{
		"RPMTAG_FILEINODES",  
		"",
		"",	
		""
		}},
	{
 	 	1097,  RPM_STRING_ARRAY_TYPE, 1, 1, 0,
		{
		"RPMTAG_FILELANGS",  
		"",
		"",	
		""
		}},
	{
 	 	1098,  RPM_STRING_ARRAY_TYPE, -1, -1, 0,
		{
		"RPMTAG_PREFIXES",  
		"",
		"",
		""
		}},
	{
		1099,  RPM_STRING_ARRAY_TYPE, -1, -1, 0,
		{
		"RPMTAG_TRIGGERRUN", 
		"",
		"",	
		""
		}},
	{
 	 	1100,  RPM_STRING_ARRAY_TYPE, 0, -1,  0, /* Internal */
		{
		"RPMTAG_TRIGGERRIN",  
		"",
		"",	
		""
		}},
	{
 	 	1101,  RPM_STRING_ARRAY_TYPE, 0, -1,  0, /* Internal */
		{
		"RPMTAG_TRIGGERPOSTUN",  
		"",
		"",	
		""
		}},
	{
 	 	1102,  RPM_STRING_TYPE, 0, -1,  1, /* Internal */
		{
		"RPMTAG_TRIGGERPOSTUNl",  
		"",
		"",	
		""
		}},
	{
 	 	1103,  RPM_STRING_TYPE, 0, -1,  1, /* Internal */
		{
		"RPMTAG_AUTOREQ",
		"",
		"",	
		""
		}},
	{
 	 	1104,  RPM_STRING_ARRAY_TYPE, 0, -1,  0, /* Internal */
		{
		"RPMTAG_AUTOPROV",
		"",
		"",	
		""
		}},
	{
 	 	1105,  RPM_STRING_ARRAY_TYPE, 0, -1,  0, /* Internal */
		{
		"RPMTAG_CAPABILITY",
		"",
		"",	
		""
		}},
	{
 	 	1106,  RPM_STRING_ARRAY_TYPE, 0, -1,  0, /* Internal */
		{
		"RPMTAG_SOURCEPACKAGE",
		"",
		"",	
		""
		}},
	{
 	 	1107,  RPM_STRING_ARRAY_TYPE, 0, -1,  0, /* Internal */
		{
		"RPMTAG_OLDORIGFILENAMES",
		"",
		"",	
		""
		}},
	{
 	 	1108,  RPM_STRING_ARRAY_TYPE, 0, -1,  0, /* Internal */
		{
		"RPMTAG_BUILDPREREQ",
		"",
		"",	
		""
		}},
	{
 	 	1109,  RPM_STRING_ARRAY_TYPE, 0, -1,  0, /* Internal */ /* Last tag of rpm-3.0 Circa 1999-04-14 */
		{
		"RPMTAG_BUILDREQUIRES",
		"",
		"",	
		""
		}},
	{
 	 	1110,  RPM_STRING_ARRAY_TYPE, 0, -1,  0, /* Internal */
		{
		"RPMTAG_BUILDCONFLICTS",
		"",
		"",	
		""
		}},
	{
 	 	1111,  RPM_STRING_ARRAY_TYPE, 0, -1,  0, /* Internal */
		{
		"RPMTAG_BUILDMACROS",
		"",
		"",	
		""
		}},
	{
 	 	1112,  RPM_INT32_TYPE, 1, 1,  0,   /* Fixme TYPE ??? */
		{
		"RPMTAG_PROVIDEFLAGS",
		"",
		"",	
		""
		}},
	{
 	 	1113,  RPM_STRING_ARRAY_TYPE, 1, 1,  0,   /* Fixme TYPE ??? */
		{
		"RPMTAG_PROVIDEVERSION",
		"",
		"",	
		""
		}},
	{
 	 	1114,  RPM_INT32_TYPE, 1, 0,  1,
    		{
		"RPMTAG_OBSOLETEFLAGS",
		"",
		"",	
		""
		}},
	{
 	 	1115,  RPM_STRING_TYPE, 1, 0,  0,  /* Fixme TYPE ??? */
    		{
		"RPMTAG_OBSOLETEVERSION",
		"",
		"",	
		""
		}},
	{
 	 	1116,  RPM_INT32_TYPE, 1, 0, 0,
		{
		"RPMTAG_DIRINDEXES",  
		"",
		"",	
		""
		}},
	{
 	 	1117,  RPM_STRING_ARRAY_TYPE, 1, 0, 0,
		{
		"RPMTAG_BASENAMES",  
		"",
		"",	
		""
		}},
	{
 	 	1118,  RPM_STRING_ARRAY_TYPE, 1, 0, 0,
		{
		"RPMTAG_DIRNAMES",
		"",
		"",	
		""
		}},
	{
 	 	1119,  RPM_STRING_ARRAY_TYPE, -1, -1, 0,
		{
		"RPMTAG_ORIGDIRINDEXES",
		"",
		"",	
		""
		}},
	{
 	 	1120,  RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_ORIGBASENAMES",
		"",
		"",	
		""
		}},
	{
 	 	1121,  RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_ORIGDIRNAMES",
		"",
		"",	
		""
		}},
	{
 	 	1122,  RPM_STRING_TYPE, 1, 0, 1,
		{
		"RPMTAG_OPTFLAGS",
		"",
		"",	
		""
		}},
	{
 	 	1123,  RPM_STRING_TYPE, 0, -1, 1,
		{
		"RPMTAG_DISTURL",
		"",
		"",	
		""
		}},
	{
 	 	1124,  RPM_STRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_PAYLOADFORMAT",
		"",
		"",	
		""
		}},
	{
 	 	1125,  RPM_STRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_PAYLOADCOMPRESSOR",
		"",
		"",	
		""
		}},
	{
 	 	1126,  RPM_STRING_TYPE, 1, 1, 1,
		{
		"RPMTAG_PAYLOADFLAGS",
		"",
		"",	
		""
		}},
	{
	   /* 1127 is the first free tag of rpm-3.0.5 */
	   /* 1127 is called RPMTAG_MULTILIBS in rpm-4.0.2 */
	   /* 1127 is called RPMTAG_INSTALLCOLOR in rpm-4.4.1 */
 	 	1127,  RPM_STRING_TYPE, -1, -1, 0,
		{
		"RPMTAG_MULTILIBS",
		"",
		"",	
		""
		}},
	{
 	 	1128,  RPM_STRING_TYPE, 0, -1, 1,
		{
		"RPMTAG_INSTALLTID",
		"",
		"",	
		""
		}},
	{
 	 	1129,  RPM_STRING_TYPE, 0, -1, 1,
		{
		"RPMTAG_REMOVETID",
		"",
		"",	
		""
		}},
	{
 	 	1130,  RPM_STRING_TYPE, 0, -1, 1,
		{
		"RPMTAG_SHA1RHN",		/* Internal Obsolete */
		"",
		"",	
		""
		}},
	{
 	 	1131,  RPM_STRING_TYPE, 1, 0, 1,  /* Deprecated by LSB */
		{
		"RPMTAG_RHNPLATFORM",
		"",
		"",	
		""
		}},
	{
 	 	1132,  RPM_STRING_TYPE, 1, 0, 1,     /* check type */
		{
		"RPMTAG_PLATFORM",
		"",
		"",	
		""
		}},
	{
 	 	1133,  RPM_STRING_ARRAY_TYPE, 0, -1, 0, /* in rpm-4.1 designated as: !< placeholder (SuSE) */
		{
		"RPMTAG_PATCHESNAME",        
		"",
		"",	
		""
		}},
	{
 	 	1134,  RPM_STRING_ARRAY_TYPE, 0, -1, 0, /* in rpm-4.1 designated as: !< placeholder (SuSE) */
		{
		"RPMTAG_PATCHESFLAGS",
		"",
		"",	
		""
		}},
	{
 	 	1135,  RPM_STRING_ARRAY_TYPE, 0, -1, 0, /* in rpm-4.1 designated as: !< placeholder (SuSE) */
		{
		"RPMTAG_PATCHESVERSION",
		"",
		"",	
		""
		}},
	{
 	 	1136,  RPM_INT32_TYPE, 0, -1, 1,
		{
		"RPMTAG_CACHECTIME",
		"",
		"",
		""
		}},
	{
 	 	1137,  RPM_STRING_TYPE, 0, -1, 1,
		{
		"RPMTAG_CACHEPKGPATH",
		"",
		"",	
		""
		}},
	{
 	 	1138,  RPM_INT32_TYPE, 0, -1, 1,
		{
		"RPMTAG_CACHEPKGSIZE",
		"",
		"",	
		""
		}},
	{
 	 	1139,  RPM_STRING_TYPE, 0, -1, 1,
		/* Last tag of rpm-4.1 Circa. 2002-09-03 */
		{
		"RPMTAG_CACHEPKGMTIME",
		"",
		"",	
		""
		}},
	{
 	 	1140,  RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_FILECOLORS", 	/* Redhat rpm 4.0 */
		"",
		"",	
		""
		}},
	{
 	 	1141,  RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_FILECLASS",  
		"",
		"",	
		""
		}},
	{
 	 	1142,  RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_FILECLASS",  
		"",
		"",	
		""
		}},
	{
 	 	1143,  RPM_INT32_TYPE, 0, -1, 1,
		{
		"RPMTAG_FILEDEPENDSX",  
		"",
		"",	
		""
		}},
	{
 	 	1144,  RPM_INT32_TYPE, 0, -1, 1,
		{
		"RPMTAG_FILEDEPENDSN",  
		"",
		"",	
		""
		}},
	{
 	 	1145,  RPM_INT32_TYPE, 0, -1, 1,
		{
		"RPMTAG_DEPENDSDICT",  
		"",
		"",	
		""
		}},
	{
 	 	1146,  RPM_STRING_ARRAY_TYPE, 0, -1, 1,
		{
		"RPMTAG_SOURCEPKGID",  
		"",
		"",
		""
		}},
	{ 
		1147 , RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_FILECONTEXTS", 
		"",
		"", 
		""
		}},
	{ 
		1148 , RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_FSCONTEXT",
		"", 
		"", 
		""
		}},
	{ 
		1149 , RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_RECONTEXTS",	
		"", 
		"", 
		""
		}},
	{ 
		1150 , RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_POLICIES", 
		"", 
		"", 
		""
		}},
	{ 
		1151 , RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_PRETRANS", 
		"", 
		"", 
		""
		}},
	{ 
		1152 , RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_POSTTRANS", 
		"", 
		"", 
		""
		}},
	{ 
		1153 , RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_PRETRANSPROG", 
		"", 
		"", 
		""
		}},
	{ 
		1154 , RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_POSTTRANSPROG", 
		"", 
		"", 
		""
		}},
	{ 
		1155 , RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_DISTTAG", 
		"", 
		"", 
		""
		}},
	{ 
		1156, RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_SUGGESTSNAME", 
		"", 
		"", 
		""
		}},
	{ 
		1157, RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_SUGGESTSVERSION", 
		"", 
		"", 
		""
		}},
	{ 
		1158, RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_SUGGESTSFLAGS", 
		"", 
		"", 
		""
		}},
	{ 
		1159, RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_ENHANCESNAME", 
		"", 
		"", 
		""
		}},
	{ 
		1160, RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_ENHANCESVERSION", 
		"", 
		"", 
		""
		}},
	{ 
		1161, RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_ENHANCESFLAGS", 
		"", 
		"", 
		""
		}},
	{ 
		1162, RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_PRIORITY",
		"", 
		"", 
		""
		}},
	{ 
		1163, RPM_STRING_ARRAY_TYPE, 0, -1, 0,
		{
		"RPMTAG_PRIORITY",
		(char*)NULL, 
		"", 
		""
		}},
	{ 
		0, (int)0, -1, -1, 0,
		{
		(char *)NULL, 
		(char *)NULL, 
		(char *)NULL, 
		(char *)NULL
		}}
	};
/* end of list */

