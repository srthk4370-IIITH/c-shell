#include "../include/token.h"
#include "../include/cmd.h"
#include "../include/grp.h"
#include "../include/bg.h"
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

int stream(Token* seg, int size, Token* clean, int* csize, int* out, int* out_pos, int* tempout, int bg)
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
        else if(!strcmp(">", seg[x].text) || !strcmp(">>", seg[x].text))
        {
            int flags = O_WRONLY | O_CREAT;
            int fd;

            if(!strcmp(">", seg[x].text))
            {
                flags |= O_TRUNC;
            }
            else
            {
                flags |= O_APPEND;
            }

            fd = open(seg[x+1].text, flags, 0644);
            if(fd < 0)
            {
                printf("cshell: Cannot create file %s\n", seg[x+1].text);
                for(int y=0; y<*out_pos; y++)
                {
                    close(out[y]);
                }
                return -1;
            }

            if(bg)
            {
                if(dup2(fd, STDOUT_FILENO) < 0)
                {
                    close(fd);
                    return -1;
                }
                close(fd);
            }
            else
            {
                if(!enco)
                {
                    *tempout = mkstemp(tmp2);
                    if(*tempout < 0)
                    {
                        close(fd);
                        return -1;
                    }
                    unlink(tmp2);
                    enco = 1;
                }
                out[(*out_pos)++] = fd;
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

int run(Token* tokens, int size, int bg)
{
    Token clean[size];
    int out[size];
    int csize = 0;
    int out_pos = 0;
    int tempout = -1;
    int fd = stream(tokens, size, clean, &csize, out, &out_pos, &tempout, bg);
    if(fd == -1)
    {
        return 1;
    }
    if(csize == 0)
    {
        if(fd >= 0)
            close(fd);

        if(tempout >= 0)
            close(tempout);

        return 1;
    }
    int stdout = dup(STDOUT_FILENO);
    if(stdout < 0)
    {
        if(fd >= 0)
            close(fd);

        if(tempout >= 0)
            close(tempout);

        return 1;
    }
    int std = dup(STDIN_FILENO);
    if(std < 0)
    {
        close(stdout);
        if(fd >= 0)
            close(fd);

        if(tempout >= 0)
            close(tempout);

        return 1;
    }

    if(tempout >= 0)
    {
        if(dup2(tempout, STDOUT_FILENO) < 0)
        {
            close(tempout);
            close(stdout);
            close(std);
            return 1;
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
            return 1;
        }

        close(fd);
    }

    int result;
    if(bg)
    {
        result = cmd_child(clean, csize);
    }
    else
    {
        result = cmd(clean, csize);
    }
    if(!bg && tempout >= 0 && !result)
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
    return result;
}

static int run_pipeline(Token* tokens, int size, int sc, int bg, pid_t* f_pid, pid_t* job_pgid, Process* processes, int* process_count)
{
    Token* stages[sc];
    int stage[sc];

    if(f_pid != NULL)
    {
        *f_pid = -1;
    }

    if(job_pgid != NULL)
    {
        *job_pgid = -1;
    }

    if(process_count != NULL)
    {
        *process_count = 0;
    }

    for(int x=0; x<sc; x++)
    {
        stages[x] = malloc(sizeof(Token)*size);

        if(stages[x] == NULL)
        {
            printf("cshell: Memory allocation failed\n");

            for(int y=0; y<x; y++)
            {
                free(stages[y]);
            }

            return 1;
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

    int pipes[sc-1][2];

    for(int x=0; x<sc-1; x++)
    {
        if(pipe(pipes[x]) < 0)
        {
            printf("cshell: Pipe creation failed\n");

            for(int y=0; y<sc; y++)
            {
                free(stages[y]);
            }

            return 1;
        }
    }

    pid_t pids[sc];
    pid_t first_pid = -1;
    pid_t pgid = -1;

    for(int x=0; x<sc; x++)
    {
        pids[x] = fork();

        if(pids[x] < 0)
        {
            printf("cshell: Fork failed\n");

            for(int y=0; y<sc-1; y++)
            {
                close(pipes[y][0]);
                close(pipes[y][1]);
            }

            for(int y=0; y<sc; y++)
            {
                free(stages[y]);
            }

            return 1;
        }

        else if(pids[x] == 0)
        {
            if(x == 0)
            {
                setpgid(0, 0);
            }
            else
            {
                setpgid(0, pgid);
            }

            if(bg && x == 0)
            {
                int fd = open("/dev/null", O_RDONLY);

                if(fd >= 0)
                {
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
            }

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
                _exit(1);
            }

            if(csize == 0)
            {
                _exit(0);
            }

            int r = cmd_child(clean, csize);

            _exit(r);
        }

        if(x == 0)
        {
            first_pid = pids[x];
            pgid = pids[x];
        }

        setpgid(pids[x], pgid);

        if(processes != NULL)
        {
            processes[x].pid = pids[x];

            if(stage[x] > 0)
            {
                strncpy(processes[x].command, stages[x][0].text, sizeof(processes[x].command)-1);
                processes[x].command[sizeof(processes[x].command)-1] = '\0';
            }
            else
            {
                processes[x].command[0] = '\0';
            }

            processes[x].done = 0;
            processes[x].status = 0;
        }
    }

    if(f_pid != NULL)
    {
        *f_pid = first_pid;
    }

    if(job_pgid != NULL)
    {
        *job_pgid = pgid;
    }

    if(process_count != NULL)
    {
        *process_count = sc;
    }

    for(int x=0; x<sc-1; x++)
    {
        close(pipes[x][0]);
        close(pipes[x][1]);
    }

    if(!bg)
    {
        int status;

        for(int x=0; x<sc; x++)
        {
            if(waitpid(pids[x], &status, 0) < 0)
            {
                perror("waitpid");
            }
        }
    }

    for(int x=0; x<sc; x++)
    {
        free(stages[x]);
    }

    return 0;
}

int disperse(Token* tokens, int size, int bg)
{
    pid_t first_pid = -1;
    pid_t pgid = -1;
    Process processes[100];
    int process_count = 0;
    int sc = count_stages(tokens, size);
    if(sc > 1)
    {
        int r = run_pipeline(tokens, size, sc, bg, &first_pid, &pgid, processes, &process_count);
        if(r < 0)
        {
            return r;
        }
        if(bg)
        {
            int ji = add(pgid, first_pid, processes, process_count);
            if(ji < 0)
            {
                return 1;
            }
            printf("[%d] %d\n", ji, first_pid);
        }
        return r;
    }
    if(bg)
    {
        pid_t pid = fork();
        if(pid < 0)
        {
            perror("fork");
            return 1;
        }
        if(pid == 0)
        {
            setpgid(0, 0);
            int fd = open("/dev/null", O_RDONLY);
            if(fd >= 0)
            {
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            int r = run(tokens, size, 1);
            _exit(r);
        }
        setpgid(pid, pid);
        Process proc;
        proc.pid = pid;
        strncpy(proc.command, tokens[0].text, sizeof(proc.command)-1);
        proc.command[sizeof(proc.command)-1] = '\0';
        proc.done = 0;
        proc.status = 0;
        int ji = add(pid, pid, &proc, 1);
        if(ji < 0)
        {
            return 1;
        }
        printf("[%d] %d\n", ji, pid);
        return 0;
    }
    else
    {
        return run(tokens, size, 0);
    }
}

int grp(Token* tokens, int size)
{
    Token* t = malloc(sizeof(Token)*size);
    if(t == NULL)
    {
        printf("cshell: Memory allocation failed\n");
        return 1;
    }
    int i = 0;
    for(int x=0; x<size; x++)
    {
        if(!strcmp(";", tokens[x].text))
        {
            if(disperse(t, i, 0))
            {
                free(t);
                return 1;
            };
            free(t);
            t = malloc(sizeof(Token)*(size-x-1));
            i = 0;
            if(t == NULL)
            {
                printf("cshell: Memory allocation failed\n");
                return 1;
            }
        }
        else if(!strcmp("&", tokens[x].text))
        {
            if(disperse(t, i, 1))
            {
                free(t);
                return 1;
            }
            free(t);
            t = malloc(sizeof(Token)*(size-x-1));
            if(t == NULL)   
            {
                printf("cshell: Memory allocation failed\n");
                return 1;
            }
            i = 0;
        }
        else
        {
            t[i] = tokens[x];
            i++;
        }
    }
    if(i > 0)
    {
        if(disperse(t, i, 0))
        {
            free(t);
            return 1;
        }
    }
    free(t);
    return 0;
}