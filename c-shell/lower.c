#include <ctype.h>

void lower(char *str) 
{
    for (int i = 0; str[i] != '\0'; i++) 
    {
        str[i] = tolower((unsigned char)str[i]);
    }
}