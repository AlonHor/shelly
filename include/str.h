#ifndef STR_H
#define STR_H

void to_lowercase(char str[], char stop);
void trim_linebreak(char str[]);

struct SplitResult
{
  char **tokens;
  int count;
};

struct SplitResult split_command(char cmd[], const char delim[]);

#endif