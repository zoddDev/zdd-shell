#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

int main(int argc, char **argv)
{
	Shell *shell;
	Shell_Construct(shell);
	Shell_Run(shell);

	return EXIT_SUCCESS;
}
