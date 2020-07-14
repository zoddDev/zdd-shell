/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "job_control.h"   // remember to compile with module job_control.c 

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

// Manejador de la señal SIGCHLD
void sigchldHandler(int signal);

// Funciones auxiliares
job *getJob(int index);
void default_wait(int pid_fork, int *status, int *info, const char *command);
void print_exit_msg(int pid_fork, const char *command, int *status, int *printed);
void print_prompt();

// Lista de trabajos background
job *jobList = NULL;

// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void) {
	jobList = new_job(0, "jobs", BACKGROUND);
	jobList->next = NULL;

	char homedir[39] = "/home/";
	strcat(homedir, getlogin());

	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	
	// probably useful variables:
	pid_t pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */

	ignore_terminal_signals();

	// Asociamos la señal SIGCHLD con el manejador sigchldHandler
	signal(SIGCHLD, sigchldHandler);

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
		print_prompt();

		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		// ------------------------------- Internal commands -------------------------------

		if(args[0]==NULL) continue;   // if empty command

		if(!strcmp(args[0], "exit")) exit(0);
		else if(!strcmp(args[0], "cd")) {
			char *dir = args[1] == NULL || !strcmp(args[1], "~") ? homedir : args[1];
			if(chdir(dir)) printf("\033[1;31m<ERROR> no directory named: %s\n", dir);
			continue;
		} else if(!strcmp(args[0], "jobs")) {
			block_SIGCHLD();
			printf("\033[1;36m");
			
			print_job_list(jobList);
			
			printf("\033[0;0m");
			unblock_SIGCHLD();
			continue;
		} else if(!strcmp(args[0], "fg")) {
			block_SIGCHLD();

			int index = args[1] == NULL ? 1 : atoi(args[1]);
			// printf("arg: %d\n", index);

			job *j = getJob(index);
			if(j != NULL) {
				printf("<INFO> Mandando tarea %d a primer plano...\n", index);
				set_terminal(j->pgid);
				// j->state = FOREGROUND;
				killpg(j->pgid, SIGCONT);
				default_wait(j->pgid, &status, &info, j->command);
				delete_job(jobList, j);
			} else printf("\033[1;31m<ERROR> job [%d] does not exist\n", index);
			unblock_SIGCHLD();
			continue;
		} else if(!strcmp(args[0], "bg")) {
			block_SIGCHLD();
			int index = args[1] == NULL ? 1 : atoi(args[1]);
			job *j = getJob(index);
			if(j != NULL) {
				printf("<INFO> Reanudando tarea \"%s\"...\n", j->command);
				j->state = BACKGROUND;
				killpg(j->pgid, SIGCONT);
			} else printf("\033[1;31m<ERROR> job [%d] does not exist\n", index);

			unblock_SIGCHLD();
			continue;
		}

		// --------------------------------------------------------------------------------
		if(!(pid_fork = fork())) { // Child process
			new_process_group(getpid());

			// Si el proceso es foreground, se le cede el terminal
			if(!background) 
				set_terminal(getpid());

			restore_terminal_signals();

			int error = execvp(inputBuffer, args);
			if(error) {
				printf("\033[1;31m<ERROR> No se reconoce el comando: \"%s\"\n", args[0]);
				exit(-1);
			}
		} else { // Parent
			if(!background) {
				// Si el proceso hijo es foreground, se le espera (wait)
				default_wait(pid_fork, &status, &info, args[0]);
			} else {
				// Introducir background job en la lista
				block_SIGCHLD();
				if(jobList != NULL) {
					job *j = new_job(pid_fork, args[0], BACKGROUND);
					add_job(jobList, j);
				}
				unblock_SIGCHLD();
				printf("\033[1;32m[1] <\033[1;33mBackground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;33mRUNNING\033[1;32m, status: \033[1;33m%d\033[1;32m\n", pid_fork, args[0], WSTOPSIG(status));
			}
		}
	} // end while
}

void sigchldHandler(int signal) {
	block_SIGCHLD();
	job *ptr = jobList->next;
	
	while(ptr != NULL) {
		int pid = ptr->pgid;
		int status;

		if(pid == waitpid(pid, &status, WUNTRACED | WNOHANG | WCONTINUED)) {
			int printed = 0;
			// Imprimir mensaje de salida/suspensión
			print_exit_msg(ptr->pgid, ptr->command, &status, &printed);
			if(printed) print_prompt();
			
			// Ha ocurrido algo con el	 trabajo
			if(WIFSTOPPED(status)) status = SUSPENDED;
			else if(WIFEXITED(status)) status = EXITED;
			else if(WIFCONTINUED(status)) status = CONTINUED;
			
			switch (status) {
				case SUSPENDED:
					ptr->state = STOPPED;
					break;
				
				case SIGNALED:
					delete_job(jobList, ptr);
					break;

				case EXITED:
					delete_job(jobList, ptr);
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

job *getJob(int index) {
	job *ptr = jobList->next;

	int i = 1;
	while(ptr != NULL) {
		if(i == index) return ptr;
		
		ptr = ptr->next;
		i++;
	}

	return NULL;
}

void print_exit_msg(int pid_fork, const char *command, int *status, int *printed) {
	if(strcmp(command, "clear")) {
		printf("\033[1;32m");
		if(WIFEXITED(*status)) { // el proceso ha terminado con un exit()
			printf("<\033[1;33mForeground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;31mEXITED\033[1;32m, status: \033[1;33m%d\033[1;32m\n", pid_fork, command, WEXITSTATUS(*status));
			*printed = 1;
		} else if(WIFSIGNALED(*status)) { // el proceso ha terminado por la recepción de una señal
			printf("<\033[1;33mForeground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;31mSIGNALED\033[1;32m, status: \033[1;33m%d\033[1;32m\n", pid_fork, command, WTERMSIG(*status));
			*printed = 1;
		} else if(WIFSTOPPED(*status)) { // el proceso se ha parado por la recepción de una señal
			printf("<\033[1;33mForeground\033[1;32m> pid: \033[1;35m%d\033[1;32m, command: \033[1;35m%s\033[1;32m, \033[1;31mSTOPPED\033[1;32m, status: \033[1;33m%d\033[1;32m\n", pid_fork, command, WSTOPSIG(*status));
			*printed = 1;
		}
		printf("\033[0;0m");
	}
}

void default_wait(int pid_fork, int *status, int *info, const char *command) {
	waitpid(pid_fork, status, WUNTRACED);
	set_terminal(getpid());

	// Comprobar si el hijo ha sido suspendido (p.ej. suspender un proceso en primer plano con Ctrl+Z)
	// Si fue suspendido, se añade a jobs
	if(analyze_status(*status, info) == SUSPENDED){
		if(jobList != NULL) {
			job *j = new_job(pid_fork, command, STOPPED);
			add_job(jobList, j);
		}
	}

	int printed;
	print_exit_msg(pid_fork, command, status, &printed);
}

void print_prompt() {
		char cwd[PATH_MAX];
		getcwd(cwd, sizeof(cwd));

		// Shell prompt
		printf("\033[1;34m");
		printf("\033[1;32m[\033[1;34m%s@\033[1;34m%s\033[1;32m]", getlogin(), "unix");
		printf("\033[1;32m-[\033[1;34m%s\033[1;32m]", cwd);

		printf("$ ");
		printf("\033[0;0m");
}