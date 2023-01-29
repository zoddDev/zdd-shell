#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>

#include "./utils/utils.h"
#include "shell.h"

Shell *ptrShell;

Shell *Shell_Construct()
{
	Shell *shell = (Shell *)malloc(sizeof(struct Shell));
	shell->log = 0;
	Shell_DisableLog(shell);

	shell->background = 0;

	shell->jobList = NULL;
	// shell->jobList = new_job(0, "jobs", BACKGROUND);
	shell->jobList = new_list("jobs");
	shell->jobList->next = NULL;

	strcpy(shell->homedir, "/home/");
	const char *username = Utils_GetCurrentUser()->pw_name;
	strcat(shell->homedir, username);

	ignore_terminal_signals();

	// Associate SIGCHLD signal with sigchldHandler function
	ptrShell = shell;
	signal(SIGCHLD, Shell_SigchldHandler);

	return shell;
}

void Shell_Run(Shell *shell)
{
	while (1)
	{
		Shell_PrintPrompt(shell);
		fflush(stdout);
		get_command(shell->inputBuffer, MAX_LINE, shell->args, &(shell->background)); /* get next command */

		// ------------------------------- Internal command -------------------------------

		if (Shell_InternalCommand(shell))
			continue;

		// ------------------------------------ Command -----------------------------------

		if (!(shell->pid_fork = fork())) // Child process ----------------------------------
		{
			new_process_group(getpid());

			// Si el proceso es foreground, se le cede el terminal
			if (!shell->background)
				set_terminal(getpid());

			restore_terminal_signals();

			int error = execvp(shell->inputBuffer, shell->args);
			if (error)
			{
				printf("\033[1;31m<ERROR> Unknown command: \"%s\"\n", shell->args[0]);
				exit(-1);
			}
		}
		else // Parent --------------------------------------------------------------------
		{
			if (!shell->background)
			{
				// Si el proceso hijo es foreground, se le espera (wait)
				Shell_DefaultWait(shell, shell->pid_fork, shell->args[0]);
			}
			else
			{
				// Introducir background job en la lista
				block_SIGCHLD();
				if (shell->jobList != NULL)
				{
					job *j = new_job(shell->pid_fork, shell->args[0], BACKGROUND);
					add_job(shell->jobList, j);
				}
				unblock_SIGCHLD();
				printf("\033[1;32m[1] <\033[1;33mBackground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;33mRUNNING\033[1;32m, status: \033[1;33m%d\033[1;32m\n", shell->pid_fork, shell->args[0], WSTOPSIG(shell->status));
			}
		}
	} // end while
}

int Shell_InternalCommand(Shell *shell)
{
	int isInternalCommand = 1;

	if (shell->args[0] == NULL)
	{
		// empty command
	}
	else if (!strcmp(shell->args[0], "cls"))
	{
		INTERNAL_CLEAR(shell);
	}
	else if (!strcmp(shell->args[0], "l"))
	{
		INTERNAL_L(shell);
	}
	else if (!strcmp(shell->args[0], "ll"))
	{
		INTERNAL_LL(shell);
	}
	else if (!strcmp(shell->args[0], "exit"))
	{
		INTERNAL_EXIT(shell);
	}
	else if (!strcmp(shell->args[0], "cd"))
	{
		INTERNAL_CD(shell);
	}
	else if (!strcmp(shell->args[0], "jobs"))
	{
		INTERNAL_JOBS(shell);
	}
	else if (!strcmp(shell->args[0], "fg"))
	{
		INTERNAL_FG(shell);
	}
	else if (!strcmp(shell->args[0], "bg"))
	{
		INTERNAL_BG(shell);
	}
	else
	{
		isInternalCommand = 0;
	}

	// No internal command
	return isInternalCommand;
}

void Shell_SigchldHandler(int signal)
{
	Shell *shell = ptrShell;
	block_SIGCHLD();
	job *ptr = shell->jobList->next;

	while (ptr != NULL)
	{
		int pid = ptr->pgid;
		int status;

		if (pid == waitpid(pid, &status, WUNTRACED | WNOHANG | WCONTINUED))
		{
			int printed = 0;

			// Imprimir mensaje de salida/suspensión
			if (shell->log)
				Shell_PrintExitMsg(shell, ptr->command, &printed);
			if (printed)
				Shell_PrintPrompt(shell);

			// Ha ocurrido algo con el	 trabajo
			if (WIFSTOPPED(status))
				status = SUSPENDED;
			else if (WIFEXITED(status))
				status = EXITED;
			else if (WIFCONTINUED(status))
				status = CONTINUED;

			switch (status)
			{
			case SUSPENDED:
				ptr->state = STOPPED;
				break;
			case SIGNALED:
				delete_job(shell->jobList, ptr);
				break;
			case EXITED:
				delete_job(shell->jobList, ptr);
				break;
			case CONTINUED:
				ptr->state = BACKGROUND;
				break;
			default:
				ptr->state = BACKGROUND;
				break;
			}
		}

		ptr = ptr->next;
	}

	unblock_SIGCHLD();
}

job *Shell_GetJob(Shell *shell, int index)
{
	job *ptr = shell->jobList->next;

	int i = 1;
	while (ptr != NULL)
	{
		if (i == index)
			return ptr;

		ptr = ptr->next;
		i++;
	}

	return NULL;
}

void Shell_DefaultWait(Shell *shell, pid_t pid, const char *command)
{
	int *info = &shell->info;

	waitpid(pid, &(shell->status), WUNTRACED);
	set_terminal(getpid());

	// Comprobar si el hijo ha sido suspendido (p.ej. suspender un proceso en primer plano con Ctrl+Z)
	// Si fue suspendido, se añade a jobs
	if (analyze_status(shell->status, info) == SUSPENDED && shell->jobList != NULL)
	{

		job *j = new_job(pid, command, STOPPED);
		add_job(shell->jobList, j);
	}

	int printed = 0;
	if (shell->log)
		Shell_PrintExitMsg(shell, command, &printed);
}

void Shell_PrintExitMsg(Shell *shell, const char *command, int *printed)
{
	int status = shell->status;
	if (strcmp(command, "clear"))
	{
		printf("\033[1;32m");
		if (WIFEXITED(status)) // el proceso ha terminado con un exit()
		{
			printf("<\033[1;33mForeground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;31mEXITED\033[1;32m, status: \033[1;33m%d\033[1;32m\n", shell->pid_fork, command, WEXITSTATUS(status));
			*printed = 1;
		}
		else if (WIFSIGNALED(status)) // el proceso ha terminado por la recepción de una señal
		{
			printf("<\033[1;33mForeground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;31mSIGNALED\033[1;32m, status: \033[1;33m%d\033[1;32m\n", shell->pid_fork, command, WTERMSIG(status));
			*printed = 1;
		}
		else if (WIFSTOPPED(status)) // el proceso se ha parado por la recepción de una señal
		{
			printf("<\033[1;33mForeground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;31mSTOPPED\033[1;32m, status: \033[1;33m%d\033[1;32m\n", shell->pid_fork, command, WSTOPSIG(status));
			*printed = 1;
		}
		printf("\033[0;0m");
	}
}

void Shell_PrintPrompt(Shell *shell)
{
	Prompt_Print();
}

void Shell_EnableLog(Shell *shell)
{
	shell->log = 1;
}

void Shell_DisableLog(Shell *shell)
{
	shell->log = 0;
}

void Shell_Destruct(Shell **shell)
{
	free((*shell)->inputBuffer);
	free((*shell)->jobList);
	free(*shell);
	shell = NULL;
}