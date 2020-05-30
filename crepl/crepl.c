#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <dlfcn.h>
extern char ** environ;
char env_path[1000];
char env[200][1000];
int env_num;
char gcc_path[200];

char * exec_argv[100]={"gcc","-fPIC","-shared","-m64","-U_FORTIFY_SOURCE","-O1","-std=gnu11"
,"-ggdb","-Wall","-Werror","-Wno-unused-result","-Wno-unused-variable","./share.c",
"-o","share.so",NULL};
//用一个share.so保存所有共享库函数,用dlsym查找
void recursive_handle()
{
  static char line[4096];
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      return;
    }
    printf("Got %zu chars.\n", strlen(line)); // WTF?
    if(line[0]=='i'&&line[1]=='n'&&line[2]=='t')//definition
    {
      char name[128];
      for(int i=3,pos=0;i<strlen(line);i++)
      {   
        if(line[i]=='(') 
        { name[pos]='\0';
          break;}
        if(line[i]!=' ')
        name[pos++]=line[i];
      }//确定名字
        
        int cpid=fork();
      if(cpid!=0)//这一部分完成加载，保存
      {
        void *func_handler;
        while((func_handler=dlopen("./share.so",RTLD_NOW))==NULL);//保证编译完才加载
        assert(0);
        void *func_addr;
        while((func_addr=dlsym(func_handler,name))==NULL);//确保函数加载完成
        recursive_handle();
      }
      else
      {
        //assert(0);
        FILE *fptr=fopen("share.c","a+");
        fprintf(fptr,"%s\n",line);
        fclose(fptr);
        execve(gcc_path,exec_argv,environ);//这里只能做到编译成共享库,记录，加载都要在父进程中进行
        perror("After execve:gcc");
      }
    }
    else//calculate
    {
    } 
}

void parse_args_envp(int argc,char **argv);
void find_gcc_path();
int main(int argc, char *argv[]) {
parse_args_envp(argc,argv);
find_gcc_path();
recursive_handle();
}

void parse_args_envp(int argc,char **argv)//把环境变量解析
{
  char *path=getenv("PATH");
  sprintf(env_path,"PATH=%s",path); 
  strtok(env_path,"=");
  char *s;
  int pos=0;
    for(;(s=strtok(NULL,":"))!=NULL;pos++)
    {
      sprintf(env[pos],"%s",s);
      //printf("length of env[%d]:%s is %u\n",pos,env[pos],(unsigned)strlen(env[pos]));
    }
  env_num=pos;
}

void find_gcc_path()//找到执行程序的路径,把它写到exec_path里去
{
  char basepath[200];
  for(int i=0;i<env_num;i++)//test env_num=8
  {
    sprintf(basepath,"%s",env[i]);
    DIR* dir=opendir(basepath);
    if(dir==NULL) continue;
    struct dirent* ptr;
    while((ptr=readdir(dir))!=NULL)
    {       
      if(strcmp(ptr->d_name,"gcc")==0)
      {
        sprintf(gcc_path,"%s/gcc",basepath);
        return;
      }
    }
  }
  return;
}
