#include "../include/token.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int nonempty(char *buffer)
{
    for(int j = 0; buffer[j] != '\0'; j++)
    {
        if(buffer[j] != '\n' && buffer[j] != '\r')
        {
            return 1;
        }
    }
    return 0;
}

int find_newline(char *buffer, int size)
{
    for(int i = size - 1; i >= 0; i--)
    {
        if(buffer[i] == '\n' || buffer[i] == '\r')
            return i;
    }

    return -1;
}

int nonempty_rev(char *buffer, ssize_t len)
{
    for(ssize_t i = 0; i < len; i++)
    {
        if(buffer[i] != '\n' && buffer[i] != '\r')
            return 1;
    }

    return 0;
}

void revfile(Token* tokens, int x, int flag, int lines)
{
    int fd = open(tokens[x].text, O_RDONLY);
    if(fd != -1)
    {
        int ii = 0;
        off_t pos = lseek(fd, 0, SEEK_END);
        off_t lp = pos;
        while(pos > 0)
        {
            char buffer[100];
            off_t cs = pos < 99 ? pos : 99;
            pos -= cs;
            lseek(fd, pos, SEEK_SET);
            ssize_t bytes = read(fd, buffer, cs);
            int nl = find_newline(buffer, bytes);
            if(nl == 0 && flag)
            {
                lp = pos-1;
                pos = pos -1;
                continue;
            }
            if(nl != -1)
            {
                off_t np = pos + nl;
                lseek(fd, np+1, SEEK_SET);
                char *line = malloc((int)(lp-np+2));
                ssize_t len = read(fd, line, lp - np + 1);
                if(flag && nonempty_rev(line, len))
                {
                    char buffer[30];
                    sprintf(buffer, "%d ", lines);
                    lines--;
                    write(STDOUT_FILENO, buffer, strlen(buffer));
                }
                write(STDOUT_FILENO, line, len);
                if(!ii)
                {
                    write(STDOUT_FILENO, "\n", 1);
                    ii = 1;
                }
                lp = np-1;
                pos = np - 1;
                free(line);
            }
        }
        if(lp)
        {
            lseek(fd, 0, SEEK_SET);

            char line[1024];
            ssize_t len = read(fd, line, lp);
            if(flag && nonempty_rev(line, len))
            {
                char buffer[30];
                sprintf(buffer, "%d ", lines);
                lines--;
                write(STDOUT_FILENO, buffer, strlen(buffer));
            }
            write(STDOUT_FILENO, line, len);
            write(STDOUT_FILENO, "\n", 1);
        }
        close(fd);
    }
    else
    {
        printf("Couldn't Open File");
    }
}

int peek(Token* tokens, int size)
{
    if(size > 1)
    {
        int x=1;
        int f= 0;
        int r = 0, n = 0;
        for(; x<size; x++)
        {
            if(strcmp("-", tokens[x].text) && tokens[x].text[0] == '-')
            {
                int l =strlen(tokens[x].text);
                for(int y=1; y<l; y++)
                {
                    if(tokens[x].text[y] == 'r')
                    {
                        r=1;
                    }
                    else if(tokens[x].text[y] == 'n')
                    {
                        n=1;
                    }
                    else
                    {
                        f=1;
                        break;
                    }
                }
                if(f)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        for(; x<size; x++)
        {
            if(strcmp("-", tokens[x].text))
            {
                struct stat st;
                
                if(stat(tokens[x].text, &st) == -1)
                {
                    printf("peek: no such file or directory\n");
                    continue;
                }
                else if(S_ISDIR(st.st_mode))
                {
                    printf("peek: is a directory\n");
                    continue;
                }

                FILE* f = fopen(tokens[x].text, "r");
                if(f == NULL)
                {
                    printf("Unable to open File\n");
                    continue;
                }
                if(r && n)
                {
                    char buffer[1025];
                    int i = 0;
                    while(fgets(buffer, 1024, f))
                    {
                        if(nonempty(buffer))
                        {
                            i++;
                        }
                    }
                    revfile(tokens, x, 1, i);
                }
                else if(r)
                {
                    revfile(tokens, x, 0 , 0);
                }
                else if(n)
                {
                    char buffer[1025];
                    int i = 1;
                    while(fgets(buffer, 1024, f))
                    {
                        if(nonempty(buffer))
                        {
                            printf("%d %s", i, buffer);
                            i++;
                        }
                        else
                        {
                            printf("\n");
                        }
                    }
                    printf("\n");
                }
                else
                {
                    char buffer[1025];
                    while(fgets(buffer, 1024, f))
                    {
                        printf("%s", buffer);
                    }
                    printf("\n");
                }
            }
            else
            {
                //TODO: Implement The Stdin function
            }
        }
    }
    return 0;
}