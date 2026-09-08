#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../include/token.h"
#include "../include/bg.h"
#include "../include/resume.h"

int resume(Token* tokens, int size)
{
    if(size < 3)
    {
        printf("resume: syntax error\n");
        return 1;
    }
    char* end;
    long jn = (int)strtol(tokens[1].text+1, &end, 10);
    if(*end != '\0' || jn <= 0)
    {
        printf("resume: syntax error\n");
        return 1;
    }
    if(!strcmp("bg", tokens[2].text))
    {
        return resume_job(jn, 1, -1);
    }
    else if(!strcmp("fg", tokens[2].text))
    {
        if(size > 3 && strcmp(tokens[3].text, ";") && strcmp("--timeout", tokens[3].text))
        {
            printf("resume: Syntax error\n");
            return 1;
        }
        int timeout = 0;
        int tm = 0;
        if(size > 3 && !strcmp(tokens[3].text, "--timeout"))
        {
            if(size > 4)
            {
                timeout = 1;
                tm = atoi(tokens[4].text);
            }
            else
            {
                printf("resume: Syntax error\n");
                return 1;
            }
        }
        if(!timeout)
        {
            return resume_job(jn, 0, -1);
        }
        if(tm < 0)
        {
            printf("resume: Timeout needs to be positive\n");
            return 1;
        }
        return resume_job(jn, 0, tm);
    }
    else
    {
        printf("resume: Syntax error\n");
        return 1;
    }
}