#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>

extern char **environ;


char *path;//path环境变量
char *exec_argv[10];//最多传十个参数
char *exec_envp[1000];
int arg_num;
int env_num;

void parse_args_envp(int argc,char **argv);
void print_message();
void read_all_file();
void find_path();
int main(int argc, char *argv[]) {
  parse_args_envp(argc,argv);
  print_message();
  assert(0);
  /*char *exec_argv[] = { "strace", "ls", NULL, };
  char *exec_envp[] = { "PATH=/bin", NULL, };
  execve("strace",          exec_argv, exec_envp);
  execve("/bin/strace",     exec_argv, exec_envp);
  execve("/usr/bin/strace", exec_argv, exec_envp);*/
  int pipefd[2];
  pid_t cpid;
  
  cpid=fork();
  if(cpid == -1)
  {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if(cpid==0)//child reads from pipefd[0]
  {

  }
  else//parent writes to pipefd[1]
  {
  }


  perror(argv[0]);
  exit(EXIT_FAILURE);
}

void parse_args_envp(int argc,char **argv)//把参数环境变量什么的都解析了
{
exec_argv[0]="strace";
for(int i=1;i<argc;i++)
exec_argv[i]=argv[i];
exec_argv[argc]=NULL;()
arg_num=argc;

char **ptr=environ;
  while(*ptr)
  {
    if(strlen(*ptr)>=5)
    { if((*ptr)[0]=='P'&&(*ptr)[1]=='A'&&(*ptr)[2]=='T'&&(*ptr)[3]=='H'&&(*ptr)[4]=='=')
      {path=*ptr;
      break;}
    }
    ptr++;
  }

strtok(path,"=");
char *s;
char temp[200]="PATH=";
int pos=0;
  for(;(s=strtok(NULL,":"))!=NULL;pos++)
  {
    strcat(temp,s);
    exec_envp[pos]=temp;
    strcpy(temp,"PATH=");
  }
env_num=pos;
}

void print_message()
{
  printf("COMMAND:%s\n",command);
  printf("ARGS:\n");
  for(int i=0;i<arg_num;i++)
  printf("arg[%d]:%s\n",i,exec_argv[i]);
  printf("ENV:\n");
  for(int i=0;i<env_num;i++)
  printf("env[%d]:%s\n",i,exec_envp[i]);
}
