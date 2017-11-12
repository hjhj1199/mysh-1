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

#define FILE_SERVER "/tmp/file_server"

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
      return exec_commands(com);
    }
    else
    {
      struct single_command* com= (*commands);
      struct single_command* com_next = (*commands+1);
      int client_socket;
      struct sockaddr_un server_addr;
      pthread_t p_thread;
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_create(&p_thread,&attr, socket_thread, (void*)com_next);

      if (fork()==0)
      {
        client_socket=socket(PF_FILE,SOCK_STREAM,0);
        if (client_socket==-1)
        {
          fprintf(stderr,"socket error!!\n");
          exit(1);
        }
        memset(&server_addr,0,sizeof(server_addr));
        server_addr.sun_family = AF_UNIX;
        strcpy(server_addr.sun_path,FILE_SERVER);

        while(1)
        {
          if (connect(client_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))==-1) continue;
          close(0);
          close(1);
          dup2(client_socket,1);
          exec_commands(com);
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

int exec_commands(struct single_command *com)
{
  int status,pid;
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
  } else if (path_resolution(com->argv)==1){
    int bg,bgpid,cnt;
    
    if (strcmp(com->argv[com->argc-1],"&")==0)
    {
      bg=1;
      com->argv[com->argc-1]=NULL;
      com->argc=com->argc-1;
    }

    if ((pid=fork())==0)
    {
      if (bg==1)
      {
        bgpid=getpid();
      }
      execv(com->argv[0],com->argv);
      fprintf(stderr,"%s:command not found\n",com->argv[0]);
      exit(1);
    }
    else
    {
      if (bg!=1) wait(&status);
    }
    return 0;
  } else {
    fprintf(stderr, "%s: command not found\n", com->argv[0]);
    return -1;
  }
  return 0; 
}

void *socket_thread(void* argv)
{
  struct single_command *com = (struct single_command*)argv;
  int server_socket,client_socket,client_addr_size;
  struct sockaddr_un server_addr, client_addr;

  if (access(FILE_SERVER,F_OK)==0) unlink(FILE_SERVER);
  
  server_socket=socket(PF_FILE,SOCK_STREAM,0);
  memset(&server_addr,0,sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path,FILE_SERVER);

  if (bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))==-1)
  {
    fprintf(stderr,"bind error!!\n");
    exit(1);
   }

   while(1)
   {
     if (listen (server_socket,5)==-1)
     {
       fprintf(stderr,"listen error!!\n");
       continue;
     }
     
     client_addr_size = sizeof(client_addr);
     client_socket=accept(server_socket,(struct sockaddr*)&client_addr,&client_addr_size);

     if (client_socket==-1)
     {
       fprintf(stderr,"access error in server!!\n");
       continue;
     }

     if (fork()==0)
     {
       close(0);
       dup2(client_socket,0);
       exec_commands(com);
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
