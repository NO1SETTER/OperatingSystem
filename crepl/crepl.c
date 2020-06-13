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
#include <regex.h>

extern char ** environ;
char env_path[1000];
char env[200][1000];
int env_num;
char gcc_path[200];

struct FUNC{
char name[128];
void *addr;
}func[1005];
int func_num=0;
int wrapper_num=0;

#if __x86_64__
char * exec_argv[100]={"gcc","-fPIC","-shared","-m64","-w",NULL,"-o",NULL,NULL};
#else
char * exec_argv[100]={"gcc","-fPIC","-shared","-m32","-w",NULL,"-o",NULL,NULL};
#endif

void recursive_handle();
void parse_args_envp(int argc,char **argv);
void find_gcc_path();
void clean();

#define INITIAL 300 
enum {
  TK_NOTYPE = 256,TK_AND,TK_LB,TK_RB,TK_ADD,TK_OR,TK_BITAND,TK_BITOR,TK_MI,TK_MUL,TK_DI,TK_NUM,TK_FUNC
};
static struct rule {
  char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},    // spaces
  {"\\+",TK_ADD},       // plus
  {"\\*",TK_MUL},       // multiply
  {"-",  TK_MI},        // minus
  {"/",  TK_DI},        // divide
  {"\\(",TK_LB},        // left bracket
  {"\\)", TK_RB},       // right bracket   
  {"[0-9]+",TK_NUM},    // number
  {"&&",TK_AND},         // and
  {"\\|\\|",TK_OR},          // or
  {"&",TK_BITAND},       // bitand
  {"\\|",TK_BITOR},        // bitor
  {"[a-zA-Z]+([^\\)]+)\\)",TK_FUNC}//TK_FUNC
};


#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )
static regex_t re[NR_REGEX] = {};

typedef struct token {
  int type;
  char str[64];
} Token;

static Token tokens[1024] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

void init_regex();
int  calculate(int s,int e,int *valid);//[s,e)左闭右开
static int make_token(char *e);
int getfuncret(char *s);

int main(int argc, char *argv[]) {
parse_args_envp(argc,argv);
find_gcc_path();
clean();
init_regex();
recursive_handle();
}



void init_regex() {
  int i;
  char error_msg[128];
  int ret;
  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
    }
  }
}

static int make_token(char *e) { 
  int position = 0;
  int i;
  regmatch_t pmatch;
  nr_token=0;
  while (e[position] != '\0') {
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch,0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;
        position += substr_len;
        switch (rules[i].token_type) {
         case TK_ADD:case TK_MI:case TK_MUL:case TK_DI:case TK_LB:case TK_RB:
         case TK_AND:case TK_OR:case TK_BITAND:case TK_BITOR:
           tokens[nr_token].type=rules[i].token_type;
           nr_token++;
           break;
         case TK_NUM:case TK_FUNC:
           tokens[nr_token].type=rules[i].token_type;
           memset(tokens[nr_token].str,'\0',sizeof(tokens[nr_token].str));
           strncpy(tokens[nr_token].str,substr_start,substr_len);
           nr_token++; 
           break;
         case TK_NOTYPE:{break;} 
         default: assert(0);
        }
        break;
      }
    }
    if (i == NR_REGEX) {
      //printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return 0;
}
}
return 1;
}


int  calculate(int s,int e,int *valid)//[s,e)左闭右开
{
if(s==e-1) 
{if(tokens[s].type==TK_NUM)
return atoi(tokens[s].str);
else if(tokens[s].type==TK_FUNC)
return getfuncret(tokens[s].str);
}
if(s==e-2)//识别负数
{
  if(tokens[s].type==TK_MI)
  return -calculate(s+1,e,valid);
}


if(tokens[s].type==TK_LB&&tokens[e-1].type==TK_RB)
    {
        int contained=1;
        int ct=0;
        for(int i=s;i<e-1;i++)
        {
            if(tokens[i].type==TK_LB) ct++;
            if(tokens[i].type==TK_RB) ct--;
            if(ct==0&&i!=e)//说明在最后一个之前匹配完了，没有被包含
            {
                contained=0;break;
            }
        }
        if (contained)
        return calculate(s+1,e-1,valid);
    }//把外层大括号去掉
int opval=1000;//当前主操作数的优先级
int pivot=0;//主操作数的位置
int bra=0;//每个括号的加成是6
for(int i=s;i<e;i++)//找到主操作数
{
   int type=tokens[i].type;
   int nowval;
    switch(type)
    {
      case TK_LB:bra=bra+6;continue;
      case TK_RB:bra=bra-6;continue;
      case TK_OR:nowval=bra;break;
      case TK_AND:nowval=bra+1;break;
      case TK_BITOR:nowval=bra+2;break;
      case TK_BITAND:nowval=bra+3;break;
      case TK_ADD:nowval=bra+4;break;
      case TK_MI:if(i==s) continue;//开头即为负号
      int mtype=tokens[i-1].type;
      if(mtype==TK_ADD||mtype==TK_MI||mtype==TK_MUL||mtype==TK_DI||mtype==TK_LB
      ||mtype==TK_NOTYPE||mtype==TK_OR||mtype==TK_AND||mtype==TK_BITOR
      ||mtype==TK_BITAND) continue;//都说明是负号
      nowval=bra+4;break;
      case TK_MUL:case TK_DI:nowval=bra+5;break;
      default:continue;
    }

  if(nowval<opval)
    {
      opval=nowval;
      pivot=i;
    }
}

if(bra!=0) 
{
  *valid=0;
  return 0;
}
int temp;
switch (tokens[pivot].type)
{case TK_ADD:return calculate(s,pivot,valid)+calculate(pivot+1,e,valid);
 case TK_MI: return calculate(s,pivot,valid)-calculate(pivot+1,e,valid);
 case TK_MUL:return calculate(s,pivot,valid)*calculate(pivot+1,e,valid);
 case TK_DI:
    temp=calculate(pivot+1,e,valid);
    if(temp==0)
      {*valid=0;
       break;
      }
    return calculate(s,pivot,valid)/temp;
 case TK_OR:return calculate(s,pivot,valid)||calculate(pivot+1,e,valid);
 case TK_AND:return calculate(s,pivot,valid)&&calculate(pivot+1,e,valid);
 case TK_BITOR:return calculate(s,pivot,valid)|calculate(pivot+1,e,valid);
 case TK_BITAND:return calculate(s,pivot,valid)&calculate(pivot+1,e,valid);
 default: *valid=0;break;
}

return 0;
}

