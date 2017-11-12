#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <linux/limits.h>

#include "built_in.h"

int do_cd(int argc, char** argv) {
  if (!validate_cd_argv(argc, argv))
    return -1;

  if (chdir(argv[1]) == -1)
    return -1;

  return 0;
}

int do_pwd(int argc, char** argv) {
  if (!validate_pwd_argv(argc, argv))
    return -1;

  char curdir[PATH_MAX];

  if (getcwd(curdir, PATH_MAX) == NULL)
    return -1;

  printf("%s\n", curdir);

  return 0;
}

int do_fg(int argc, char** argv) {
  if (!validate_fg_argv(argc, argv))
    return -1;

  // TODO: Fill this.

  return 0;
}

int do_exec(int argc, char** argv){
  int pid;
  if (pid=fork()==0)
  {
    if (strcmp(argv[argc-1], "&")==0)
    {
      argv[argc-1]=NULL;
    }
    execv(argv[0],argv);
  }

  if (strcmp(argv[argc-1],"&")!=0) wait(0);
}
int validate_cd_argv(int argc, char** argv) {
  if (argc != 2) return 0;
  if (strcmp(argv[0], "cd") != 0) return 0;

  struct stat buf;
  stat(argv[1], &buf);

  if (strcmp(argv[1],"~")==0) return 1;

  if (!S_ISDIR(buf.st_mode)) return 0;

  return 1;
}

int validate_pwd_argv(int argc, char** argv) {
  if (argc != 1) return 0;
  if (strcmp(argv[0], "pwd") != 0) return 0;

  return 1;
}

int validate_fg_argv(int argc, char** argv) {
  if (argc != 1) return 0;
  if (strcmp(argv[0], "fg") != 0) return 0;
  
  return 1;
}

char* pathresolution(char** argv)
{
  if (access(argv[0],X_OK)==0) return argv[0];
  
  const char *env=getenv("PATH");
  char *path = malloc(strlen(env));
  strcpy(path,env);
  char *token = strtok(path,":");
  
  while(token!=NULL)
  {
    char *pathresol=malloc(strlen(token)+strlen(argv[0])+1);
    strcpy(pathresol,token);
    strcat(pathresol,"/");
    strcat(pathresol,argv[0]);
    
    if (access(pathresol,X_OK)==0)
    {
      argv[0]=malloc(strlen(pathresol)+1);
      strcpy(argv[0],pathresol);
      return argv[0];
    }
    token = strtok(NULL,":");
  }
  return NULL;
}
