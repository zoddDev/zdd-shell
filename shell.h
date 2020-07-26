#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "job_control.h"

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

typedef struct Shell Shell;
struct Shell
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	
	pid_t pid_fork; 			/* pid for created and waited process */
	int status;             	/* status returned by wait */
	enum status status_res; 	/* status processed by analyze_status() */
	int info;					/* info processed by analyze_status() */
	char homedir[64];

    job *jobList; // Background jobs list

    int log;
};

Shell *Shell_Construct();

void Shell_Run(Shell *shell);

int Shell_InternalCommand(Shell *shell);

void Shell_SigchldHandler(int signal);

job *Shell_GetJob(Shell *shell, int index);

void Shell_DefaultWait(Shell *shell, pid_t pid, const char *command);

void Shell_PrintExitMsg(Shell *shell, const char *command, int *printed);

void Shell_PrintPrompt(Shell *shell);

void Shell_EnableLog(Shell *shell);

void Shell_DisableLog(Shell *shell);

void Shell_Destruct(Shell **shell);