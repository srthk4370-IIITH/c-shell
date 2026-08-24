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

int stream(Token* seg, int size, Token* clean, int* csize, int* out, int* out_pos, int* tempout)
{
    char tmp1[] = "/tmp/cshell_in_XXXXXX";
    char tmp2[] = "/tmp/cshell_out_XXXXXX";
    int tempfd= -1;
    *tempout = -1;
    int enc = 0;
    int enco = 0;
    char buf[4097];
    *out_pos = 0;
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
        else if(!strcmp(">", seg[x].text))
        {
            if(!enco)
            {
                *tempout = mkstemp(tmp2);
                if(*tempout < 0 )return -1;
                unlink(tmp2);
                enco = 1;
            }
            out[(*out_pos)++] = open(seg[x+1].text, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if(out[(*out_pos)-1] < 0)
            {
                printf("cshell: Cannot create file %s\n", seg[x+1].text);
                for(int x=0; x<*out_pos-1; x++)
                {
                    close(out[x]);
                }
                return -1;
            }
            x++;
        }
        else if(!strcmp(">>", seg[x].text))
        {
            if(!enco)
            {
                *tempout = mkstemp(tmp2);
                if(*tempout < 0 )return -1;
                unlink(tmp2);
                enco = 1;
            }
            out[(*out_pos)++] = open(seg[x+1].text, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if(out[(*out_pos)-1] < 0)
            {
                printf("cshell: Cannot create file %s\n", seg[x+1].text);
                for(int x=0; x<*out_pos-1; x++)
                {
                    close(out[x]);
                }
                return -1;
            }
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
    int out[size];
    int csize = 0;
    int out_pos = 0;
    int tempout = -1;
    int fd = stream(tokens, size, clean, &csize, out, &out_pos, &tempout);
    if(fd == -1)
    {
        return;
    }
    if(csize == 0)
    {
        if(fd >= 0)
            close(fd);

        if(tempout >= 0)
            close(tempout);

        return;
    }
    int stdout = dup(STDOUT_FILENO);
    if(stdout < 0)
    {
        if(fd >= 0)
            close(fd);

        if(tempout >= 0)
            close(tempout);

        return;
    }
    int std = dup(STDIN_FILENO);
    if(std < 0)
    {
        close(stdout);
        if(fd >= 0)
            close(fd);

        if(tempout >= 0)
            close(tempout);

        return;
    }

    if(tempout >= 0)
    {
        if(dup2(tempout, STDOUT_FILENO) < 0)
        {
            close(tempout);
            close(stdout);
            close(std);
            return;
        }
    }

    if(fd >= 0)
    {
        if(dup2(fd, STDIN_FILENO) < 0)
        {
            close(fd);
            close(std);
            dup2(stdout, STDOUT_FILENO);
            close(stdout);
            return;
        }

        close(fd);
    }

    cmd(clean, csize);

    if(tempout >= 0)
    {
        lseek(tempout, 0, SEEK_SET);

        char buf[4096];
        ssize_t r;

        while((r = read(tempout, buf, sizeof(buf))) > 0)
        {
            for(int x = 0; x < out_pos; x++)
            {
                ssize_t off = 0;

                while(off < r)
                {
                    ssize_t w = write(out[x], buf + off, r - off);

                    if(w < 0)
                        break;

                    off += w;
                }
            }
        }

        for(int x = 0; x < out_pos; x++)
        {
            close(out[x]);
        }

        close(tempout);
    }

    dup2(std, STDIN_FILENO);
    dup2(stdout, STDOUT_FILENO);

    close(std);
    close(stdout);
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