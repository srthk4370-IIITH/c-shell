#ifndef CMD_H
#define CMD_H
#include "token.h"

int cmd(Token* tokens, int size);
int execute(char *path, char **argv);
int execute_child(char *path, char **argv);
int external(char **argv);
int external_child(char **argv);
int cmd_child(Token *tokens, int size);

#endif