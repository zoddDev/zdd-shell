#include <stdio.h>
#include <stdlib.h>

#include "job_control.h"
#include "shell.h"

Shell *ptrShell;

void Shell_SigchldHandler(int signal);

void Shell_Construct(Shell *shell)
{
    shell = (Shell*) malloc(sizeof(struct Shell));

    shell->log = 0;
    Shell_DisableLog(shell);

	shell->background = 0;

    shell->jobList = NULL;
    shell->jobList = new_job(0, "jobs", BACKGROUND);
	shell->jobList->next = NULL;

    // strcpy(shell->homedir, "/home/");
	
	shell->homedir = (char*) malloc(64 * sizeof(char));
	strcat(shell->homedir, "/home/");

	strcat(shell->homedir, getlogin());

	ignore_terminal_signals();

	// Associate SIGCHLD signal with sigchldHandler function
    ptrShell = shell;
	signal(SIGCHLD, Shell_SigchldHandler);
}

void Shell_Run(Shell *shell)
{
	printf("STATEEEE: %c\n", ptrShell->args);

	while (1)
	{   		
		Shell_PrintPrompt(shell);
		int *background = &shell->background;
		fflush(stdout);
		get_command(shell->inputBuffer, MAX_LINE, shell->args, background);  /* get next command */
		
		// ------------------------------- Internal command -------------------------------

        if (Shell_InternalCommand(shell)) continue;
		
        // ------------------------------- Command -------------------------------
		
        if(!(shell->pid_fork = fork())) // Child process
        { 
			new_process_group(getpid());
			
			// Si el proceso es foreground, se le cede el terminal
			if(!shell->background) 
				set_terminal(getpid());

			restore_terminal_signals();

			int error = execvp(shell->inputBuffer, shell->args);
			if(error) 
            {
				printf("\033[1;31m<ERROR> No se reconoce el comando: \"%s\"\n", shell->args[0]);
				exit(-1);
			}
		} 
        else // Parent 
        {
			if(!shell->background) 
            {
				// Si el proceso hijo es foreground, se le espera (wait)
				Shell_DefaultWait(shell, shell->pid_fork, shell->args[0]);
			} 
            else
            {
				// Introducir background job en la lista
				block_SIGCHLD();
				if(shell->jobList != NULL) 
                {
					job *j = new_job(shell->pid_fork, shell->args[0], BACKGROUND);
					add_job(shell->jobList, j);
				}
				unblock_SIGCHLD();
				printf("\033[1;32m[1] <\033[1;33mBackground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;33mRUNNING\033[1;32m, status: \033[1;33m%d\033[1;32m\n", shell->pid_fork, shell->args[0], WSTOPSIG(*(shell->status)));
			}
		}
	} // end while
}

int Shell_InternalCommand(Shell *shell)
{
    if(shell->args[0]==NULL) return 1;   // if empty command

    if(!strcmp(shell->args[0], "exit"))
    {
        exit(0);
    }
    else if(!strcmp(shell->args[0], "cd"))
    {
        char *dir = shell->args[1] == NULL || !strcmp(shell->args[1], "~") ? shell->homedir : shell->args[1];
        if(chdir(dir)) printf("\033[1;31m<ERROR> no directory named: %s\n", dir);
        return 1;
    } 
    else if(!strcmp(shell->args[0], "jobs")) 
    {
        block_SIGCHLD();
        printf("\033[1;36m");

        print_job_list(shell->jobList);
        
        printf("\033[0;0m");
        unblock_SIGCHLD();
        return 1;
    } 
    else if(!strcmp(shell->args[0], "fg"))
    {
        block_SIGCHLD();

        int index = shell->args[1] == NULL ? 1 : atoi(shell->args[1]);
        // printf("arg: %d\n", index);

        job *j = Shell_GetJob(shell, index);
        if(j != NULL) 
        {
            printf("<INFO> Mandando tarea %d a primer plano...\n", index);
            set_terminal(j->pgid);
            // j->state = FOREGROUND;
            killpg(j->pgid, SIGCONT);
            Shell_DefaultWait(shell, j->pgid, j->command);
            delete_job(shell->jobList, j);
        } else printf("\033[1;31m<ERROR> job [%d] does not exist\n", index);
        unblock_SIGCHLD();

        return 1;
    } 
    else if(!strcmp(shell->args[0], "bg")) 
    {
        block_SIGCHLD();
        int index = shell->args[1] == NULL ? 1 : atoi(shell->args[1]);
        job *j = Shell_GetJob(shell, index);
        if(j != NULL) 
        {
            printf("<INFO> Reanudando tarea \"%s\"...\n", j->command);
            j->state = BACKGROUND;
            killpg(j->pgid, SIGCONT);
        } else printf("\033[1;31m<ERROR> job [%d] does not exist\n", index);

        unblock_SIGCHLD();
        return 1;
    }

    // No internal command
    return 0;
}

