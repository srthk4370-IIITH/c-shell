#ifndef CMD_H
#define CMD_H
#include "token.h"

void cmd(Token* tokens, int size);
void execute_child(char *path, char **argv);
void cmd_child(Token *tokens, int size);

#endif