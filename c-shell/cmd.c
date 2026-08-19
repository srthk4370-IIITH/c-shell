#include "include/echo.h"
#include "include/token.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

void lower(char *str) 
{
    for (int i = 0; str[i] != '\0'; i++) 
    {
        str[i] = tolower((unsigned char)str[i]);
    }
}

void cmd(Token* tokens, int size)
{
    if(size> 0)
    {
        lower(tokens[0].text);
        if(!strcmp("echo", tokens[0].text))
        {
            echo(tokens, size);
        }
        else
        {
            printf("Command not build yet.... builder is busy in NAB construction\n");
        }
    }
}