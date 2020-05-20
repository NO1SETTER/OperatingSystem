#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/dir.h>
#define interval 3
#define bool int
#define showpids needs[0]
#define nsort needs[1]
#define version needs[2]

const char set[6][20]={"-p","--show-pids","-n","--numeric-sort","-V","--version"};
struct PNODE
{
pid_t pid;
pid_t ft;
pid_t son[1000];
int son_num;
bool is_main;//标记是不是主线程
int num;//当不是主线程时使用
char name[256];
}pnode[3000];//从小到大的要求只需要对son排序即可实现
int id[100000];//用于记录进程map到的节点
bool exist[100000];
bool need_line[3000];//记录哪些部分需要打印竖线
bool printed[100000];//记录哪个线程的子线程已经被打印，用于合并打印线程
bool needs[3];

void init();
void build_tree();
void parse_args(int argc,char*argv[]);
void GetName(pid_t pid,pid_t taskpid,char *name);
int GetPpid(pid_t pid);
int GetTgid(pid_t pid);
int GetThreads(pid_t pid);
void print_tree(int newindent,int indent,pid_t pid,int son_id,int son_num);//记录当前已有缩进和当前的进程号,son_id记录它是它的父进程的第几个id用于决定缩进
void print_version();
/*
根据打印缩进->打印字符串->打印换行符的顺序进行
*/

	

int main(int argc, char *argv[]) {
  /*for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }*/
  assert(!argv[argc]);
  
  init();
  parse_args(argc,argv);
  build_tree();
  print_version();
  print_tree(0,0,1,1,1);
  return 0;
  return 0;
}

void parse_args(int argc,char*argv[])
{
for(int i=1;i<argc;i++)
for(int j=0;j<6;j++)
{if(strcmp(argv[i],set[j])==0)
   needs[j/2]=1;
//if(strcmp(argv[i],"/dev/null")==0)
//exit(0);
}
}

int count=0;
void build_tree()
{
    pid_t pid,taskpid;
    pid_t ppid;
    DIR * prop=opendir("/proc");
    DIR * taskp;
    struct dirent *pentry;
    struct dirent *tentry;//分别为进程指针和线程指针
    char pro_name[256],task_path[256],task_name[256];
    int pro_num=0;
    int threads=1; 
    pnode[0].pid=0;
    id[0]=0;
    
    while((pentry=readdir(prop))!=NULL)
    {
    if((pid=atoi(pentry->d_name))==0) continue;
    //printf("pid=%d\n",pid);
    sprintf(task_path,"/proc/%d/task",pid);
    taskp=opendir(task_path);
    GetName(pid,pid,pro_name);
    threads=GetThreads(pid);
    //printf("threads= %d\n",threads);
    while((tentry=readdir(taskp))!=NULL)
      {
      if((taskpid=atoi(tentry->d_name))==0) continue;
      if (exist[taskpid]) continue;
      exist[taskpid]=1;
      //GetName(pid,taskpid,pro_name);
      count+=1;
      //printf("name of %d is %s\n",pid,proname+1);
      //printf("ppid of %d is %d\n",pid,ppid);
      pnode[++pro_num].pid=taskpid;
      if(pid==taskpid)
      {
      pnode[pro_num].ft=GetPpid(taskpid);
      pnode[pro_num].is_main=1;
      sprintf(task_name,"%s",pro_name+1);
      }
      else
      {
      pnode[pro_num].ft=GetTgid(taskpid);
      sprintf(task_name,"{%s}",pro_name+1);
      pnode[pro_num].num=threads;
      }
      ppid=pnode[pro_num].ft;
      pnode[id[ppid]].son[pnode[id[ppid]].son_num++]=taskpid;
      strcpy(pnode[pro_num].name,task_name);
      //printf("pid=%d taskpid=%d name=%s\n",pid,taskpid,pnode[pro_num].name);
      id[taskpid]=pro_num;
      }
    while(closedir(taskp));
   
   }
   while(closedir(prop));
}

void init()
{  for(int i=0;i<1000;i++)
   pnode[i].son_num=0;
   memset(need_line,0,sizeof(need_line));
   memset(needs,0,sizeof(needs));
   memset(exist,0,sizeof(exist));
   memset(printed,0,sizeof(printed));
}

