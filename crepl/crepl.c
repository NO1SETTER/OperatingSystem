#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <dlfcn.h>
extern char ** environ;
struct FUNC
{
  char name[128];
  void* addr;
}func[1005];
int func_num=0;

char * exec_argv[100]={"gcc","-fPIC","-shared","-m64","-U_FORTIFY_SOURCE","-O1","-std=gnu11"
,"-ggdb","-Wall","-Werror","-Wno-unused-result","-Wno-unused-variable","./share.c",
"-o","share.so",NULL};

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
        
        char name_c[128];
        char name_c_arg[128];
        char name_so[128];
        
        sprintf(name_c,"%s.c",name);
        sprintf(name_c_arg,"./%s.c",name);
        sprintf(name_so,"%s.so",name);
        
        exec_argv[12]=name_c_arg; 
        exec_argv[14]=name_so;
        int cpid=fork();
      if(cpid!=0)//这一部分完成加载，保存
      {
        FILE* fptr;
        while((fptr=fopen(name_so,"rwq")==NULL);
        strcpy(func[func_num].name,name);
        void *func_addr=dlopen(fptr,RTLD_GLOBAL);
        func[func_num].addr=func_addr;
        func_num=func_num+1;
        fclose(fptr);
        recursive_handle();
      }
      else
      {
        //assert(0);
        FILE *fptr=fopen(name_c,"a+");
        fprintf(fptr,line);
        fclose(fptr);
        execve("gcc",exec_argv,environ);//这里只能做到编译成共享库,记录，加载都要在父进程中进行
        perror("After execve:gcc");
      }
    }
    else//calculate
    {

    } 
}

int main(int argc, char *argv[]) {
recursive_handle();
}
