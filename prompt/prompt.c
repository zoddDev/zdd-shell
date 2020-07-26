#include "prompt.h"

void Prompt_Print()
{
    Colors_SetColor(BLUE);
    printf("%s ", Prompt_FancyPWD());

    Colors_SetColor(RED_BOLD);
	printf("❯");
	Colors_SetColor(YELLOW_BOLD);
	printf("❯");
	Colors_SetColor(GREEN_BOLD);
	printf("❯ ");
    Colors_SetColor(RESET);
}

char* Prompt_PWD()
{
    char pwd[4096];
    getcwd(pwd, sizeof(pwd));
    char *ptr = pwd;
    return ptr;
}

char* Prompt_FancyPWD()
{
    int username_length = strlen(getlogin());
    const int home_length = strlen("/home/") + username_length;

    char HOME_DIR[home_length + 1];
    strcpy(HOME_DIR, "/home/");
    strcat(HOME_DIR, getlogin());
    HOME_DIR[home_length] = '\0';

    char *pwd = Prompt_PWD();

    if (strlen(pwd) >= home_length)
    {
        char firstCharacters[home_length + 1];
        strncpy(firstCharacters, pwd, home_length);
        firstCharacters[home_length] = '\0';
        
        if (!strcmp(firstCharacters, HOME_DIR))
        {
            char pwd_fancy[4096];
            strcpy(pwd_fancy, "~");
            strcat(pwd_fancy, pwd + home_length);

            char *ptr = pwd_fancy;
            return ptr;
        }
    }

    return pwd;
}
