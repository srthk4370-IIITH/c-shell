#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "include/token.h"

void reveal(Token* tokens, int size)
{
    char* curr = getcwd(NULL, 0);
    if(size > 1)
    {
        int h = 0, c = 0, p = 0, l = 0, a = 0, t = 0, n = 0;
        for(int x=1; x<size; x++)
        {
            if(!strcmp("~", tokens[x].text) && !c && !p && !l && !n)
            {
                h = 1;
            }
            else if(!strcmp(".", tokens[x].text) && !h && !p && !l&& !n)
            {
                c = 1;
            }
            else if(!strcmp("..", tokens[x].text) && !c && !h && !l && !n)
            {
                p = 1;
            }
            else if(!strcmp("-", tokens[x].text) && !c && !p && !h && !n)
            {
                l = 1;
            }
            else if(tokens[x].text[0] == '-')
            {
                int length = strlen(tokens[x].text);
                for(int y=1; y<length; y++)
                {
                    if(tokens[x].text[y] == 'a')
                    {
                        a = 1;
                    }
                    else if(tokens[x].text[y] == 't')
                    {
                        t = 1;
                    }
                    else
                    {
                        printf("reveal: syntax error\n");
                        return;
                    }
                }
            }
            else if(!h && !c && !p && !l)
            {
                if(chdir(tokens[x].text))
                {
                    printf("reveal: no such directory\n");
                    return;
                }
            }
            else
            {
                printf("reveal: syntax error\n");
                return;
            }
        }
        int cd = 0;
        if(h) cd = chdir("~");
        else if(p) cd = chdir("..");
        else if(l)
        {
            cd = chdir(getenv("OLDPWD"));
        }
        if(cd)
        {
            printf("reveal: no such directory");
            return;
        }
        if(a && t)
        {
            system("ls -aR");
        }
        else if(a)
        {
            system("ls -a");
        }
        else if(t)
        {
            system("ls -R");
        }
        else
        {
            system("ls");
        }
        chdir(curr);
    }
    else
    {
        system("ls");
    }
}