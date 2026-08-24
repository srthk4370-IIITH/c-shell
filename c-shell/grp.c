#include "include/token.h"
#include "include/cmd.h"
#include "include/grp.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int stream(Token* seg, int size, Token* clean, int* csize)
{
    char tmp1[] = "/tmp/cshell_in_XXXXXX";
    int tempfd= -1;
    int enc = 0;
    char buf[4097];

    *csize = 0;
    for(int x=0; x<size; x++)
    {
        if(!strcmp("<", seg[x].text))
        {
            int fd;
            ssize_t r;
            if(!enc)
            {
                tempfd = mkstemp(tmp1);
                if(tempfd < 0 ) return -1;
                enc = 1;
                unlink(tmp1);
            }
            fd = open(seg[x+1].text, O_RDONLY);
            if(fd < 0)
            {
                printf("cshell: No such file or directory\n");
                close(tempfd);
                return -1;
            }
            while ((r = read(fd, buf, sizeof(buf))) > 0)
            {
                ssize_t off = 0;
                while (off < r)
                {
                    ssize_t w = write(tempfd, buf + off, (size_t)(r - off));
                    if (w < 0)
                    {
                        close(fd);
                        close(tempfd);
                        return -1;
                    }
                    off += w;
                }
            }
            close(fd);
            x++;
        }
        else
        {
            clean[(*csize)++] = seg[x];
        }
    }
    if(!enc)
    {
        return -2;
    }
    if (lseek(tempfd, 0, SEEK_SET) < 0)
    {
        close(tempfd);
        return -1;
    }
    return tempfd;
}

void run(Token* tokens, int size)
{
    Token clean[size];
    int csize = 0;
    int fd = stream(tokens, size, clean, &csize);
    if(fd == -1)
    {
        return;
    }
    if(csize == 0)
    {
        if(fd >= 0)
            close(fd);
        return;
    }
    if(fd == -2)
    {
        cmd(clean, csize);
        return;
    }
    int std = dup(STDIN_FILENO);
    if(fd < 0)
    {
        close(fd);
        close(std);
        return;
    }
    if(dup2(fd, STDIN_FILENO) < 0)
    {
        close(fd);
        close(std);
        return;
    }
    close(fd);
    cmd(clean, csize);
    dup2(std, STDIN_FILENO);
    close(std);
}

void grp(Token* tokens, int size)
{
    Token* t = malloc(sizeof(Token)*size);
    if(t == NULL)
    {
        printf("cshell: Memory allocation failed\n");
        return;
    }
    int i = 0;
    for(int x=0; x<size; x++)
    {
        if(!strcmp("|", tokens[x].text))
        {
            run(t, i);
            i=0;
        }
        else if(!strcmp(";", tokens[x].text))
        {
            run(t, i);
            i=0;
            return;
        }
        else if(!strcmp("&", tokens[x].text))
        {
            run(t, i);
            i=0;
            return;
        }
        else
        {
            t[i++] = tokens[x];
        }
    }
    run(t, i);
    free(t);
}