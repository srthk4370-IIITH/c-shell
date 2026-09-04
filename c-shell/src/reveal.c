#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "../include/token.h"

int reveal(Token* tokens, int size)
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
                        return 1;
                    }
                }
            }
            else if(!h && !c && !p && !l)
            {
                if(chdir(tokens[x].text))
                {
                    printf("reveal: no such directory\n");
                    return 1;
                }
            }
            else
            {
                printf("reveal: syntax error\n");
                return 1;
            }
        }
        int cd = 0;
        if(h) cd = chdir(home);
        else if(p) cd = chdir("..");
        else if(l)
        {
            cd = chdir(getenv("OLDPWD"));
        }
        if(cd)
        {
            printf("reveal: no such directory");
            return 1;
        }
        if(a && t)
        {
            char ans[1024];
            FILE* fp =popen("ls -aR", "r");
            while(1)
            {
                if(fgets(ans, sizeof(ans), fp) != NULL)
                {
                    printf("%s", ans);
                    continue;
                }
                if(ferror(fp) && errno == EINTR)
                {
                    clearerr(fp);
                    continue;
                }
                break;
            }
            printf("\n");
            pclose(fp);
        }
        else if(a)
        {
            FILE* fp =popen("ls -a", "r");
            char ans[1024];
            while(1)
            {
                if(fgets(ans, sizeof(ans), fp) != NULL)
                {
                    printf("%s", ans);
                    continue;
                }
                if(ferror(fp) && errno == EINTR)
                {
                    clearerr(fp);
                    continue;
                }
                break;
            }
            printf("\n");
            pclose(fp);
        }
        else if(t)
        {
            FILE* fp =popen("ls -R", "r");
            char ans[1024];
            while(1)
            {
                if(fgets(ans, sizeof(ans), fp) != NULL)
                {
                    printf("%s", ans);
                    continue;
                }
                if(ferror(fp) && errno == EINTR)
                {
                    clearerr(fp);
                    continue;
                }
                break;
            }
            printf("\n");
            pclose(fp);
        }
        else
        {
            FILE* fp =popen("ls", "r");
            char ans[1024];
            while(1)
            {
                if(fgets(ans, sizeof(ans), fp) != NULL)
                {
                    printf("%s", ans);
                    continue;
                }
                if(ferror(fp) && errno == EINTR)
                {
                    clearerr(fp);
                    continue;
                }
                break;
            }
            printf("\n");
            pclose(fp);
        }
        chdir(curr);
    }
    else
    {
        FILE* fp =popen("ls", "r");
        char ans[1024];
        while(1)
        {
            if(fgets(ans, sizeof(ans), fp) != NULL)
            {
                printf("%s", ans);
                continue;
            }
            if(ferror(fp) && errno == EINTR)
            {
                clearerr(fp);
                continue;
            }
            break;
        }
        printf("\n");
        pclose(fp);
    }
    return 0;
}