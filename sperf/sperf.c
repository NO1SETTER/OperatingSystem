#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
char *path;//path环境变量
char PATH1[200];//PATH1用于保存做exec_env
char PATH2[200];//PATH2用于strtok
char *exec_argv[200];//最多传一百个参数
char *exec_env[200];
char env[200][1000];
int arg_num;
int env_num;
char strace_path[200];

void parse_args_envp(int argc,char **argv);
void print_message();
void find_strace_path();

typedef struct
{
  char name[50];
  double t;
  int ratio;//比例取整
}SYSCTRL;
SYSCTRL sysctrl[1000];
double total=0;//总时间

int sys_num = 0;//已出现的系统调用
static int syscmp(const void* ptr1,const void* ptr2)
{
  SYSCTRL *a=(SYSCTRL *)ptr1;
  SYSCTRL *b=(SYSCTRL *)ptr2;
  return a->t < b->t;
}

void error_dfs(int k)
{
  error_dfs(k+1);
}
int main(int argc, char *argv[]) {

  parse_args_envp(argc,argv);
  //find_strace_path();
  //assert(strace_path[0]);
  //print_message();
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

  if(cpid!= 0)//parent reads from pipefd[0]
  {

    close(pipefd[1]);
    char buf;
    char buffer[1000];
    int len=0;
    time_t pretime;
    time_t nowtime; 
    time(&pretime);
    int reachend=0;//是否程序结束
    int ct=1;
    while(1)
    {
      while(read(pipefd[0],&buf,1)>0)
      {
        if(buf!='\n') buffer[len++]=buf;
        else//读到一行终点
        {
          buffer[len]='\0';//读取了一行的数据,进行分析
          printf("%s\n",buffer);
          if(buffer[0]=='+') 
          {reachend=1;
          break;}
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
          total=total+t;
          //printf("name=%s t=%f\n\n",name,t);
          int rec=0;
          for(int i=0;i<sys_num;i++)
          {
            if(strcmp(name,sysctrl[i].name)==0)
            {sysctrl[i].t=sysctrl[i].t+t;
            rec=1;
            }
          }
          if(!rec)
          {
            strcpy(sysctrl[sys_num].name,name);
            sysctrl[sys_num].t=t;
            sys_num=sys_num+1;
          }
          len=0;
          time(&nowtime);
          //printf("nowtime=%ld\n",nowtime.tv_usec);
          if(nowtime-pretime>=1)
          {
            pretime=nowtime;
            break;
          }
        }
      }
      printf("errno=%d\n",errno);
    if(errno==0)
      {qsort(sysctrl,sys_num,sizeof(SYSCTRL),syscmp);
      if(ct!=1)
      {for(int i=0;i<6;i++)
      { printf("\033[1A");
        printf("\r\033[K");
      }
      }
      printf("Time #%d\n",ct++);
      for(int i=0;i<5;i++)
      { 
        sysctrl[i].ratio=(int)(100*sysctrl[i].t/total);
        printf("%s(%d%%)\n",sysctrl[i].name,sysctrl[i].ratio);
        }
      fflush(stdout);
      total=0;
      for(int i=0;i<sys_num;i++)//统计后清零
        sysctrl[i].t=0;
      if(reachend) break;}
  }
  char ch='\0';
  for(int i=0;i<80;i++)
  printf("%c",ch);
  }
  else//child writes to pipefd[1]
  {
    close(pipefd[0]);
    //close(STDOUT_FILENO);
    //int rec=pipefd[1];
    int devno=open("/dev/null",O_WRONLY);
    int ret1=dup2(devno,STDOUT_FILENO);
    assert(ret1==STDOUT_FILENO);
    int ret2=dup2(pipefd[1],STDERR_FILENO);
    assert(ret2==STDERR_FILENO);
    
    //  for(int i=0;i<env_num;i++)
    for(int i=env_num-1;i>=0;i--)//前面的都没问题但是没有strace,最后一次有问题
    {

      sprintf(strace_path,"%s/strace",env[i]);
      DIR* dir=opendir(env[i]);
      if(dir==NULL&&i==env_num-1) error_dfs(0);
    execve(strace_path,exec_argv,exec_env);
           if(i==env_num-1) error_dfs(0);
    }
    //perror("After execve");
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

  //path,exec_env都是空指针,Path,env是分配了空间的
  path=getenv("PATH");
  sprintf(PATH1,"PATH=%s",path);
  strcpy(PATH2,PATH1);
  exec_env[0]=PATH1;
  exec_env[1]=NULL;
  
  strtok(PATH2,"=");
  char *s;
  int pos=0;
    for(;(s=strtok(NULL,":"))!=NULL;pos++)
    {
      sprintf(env[pos],"%s",s);
    }
  env_num=pos;
  
}

void print_message()
{
  printf("%d EXEC_ARGS:\n",arg_num);
  for(int i=0;i<arg_num;i++)
  printf("arg[%d]:%s\n",i,exec_argv[i]);
  printf("EXEC_ENV:%s\n",exec_env[0]);
  printf("%d ENV:\n",env_num);
  for(int i=0;i<env_num;i++)
  printf("%s\n",env[i]);
  printf("Strace at %s\n",strace_path);
}

int get_strace=0;
void read_all_file(char *basepath)//寻找strace,找到返回1，否则返回0
{
  if(get_strace) return;
  DIR* dir;
  struct dirent* ptr;
  if((dir=opendir(basepath))==NULL)//这里失败了
  {
    printf("Failed opening %s\n",basepath);//错误是ENOENT
    assert(errno==ENOENT);
    //assert(0);
  }

  char base[500];
  while((ptr=readdir(dir))!=NULL)
  {
    if(get_strace) return;
    if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0) continue;
    if(ptr->d_type!=DT_DIR)
    {
      printf("%s\n",basepath);
      if(strcmp(ptr->d_name,"strace")==0)//找到
      {
      sprintf(base,"%s/%s",basepath,ptr->d_name);
      strcpy(strace_path,base);
        get_strace=1;
        return;
      }
    }
    else
    {
      continue;
      sprintf(base,"%s/%s",basepath,ptr->d_name);
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
  
  for(int i=0;i<env_num;i++)
  {
    strcpy(basepath,env[i]);
    assert(i!=env_num-1);
    read_all_file(basepath);
    if(get_strace) return;
  }
}

