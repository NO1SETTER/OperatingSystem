#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
extern char ** environ;
struct FUNC
{
  char name[128];
  void* addr;
}func[1005];
int func_num=0;


void *map_file(const char *fname);
char * exec_argv[100]={"gcc","-fPIC","-shared","-m64","-U_FORTIFY_SOURCE","-O1","-std=gnu11"
,"-ggdb","-Wall","-Werror","-Wno-unused-result","-Wno-unused-variable","./share.c",
"-o","share.so",NULL};
int main(int argc, char *argv[]) {
  static char line[4096];
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    printf("Got %zu chars.\n", strlen(line)); // WTF?
    if(line[0]=='i'&&line[1]=='n'&&line[2]=='t')//定义函数
    {
      char name[128];
      for(int i=3,pos=0;i<strlen(line);i++)
      {   
        if(line[i]=='(') 
        { name[pos]='\0';
          break;}
        if((line[i]>='a'&&line[i]<='z')||(line[i]>='A'&&line[i]<='Z'))
        name[pos++]=line[i];
      }//确定名字
      
        char name_c[128];
        char name_c_arg[128];
        char name_so[128];
        
        sprintf(name_c,"%s.c",name);
        sprintf(name_c_arg,"./%s.c",name);
        sprintf(name_so,"%s.so",name);
        printf("name_c=%s\n",name_c);
        printf("name_c_arg=%s\n",name_c_arg);
        printf("name_so=%s\n",name_so);
        assert(0);
        exec_argv[12]=name_c_arg; 
        exec_argv[14]=name_so;
        FILE *fptr=fopen(name_c,"a+");
        execve("gcc",exec_argv,environ);
        void *func_addr=map_file(name_so);
        strcpy(func[func_num].name,name);
        func[func_num].addr=func_addr;
    }
    else
    {

    } 
  }
}

void *map_file(const char *fname)
{
  int fd =open(fname,O_RDONLY);
  if(fd<0) return NULL;
  void *ret=mmap(NULL,
  4096,
  PROT_READ | PROT_WRITE | PROT_EXEC,
  MAP_PRIVATE, 
  fd, 0);
  if((intptr_t)ret==-1) return NULL;
  return ret;
}
