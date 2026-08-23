#include <string.h>
#include <stdlib.h>
#include "include/token.h"
#include "include/parser.h"
#include "include/lexer.h"
#include <stdio.h>

void tokencpy(Token* tokens, int start, int x, TID t, int size, char* s)
{
    tokens[size].text = (char*)malloc(x-start+1);
    memcpy(tokens[size].text, &s[start], x-start);
    tokens[size].text[x-start] = '\0';
    tokens[size].type = t;
}

void tokenize(char* s)
{
    int l = strlen(s);
    Token tokens[1000];
    int size= 0;
    int start = 0;
    int iq = 0;
    int diq = 0, siq = 0;
    for(int x=0; x<=l; x++)
    {
        char c = s[x];
        if(c == '\\' && s[x+1] == '\0')
        {
            printf("cshell: Invalid Syntax\n");
            return;
        }
        if(!iq)
        {
            if(c == ' ' || c == '\0')
            {
                if(start != x)
                {
                    tokencpy(tokens, start, x, WORD, size, s);
                    size++;
                }
                start = x+1;
            }
            else if(c == '|')
            {
                if(start != x)
                {
                    tokencpy(tokens, start, x, WORD, size, s);
                    size++;
                }
                tokencpy(tokens, x, x+1, OP_PIPE, size, s);
                size++;
                start = x+1;
            }
            else if(c == '&')
            {
                if(start != x)
                {
                    tokencpy(tokens, start, x, WORD, size, s);
                    size++;
                }
                tokencpy(tokens, x, x+1, OP_AMP, size, s);
                size++;
                start = x+1;
            } 
            else if(c == ';')
            {
                if(start != x)
                {
                    tokencpy(tokens, start, x, WORD, size, s);
                    size++;
                }
                tokencpy(tokens, x, x+1, OP_SEMI, size, s);
                size++;
                start = x+1;
            }
            else if(c == '<')
            {
                if(start != x)
                {
                    tokencpy(tokens, start, x, WORD, size, s);
                    size++;
                }
                tokencpy(tokens, x, x+1, OP_LT, size, s);
                size++;
                start = x+1;
            }
            else if(c == '>')
            {
                if(start != x)
                {
                    tokencpy(tokens, start, x, WORD, size, s);
                    size++;
                }
                if(s[x+1] == '>')
                {
                    tokencpy(tokens, x, x+2, OP_GTGT, size, s);
                    size++;
                    x++;
                    start = x+1;
                }
                else
                {
                    tokencpy(tokens, x, x+1, OP_GT, size, s);
                    size++;
                    start = x+1;
                }
            }
            else if(c == '"')
            {
                if(start != x)
                {
                    tokencpy(tokens, start, x, WORD, size, s);
                    size++;
                }
                iq = 1;
                diq = 1;
                start = x+1;
            }
            else if(c == '\'')
            {
                if(start != x)
                {
                    tokencpy(tokens, start, x, WORD, size, s);
                    size++;
                }
                iq = 1;
                siq = 1;
                start = x+1;
            }
        }
        else
        {
            if(diq && c == '"')
            {
                tokencpy(tokens, start, x, WORD, size, s);
                size++;
                iq = 0;
                diq = 0;
                start = x+1;
            }
            else if(siq && c == '\'')
            {
                tokencpy(tokens, start, x, WORD, size, s);
                size++;
                iq = 0;
                siq = 0;
                start = x+1;
            }
        }
    }
    if(iq)
    {
        printf("cshell: Invalid Syntax\n");
        return;
    }
    //Print the tokens array
    /*for(int x=0; x<size; x++)
    {
        printf("Token %d: Type: %d, Text: %s\n", x  + 1, tokens[x].type, tokens[x].text);
    }*/
    parse(tokens, size);
}