void print_version()
{if(version)
 {fprintf(stderr,"pstree (PSmisc) 23.1\nCopyright (C) 1993-2017 Werner Almesberger and Craig Small\n\nPSmisc comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it under\nthe terms of the GNU General Public License.\nFor more information about these matters, see the files named COPYING.\n");
}
}

void print_tree(int newindent,int indent,pid_t pid,int son_id,int son_num)
{
/*if(version)
{printf("pstree (PSmisc) 23.1\nCopyright (C) 1993-2017 Werner Almesberger and Craig Small\n\nPSmisc comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it under\nthe terms of the GNU General Public License.\nFor more information about these matters, see the files named COPYING.\n");
}*/
if(version) return;if(!showpids)//合并的前期准备
{
if(printed[pnode[id[pid]].ft]&&!pnode[id[pid]].is_main)//这个子线程已经打印过了
return;
if(!pnode[id[pid]].is_main)
son_num-=pnode[id[pid]].num-1;
}

if(newindent==interval)
{
if(son_num==1)
printf("───");
else
{printf("─┬─");
need_line[indent-2]=1;
}
}
else if(indent!=0)
{
for(int i=0;i<indent-2;i++)
{if(!need_line[i])
printf(" ");
else printf("│");
}
if(son_id<son_num)
printf("├─");
else
{
printf("└─");
need_line[indent-2]=0;
}
}

char temp[50];
if(showpids)
{sprintf(temp,"%s(%d)",pnode[id[pid]].name,pid);
}
else
{if(pnode[id[pid]].is_main)
{sprintf(temp,"%s",pnode[id[pid]].name);}
else
{
if(pnode[id[pid]].num==1)
sprintf(temp,"%s",pnode[id[pid]].name);
else
sprintf(temp,"%d*[%s]",pnode[id[pid]].num,pnode[id[pid]].name);
printed[pnode[id[pid]].ft]=1;
}
}
printf("%s",temp); 
if(pnode[id[pid]].son_num==0) 
printf("\n");

for(int i=0;i<pnode[id[pid]].son_num;i++)
{
if(i==0) 
print_tree(interval,indent+interval+strlen(temp),pnode[id[pid]].son[i],1,pnode[id[pid]].son_num);
else
print_tree(0,indent+interval+strlen(temp),pnode[id[pid]].son[i],i+1,pnode[id[pid]].son_num);
}
return ;
}

FILE *fp;
void GetName(pid_t pid,pid_t taskpid,char *name)
{
  //printf("pid=%d taskpid=%d\n",pid,taskpid);
  char pathname[100];
  sprintf(pathname,"/proc/%d/task/%d/status",pid,taskpid);
  //printf("%s\n",pathname);
  //fseek(fp,0,SEEK_SET); 
  fp=fopen(pathname,"r");
  assert(fp!=NULL);
  fscanf(fp,"%s",name);
  fgets(name,100,fp);
  char *temp=name;
  //printf("name = %s",temp);
  //printf("count = %d\n",count);
  while((*temp!='\n')&&(*temp!='/'))
  {temp++;}
  *temp='\0';
  while(fclose(fp));
}

int GetPpid(pid_t pid)
{
char pathname[100];
sprintf(pathname,"/proc/%d/status",pid);
//fseek(fp,0,SEEK_SET); 
fp=fopen(pathname,"r");
char str[100];
for(int i=0;i<6;i++)
fgets(str,100,fp);
for(int i=0;i<2;i++)
fscanf(fp,"%s\n",str);
while(fclose(fp));
return atoi(str);
}

int GetTgid(pid_t pid)
{
char pathname[100];
sprintf(pathname,"/proc/%d/status",pid);
//fseek(fp,0,SEEK_SET); 
fp=fopen(pathname,"r");
char str[100];
for(int i=0;i<3;i++)
fgets(str,100,fp);
for(int i=0;i<2;i++)
fscanf(fp,"%s\n",str);
while(fclose(fp));
return atoi(str);
}

int GetThreads(pid_t pid)
{

char pathname[500];
sprintf(pathname,"/proc/%d/status",pid);
//fseek(fp,0,SEEK_SET); 
fp=fopen(pathname,"r");
assert(fp!=NULL);
char str[100];
for(int i=0;i<33;i++)
fgets(str,100,fp);
for(int i=0;i<2;i++)
fscanf(fp,"%s\n",str);
while(fclose(fp));
for(int i=0;i<strlen(str);i++)
if(str[i]==',') return 0;
return atoi(str)-1;
}

