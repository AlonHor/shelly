#ifndef STR_H
#define STR_H

void to_lowercase(char str[], char stop);
void trim_linebreak(char str[]);

typedef struct SplitResult
{
  char **tokens;
  int count;
} SplitResult;

SplitResult split_command(char cmd[], const char delim[]);

#endif