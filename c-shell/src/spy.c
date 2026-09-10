#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include "../include/token.h"

void print(int pid, char* fd, char* type, char* path)
{
    printf("%d\t%s\t%s\t%s\n", pid, fd, type, path);
}

int spy(Token* tokens, int size)
{
    if(size != 1 && size != 2)
    {
        printf("spy: invalid syntax\n");
        return 1;
    }
    pid_t pid;
    if(size == 2)
    {
        char* end;
        pid = (int)strtol(tokens[1].text, &end, 10);
        if(*end != '\0' || pid < 0)
        {
            printf("spy: invalid syntax\n");
            return 1;
        }
    }
    else
    {
        pid = getpid();
    }
    char path[128];
    struct stat statbuf;
    snprintf(path, sizeof(path), "/proc/%d", pid);
    if(stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode))
    {
        printf("PID\tFD\tTYPE\tPATH\n");
        snprintf(path, sizeof(path), "/proc/%d/cwd", pid);
        char buffer[PATH_MAX];
        ssize_t n = readlink(path, buffer, PATH_MAX-1);
        if(n>=0)
        {
            buffer[n] = '\0';
        }
        print(pid, "cwd", "DIR", buffer);
        snprintf(path, sizeof(path), "/proc/%d/exe", pid);
        n = readlink(path, buffer, PATH_MAX-1);
        if(n>=0)
        {
            buffer[n] = '\0';
        }
        print(pid, "txt", "REG", buffer);
        return 0;
    }
    else
    {
        printf("spy: no such process\n");
        return 1;
    }
}

