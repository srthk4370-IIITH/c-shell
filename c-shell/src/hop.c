#include "../include/token.h"
#include "../include/lower.h"
#include "../include/hop.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#define SIZE 10

int pos = 0;                 // current number of entries
char *recent[SIZE];          // recent[0] = most recent
int freq[SIZE];              // frequency of each entry
int standings[SIZE];         // indices sorted by score
char path[PATH_MAX];
void path_init()
{
    strcpy(path, home);
    strcat(path, "/.cshell_history");
}


void save_state()
{
    FILE* f = fopen(path, "w");
    if(f == NULL)
    {
        printf("cshell: Unable to save state\n");
        return;
    }
    fprintf(f, "%d\n", pos);
    for(int x=0; x<pos; x++)
    {
        fprintf(f, "%d %d %s\n", standings[x], freq[standings[x]], recent[standings[x]]);
    }
    fclose(f);
}

void load()
{
    FILE* f = fopen(path, "r");
    if(f == NULL)
    {
        return;
    }
    fscanf(f, "%d\n", &pos);
    for(int x=0; x<pos; x++)
    {
        int index, frequency;
        char buffer[PATH_MAX];
        fscanf(f, "%d %d %[^\n]\n", &index, &frequency, buffer);
        standings[x] = index;
        recent[index] = strdup(buffer);
        freq[index] = frequency;
    }
    fclose(f);
}

void update_standings()
{
    for (int i = 0; i < pos; i++)
        standings[i] = i;

    for (int i = 0; i < pos - 1; i++)
    {
        for (int j = i + 1; j < pos; j++)
        {
            int score_i = pos - standings[i] + 2 * freq[standings[i]];
            int score_j = pos -  standings[j] + 2 * freq[standings[j]];

            if (score_j > score_i)
            {
                int temp = standings[i];
                standings[i] = standings[j];
                standings[j] = temp;
            }
        }
    }
    save_state();
}


void update(char *cwd)
{
    int old_pos = -1;
    int old_freq = 0;

    // Check whether cwd already exists
    for (int i = 0; i < pos; i++)
    {
        if (strcmp(recent[i], cwd) == 0)
        {
            old_pos = i;
            old_freq = freq[i];
            break;
        }
    }

    if (old_pos != -1)
    {
        for (int i = old_pos; i > 0; i--)
        {
            recent[i] = recent[i - 1];
            freq[i] = freq[i - 1];
        }
        recent[0] = cwd;
        freq[0] = old_freq + 1;
    }
    else
    {
        // New entry
        if (pos < SIZE)
            pos++;
        else if(recent[SIZE - 1] != home)
            free(recent[SIZE - 1]);
        for (int i = pos - 1; i > 0; i--)
        {
            recent[i] = recent[i - 1];
            freq[i] = freq[i - 1];
        }

        recent[0] = cwd;
        freq[0] = 1;
    }
    update_standings();
}

void hop(Token* tokens, int size)
{
    path_init();
    load();
    if(size == 1)
    {
        chdir(home);
        update(home);
    }
    for(int x=1; x<size; x++)
    {
        char *current = getcwd(NULL, 0);
        if(!strcmp("~", tokens[x].text))
        {
            chdir(home);
            update(home);
        }
        else if(strcmp(".", tokens[x].text))
        {
            if(!strcmp("-", tokens[x].text))
            {
                char* o = getenv("OLDPWD");
                printf("%s\n", o);
                if(o != NULL)
                {
                    if(chdir(o))
                    {
                        printf("hop: No such directory\n");
                    }
                    else
                    {
                        update(o);
                        setenv("OLDPWD", current, 1);
                    }
                }
            }
            else if(!chdir(tokens[x].text))
            {
                char* o = getcwd(NULL, 0);
                update(o);
                setenv("OLDPWD", current, 1);
            }
            else
            {
                int flag = 0;
                for(int y=0; y<pos; y++)
                {
                    if(strstr(recent[standings[y]], tokens[x].text))
                    {
                        if(!chdir(recent[standings[y]]))
                        {
                            char* o = getcwd(NULL, 0);
                            update(o);
                            setenv("OLDPWD", current, 1);
                            flag = 1;
                            break;
                        }
                    }
                }
                if(!flag)
                {
                    printf("hop: Command not found\n");
                }
            }
        }
        else
        {
            update(current);
        }
    }
    for(int x=0; x<pos; x++)
    {
        free(recent[x]);
    }
}