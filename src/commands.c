#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include "commands.h"
#include "built_in.h"

#define FILE_SERVER "/tmp/test_server"

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands > 0) {
    if (n_commands == 1)
    {
      struct single_command* com = (*commands);
      return exec_com(com);
    }
    else
    {
      struct single_command* com= (*commands);
      struct single_command* next_com = (*commands+1);
      
      pthread_t p_thread;
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_create(&p_thread,&attr, socket_thread, (void*)next_com);

      if (fork()==0)
      {
        int client_socket;
        struct sockaddr_un server_addr;
        client_socket=socket(PF_FILE,SOCK_STREAM,0);
        if (-1==client_socket)
        {
          fprintf(stderr,"socket error!!\n");
          exit(1);
        }
        memset(&server_addr,0,sizeof(server_addr));
        server_addr.sun_family = AF_UNIX;
        strcpy(server_addr.sun_path,FILE_SERVER);

        while(1)
        {
          if (-1==connect(client_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))) continue;
          close(0);
          close(1);
          dup2(client_socket,1);
          exec_com(com);
          close(client_socket);
          pthread_join(p_thread,NULL);
          exit(0);
        }
      }
      pthread_join(p_thread,NULL);
    }
  }

  return 0;
}

int exec_com(struct single_command (*com))
{
  assert(com->argc != 0);

  int built_in_pos = is_built_in_command(com->argv[0]);
  if (built_in_pos != -1) {
    if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
      if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
        fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
      }
    } else {
      fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
      return -1;
    }
  } else if (strcmp(com->argv[0], "") == 0) {
    return 0;
  } else if (strcmp(com->argv[0], "exit") == 0) {
    return 1;
  } else if (pathresolution(com->argv)){
    do_exec(com->argc,com->argv);
    return 0;
  } else {
    fprintf(stderr, "%s: command not found\n", com->argv[0]);
    return -1;
  }
  return 0; 
}

void *socket_thread(void* arg)
{
  struct single_command *com2 = (struct single_command*)arg;
  int server_socket;

  if (0==access(FILE_SERVER,F_OK)) unlink(FILE_SERVER);
  
  server_socket=socket(PF_FILE,SOCK_STREAM,0);
  struct sockaddr_un server_addr;
  memset(&server_addr,0,sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path,FILE_SERVER);

  if (-1 == bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr)))
  {
    fprintf(stderr,"bind error!!\n");
    exit(1);
   }

   while(1)
   {
     if (-1 == listen (server_socket,5))
     {
       fprintf(stderr,"listen error!!\n");
       continue;
     }
     
     struct sockaddr_un client_addr;
     int client_socket;
     int client_addr_size;
     client_addr_size = sizeof(client_addr);
     client_socket=accept(server_socket,(struct sockaddr*)&client_addr,&client_addr_size);

     if (-1 == client_socket)
     {
       fprintf(stderr,"access error in server!!\n");
       continue;
     }

     if (fork()==0)
     {
       close(0);
       dup2(client_socket,0);
       exec_com(com2);
       close(0);
       exit(0);
     }
     wait(0);
     close(client_socket);
     break;
   }

   pthread_exit(0);
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
