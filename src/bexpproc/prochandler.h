/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCprochandlerh
#define INCprochandlerh

#include <sys/types.h>
#include <unistd.h>

#define PROC_RECORD_SIZE 8

#define EXPPROC 0
#define B12PROC 1
#define PROCPROC 2
#define AUTOPROC 3
#define INFOPROC 4
#define ATPROC 5
#define STATPROC 6
#define LASTPROC STATPROC

/* Same order as defined by EXPPROC, RECVPROC, SENDPROC, PROCPROC, ROBOPROC, etc... */
#define EXP_RECORD { "Expproc", "/acqbin/Expproc", (pid_t) -1,    1 }
#define B12_RECORD { "B12proc", "/acqbin/B12proc", (pid_t) -1, 0 }
#define PROC_RECORD { "Procproc", "/acqbin/Procproc", (pid_t) -1, 1 }
#define AUTO_RECORD { "Autoproc", "/acqbin/Autoproc", (pid_t) -1, 0 }
#define INFO_RECORD { "Infoproc", "/acqbin/Infoproc", (pid_t) -1, 1 }
#define AT_RECORD   { "Atproc",   "/acqbin/Atproc",   (pid_t) -1, 1 }
#define STAT_RECORD { "Infostat", "/bin/Infostat", (pid_t) -1, 1 }
#define NULL_RECORD { " ", " ", -1, 0 }

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*
*  record structure for process with msgQs that this process
*  may wish to talk to.
*/
typedef struct _procrec {
			char *procName;
			char *procPath;
			pid_t taskPid;
                        int   autostart;
			}  TASK_RECORD;



#ifdef __cplusplus
}
#endif

#endif
