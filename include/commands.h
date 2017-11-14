#ifndef MYSH_COMMANDS_H_
#define MYSH_COMMANDS_H_

struct single_command
{
  int argc;
  char** argv;
};

struct background{
  int pid;
  char *argv;
  int flag;
}bg;

int evaluate_command(int n_commands, struct single_command (*commands)[512]);

int exec_commands(struct single_command *com);

void *background_thread(void *argv);

void *socket_thread(void *argv);

void free_commands(int n_commands, struct single_command (*commands)[512]);

#endif // MYSH_COMMANDS_H_
