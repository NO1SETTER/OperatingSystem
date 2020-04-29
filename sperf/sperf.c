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
char Path[200];
char *exec_argv[100];//最多传一百个参数
char *exec_env[100];
char env[100][1000];
int arg_num;
int env_num;
char strace_path[50];

void parse_args_envp(int argc,char **argv);
void print_message();
void find_strace_path();
void modify_path();//修改环境变量的版本

int main(int argc, char *argv[]) {
  parse_args_envp(argc,argv);
  find_strace_path();
  modify_path();
  print_message();

  /*char *exec_argv[] = { "strace", "ls", NULL, };
  char *exec_env[] = { "PATH=/bin", NULL, };
  execve("strace",          exec_argv, exec_env);
  execve("/bin/strace",     exec_argv, exec_env);
  execve("/usr/bin/strace", exec_argv, exec_env);*/
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
    assert(0);
  }
  else//parent writes to pipefd[1]
  {
   
    execve(strace_path,exec_argv,exec_env);
  }


  perror(argv[0]);
  exit(EXIT_FAILURE);
}

void parse_args_envp(int argc,char **argv)//把参数环境变量什么的都解析了
{
exec_argv[0]="strace";
for(int i=1;i<argc;i++)
exec_argv[i]=argv[i];
exec_argv[argc]=NULL;
arg_num=argc;


char **ptr=environ;
  while(*ptr)
  {
    if(strlen(*ptr)>=5)
    {  if((*ptr)[0]=='P'&&(*ptr)[1]=='A'&&(*ptr)[2]=='T'&&(*ptr)[3]=='H'&&(*ptr)[4]=='=')
       {path=*ptr;
       break;}
    }
    ptr++;
  }
strcpy(Path,path);
strtok(path,"=");
char *s;
int pos=0;
  for(;(s=strtok(NULL,":"))!=NULL;pos++)
  {
    sprintf(env[pos],"%s",s);
  }
env_num=pos;
exec_env[0]=Path;
exec_env[1]=NULL;
}

void print_message()
{
  printf("%d ARGS:\n",arg_num);
  for(int i=0;i<arg_num;i++)
  printf("arg[%d]:%s\n",i,exec_argv[i]);
  printf("%d ENV:\n",env_num);
  for(int i=0;i<env_num;i++)
  printf("env[%d]:%s\n",i,env[i]);
  printf("Strace at %s\n",strace_path);
}

int get_strace=0;
void read_all_file(char *basepath)//寻找strace,找到返回1，否则返回0
{
  printf("basepath: %s\n",basepath);
  DIR* dir;
  struct dirent* ptr;
  if((dir=opendir(basepath))==NULL)
  {
    printf("Failed open %s\n",basepath);
    assert(0);
  }

  char base[200];
  strcpy(base,basepath);
  while((ptr=readdir(dir))!=NULL)
  {
    if(get_strace) break;
    if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0)
    continue;
    if(ptr->d_type!=4)
    {
      if(strcmp(ptr->d_name,"strace")==0)//找到
      {
      strcat(base,"/");
      strcat(base,ptr->d_name);
      strcpy(strace_path,basepath);
        get_strace=1;
      }
    }
    else
    {
      char base[100];
      strcat(base,"/");
      strcat(base,ptr->d_name);
      read_all_file(base);
    }
  }
  closedir(dir);
}

void find_strace_path()//找到执行程序的路径,把它写到exec_path里去
{
char basepath[200];
memset(basepath,0,sizeof(basepath));
getcwd(basepath,sizeof(basepath));
read_all_file(basepath);
if(get_strace) return;
get_strace=0;
for(int i=0;i<env_num;i++)
{
  strcpy(basepath,env[i]);
  read_all_file(basepath);
  if(get_strace) break;
  get_strace=0;
}
}

