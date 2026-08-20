#include "include/echo.h"
#include "include/token.h"
#include "include/lower.h"
#include "include/hop.h"
#include "include/cmd.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>



void cmd(Token* tokens, int size)
{
    if(size> 0)
    {
        lower(tokens[0].text);
        if(!strcmp("echo", tokens[0].text))
        {
            echo(tokens, size);
        }
        else if(!strcmp("hop", tokens[0].text))
        {
            hop(tokens, size);
        }
        else
        {
            printf("Command not build yet.... builder is busy in NAB construction\n");
        }
    }
}