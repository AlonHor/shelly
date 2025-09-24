#include "internal.h"
#include "str.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
static const int DEBUG = 0;

void cd(char *args[]) {
  if (chdir(args[0]))
    printf("Error: Path not found\n");
}

void dir(char *args[]) {
  char path[256] = ".";
  if (args[0] != NULL) {
    strncpy(path, args[0], sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';
  }

  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(path)) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      char type[5] = "FILE";
      char spacing[2] = "";
      if (ent->d_type == 4) {
        strncpy(type, "DIR", 4);
        strncpy(spacing, " ", 2);
      }
      printf("<%s>%s      %s\n", type, spacing, ent->d_name);
    }
    closedir(dir);
  } else
    printf("Error: Path not found\n");
}

void copy(char *args[]) {
  char *src = args[0];
  char *dst = args[1];

  FILE *src_f, *dst_f;
  if ((src_f = fopen(src, "r")) == NULL)
    return;
  if ((dst_f = fopen(dst, "w")) == NULL) {
    fclose(src_f);
    return;
  }

  char ch;
  while ((ch = fgetc(src_f)) != EOF)
    fputc(ch, dst_f);

  fclose(src_f);
  fclose(dst_f);
}

void set(char *args[]) {
  if (args[0] == NULL) {
    for (char **env = environ; *env != NULL; env++)
      printf("%s\n", *env);
    return;
  }

  char *value;
  if ((value = getenv(args[0])) != NULL)
    printf("%s=%s\n", args[0], value);
  else {
    char args_copy[strlen(args[0]) + 1];
    strcpy(args_copy, args[0]);

    SplitResult key_value_split = split_command(args[0], "=", 2);

    if (strcmp(key_value_split.tokens[0], args_copy) == 0) {
      printf("Error: Environment variable '%s' not found\n", args[0]);
      free(key_value_split.tokens);
      return;
    }

    char key[128];
    char value[128];

    strcpy(key, key_value_split.tokens[0]);
    strcpy(value, key_value_split.tokens[1]);

    setenv(key, value, 1);

    free(key_value_split.tokens);
  }
}

void help(char *args[]) {
  if (args[0] == NULL) {
    printf("Welcome to the help menu!\n");
    printf("Available commands:\n");
    printf("  dir  - List the contents of the current directory.\n");
    printf("  help - Show information about available commands.\n");
    printf("  set  - Set or display environment variables.\n");
    printf("  cd   - Change the current working directory.\n");
    return;
  }

  if (strcmp(args[0], "dir") == 0) {
    printf("dir: Lists the contents of a directory.\n");
    printf("Usage: dir [directory]\n");
    printf("If no directory is specified, shows contents on the current "
           "directory.\n");
  } else if (strcmp(args[0], "help") == 0) {
    printf("help: Shows information about available commands.\n");
    printf("Usage: help [command]\n");
    printf("If a command is specified, displays detailed help for that "
           "command.\n");
  } else if (strcmp(args[0], "set") == 0) {
    printf("set: Sets or displays environment variables.\n");
    printf("Usage: set [VARIABLE=VALUE]\n");
    printf("If no arguments are given, displays all environment variables.\n");
  } else if (strcmp(args[0], "cd") == 0) {
    printf("cd: Changes the current working directory.\n");
    printf("Usage: cd [directory]\n");
  } else {
    printf("No help available for '%s'.\n", args[0]);
  }
}

void type(char *args[]) {
  if (args[0] == NULL) {
    printf("Error: No file specified\n");
    return;
  }

  FILE *file = fopen(args[0], "r");
  if (!file) {
    printf("Error: Cannot open file '%s'\n", args[0]);
    return;
  }

  int c;
  while ((c = fgetc(file)) != EOF)
    putchar(c);
  fclose(file);
}

void cls(char *args[]) {
  (void)args;
  system("clear");
  printf("\033[2J\033[H");
}

void exit_i(char *args[]) {
  (void)args;
  exit(0);
}

typedef void (*func_t)(char **);

struct FuncMap {
  const char *cmd;
  func_t func;
};

struct FuncMap internal_proc_functions[] = {
    {"dir", dir}, {"copy", copy}, {"help", help}, {"type", type}, {NULL, NULL}};

struct FuncMap internal_noproc_functions[] = {
    {"cd", cd}, {"set", set}, {"cls", cls}, {"exit", exit_i}, {NULL, NULL}};

