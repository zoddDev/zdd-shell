#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

int main(int argc, char **argv)
{
	Shell *shell = Shell_Construct();
	Shell_Run(shell);

	Shell_Destruct(&shell);
	return EXIT_SUCCESS;
}
