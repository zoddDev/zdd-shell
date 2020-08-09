#ifndef SHELL_H_
#define SHELL_H_

#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "job_control.h"
#include "prompt/prompt.h"

// --------------------------------------- MACROS ---------------------------------------

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

#define ERROR_COLOR RED_BOLD /* default color for error output */


// ------------------------------------ Shell struct ------------------------------------

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

    job *jobList; 				/* Background jobs list */

    int log;					/* if equals 1, exit messages are printed */
};


// ------------------------------------- FUNCTIONS -------------------------------------

Shell *Shell_Construct(); // returns a fully constructed shell instance

void Shell_Run(Shell *shell); // starts shell program

int Shell_InternalCommand(Shell *shell); // checks if given command corresponds to an internal (built-in) command, if so, executes it

void Shell_SigchldHandler(int signal); // handles process signals

job *Shell_GetJob(Shell *shell, int index); // returns job from jobList given a specific index 

void Shell_DefaultWait(Shell *shell, pid_t pid, const char *command); // waits for a specific PID to be finished

void Shell_PrintExitMsg(Shell *shell, const char *command, int *printed); // prints exit message if log is enabled

void Shell_PrintPrompt(Shell *shell); // prompt printing

void Shell_EnableLog(Shell *shell); // enables info output when executing commands

void Shell_DisableLog(Shell *shell); // disables info output when executing commands

void Shell_Destruct(Shell **shell); // frees memory corresponding to the shell

#include "internal_commands/internal_commands.h"
#endif