#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>

extern char **environ;


char *path;
char *command;
char *exec_argv[10];//最多传十个参数
char *exec_envp[1000];
int arg_num;
int env_num;
void parse_args_envp(int argc,char **argv)//把参数环境变量什么的都解析了
{
command=argv[1];
for(int i=2;i<argc;i++)
{exec_argv[i-2]=argv[i];
printf("[][]%s\n",exec_argv[i-2]);
}
exec_argv[argc-1]=NULL;
arg_num=argc-2;
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
int pos=0;
  for(;(s=strtok(NULL,":"))!=NULL;pos++)
  {
    printf("%s\n",s);
    exec_envp[pos]=s;
      printf("%s\n",exec_envp[pos]);
  }
env_num=pos;

/*printf("arg_num=%d env_num=%d\n",arg_num,env_num);
for(int i=0;i<arg_num;i++)
{
  printf("arg[%d]:%s\n",i,exec_argv[i]);
}*/


for(int i=0;i<env_num;i++)
{ assert(0);
  printf("env[%d]:%s",i,exec_envp[i]);
}

}

int main(int argc, char *argv[]) {
  parse_args_envp(argc,argv);
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
    if(command[0]=='/')//绝对路径
    {
      execve(command,exec_argv,exec_envp);
    }
    else
    {

    }

  }


  perror(argv[0]);
  exit(EXIT_FAILURE);
}
