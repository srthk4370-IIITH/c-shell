#ifndef RESULT_H
#define RESULT_H

#include <stdlib.h>
#include <string.h>

typedef struct
{
    char *output;
    int status;
} CommandResult;

static inline CommandResult result_text(const char *text, int status)
{
    size_t length = strlen(text) + 1;
    char *output = malloc(length);
    if(output == NULL)
    {
        return (CommandResult){NULL, 1};
    }
    memcpy(output, text, length);
    return (CommandResult){output, status};
}

#endif