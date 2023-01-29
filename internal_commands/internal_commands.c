#include <stdio.h>
#include "internal_commands.h"

void INTERNAL_EXIT(Shell *shell)
{
    exit(0);
}

void INTERNAL_CLEAR(Shell *shell)
{
    system("clear");
}

void INTERNAL_L(Shell *shell)
{
    system("ls -a");
}

void INTERNAL_LL(Shell *shell)
{
    system("ls -al");
}

void INTERNAL_CD(Shell *shell)
{
    char *dir = shell->args[1] == NULL || !strcmp(shell->args[1], "~") ? shell->homedir : shell->args[1];

    if (chdir(dir)) // directory not found
    {
        Colors_SetColor(ERROR_COLOR);
        printf("<ERROR> directory \"%s\" does not exist\n", dir);
        Colors_SetColor(RESET);
    }
}

void INTERNAL_JOBS(Shell *shell)
{
    block_SIGCHLD();
    Colors_SetColor(CYAN_BOLD);

    print_job_list(shell->jobList);

    Colors_SetColor(RESET);
    unblock_SIGCHLD();
}

void INTERNAL_FG(Shell *shell)
{
    block_SIGCHLD();

    int index = shell->args[1] == NULL ? 1 : atoi(shell->args[1]);

    job *j = Shell_GetJob(shell, index);
    if (j != NULL)
    {
        printf("<INFO> Sending job %d to foreground...\n", index);
        set_terminal(j->pgid);
        // j->state = FOREGROUND;
        killpg(j->pgid, SIGCONT);
        Shell_DefaultWait(shell, j->pgid, j->command);
        delete_job(shell->jobList, j);
    }
    else
    {
        Colors_SetColor(ERROR_COLOR);
        printf("\033[1;31m<ERROR> job [%d] does not exist\n", index);
        Colors_SetColor(RESET);
    }

    unblock_SIGCHLD();
}

void INTERNAL_BG(Shell *shell)
{
    block_SIGCHLD();
    int index = shell->args[1] == NULL ? 1 : atoi(shell->args[1]);
    job *j = Shell_GetJob(shell, index);
    if (j != NULL)
    {
        printf("<INFO> Resuming job \"%s\"...\n", j->command);
        j->state = BACKGROUND;
        killpg(j->pgid, SIGCONT);
    }
    else
        printf("\033[1;31m<ERROR> job [%d] does not exist\n", index);

    unblock_SIGCHLD();
}