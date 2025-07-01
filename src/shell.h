#ifndef __SHELL_H__
#define __SHELL_H__

#include <termios.h>

#define SHELL_RL_BUFSIZE 1024
#define SHELL_TOK_BUFSIZE 64
#define SHELL_TOK_DELIM " \t\r\n\a"
#define HISTORY_SIZE 100
#define MAX_CMD_LEN 1024

/*
  Function Declarations for builtin shell commands:
 */
int shell_cd(char **args);
int shell_help(char **args);
int shell_exit(char **args);

void enable_raw_mode(struct termios *original);
void disable_raw_mode(struct termios *original);

#endif // __SHELL_H__