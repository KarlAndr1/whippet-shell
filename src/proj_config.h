
#define PROJ_NAME "WhippetShell"

#ifndef VERSION_NAME
	#define VERSION_NAME "STANDARD EDITION"
#endif

#ifndef VERSION_CODE
	#define VERSION_CODE "0.0.4"
#endif

#ifndef SETTING_APPROVE_COMMANDS
	#ifdef DEBUG
		#define SETTING_APPROVE_COMMANDS 1
	#else
		#define SETTING_APPROVE_COMMANDS 0
	#endif
#endif

#ifndef SETTING_RICH_TERMINAL
	#define SETTING_RICH_TERMINAL 1
#endif


