#ifndef swevents_h_200403
#define swevents_h_200403

#define SWEVENT_VALUE_PREVIEW		"preview"
#define SWEVENT_KW_STATUS		"status"
#define SWEVENT_ATT_DELIM		"="
#define SWEVENT_STATUS_PFX		SWEVENT_KW_STATUS SWEVENT_ATT_DELIM
#define SWEVENT_ATT_STATUS_PREVIEW	SWEVENT_STATUS_PFX SWEVENT_VALUE_PREVIEW

#define SW_ILLEGAL_STATE_TRANSITION 	1
#define SW_BAD_SESSION_CONTEXT 		2
#define SW_ILLEGAL_OPTION 		3
#define SW_ACCESS_DENIED 		4
#define SW_MEMORY_ERROR 		5
#define SW_RESOURCE_ERROR 		6
#define SW_INTERNAL_ERROR 		7
#define SW_IO_ERROR 			8
#define SW_AGENT_INITIALIZATION_FAILED 	10
#define SW_SERVICE_NOT_AVALIABLE 	11
#define SW_OTHER_SESSIONS_IN_PROGRESS 	12
#define SW_SESSION_BEGINS 		28
#define SW_SESSION_ENDS 		29
#define SW_CONNECTION_LIMIT_EXCEEDED 	30
#define SW_SOC_DOES_NOT_EXIST 		31
#define SW_SOC_IS_CORRUPT 		32
#define SW_SOC_CREATED 			34
#define SW_CONFLICTING_SESSION_IN_PROGRESS	35
#define SW_SOC_LOCK_FAILURE		36
#define SW_SOC_IS_READ_ONLY		37
#define SW_SOC_IS_REMOTE		38
#define SW_SOC_IS_SERIAL 		40
#define SW_SOC_INCORRECT_TYPE		41
#define SW_CANNOT_OPEN_LOGFILE		42
#define SW_SOC_AMBIGUOUS_TYPE		49
#define SW_ANALYSIS_BEGINS 		52
#define SW_ANALYSIS_ENDS 		53
#define SW_EXREQUISITE_EXCLUDE		56
#define SW_CHECK_SCRIPT_EXCLUDE 	57
#define SW_CONFIGURE_EXCLUDE 		58
#define SW_SELECTION_IS_CORRUPT		59
#define SW_SOURCE_ACCESS_ERROR 		60
#define SW_SELECTION_NOT_FOUND		62
#define SW_SELECTION_NOT_FOUND_RELATED	63
#define SW_SELECTION_NOT_FOUND_AMBIG	64
#define SW_FILESYSTEMS_NOT_MOUNTED	65
#define SW_FILESYSTEMS_MORE_MOUNTED	66
#define SW_HIGHER_REVISION_INSTALLED 	67
#define SW_NEW_MULTIPLE_VERSION		68
#define SW_EXISTING_MULTIPLE_VERSION	69
#define SW_DEPENDENCY_NOT_MET		70
#define SW_NOT_COMPATIBLE		71
#define SW_CHECK_SCRIPT_WARNING		72
#define SW_CHECK_SCRIPT_ERROR 		73
#define SW_DSA_OVER_LIMIT		75
#define SW_DSA_FAILED_TO_RUN		76
#define SW_SAME_REVISION_INSTALLED	77
#define SW_ALREADY_CONFIGURED		78
#define SW_SKIPPED_GLOBAL_ERROR 	80
#define SW_FILE_WARNING			84
#define SW_FILE_ERROR			85
#define SW_NOT_LOCATABLE		86
#define SW_SAME_REVISION_SKIPPED 	87
#define SW_EXECUTION_BEGINS 		88
#define SW_EXECUTION_ENDS 		89
#define SW_FILESET_ERROR		98
#define SW_PRE_SCRIPT_WARNING		95
#define SW_PRE_SCRIPT_ERROR		96
#define SW_FILESET_WARNING		97
#define SW_FILESET_ERROR		98
#define SW_POST_SCRIPT_WARNING		99
#define SW_POST_SCRIPT_ERROR		100
#define SW_CONFIGURE_WARNING		103
#define SW_CONFIGURE_ERROR		104
#define SW_DATABASE_UPDATE_ERROR	105
#define SW_FILESET_BEGINS		117
#define SW_CONTROL_SCRIPT_BEGINS	118
#define SW_CONTROL_SCRIPT_NOT_FOUND	119

