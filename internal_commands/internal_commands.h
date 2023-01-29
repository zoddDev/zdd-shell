#ifndef INTERNAL_COMMANDS_H_
#define INTERNAL_COMMANDS_H_

#include "../shell.h"

void INTERNAL_EXIT(Shell *shell);
void INTERNAL_CLEAR(Shell *shell);
void INTERNAL_LL(Shell *shell);
void INTERNAL_L(Shell *shell);
void INTERNAL_CD(Shell *shell);
void INTERNAL_JOBS(Shell *shell);
void INTERNAL_FG(Shell *shell);
void INTERNAL_BG(Shell *shell);

#endif