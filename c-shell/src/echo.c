#include "../include/echo.h"
#include "../include/token.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int echo(Token* tokens, int size)
{
    for(int x=1; x<size; x++)
    {
        printf("%s ", tokens[x].text);
    }
    printf("\n");
    return 0;
}