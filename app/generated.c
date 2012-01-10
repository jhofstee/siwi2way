// Principal ADL header include
#include "adl_global.h"
// List of embedded components
const char mos_headerSEList[] =
"Developer Studio\0" "2.1.0.201108081252-R6801\0"
"Open AT Embedded Software Suite package\0" "2.36.0.201108300638\0"
"Open AT OS Package\0" "6.36.0.201108111228\0"
"Firmware Package\0" "7.46.0.201108091301\0"
"WIP Plug-in Package\0" "5.42.0.201108100923\0"
"\0";

#if __OAT_API_VERSION__ >= 636
// Application debug/release mode tag (only supported from Open AT OS 6.36)
#ifdef DS_DEBUG
const adl_CompilationMode_e adl_CompilationMode = ADL_COMPILATION_MODE_DEBUG;
#else
const adl_CompilationMode_e adl_CompilationMode = ADL_COMPILATION_MODE_RELEASE;
#endif
#endif

// Application name definition
const ascii adl_InitApplicationName[] = "siwi2way";

// Company name definition
const ascii adl_InitCompanyName[] = "Victron Energy";

// Application version definition
const ascii adl_InitApplicationVersion[] = "1.0.0";

// Low level handlers execution context call stack size
const u32 adl_InitIRQLowLevelStackSize = 4096;

// High level handlers execution context call stack size
const u32 adl_InitIRQHighLevelStackSize = 4096;

// Application tasks prototypes
extern void main_task ( void );



// Application tasks declaration table
const adl_InitTasks_t adl_InitTasks [] =
{
        { main_task,  3072, "main", 1 },

    { 0, 0, 0, 0 }
};
