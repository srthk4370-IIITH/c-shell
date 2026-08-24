#include "include/echo.h"
#include "include/token.h"
#include "include/lower.h"
#include "include/hop.h"
#include "include/reveal.h"
#include "include/locate.h"
#include "include/cmd.h"
#include "include/peek.h"
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
        else if(!strcmp("reveal", tokens[0].text))
        {
            reveal(tokens, size);
        }
        else if(!strcmp("locate", tokens[0].text))
        {
            locate(tokens, size);
        }
        else if(!strcmp("peek", tokens[0].text))
        {
            peek(tokens, size);
        }
        else
        {
            
            printf("Command not build yet.... builder is busy in NAB construction\n");
        }
    }
}