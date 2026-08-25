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
#include <sys/wait.h>

static int count_stages(Token* tokens, int size)
{
    int stages = 1;
    for(int x=0; x<size; x++)
    {
        if(tokens[x].type == OP_PIPE)
        {
            stages++;
        }
    }
    return stages;
}

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

static int prepare_pipeline_stage(Token* input, int input_size, Token* clean, int* clean_size)
{
    char template[] = "/tmp/cshell_pipe_in_XXXXXX";
    int tempfd = -1;
    int has_input = 0;
    char buf[4096];

    *clean_size = 0;

    for(int i = 0; i < input_size; i++)
    {
        if(!strcmp("<", input[i].text))
        {
            int fd;
            ssize_t r;

            if(i + 1 >= input_size)
            {
                return -1;
            }

            if(!has_input)
            {
                tempfd = mkstemp(template);

                if(tempfd < 0)
                {
                    return -1;
                }

                unlink(template);
                has_input = 1;
            }

            fd = open(input[i + 1].text, O_RDONLY);

            if(fd < 0)
            {
                printf("cshell: no such file or directory\n");

                close(tempfd);
                return -1;
            }

            while((r = read(fd, buf, sizeof(buf))) > 0)
            {
                ssize_t off = 0;

                while(off < r)
                {
                    ssize_t w = write(
                        tempfd,
                        buf + off,
                        (size_t)(r - off)
                    );

                    if(w < 0)
                    {
                        close(fd);
                        close(tempfd);
                        return -1;
                    }

                    off += w;
                }
            }

            close(fd);

            i++;
        }
        else if(!strcmp(">", input[i].text))
        {
            int fd;

            if(i + 1 >= input_size)
            {
                return -1;
            }

            fd = open(
                input[i + 1].text,
                O_WRONLY | O_CREAT | O_TRUNC,
                0644
            );

            if(fd < 0)
            {
                printf("cshell: unable to create file for writing\n");
                return -1;
            }

            if(dup2(fd, STDOUT_FILENO) < 0)
            {
                close(fd);
                return -1;
            }

            close(fd);

            i++;
        }
        else if(!strcmp(">>", input[i].text))
        {
            int fd;

            if(i + 1 >= input_size)
            {
                return -1;
            }

            fd = open(
                input[i + 1].text,
                O_WRONLY | O_CREAT | O_APPEND,
                0644
            );

            if(fd < 0)
            {
                printf("cshell: unable to create file for writing\n");
                return -1;
            }

            if(dup2(fd, STDOUT_FILENO) < 0)
            {
                close(fd);
                return -1;
            }

            close(fd);

            i++;
        }
        else
        {
            clean[(*clean_size)++] = input[i];
        }
    }

    if(has_input)
    {
        if(lseek(tempfd, 0, SEEK_SET) < 0)
        {
            close(tempfd);
            return -1;
        }

        if(dup2(tempfd, STDIN_FILENO) < 0)
        {
            close(tempfd);
            return -1;
        }

        close(tempfd);
    }

    return 0;
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

static void run_pipeline(Token* tokens, int size, int sc)
{
    Token* stages[sc];
    int stage[sc];
    for(int x=0; x<sc; x++)
    {
        stages[x] = malloc(sizeof(Token)*size);
        if(stages[x] == NULL)
        {
            printf("cshell: Memory allocation failed\n");
            return;
        }
        stage[x] = 0;
    }
    int cs = 0;
    for(int x=0; x<size; x++)
    {
        if(tokens[x].type == OP_PIPE)
        {
            cs++;
        }
        else
        {
            stages[cs][stage[cs]++] = tokens[x];
        }
    }
    int pipes[sc][2];
    for(int x=0; x<sc-1; x++)
    {
        if(pipe(pipes[x]) < 0)
        {
            printf("cshell: Pipe creation failed\n");
            for(int y=0; y<sc; y++)
            {
                free(stages[y]);
            }
            return;
        }
    }
    pid_t pids[sc];
    for(int x=0; x<sc; x++)
    {
        pids[x] = fork();
        if(pids[x] < 0)
        {
            printf("cshell: Fork failed\n");
            for(int y=0; y<sc; y++)
            {
                free(stages[y]);
            }
            for(int y=0; y<sc-1; y++)
            {
                close(pipes[y][0]);
                close(pipes[y][1]);
            }
            return;
        }
        else if(pids[x] == 0)
        {
            if(x > 0)
            {
                dup2(pipes[x-1][0], STDIN_FILENO);
            }
            if(x < sc-1)
            {
                dup2(pipes[x][1], STDOUT_FILENO);
            }
            for(int y=0; y<sc-1; y++)
            {
                close(pipes[y][0]);
                close(pipes[y][1]);
            }
            Token clean[stage[x]];
            int csize = 0;
            if(prepare_pipeline_stage(stages[x], stage[x], clean, &csize) < 0)
            {
                printf("cshell: Error preparing pipeline stage\n");
                for(int y=0; y<sc; y++)
                {
                    free(stages[y]);
                }
                _exit(1);
            }
            if(csize == 0)
            {
                for(int y=0; y<sc; y++)
                {
                    free(stages[y]);
                }
                _exit(0);
            }
            cmd_child(clean, csize);
            _exit(0);
        }
    }
    for (int x = 0; x < sc - 1; x++)
    {
        close(pipes[x][0]);
        close(pipes[x][1]);
    }
    for(int x=0; x<sc; x++)
    {
        waitpid(pids[x], NULL, 0);
    }
    for(int x=0; x<sc; x++)
    {
        free(stages[x]);
    }
}

void grp(Token* tokens, int size)
{
    Token* t = malloc(sizeof(Token)*size);
    if(t == NULL)
    {
        printf("cshell: Memory allocation failed\n");
        return;
    }
    int sc = count_stages(tokens, size);
    if(sc > 1)
    {
        run_pipeline(tokens, size, sc);
        free(t);
        return;
    }
    int i = 0;
    for(int x=0; x<size; x++)
    {
        if(!strcmp(";", tokens[x].text))
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