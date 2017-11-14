#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <linux/limits.h>
#include "commands.h"
#include "built_in.h"

int do_cd(int argc, char** argv) {
  if (!validate_cd_argv(argc, argv))
    return -1;

  if (chdir(argv[1]) == -1)
  {
    if (strcmp(argv[1],"~")==0)
    {
      chdir(getenv("HOME"));
      return 0;
    }
    return -1;
  }
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
  int status,pstate;

  if (!validate_fg_argv(argc, argv))
    return -1;
  pstate=waitpid(bg.pid,&status,WNOHANG);
  if (pstate>0) fprintf(stderr,"%d DONE\t%s\n",bg.pid,bg.argv);
  else if (pstate==0) 
  {
    fprintf(stderr,"%d RUNNING\t%s\n",bg.pid,bg.argv);
    waitpid(bg.pid,&status,0);
  }
  else return 0;
  
  return 0;
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

int path_resolution(char** argv)
{
  if (access(argv[0],X_OK)==0) return 1;
  
  const char *envpath=getenv("PATH");
  char *path,*token;
  path = malloc(strlen(envpath));
  strcpy(path,envpath);
  
  token = strtok(path,":");
  
  do
  {
    if (token==NULL) break;
    char *absolutepath=malloc(strlen(token)+strlen(argv[0])+1);
    strcpy(absolutepath,token);
    strcat(absolutepath,"/");
    strcat(absolutepath,argv[0]);
    
    if (access(absolutepath,X_OK)==0)
    {
      argv[0]=realloc(argv[0],strlen(absolutepath));
      strcpy(argv[0],absolutepath);
      return 1;
    }
    token = strtok(NULL,":");
  }while(1);
  return -1;
}
