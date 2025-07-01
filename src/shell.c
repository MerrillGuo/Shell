#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>    // open()
#include <sys/stat.h> // O_CREAT等宏
#include <ctype.h>

#include "shell.h"

/* Global declartion function */
char *history[HISTORY_SIZE];
int history_head = 0;    // where to store next
int history_size = 0;    // number of commands stored (max = HISTORY_SIZE)
int history_index = -1;  // for arrow key navigation

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &shell_cd,
  &shell_help,
  &shell_exit
};

int shell_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int shell_cd(char **args)
{
  if (args[1] == NULL) {
    // cd with no arguments -> go to $HOME
    char *home = getenv("HOME");
    if (home == NULL) {
      fprintf(stderr, "gsh: HOME not set\n");
    } else if (chdir(home) != 0) {
      perror("shell");
    }
  } 
  else {
    if (strcmp(args[1], "..") == 0) {
      char *cwd = getcwd(NULL, 0);
      strcpy(args[1], dirname(cwd));
      free(cwd);
      
    }
    if (chdir(args[1]) != 0) {
      perror("shell");
    }
  }
  return 1;
}

int shell_help(char **args)
{
  int i;
  printf("Giri Kishore's shell \n");

  if (sizeof(args) > 8 ) {
    printf("help command does not support arguments\n");
  }

  printf("Type program names and arguments, and hit enter.\n");
  printf("Built-in progams are below:\n");

  for (i = 0; i < shell_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int shell_exit(char **args)
{
  if (sizeof(args) > 8) {
    printf("exit command does not support arguments\n");
  }
  return 0;
}

char *shell_read_line(void)
{
    int bufsize = SHELL_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    struct termios original_term;
    enable_raw_mode(&original_term);

    if (!buffer) {
        fprintf(stderr, "gsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
      c = getchar();
      if (c == '\x1b') { // ESC
        char seq[2];
        seq[0] = getchar();
        seq[1] = getchar();

        if (seq[0] == '[') {
          if (seq[1] == 'A') {
            if (history_size > 0) {
              if (history_index < history_size - 1) {
                history_index++;
              }
              
              int idx = (history_head + HISTORY_SIZE - 1 - history_index) % HISTORY_SIZE;
              strcpy(buffer, history[idx]);
              position = strlen(buffer);
                
              // Clear line and reprint prompt + buffer
              printf("\33[2K\r%s > %s", getcwd(NULL, 0), buffer);
            }
          } 
          else if (seq[1] == 'B') {
            if (history_index > 0) {
              history_index--;
              int idx = (history_head + HISTORY_SIZE - 1 - history_index) % HISTORY_SIZE;
              strcpy(buffer, history[idx]);
              position = strlen(buffer);
            } 
            else {
              // No more recent command — clear the buffer
              history_index = -1;
              buffer[0] = '\0';
              position = 0;
            }
            
            // Clear line and print prompt + buffer
            printf("\33[2K\r%s > %s", getcwd(NULL, 0), buffer);
            fflush(stdout);
          }
        }
        continue;
      }

      if (c == '\n') {
        buffer[position] = '\0';
        if (strcmp(buffer, "\n") != 0) {
          if (history[history_head]) {
            free(history[history_head]);
          }
          history[history_head] = strdup(buffer);
          history_head = (history_head + 1) % HISTORY_SIZE;
        
          if (history_size < HISTORY_SIZE) {
            history_size++;
          }
        }
        printf("\n");
        break;
      } 
      else if (c == 127 || c == '\b') {  // handle backspace
        if (position > 0) {
          position--;
          printf("\b \b");
        }
      } 
      else {
        buffer[position++] = c;
        putchar(c);
      }

      if (position >= bufsize) {
        bufsize += SHELL_RL_BUFSIZE;
        buffer = realloc(buffer, bufsize);
        if (!buffer) {
          fprintf(stderr, "gsh: allocation error\n");
          exit(EXIT_FAILURE);
        }
      }
      
    }

    disable_raw_mode(&original_term);
    return buffer;
}


char **shell_split_line(char *line)
{
  int bufsize = SHELL_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *start = line;
  int in_quote = 0;

  while (*line) {
      if (*line == '"') {
          if (!in_quote) {
              // 进入引号区域
              *line = '\0';
              if (line > start) {
                  tokens[position++] = start;  // 添加引号前的token
              }
              start = line + 1;  // 跳过引号
              in_quote = 1;
          } else {
              // 退出引号区域
              *line = '\0';
              tokens[position++] = start;  // 添加引号内的内容
              start = line + 1;
              in_quote = 0;
          }
          line++;
          continue;
      }

      if (!in_quote && isspace(*line)) {
          *line = '\0';
          if (line > start) {
              tokens[position++] = start;
          }
          start = line + 1;
      }
      line++;
  }

  // 添加最后一个token
  if (line > start) {
      tokens[position++] = start;
  }

  // 检查未闭合的引号
  if (in_quote) {
      fprintf(stderr, "gsh: unmatched quote\n");
      free(tokens);
      return NULL;
  }

  tokens[position] = NULL;
  return tokens;
}

int shell_execute_pipe(char **args, int pipe_pos) {
  int pipefd[2];
  pid_t pid1, pid2;
  
  if (pipe(pipefd) == -1) {
      perror("gsh: pipe failed");
      return 1;
  }

  // 第一个命令（写管道）
  pid1 = fork();
  if (pid1 == 0) {
      close(pipefd[0]);          // 关闭读端
      dup2(pipefd[1], STDOUT_FILENO);  // 标准输出 -> 管道写端
      close(pipefd[1]);
      
      args[pipe_pos] = NULL;     // 截断管道符号后的参数
      execvp(args[0], args);
      perror("gsh: exec failed");
      exit(EXIT_FAILURE);
  }

  // 第二个命令（读管道）
  pid2 = fork();
  if (pid2 == 0) {
      close(pipefd[1]);          // 关闭写端
      dup2(pipefd[0], STDIN_FILENO);   // 标准输入 <- 管道读端
      close(pipefd[0]);
      
      execvp(args[pipe_pos+1], &args[pipe_pos+1]);
      perror("gsh: exec failed");
      exit(EXIT_FAILURE);
  }

  // 父进程关闭所有管道端
  close(pipefd[0]);
  close(pipefd[1]);
  
  // 等待两个子进程结束
  waitpid(pid1, NULL, 0);
  waitpid(pid2, NULL, 0);
  
  return 1;
}

int shell_launch(char **args)
{
  pid_t pid;
  int status;
  int fd;
  int i = 0;

  // 保存原始标准输入输出
  int stdin_copy = dup(STDIN_FILENO);
  int stdout_copy = dup(STDOUT_FILENO);

  // 检查重定向符号
  while (args[i] != NULL) {
      if (strcmp(args[i], ">") == 0) {  // 输出重定向
          if (args[i+1] == NULL) {
              fprintf(stderr, "gsh: syntax error near >\n");
              return 1;
          }
          fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if (fd == -1) {
              perror("gsh");
              return 1;
          }
          args[i] = NULL;  // 截断命令参数
          dup2(fd, STDOUT_FILENO);  // 重定向标准输出
          close(fd);
          break;
      } 
      else if (strcmp(args[i], "<") == 0) {  // 输入重定向
          if (args[i+1] == NULL) {
              fprintf(stderr, "gsh: syntax error near <\n");
              return 1;
          }
          fd = open(args[i+1], O_RDONLY);
          if (fd == -1) {
              perror("gsh");
              return 1;
          }
          args[i] = NULL;
          dup2(fd, STDIN_FILENO);  // 重定向标准输入
          close(fd);
          break;
      }
      i++;
  }

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("gsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("gsh");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  // 恢复标准输入输出
  dup2(stdin_copy, STDIN_FILENO);
  dup2(stdout_copy, STDOUT_FILENO);
  close(stdin_copy);
  close(stdout_copy);

  return 1;
}

int shell_execute(char **args)
{
  int i;
  if (args[0] == NULL) return 1;

  // 检查管道符号
  for (i = 0; args[i] != NULL; i++) {
      if (strcmp(args[i], "|") == 0) {
          return shell_execute_pipe(args, i);
      }
  }

  for (i = 0; i < shell_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return shell_launch(args);
}

void print_prompt() {
  // 直接写入终端（文件描述符2是标准错误，默认不被重定向）
  dprintf(STDERR_FILENO, "%s > ", getcwd(NULL, 0));
}


void shell_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    // printf("%s > ", getcwd(NULL, 0));
    print_prompt();
    line = shell_read_line();
    args = shell_split_line(line);
    // for(int i=0; args[i]!=NULL; i++) {
    //   printf("token[%d]: %s\n", i, args[i]);
    // }
    status = shell_execute(args);

    free(line);
    free(args);
  } while (status);
}


int main()
{

  // Run command loop.
  shell_loop();

  return EXIT_SUCCESS;
}