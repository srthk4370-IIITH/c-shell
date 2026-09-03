#include "../include/echo.h"
#include "../include/token.h"
#include "../include/lower.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/locate.h"
#include "../include/cmd.h"
#include "../include/peek.h"
#include "../include/bg.h"
#include "../include/bg.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>


char* search(char* ex)  
{
    char* path_env = getenv("PATH");
    if(path_env == NULL)
    {
        return NULL;
    }
    char* path_copy = strdup(path_env);
    if(path_copy == NULL)
    {
        return NULL;
    }
    char* directory = strtok(path_copy, ":");
    while(directory != NULL)
    {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", directory, ex);
        if(access(path, X_OK) == 0)
        {
            char* result = strdup(path);
            free(path_copy);
            return result;
        }
        directory = strtok(NULL, ":");
    }
    free(path_copy);
    return NULL;
}

int execute(char* path, char** argv)
{
    pid_t pid = fork();
    if(pid < 0)
    {
        perror("fork");
        return 1;
    }
    if(pid == 0)
    {
        execv(path, argv);
        printf("cshell: Command not found(%s)\n", argv[0]);
        _exit(127);
    }
    int status;
    if(waitpid(pid, &status, 0) < 0)
    {
        perror("waitpid");
        return 1;
    }
    if(WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
        return 0;
    }
    return 1;
}

int execute_child(char* path, char** argv)
{
    execv(path, argv);
    printf("cshell: Command not found(%s)\n", argv[0]);
    _exit(127);
}

int external(char** argv)
{
    char* command = argv[0];
    if(command[0] == '%')
    {
        command++;
        char* path = search(command);
        if(path != NULL)
        {
            argv[0] = command;
            int result = execute(path, argv);
            free(path);
            return result;
        }
        else
        {
            printf("cshell: Command not found(%s)\n", argv[0]);
            return 1;
        }
    }
    else if(strchr(argv[0], '/') != NULL)
    {
        if(!access(argv[0], X_OK))
        {
            return execute(argv[0], argv);
        }
        else
        {
            printf("cshell: Command not found(%s)\n", argv[0]);
            return 1;
        }
    }
    else
    {
        char path[1025];
        snprintf(path, 1024, "./%s", command);
        if(!access(path, X_OK))
        {
            return execute(path, argv);
        }
        char* ex_path = search(command);
        if(ex_path != NULL)
        {
            int result = execute(ex_path, argv);
            free(ex_path);
            return result;
        }
        else
        {
            printf("cshell: Command not found(%s)\n", command);
            return 1;
        }
    }
}

int external_child(char** argv)
{
    char* command = argv[0];
    if(command[0] == '%')
    {
        command++;
        char* path = search(command);
        if(path != NULL)
        {
            argv[0] = command;
            execute_child(path, argv);
            free(path);
        }
        else
        {
            printf("cshell: Command not found(%s)\n", argv[0]);
            _exit(127);
        }
    }
    else if(strchr(argv[0], '/') != NULL)
    {
        if(!access(argv[0], X_OK))
        {
            execute_child(argv[0], argv);
        }
        else
        {
            printf("cshell: Command not found(%s)\n", argv[0]);
            _exit(127);
        }
    }
    else
    {
        char path[1025];
        snprintf(path, 1024, "./%s", command);
        if(!access(path, X_OK))
        {
            execute_child(path, argv);
        }
        char* ex_path = search(command);
        if(ex_path != NULL)
        {
            execute_child(ex_path, argv);
            free(ex_path);
        }
        else
        {
            printf("cshell: Command not found(%s)\n", command);
            _exit(127);
        }
    }
    return 0;
}

int cmd(Token* tokens, int size)
{
    if(size> 0)
    {
        lower(tokens[0].text);
        if(!strcmp("echo", tokens[0].text))
        {
            return echo(tokens, size);
        }
        else if(!strcmp("hop", tokens[0].text))
        {
            return hop(tokens, size);
        }
        else if(!strcmp("reveal", tokens[0].text))
        {
            return reveal(tokens, size);
        }
        else if(!strcmp("locate", tokens[0].text))
        {
            return locate(tokens, size);
        }
        else if(!strcmp("peek", tokens[0].text))
        {
            return peek(tokens, size);
        }
        else if(!strcmp("activities", tokens[0].text))
        {
            return activities();
        }
        else
        {
            char* argv[size+1];
            for(int x=0; x<size; x++)
            {
                argv[x] = tokens[x].text;
            }
            argv[size] = NULL;
            return external(argv);
        }
    }
    return 0;
}

int cmd_child(Token* tokens, int size)
{
    if(size> 0)
    {
        lower(tokens[0].text);
        if(!strcmp("echo", tokens[0].text))
        {
            return echo(tokens, size);
        }
        else if(!strcmp("hop", tokens[0].text))
        {
            return hop(tokens, size);
        }
        else if(!strcmp("reveal", tokens[0].text))
        {
            return reveal(tokens, size);
        }
        else if(!strcmp("locate", tokens[0].text))
        {
            return locate(tokens, size);
        }
        else if(!strcmp("peek", tokens[0].text))
        {
            return peek(tokens, size);
        }
        else
        {
            char* argv[size+1];
            for(int x=0; x<size; x++)
            {
                argv[x] = tokens[x].text;
            }
            argv[size] = NULL;
            return external_child(argv);
        }
    }
    return 0;
}