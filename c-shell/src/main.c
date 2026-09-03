#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include "../include/lexer.h"
#include "../include/token.h"
#include "../include/bg.h"

#define RED     "\033[1m\033[95m"
#define BLUE     "\033[1m\033[96m"
#define RESET     "\033[0m"

char* home;

void sig()
{
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);
}

int main()
{
    struct passwd *pw = getpwuid(getuid());
    char* user = pw->pw_name;
    char hostname[HOST_NAME_MAX+1];
    gethostname(hostname, sizeof(hostname));
    home = getcwd(NULL, 0);
    tokenize("hop .");
    tokenize("echo \"Welcome to my C-Shell\"");
    sig();
    while(1)
    {
        char *cwd = getcwd(NULL, 0);
        if(strncmp(cwd, home, strlen(home)) == 0)
        {
            char* temp = cwd;
            cwd = malloc(strlen(cwd) - strlen(home) + 2);
            cwd[0] = '~';
            strcpy(cwd+1, temp+strlen(home));
            free(temp);
        }
        printf(RED"<%s@%s:"BLUE"%s> "RESET, user, hostname, cwd);
        char s[100];
        if(fgets(s, 99, stdin) == NULL)
        {
            clearerr(stdin);
            printf("\n");
            continue;
        }
        s[strcspn(s, "\n")] = '\0';
        tokenize(s);
    }
    return 0;
}