void Shell_SigchldHandler(int signal)
{
    printf("HANDLING SIGNAL: %d\n", signal);
    Shell *shell = ptrShell;
	block_SIGCHLD();
	job *ptr = shell->jobList->next;
	
	while(ptr != NULL) {
		int pid = ptr->pgid;
		int status;

		if(pid == waitpid(pid, &status, WUNTRACED | WNOHANG | WCONTINUED)) {
			int printed = 0;
			// Imprimir mensaje de salida/suspensión
			if (shell->log) Shell_PrintExitMsg(shell, ptr->command, &printed);
			if(printed) Shell_PrintPrompt(shell);
			
			// Ha ocurrido algo con el	 trabajo
			if(WIFSTOPPED(status)) status = SUSPENDED;
			else if(WIFEXITED(status)) status = EXITED;
			else if(WIFCONTINUED(status)) status = CONTINUED;
			
			switch (status) {
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
	while(ptr != NULL) {
		if(i == index) return ptr;
		
		ptr = ptr->next;
		i++;
	}

	return NULL;
}

void Shell_DefaultWait(Shell *shell, pid_t pid, const char *command)
{
    int *info = &shell->info;

    waitpid(pid, shell->status, WUNTRACED);
	set_terminal(getpid());

	// Comprobar si el hijo ha sido suspendido (p.ej. suspender un proceso en primer plano con Ctrl+Z)
	// Si fue suspendido, se añade a jobs
	if(analyze_status(*(shell->status), info) == SUSPENDED && shell->jobList != NULL) 
    {
        job *j = new_job(pid, command, STOPPED);
        add_job(shell->jobList, j);
	}

	int printed;
	Shell_PrintExitMsg(shell, command, &printed);
}

void Shell_PrintExitMsg(Shell *shell, const char *command, int *printed)
{
    int status = *(shell->status);
    if(strcmp(command, "clear")) {
		printf("\033[1;32m");
		if(WIFEXITED(status)) { // el proceso ha terminado con un exit()
			printf("<\033[1;33mForeground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;31mEXITED\033[1;32m, status: \033[1;33m%d\033[1;32m\n", shell->pid_fork, command, WEXITSTATUS(status));
			*printed = 1;
		} else if(WIFSIGNALED(status)) { // el proceso ha terminado por la recepción de una señal
			printf("<\033[1;33mForeground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;31mSIGNALED\033[1;32m, status: \033[1;33m%d\033[1;32m\n", shell->pid_fork, command, WTERMSIG(status));
			*printed = 1;
		} else if(WIFSTOPPED(status)) { // el proceso se ha parado por la recepción de una señal
			printf("<\033[1;33mForeground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;31mSTOPPED\033[1;32m, status: \033[1;33m%d\033[1;32m\n", shell->pid_fork, command, WSTOPSIG(status));
			*printed = 1;
		}
		printf("\033[0;0m");
	}
}

void Shell_PrintPrompt(Shell *shell)
{
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    // Shell prompt
    printf("\033[1;34m");
    printf("\033[1;32m[\033[1;34m%s@\033[1;34m%s\033[1;32m]", getlogin(), "unix");
    printf("\033[1;32m-[\033[1;34m%s\033[1;32m]", cwd);

    printf("$ ");
    printf("\033[0;0m");
}

void Shell_EnableLog(Shell *shell)
{
    shell->log = 1;
}

void Shell_DisableLog(Shell *shell)
{
    shell->log = 0;
}