int getfuncret(char *s)//s[l,e)
{ 
    //printf("calculating function %s\n",s);
    char name_c[256];
    char name_so[256];
    char name_func[256];
    
    sprintf(name_c,"/tmp/expr_wrapper%d.c",wrapper_num);
    sprintf(name_so,"/tmp/expr_wrapper%d.so",wrapper_num);
    sprintf(name_func,"expr_wrapper%d",wrapper_num);
    FILE* fptr=fopen(name_c,"w+");
    assert(fptr);
    fprintf(fptr,"int expr_wrapper%d(){ return %s;}\n",wrapper_num,s);
    fclose(fptr);
    wrapper_num=wrapper_num+1;
    int cpid=fork();
    if(cpid!=0)
    {
        void *func_handler;
        while((func_handler=dlopen(name_so,RTLD_NOW|RTLD_GLOBAL|RTLD_NODELETE))==NULL);//保证编译完才加载
        int (*func_addr)();
        while((func_addr=(int(*)())dlsym(func_handler,name_func))==NULL);//确保函数加载完成
        return (int)(*func_addr)();
    }
    if(cpid==0)
    {
      exec_argv[5]=name_c;
      exec_argv[7]=name_so;
      execve(gcc_path,exec_argv,environ);
      perror("after gcc");
    }
    return 0;
}

void clean()
{
DIR *dir=opendir("/tmp");
struct dirent* ptr;
while((ptr=readdir(dir))!=NULL)
{
  if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0)
  continue;
  char pathname[300];
  sprintf(pathname,"/tmp/%s",ptr->d_name);
  remove(pathname);
}

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

void recursive_handle()
{
  static char line[4096];
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      return;
    }
    int tplen=strlen(line);
    line[tplen-1]='\0';//把回车键替掉
    //printf("Got %zu chars.\n", strlen(line)); // WTF?

      char name[128];
      for(int i=3,pos=0;i<strlen(line);i++)
      {   
        if(line[i]=='(') 
        { name[pos]='\0';
          break;}
        if(line[i]!=' ')
        name[pos++]=line[i];
      }
    
    char name_c[128];
    char name_so[128];

    sprintf(name_c,"/tmp/%s.c",name);
    sprintf(name_so,"/tmp/%s.so",name);

    if(line[0]=='i'&&line[1]=='n'&&line[2]=='t')//definition
    {  
       exec_argv[5]=name_c;
       exec_argv[7]=name_so;
      int cpid=fork();
      if(cpid!=0)//这一部分完成加载，保存
      {
        void *func_handler;
        while((func_handler=dlopen(name_so,RTLD_NOW|RTLD_GLOBAL|RTLD_NODELETE))==NULL);//保证编译完才加载
        void *func_addr;
        while((func_addr=dlsym(func_handler,name))==NULL);//确保函数加载完成
        strcpy(func[func_num].name,name);
        func[func_num].addr=func_addr;
        func_num=func_num+1;
        printf("New function definition\n");
        recursive_handle();
      }
      else
      {
        FILE *fptr=fopen(name_c,"a+");
        fprintf(fptr,"%s\n",line);
        fclose(fptr);
        execve(gcc_path,exec_argv,environ);//这里只能做到编译成共享库,记录，加载都要在父进程中进行
        perror("After execve:gcc");
      }
  }
    else//calculate
    {
      int valid=1;
      if(!make_token(line)) valid=0;//只会有无法匹配，除零两种错误
      int ans;
      if(valid)
        ans=calculate(0,nr_token,&valid);
      if(valid)
        printf("%d\n",ans);
      else
        printf("Invalid expression\n");
      
      recursive_handle();
    } 
}
