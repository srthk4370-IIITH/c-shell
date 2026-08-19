#ifndef TOKEN_H
#define TOKEN_H

typedef enum TID
{
    OP_PIPE,
    OP_AMP,
    OP_SEMI,
    OP_LT,
    OP_GT,
    OP_GTGT,
    WORD
}TID;

typedef struct Token
{
    char* text;
    TID type;
}Token;

#endif