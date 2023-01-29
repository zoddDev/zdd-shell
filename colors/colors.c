#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"

void Colors_SetColor(enum Color c)
{
    printf("\033%s", Colors[c]);
}

char *Colors_Color(char *text, enum Color c)
{
    char *coloredText = malloc((strlen(text) * sizeof(char)) + (14 * sizeof(char)));
    sprintf(coloredText, "\033%s%s\033[0m", Colors[c], text);
    return coloredText;
}