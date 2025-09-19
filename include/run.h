#ifndef RUN_H
#define RUN_H

void handle_command(char *cmd, char cwd[]);
void run(char *cmd, int stdin_fd, int stdout_fd, char *cwd);

#endif