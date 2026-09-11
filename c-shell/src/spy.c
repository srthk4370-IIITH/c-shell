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
        snprintf(path, sizeof(path), "/proc/%d/maps", pid);
        FILE* f = fopen(path, "r");
        if(f != NULL)
        {
            char line[1024];
            while(fgets(line, 1023, f) != NULL)
            {
                char pathname[1024];
                int n = sscanf(line, "%*s %*s %*s %*s %*lu %s", pathname);
                if(n == 1)
                {
                    struct stat st;
                    if(stat(pathname, &st) == 0)
                    {
                        if(S_ISREG(st.st_mode))
                            print(pid, "mem", "REG", pathname);
                        else if(S_ISDIR(st.st_mode))
                            print(pid, "mem", "DIR", pathname);
                        else if(S_ISCHR(st.st_mode))
                            print(pid, "mem", "chR", pathname);
                        else if(S_ISBLK(st.st_mode))
                            print(pid, "mem", "BLK", pathname);
                        else if(S_ISFIFO(st.st_mode))
                            print(pid, "mem", "FIFO", pathname);
                        else if(S_ISSOCK(st.st_mode))
                            print(pid, "mem", "SOCK", pathname);
                    }
                }
            }
        }
        snprintf(path, sizeof(path), "/proc/%d/fd", pid);
        DIR* dir = opendir(path);
        struct dirent *entry;
        while((entry = readdir(dir)) != NULL)
        {
            char path1[1025];
            snprintf(path1, 1024, "/proc/%d/fd/%s", pid, entry->d_name);
            char target[PATH_MAX];
            n = readlink(path1, target, PATH_MAX-1);
            if(n > 0)
            {
                target[n] = '\0';
                struct stat st;
                if(stat(path1, &st) == 0)
                {
                    if(S_ISREG(st.st_mode))
                        print(pid, entry->d_name, "REG", target);
                    else if(S_ISDIR(st.st_mode))
                        print(pid, entry->d_name, "DIR", target);
                    else if(S_ISCHR(st.st_mode))
                        print(pid, entry->d_name, "CHR", target);
                    else if(S_ISBLK(st.st_mode))
                        print(pid, entry->d_name, "BLK", target);
                    else if(S_ISFIFO(st.st_mode))
                        print(pid, entry->d_name, "FIFO", target);
                    else if(S_ISSOCK(st.st_mode))
                        print(pid, entry->d_name, "SOCK", target);
                }
            }
        }
        return 0;
    }
    else
    {
        printf("spy: no such process\n");
        return 1;
    }
}

