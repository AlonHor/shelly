#include "str.h"
#include "internal.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>

extern char **environ;

void cd(char *args[])
{
  if (chdir(args[0]))
    printf("Error: Path not found\n");
}

void dir(char *args[])
{
  char path[256] = ".";
  if (args[0] != NULL)
    memcpy(path, args[0], 256);

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(path)) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
    {
      char type[5] = "FILE";
      char spacing[2] = "";
      if (ent->d_type == 4)
      {
        memcpy(type, "DIR", 4);
        memcpy(spacing, " ", 2);
      }
      printf("<%s>%s      %s\n", type, spacing, ent->d_name);
    }
    closedir(dir);
  }
  else
    printf("Error: Path not found\n");
}

void copy(char *args[])
{
  char *src = args[0];
  char *dst = args[1];

  FILE *src_f, *dst_f;
  if ((src_f = fopen(src, "r")) == NULL)
    return;
  if ((dst_f = fopen(dst, "w")) == NULL)
  {
    fclose(src_f);
    return;
  }

  char ch;
  while ((ch = fgetc(src_f)) != EOF)
    fputc(ch, dst_f);

  fclose(src_f);
  fclose(dst_f);
}

void set(char *args[])
{
  if (args[0] == NULL)
  {
    for (char **env = environ; *env != NULL; env++)
      printf("%s\n", *env);
    return;
  }

  char *value;
  if ((value = getenv(args[0])) != NULL)
    printf("%s=%s\n", args[0], value);
  else
    printf("Error: Environment variable not found\n");
}

void foo(char *args[])
{
  (void)args;
  printf("bar!\n");
}

typedef void (*func_t)(char **);

struct FuncMap
{
  const char *cmd;
  func_t func;
};

struct FuncMap internal_proc_functions[] = {
    {"foo", foo},
    {"dir", dir},
    {"copy", copy},
    {"set", set},
    {NULL, NULL}};

struct FuncMap internal_noproc_functions[] = {
    {"cd", cd},
    {NULL, NULL}};

func_t get_internal(const char *cmd, int proc)
{
  if (proc)
  {
    for (int i = 0; internal_proc_functions[i].cmd != NULL; i++)
      if (strcmp(cmd, internal_proc_functions[i].cmd) == 0)
        return internal_proc_functions[i].func;
  }
  else
  {
    for (int i = 0; internal_noproc_functions[i].cmd != NULL; i++)
      if (strcmp(cmd, internal_noproc_functions[i].cmd) == 0)
        return internal_noproc_functions[i].func;
  }
  return NULL;
}

int try_handle_path_command(const char *cmd, SplitResult args_split, char *path_value)
{
  SplitResult path_split = split_command(path_value, ":", 512);

  DIR *dir;
  struct dirent *ent;
  for (int i = 0; i < path_split.count; i++)
  {
    char *path = path_split.tokens[i];
    if ((dir = opendir(path)) != NULL)
    {
      while ((ent = readdir(dir)) != NULL)
      {
        if (ent->d_type != 8)
          continue;

        if (strcmp(ent->d_name, cmd) == 0)
        {
          char *exec_cmd = malloc(sizeof(char) * 1024);
          sprintf(exec_cmd, "%s/%s", path, cmd);

          char **argv = malloc(sizeof(char *) * 512);
          argv[0] = exec_cmd;

          for (int i = 1; i < args_split.count + 1; i++)
            argv[i] = args_split.tokens[i - 1];

          execvp(exec_cmd, argv);

          free(path_split.tokens);
          free(exec_cmd);

          closedir(dir);

          return 1;
        }
      }
    }
    closedir(dir);
  }

  free(path_split.tokens);

  return 0;
}

int handle_internal(char *full_command, int proc)
{
  SplitResult full_command_split = split_command(full_command, " ", 1);

  char *command_name = full_command_split.tokens[0];
  char *args_str = command_name + strlen(command_name) + 1;
  char args_str_copy[strlen(args_str) + 1];
  memcpy(args_str_copy, args_str, strlen(args_str) + 1);

  SplitResult args_split = split_command(args_str_copy, " ", 128);

  func_t internal = get_internal(command_name, proc);
  int found_internal = 0;
  if (internal != NULL)
  {
    internal(args_split.tokens);
    found_internal = 1;
  }

  free(full_command_split.tokens);
  free(args_split.tokens);

  return found_internal;
}

int handle_path(char *full_command)
{
  char full_command_copy[strlen(full_command) + 1];
  memcpy(full_command_copy, full_command, strlen(full_command) + 1);
  SplitResult full_command_split = split_command(full_command, " ", 1);

  char *command_name = full_command_split.tokens[0];
  char *args_str = command_name + strlen(command_name) + 1;
  char args_str_copy[strlen(args_str) + 1];
  memcpy(args_str_copy, args_str, strlen(args_str) + 1);

  SplitResult args_split = split_command(args_str_copy, " ", 128);

  if (strcmp(full_command_split.tokens[0], full_command_copy) == 0)
    args_split.count = 0;

  char *path_value;
  if ((path_value = getenv("PATH")) == NULL)
    return 0;

  char path_value_copy[strlen(path_value) + 1];
  memcpy(path_value_copy, path_value, strlen(path_value) + 1);

  int found_path = try_handle_path_command(command_name, args_split, path_value_copy);

  free(full_command_split.tokens);
  free(args_split.tokens);

  return found_path;
}