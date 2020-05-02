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

typedef struct
{
  char name[50];
  double t;
  double ratio;
}SYSCTRL;
SYSCTRL sysctrl[1000];
SYSCTRL* maxfive[5];//记录最大的五个
double total=0;//总时间

int sys_num = 0;//已出现的系统调用
static int syscmp(const void* ptr1,const void* ptr2)
{
  SYSCTRL *a=(SYSCTRL *)ptr1;
  SYSCTRL *b=(SYSCTRL *)ptr2;
  return a->t>b->t;
}

int main(int argc, char *argv[]) {
  parse_args_envp(argc,argv);
  find_strace_path();
  print_message();

  for(int i=0;i<1000;i++)
  {
    memset(sysctrl[i].name,0,sizeof(sysctrl[i].name));
    sysctrl[i].t=0;
    sysctrl[i].ratio=0;
  }
  int pipefd[2];
  pid_t cpid;
  
  if(pipe(pipefd)==-1)
  {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  cpid=fork();
  if(cpid == -1)
  {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if(cpid==0)//child reads from pipefd[0]
  {
    close(pipefd[1]);
    int ret=dup2(pipefd[0],STDIN_FILENO);
    assert(ret==STDIN_FILENO);
    char buf;
    char buffer[1000];
    int len=0;
    while(read(pipefd[0],&buf,1)>0)
    {
      if(buf!='\n') buffer[len++]=buf;
      else
      {
        buffer[len]='\0';//读取了一行的数据,进行分析
        if(buffer[0]=='+') continue;
        printf("%s\n",buffer);
        char name[50];
        char tstr[20];
        memset(name,0,sizeof(name));
        memset(tstr,0,sizeof(tstr));
        double t;
        for(int i=0;i<len;i++)//定位名字
        {
          if(buffer[i]!='(')
          name[i]=buffer[i];
          else
          {
            name[i]='\0';
            break;
          }
        }
        int pos=1000;
        for(int i=0;i<len;i++)//定位时间
        {
         if(buffer[i]=='<')
           pos=i;
          if(i>pos)
           {
             if(buffer[i]!='>')
             tstr[i-pos-1]=buffer[i];
             else
             tstr[i]='\0';
           }
        }
        if(pos==1000) t=0;
        else 
        t=atof(tstr);
        printf("name=%s t=%f\n\n",name,t);
        int rec=0;
        for(int i=0;i<sys_num;i++)
        {
          if(strcmp(name,sysctrl[i].name)==0)
          sysctrl[i].t=sysctrl[i].t+t;
        }
        if(!rec)
        {
          strcpy(sysctrl[sys_num].name,name);
          sysctrl[sys_num].t=t;
          sys_num=sys_num+1;
        }

        len=0;
      }
    }
    qsort(sysctrl,sys_num,sizeof(sysctrl),syscmp);
  }
  else//parent writes to pipefd[1]
  {
    close(pipefd[0]);
    int ret=dup2(pipefd[1],STDERR_FILENO);
    assert(ret==STDERR_FILENO);
    execve(strace_path,exec_argv,exec_env);
  }


  perror(argv[0]);
  exit(EXIT_FAILURE);
}

void parse_args_envp(int argc,char **argv)//把参数环境变量什么的都解析了
{
  exec_argv[0]="strace";
  exec_argv[1]="-T";
  for(int i=1;i<argc;i++)
  exec_argv[i+1]=argv[i];
  exec_argv[argc+1]=NULL;
  arg_num=argc+1;

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
  printf("%d EXEC_ARGS:\n",arg_num);
  for(int i=0;i<arg_num;i++)
  printf("arg[%d]:%s\n",i,exec_argv[i]);
  printf("%d ENV:\n",env_num);
  printf("EXEC_ENV:%s\n",exec_env[0]);
  printf("Strace at %s\n",strace_path);
}

int get_strace=0;
void read_all_file(char *basepath)//寻找strace,找到返回1，否则返回0
{
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
      strcpy(strace_path,base);
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
    if(get_strace) return;
    get_strace=0;
  }
}

