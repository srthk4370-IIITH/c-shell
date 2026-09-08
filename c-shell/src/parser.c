#include "../include/token.h"
#include "../include/grp.h"
#include "../include/parser.h"
#include <stdio.h>
#include "../include/cmd.h"

int transition(int c, TID input)
{
    // Line  -> 0
    // ARG -> 1
    // TGT -> 2
    // CMD -> 3
    // BG -> 4
    if(c == 0 && input == WORD)
    {
        return 1;
    }
    else if(c == 1)
    {
        if(input == WORD)
        {
            return 1;
        }
        else if(input == OP_LT || input == OP_GT || input == OP_GTGT)
        {
            return 2;
        }
        else if(input == OP_PIPE || input == OP_SEMI)
        {
            return 3;
        }
        else if(input == OP_AMP)
        {
            return 4;
        }
    }
    else if(c == 2 && input == WORD)
    {
        return 1;
    }
    else if(c == 3 && input == WORD)
    {
        return 1;
    }
    else if(c == 4 && input == WORD)
    {
        return 1;
    }
    return 0;
}

void parse(Token* tokens, int size, char* s)
{
    if(size == 0)
    {
        return;
    }
    int c = 0;
    for(int x=0; x<size; x++)
    {
        c = transition(c, tokens[x].type);
        if(!c)
        {
            break;
        }
    }
    if(c == 1 || c == 4)
    {
        grp(tokens, size, s);
        return;
    }
    printf("cshell: Invalid Syntax\n");
}