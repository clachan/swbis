/* File: debug_config.h */
/*
*  You may edit this file to control debugging output on stderr.
*/


/* 
 * To turn all debugging off:  define DEBUG_CONTROL_ALL_OFF 
 */

/* #undef DEBUG_CONTROL_ALL_OFF */
/* #define DEBUG_CONTROL_ALL_OFF 1 */

#define DEBUG_CONTROL_ALL_OFF 1
#undef  DEBUG_CONTROL_ALL_OFF 

#ifdef DEBUG_CONTROL_ALL_OFF

#undef UINFILENEEDDEBUG
#undef AHSNEEDDEBUG
#undef UXFIONEEDDEBUG
#undef SWVARFSNEEDDEBUG
#undef XFORMATNEEDDEBUG
#undef TARUNEEDDEBUG
#undef TARNEEDDEBUG
#undef SWBISNEEDDEBUG
#undef SWHEADERNEEDDEBUG
#undef SWITERNEEDDEBUG
#undef SWPATHNEEDDEBUG
#undef SWSWINEEDDEBUG
#undef TARUIBNEEDDEBUG
#undef HLLISTNEEDDEBUG
#undef SWEXTDEFNEEDDEBUG
#undef SWPACKAGEFILENEEDDEBUG
#undef SWMETADATANEEDDEBUG
#undef SWEXFILESETNEEDDEBUG
#undef SWDEFINITIONNEEDDEBUG
#undef SWINSTALLNEEDDEBUG
#else

/*
*  swinstall
* #undef SWINSTALLNEEDDEBUG
* #define SWINSTALLNEEDDEBUG 1
*/
#define SWINSTALLNEEDDEBUG 1
#undef SWINSTALLNEEDDEBUG

/*
*  The package opening and identification routines.
* #undef UINFILENEEDDEBUG
* #define UINFILENEEDDEBUG 1
*/
#define UINFILENEEDDEBUG 1
#undef UINFILENEEDDEBUG

/*
*  The archive header accessor module.
* #undef AHSNEEDDEBUG
* #define AHSNEEDDEBUG 1
*/
#undef AHSNEEDDEBUG

/*
*  The hard link list module.
* #undef HLLISTNEEDDEBUG
* #define HLLISTNEEDDEBUG 1
*/
#undef HLLISTNEEDDEBUG

/*
*  The Unix-API buffered file I/O module.
* #undef UXFIONEEDDEBUG
* #define UXFIONEEDDEBUG 1
*/
#define UXFIONEEDFAIL 1   /* Normal to be always on. */
#define UXFIONEEDDEBUG 1
#undef UXFIONEEDDEBUG

/*
*  The virtual archive file system module.
* #undef SWVARFSNEEDDEBUG
* #define SWVARFSNEEDDEBUG 1
*/
#define SWVARFSNEEDDEBUG 1
#undef SWVARFSNEEDDEBUG

/*
*  The tar/cpio archive library.
* #undef XFORMATNEEDDEBUG
* #define XFORMATNEEDDEBUG 1
*/
#define XFORMATNEEDDEBUG 1
#undef XFORMATNEEDDEBUG

/*
*  The tar/cpio archive low-level routines.
* #undef TARUNEEDDEBUG
* #define TARUNEEDDEBUG 1
*/
#define TARUNEEDDEBUG 1
#undef TARUNEEDDEBUG

/*
*  The tar header routines.
* #undef TARNEEDDEBUG
* #define TARNEEDDEBUG 1
*/
#define TARNEEDDEBUG 1
#undef TARNEEDDEBUG

/*
*  The swpath decoder object 
* #undef SWPATHNEEDDEBUG
* #define SWPATHNEEDDEBUG 1
*/
#define SWPATHNEEDDEBUG 1
#undef SWPATHNEEDDEBUG

/*
*  The swheader accessor module in swsupplib/misc/swheader.c
* #undef SWHEADERNEEDDEBUG
* #define SWHEADERNEEDDEBUG 1
*/
#define SWHEADERNEEDDEBUG 1
#undef SWHEADERNEEDDEBUG

/*
*  The swi decoder module in swprogs/.
* #define SWSWINEEDDEBUG 1
* #undef SWSWINEEDDEBUG
*/
#define SWSWINEEDDEBUG 1
#undef SWSWINEEDDEBUG

/*
*  The taruib module 
*/
#define TARUIBNEEDDEBUG 1
#undef  TARUIBNEEDDEBUG

/*
*  Everything else
* #undef SWBISNEEDDEBUG
* #define SWBISNEEDDEBUG 1
*/
#define SWBISNEEDDEBUG 1
#undef SWBISNEEDDEBUG

/*
*  The C++ swstruct collection Iterator.
* #undef SWITERNEEDDEBUG
* #define SWITERNEEDDEBUG 1
*/
#define SWITERNEEDDEBUG 1
#undef SWITERNEEDDEBUG

/*
*  The C++ Extended Definition Object used by swpackage
* #undef SWEXTDEFNEEDDEBUG
* #define SWEXTDEFNEEDDEBUG 1
*/
#define SWEXTDEFNEEDDEBUG 1
#undef SWEXTDEFNEEDDEBUG

/*
*  The C++ Swpackagefile Object used by swpackage
* #undef SWPACKAGEFILENEEDDEBUG
* #define SWPACKAGEFILENEEDDEBUG 1
*/
#define SWPACKAGEFILENEEDDEBUG 1
#undef SWPACKAGEFILENEEDDEBUG

/*
*  The C++ swMetaData Object used by swpackage
* #undef SWMETADATANEEDDEBUG
* #define SWMETADATANEEDDEBUG 1
*/
#define SWMETADATANEEDDEBUG 1
#undef SWMETADATANEEDDEBUG

/*
*  The C++ swExFileset object used by swpackage
* #undef SWEXFILESETNEEDDEBUG
* #define SWEXFILESETNEEDDEBUG 1
*/
#define SWEXFILESETNEEDDEBUG 1
#undef SWEXFILESETNEEDDEBUG

/*
*  The C++ swDefinition object used by swpackage: swdefinition.h
* #undef SWDEFINITIONNEEDDEBUG
* #define SWDEFINITIONNEEDDEBUG 1
*/
#define SWDEFINITIONNEEDDEBUG 1
#undef SWDEFINITIONNEEDDEBUG

#endif
/* end of file */