#define SW_SOURCE_ACCESS_BEGINS 	260 /* Not Posix */
#define SW_SOURCE_ACCESS_ENDS 		261 /* Not Posix */
#define SW_CONTROL_SCRIPT_ENDS		262 /* Not Posix */
#define SW_SOC_LOCK_CREATED		263 /* Not Posix */
#define SW_SOC_LOCK_REMOVED		264 /* Not Posix */
#define SW_SELECTION_EXECUTION_BEGINS	265 /* Not Posix */
#define SW_SELECTION_EXECUTION_ENDS	266 /* Not Posix */
#define SW_ISC_INTEGRITY_CONFIRMED 267 /* Not Posix */
#define SW_ISC_INTEGRITY_NOT_CONFIRMED 268 /* Not Posix */
#define SW_SPECIAL_MODE_BEGINS		269 /* Not Posix */
#define SW_SOC_INTEGRITY_CONFIRMED 	270 /* Not Posix */
#define SW_SOC_INTEGRITY_NOT_CONFIRMED	271 /* Not Posix */
#define SW_ABORT_SIGNAL_RECEIVED	272 /* Not Posix */
#define SW_FILE_EXISTS			273 /* Not Posix */

#define SWI_PRODUCT_SCRIPT_ENDS		280 /* Not Posix */
#define SWI_FILESET_SCRIPT_ENDS		281 /* Not Posix */

#define SWI_CATALOG_UNPACK_BEGINS	302 /* Not Posix */
#define SWI_CATALOG_UNPACK_ENDS		303 /* Not Posix */
#define SWI_TASK_BEGINS			304 /* Not Posix */
#define SWI_TASK_ENDS			305 /* Not Posix */
#define SWI_SWICOL_ERROR		306 /* Not Posix */
#define SWI_CATALOG_ANALYSIS_BEGINS	307 /* Not Posix */
#define SWI_CATALOG_ANALYSIS_ENDS	308 /* Not Posix */
#define SWBIS_TARGET_BEGINS		309 /* Not Posix */
#define SWBIS_TARGET_ENDS		310 /* Not Posix */
#define SWI_NORMAL_EXIT			311 /* Not Posix */
#define SWI_SELECTION_BEGINS		312 /* Not Posix */
#define SWI_SELECTION_ENDS		313 /* Not Posix */
#define SWI_MSG				314 /* Not Posix */
#define SWI_ATTRIBUTE			315 /* Not Posix */
#define SWI_GROUP_BEGINS		316 /* Not Posix */
#define SWI_GROUP_ENDS			317 /* Not Posix */
#define SWI_TASK_CTS			318 /* Not Posix  ClearToSend */
#define SWI_MAIN_SCRIPT_ENDS		319 /* Not Posix */

struct swEvents {
		char * codeM;	     /* POSIX event code	*/
		int valueM;	     /* POSIX event value	*/
		int verbose_threshholdM;
		char * msgM;	     /* Message text	   	*/
		int is_swi_eventM;   /* May occur in a swicol_<*> task*/
		int default_statusM; /* Worst status is default */
	};


/* Source events */
#define SEVENT(p_fd, p_vlv, p_code, p_arg) swevent_shell_echo(p_fd, \
					p_vlv, "source", p_code, \
					"noused" /*hostname*/, p_arg)

/* Target events */
#define TEVENT(p_fd, p_vlv, p_code, p_arg) swevent_shell_echo(p_fd, \
					p_vlv, "target", p_code, \
					"noused" /*hostname*/, p_arg)
int swevent_parse_attribute_event(char * line, char ** attribute, char ** value);
int swevent_is_error(char * line, int * statusp);
int swevent_get_value(struct swEvents * e, char * msg);
char * swevent_shell_echo(int fdredir, int verbose_level, char * target, int value, char * host, char * msg);
ssize_t swevent_write_rpsh_event(int fd, char * p, int len);
int swevent_parse_message(char * line, char ** attribute, char ** value);
struct swEvents * swevent_get_events_array(void);
char * swevent_code(int value);
void swevent_set_verbose(int v);
struct swEvents * swevents_get_struct_by_message(char * line, struct swEvents * evnt);
void swevent_s_arr_reset(void);
void swevent_s_arr_delete(void);
#endif
