#include <stdio.h>

#include "colors.h"

void Colors_SetColor(enum Color c) 
{ 
    printf("\033%s", Colors[c]); 
}