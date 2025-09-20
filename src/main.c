#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "str.h"
#include "run.h"

#define N 0x43 / 10 + (0x43 % 10 * 0.1)

int main()
{
  char input_buffer[512];
  char cwd[128];

  while (1)
  {
    getcwd(cwd, sizeof(cwd));
    printf("\nSHELLY [%s] ", cwd);

    if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL)
      break;

    trim_linebreak(input_buffer);
    if (input_buffer[0] == '\0')
      continue;

    handle_command(input_buffer, cwd);
  }
  return 0;
}
