#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <limits.h>
#include <string.h>
#include "include/lexer.h"

#define RED     "\033[1m\033[95m"
#define BLUE     "\033[1m\033[96m"
#define RESET     "\033[0m"

int main()
{
    struct passwd *pw = getpwuid(getuid());
    char* user = pw->pw_name;
    char hostname[HOST_NAME_MAX+1];
    gethostname(hostname, sizeof(hostname));
    tokenize("hop .");
    tokenize("echo \"Welcome to my C-Shell\"");
    while(1)
    {
        char *cwd = getcwd(NULL, 0);
        printf(RED"<%s@%s:"BLUE"%s> "RESET, user, hostname, cwd);
        char s[100];
        fgets(s, 99, stdin);
        s[strcspn(s, "\n")] = '\0';
        tokenize(s);
    }
    return 0;
}