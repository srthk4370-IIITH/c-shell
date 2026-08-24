#include "include/echo.h"
#include "include/token.h"
#include "include/lower.h"
#include "include/hop.h"
#include "include/reveal.h"
#include "include/locate.h"
#include "include/cmd.h"
#include "include/peek.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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

void execute(char* path, char** argv)
{
    pid_t pid = fork();
    if(pid < 0)
    {
        perror("fork");
        return;
    }
    if(pid == 0)
    {
        execv(path, argv);
        printf("cshell: Command not found(%s)\n", argv[0]);
        _exit(127);
    }
    waitpid(pid, NULL, 0);
}

void external(char** argv)
{
    char* command = argv[0];
    if(command[0] == '%')
    {
        command++;
        char* path = search(command);
        if(path != NULL)
        {
            argv[0] = command;
            execute(path, argv);
            free(path);
        }
        else
        {
            printf("cshell: Command not found(%s)\n", argv[0]);
        }
    }
    else if(strchr(argv[0], '/') != NULL)
    {
        if(!access(argv[0], X_OK))
        {
            execute(argv[0], argv);
        }
        else
        {
            printf("cshell: Command not found(%s)\n", argv[0]);
        }
    }
    else
    {
        char path[1025];
        snprintf(path, 1024, "./%s", command);
        if(!access(path, X_OK))
        {
            execute(path, argv);
            return;
        }
        char* ex_path = search(command);
        if(ex_path != NULL)
        {
            execute(ex_path, argv);
            free(ex_path);
        }
        else
        {
            printf("cshell: Command not found(%s)\n", command);
        }
    }
}

void cmd(Token* tokens, int size)
{
    if(size> 0)
    {
        lower(tokens[0].text);
        if(!strcmp("echo", tokens[0].text))
        {
            echo(tokens, size);
        }
        else if(!strcmp("hop", tokens[0].text))
        {
            hop(tokens, size);
        }
        else if(!strcmp("reveal", tokens[0].text))
        {
            reveal(tokens, size);
        }
        else if(!strcmp("locate", tokens[0].text))
        {
            locate(tokens, size);
        }
        else if(!strcmp("peek", tokens[0].text))
        {
            peek(tokens, size);
        }
        else
        {
            char* argv[size+1];
            for(int x=0; x<size; x++)
            {
                argv[x] = tokens[x].text;
            }
            argv[size] = NULL;
            external(argv);
        }
    }
}
