#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include "../include/token.h"

int locate(Token* tokens, int size)
{
    if(size == 1)
    {
        printf("locate: syntax error\n");
        return 1;
    }
    for(int x=1; x<size; x++)
    {
        char * cwd = getcwd(NULL, 0);
        int f = 1;
        while(1)
        {
            char path[1024] = "";
            strcat(path, cwd);
            if(strcmp(path, "/"))
                strcat(path, "/");
            strcat(path, tokens[x].text);
            if(!access(path, F_OK) && !access(path, X_OK))
            {
                printf("%s\n", path);
                f = 0;
            }
            char old[1024];
            strcpy(old, cwd);
            char* p = dirname(cwd);
            if(!strcmp(old, p))
            {
                break;
            }
            else
            {
                strcpy(cwd, p);
            }
        }
        if(f)
        {
            printf("locate: command not found(%s)\n", tokens[x].text);
            return 1;
        }
    }
    return 0;
}