func_t get_internal(const char *cmd, int proc) {
  if (proc) {
    for (int i = 0; internal_proc_functions[i].cmd != NULL; i++)
      if (strcmp(cmd, internal_proc_functions[i].cmd) == 0)
        return internal_proc_functions[i].func;
  } else {
    for (int i = 0; internal_noproc_functions[i].cmd != NULL; i++)
      if (strcmp(cmd, internal_noproc_functions[i].cmd) == 0)
        return internal_noproc_functions[i].func;
  }
  return NULL;
}

int try_handle_path_command(const char *cmd, SplitResult args_split,
                            char *path_value) {
  SplitResult path_split = split_command(path_value, ":", 512);

  DIR *dir;
  struct dirent *ent;
  for (int i = 0; i < path_split.count; i++) {
    char *path = path_split.tokens[i];
    if ((dir = opendir(path)) != NULL) {
      while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type != 8)
          continue;

        if (strcmp(ent->d_name, cmd) == 0) {
          char *exec_cmd = malloc(sizeof(char) * 1024);
          sprintf(exec_cmd, "%s/%s", path, cmd);

          char **argv = malloc(sizeof(char *) * 512);
          argv[0] = exec_cmd;

          for (int i = 1; i < args_split.count + 1; i++)
            argv[i] = args_split.tokens[i - 1];

          const char *cmd_ptr = cmd;
          while (*cmd_ptr != '0') {
            if (strcmp(cmd_ptr, ".py") == 0) {
              strcpy(exec_cmd, "python");
              char argv1[256];
              sprintf(argv1, "%s/%s", path, cmd);
              argv[1] = argv1;
              argv[2] = NULL;
            }
            cmd_ptr++;
          }

          if (DEBUG)
            printf("DEBUG: [%s], argv[0]='%s' argv[1]='%s' argv[2]='%s'\n",
                   exec_cmd, argv[0], argv[1], argv[2]);

          pid_t pid = fork();
          if (pid == 0) {
            if (execvp(exec_cmd, argv) == -1) {
              perror("execvp failed");
              exit(EXIT_FAILURE);
            }
          } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
          } else
            perror("fork failed");

          free(path_split.tokens);
          free(exec_cmd);
          free(argv);

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

int handle_internal(char *full_command, int proc) {
  char full_command_copy[strlen(full_command) + 1];
  strcpy(full_command_copy, full_command);
  SplitResult full_command_split = split_command(full_command, " ", 1);

  char *command_name = full_command_split.tokens[0];
  char *args_str = command_name + strlen(command_name) + 1;
  char args_str_copy[strlen(args_str) + 1];
  strcpy(args_str_copy, args_str);

  SplitResult args_split = split_command(args_str_copy, " ", 128);

  func_t internal = get_internal(command_name, proc);
  int found_internal = 0;
  if (internal != NULL) {
    if (strcmp(command_name, full_command_copy) == 0) {
      char *null_char = NULL;
      char **null_args = &null_char;
      internal(null_args);
    } else
      internal(args_split.tokens);
    found_internal = 1;
  }

  free(full_command_split.tokens);
  free(args_split.tokens);

  return found_internal;
}

int handle_path(char *full_command) {
  char full_command_copy[strlen(full_command) + 1];
  strcpy(full_command_copy, full_command);
  SplitResult full_command_split = split_command(full_command, " ", 1);

  char *command_name = full_command_split.tokens[0];
  char *args_str = command_name + strlen(command_name) + 1;
  char args_str_copy[strlen(args_str) + 1];
  strcpy(args_str_copy, args_str);

  SplitResult args_split = split_command(args_str_copy, " ", 128);

  if (strcmp(full_command_split.tokens[0], full_command_copy) == 0)
    args_split.count = 0;

  const char N_TERM = '\0';

  const char *path_value;
  if ((path_value = getenv("PATH")) == NULL)
    path_value = &N_TERM;

  const char *my_path_value;
  if ((my_path_value = getenv("SHELLY_PY_SCRIPTS")) == NULL)
    my_path_value = &N_TERM;

  char combined_path_value[strlen(path_value) + strlen(my_path_value) + 2];
  sprintf(combined_path_value, "%s:%s", path_value, my_path_value);

  int found_path =
      try_handle_path_command(command_name, args_split, combined_path_value);

  free(full_command_split.tokens);
  free(args_split.tokens);

  return found_path;
}