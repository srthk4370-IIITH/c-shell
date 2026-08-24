#include "include/token.h"
#include "include/cmd.h"
#include "include/grp.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void grp(Token* tokens, int size)
{
    Token* t = malloc(sizeof(Token)*size);
    if(t == NULL)
    {
        printf("cshell: Memory allocation failed\n");
        return;
    }
    int i = 0;
    for(int x=0; x<size; x++)
    {
        if(!strcmp("|", tokens[x].text))
        {
            cmd(t, i);
            i=0;
        }
        else if(!strcmp(";", tokens[x].text))
        {
            cmd(t, i);
            i=0;
            return;
        }
        else if(!strcmp("&", tokens[x].text))
        {
            cmd(t, i);
            i=0;
            return;
        }
        else
        {
            t[i++] = tokens[x];
        }
    }
    cmd(t, i);
    free(t);
}