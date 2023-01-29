/*--------------------------------------------------------
UNIX Shell Project
job_control module

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.
--------------------------------------------------------*/

#include "job_control.h"
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "./shell.h"

// -----------------------------------------------------------------------
//  get_command() reads in the next command line, separating it into distinct tokens
//  using whitespace as delimiters. setup() sets the args parameter as a
//  null-terminated string.
// -----------------------------------------------------------------------

void get_command(char inputBuffer[], int size, char *args[], int *background)
{

	int length, /* # of characters in the command line */
		i,		/* loop index for accessing inputBuffer array */
		start,	/* index where beginning of next command parameter is */
		ct;		/* index of where to place the next parameter into args[] */
	ct = 0;

	*background = 0;

	// TODO Implement a reader that reads every character and stores it in inputBuffer
	/* read what the user enters on the command line */
	// system ("/bin/stty raw"); // avoid the need to press Enter
	// int stop = 0;
	// int sz = 0;
	// while (!stop && sz < size)
	// {
	// 	int c = getchar();
	// 	if (c == 0x0D)
	// 	{
	// 		stop = 1;
	// 	} 
	// 	else if (c == 0x0C)
	// 	{
	// 		system("clear");
	// 	}
	// 	else if (c == 0x04)
	// 	{
	// 		exit(0);
	// 	}
	// 	else {
	// 		inputBuffer[sz++] = c;
	// 	}
	// }
	// length = i;
	// printf("\n\n>>%s<<\n\n", inputBuffer);

	length = read(STDIN_FILENO, inputBuffer, size);

	start = -1;
	if (length == 0)
	{
		exit(0); /* ^d was entered, end of user command stream */
	}
	if (length < 0)
	{
		perror("error reading the command");
		exit(-1); /* terminate with error code of -1 */
	}

	/* examine every character in the inputBuffer */
	for (i = 0; i < length; i++)
	{
		switch (inputBuffer[i])
		{
		case ' ':
		case '\t': /* argument separators */
			if (start != -1)
			{
				args[ct] = &inputBuffer[start]; /* set up pointer */
				ct++;
			}
			inputBuffer[i] = '\0'; /* add a null char; make a C string */
			start = -1;
			break;

		case '\n': /* should be the final char examined */
			if (start != -1)
			{
				args[ct] = &inputBuffer[start];
				ct++;
			}
			inputBuffer[i] = '\0';
			args[ct] = NULL; /* no more arguments to this command */
			break;

		default: /* some other character */

			if (inputBuffer[i] == '&') // background indicator
			{
				*background = 1;
				if (start != -1)
				{
					args[ct] = &inputBuffer[start];
					ct++;
				}
				inputBuffer[i] = '\0';
				args[ct] = NULL; /* no more arguments to this command */
				i = length;		 // make sure the for loop ends now
			}
			else if (start == -1)
				start = i; // start of new argument
		}				   // end switch
	}					   // end for
	args[ct] = NULL;	   /* just in case the input line was > MAXLINE */

	// for (int j = 0; j < sz; j++)
	// {
	// 	inputBuffer[j] = '\0';
	// }
}

// -----------------------------------------------------------------------
/* devuelve puntero a un nodo con sus valores inicializados,
devuelve NULL si no pudo realizarse la reserva de memoria*/
job *new_job(pid_t pid, const char *command, enum job_state state)
{
	job *aux;
	aux = (job *)malloc(sizeof(job));
	aux->pgid = pid;
	aux->state = state;
	aux->command = strdup(command);
	aux->next = NULL;
	return aux;
}

// -----------------------------------------------------------------------
/* inserta elemento en la cabeza de la lista */
void add_job(job *list, job *item)
{
	job *aux = list->next;
	list->next = item;
	item->next = aux;
	list->pgid++;
}

// -----------------------------------------------------------------------
/* elimina el elemento indicado de la lista
devuelve 0 si no pudo realizarse con exito */
int delete_job(job *list, job *item)
{
	job *aux = list;
	while (aux->next != NULL && aux->next != item)
		aux = aux->next;
	if (aux->next)
	{
		aux->next = item->next;
		free(item->command);
		free(item);
		list->pgid--;
		return 1;
	}
	else
		return 0;
}
// -----------------------------------------------------------------------
/* busca y devuelve un elemento de la lista cuyo pid coincida con el indicado,
devuelve NULL si no lo encuentra */
job *get_item_bypid(job *list, pid_t pid)
{
	job *aux = list;
	while (aux->next != NULL && aux->next->pgid != pid)
		aux = aux->next;
	return aux->next;
}
// -----------------------------------------------------------------------
job *get_item_bypos(job *list, int n)
{
	job *aux = list;
	if (n < 1 || n > list->pgid)
		return NULL;
	n--;
	while (aux->next != NULL && n)
	{
		aux = aux->next;
		n--;
	}
	return aux->next;
}

// -----------------------------------------------------------------------
/*imprime una linea en el terminal con los datos del elemento: pid, nombre ... */
void print_item(job *item)
{

	printf("pid: %d, command: %s, state: %s\n", item->pgid, item->command, state_strings[item->state]);
}

// -----------------------------------------------------------------------
/*recorre la lista y le aplica la funcion pintar a cada elemento */
void print_list(job *list, void (*print)(job *))
{
	if (list != NULL && list->next == NULL)
	{
		printf("jobs: there are no jobs\n");
	}
	else
	{
		int n = 1;
		job *aux = list;
		printf("jobs:\n");

		while (aux->next != NULL)
		{
			printf(" [%d] ", n);
			print(aux->next);
			n++;
			aux = aux->next;
		}
	}
}

// -----------------------------------------------------------------------
/* interpretar valor estatus que devuelve wait */
enum status analyze_status(int status, int *info)
{
	if (WIFSTOPPED(status))
	{
		*info = WSTOPSIG(status);
		return (SUSPENDED);
	}
	else if (WIFCONTINUED(status))
	{
		*info = 0;
		return (CONTINUED);
	}
	else
	{
		// el proceso termio
		if (WIFSIGNALED(status))
		{
			*info = WTERMSIG(status);
			return (SIGNALED);
		}
		else
		{
			*info = WEXITSTATUS(status);
			return (EXITED);
		}
	}
	return 0;
}

// -----------------------------------------------------------------------

void Handler_IgnoredSIGINT()
{
	// printf("%s", Colors_Color("Ignored SIGINT", MAGENTA));
	printf("\n");
	Shell_PrintPrompt(NULL);
	// printf("\n");
}

// cambia la accion de las seÃ±ales relacionadas con el terminal
void terminal_signals(void (*func)(int))
{
	signal(SIGINT, func == SIG_IGN ? Handler_IgnoredSIGINT : func); // crtl+c interrupt tecleado en el terminal

	signal(SIGQUIT, func); // ctrl+\ quit tecleado en el terminal
	signal(SIGTSTP, func); // crtl+z Stop tecleado en el terminal
	signal(SIGTTIN, func); // proceso en segundo plano quiere leer del terminal
	signal(SIGTTOU, func); // proceso en segundo plano quiere escribir en el terminal
}

// -----------------------------------------------------------------------
void block_signal(int signal, int block)
{
	/* declara e inicializa máscara */
	sigset_t block_sigchld;
	sigemptyset(&block_sigchld);
	sigaddset(&block_sigchld, signal);
	if (block)
	{
		/* bloquea señal */
		sigprocmask(SIG_BLOCK, &block_sigchld, NULL);
	}
	else
	{
		/* desbloquea señal */
		sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
	}
}
