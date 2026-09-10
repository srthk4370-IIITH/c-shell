#include "../include/bg.h"
#include "../include/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

int ping(Token* tokens, int size)
{
    if(size != 3)
    {
        printf("ping: Syntax error\n");
        return 1;
    }
    char *end;
    int sig = (int)strtol(tokens[2].text, &end, 10);
    if(*end != '\0' || sig < 0)
    {
        printf("ping: invalid syntax\n");
        return 1;
    }
    int send = sig%64;
    if(tokens[1].text[0] == '%')
    {
        char* e;
        int jn = (int)strtol(tokens[1].text + 1, &e, 10);
        if(*e != '\0' || jn < 0)
        {
            printf("ping: invalid syntax\n");
            return 1;
        }
        int pgid = get_pgid(jn);
        if(pgid == -1)
        {
            printf("ping: no such process found\n");
            return 1;
        }
        kill(-pgid, send);
        printf("Sent Signal %d to %d\n", sig, pgid);
        return 0;
    }
    else
    {
        char* e;
        int pid = (int) strtol(tokens[1].text, &e, 10);
        if(*e != '\0' || pid < 0)
        {
            printf("ping: invalid syntax\n");
            return 1;
        }
        if(pidExists(pid))
        {
            if(kill(pid, send) < 0)
            {
                printf("ping: error sending signal\n");
                return 1;
            }
            else
            {
                printf("Sent signal %d to %d\n", sig, pid);
                return 0;
            }
        }
        else
        {
            printf("ping: no such process found\n");
            return 1;
        }
    }
}