#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<assert.h>
#include<fcntl.h>
#include<unistd.h>
#include<wchar.h>
#include<sys/mman.h>
#include<sys/stat.h>
//#define _DEBUG
enum DIR_ARRTIBUTE{ATTR_READ_ONLY=0x1,ATTR_HIDDEN=0x2,ATTR_SYSTEM=0x4,
ATTR_VOLUME_ID=0x8,ATTR_DIRECTORY=0x10,ATTR_ARCHIVE=0x20,
ATTR_LONG_NAME=ATTR_READ_ONLY| ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID};
enum CLUSTER_TYPE{DIRECTORY_ENTRY,BMP_HEADER,BMP_DATA,UNUSED,UNCERTAIN};
//UNCERTAIN可能是bmp数据或者未使用
/*
----------------------------------------------------
|           |        |           |                 |
| reserved  |  FAT   |   Root    | File&Directory  |
|  region   | region | Directory |   Region        |
|           |        | (NO FAT32)|                 |
----------------------------------------------------
*/

struct fat_header//考虑FAT32,注意是小端模式！！！！
{
    uint8_t  BS_jmpBoot[3];//boot code
    uint8_t  BS_OEMName[8];//代工厂名字?
    uint32_t BPB_BytePerSec:16;//每一个扇区的字节数
    uint8_t  BPB_SecPerClus;//每一个cluster的扇区数
    uint32_t BPB_RsvdSecCnt:16;//reserved region的扇区数
    uint8_t  BPB_NumFATs;//FAT表的数量1或２
    uint32_t BPB_RootEntCnt:16;//不重要，必须是０
    uint32_t BPB_TotSec16:16;//不重要，必须是０
    uint8_t  BPB_Media;//不重要
    uint32_t BPB_FATSz16:16;//不重要，必须是０
    uint32_t BPB_SecPerTrk:16;//不重要
    uint32_t BPB_NumHeads:16;//不重要
    uint32_t BPB_HiddSec;//不重要
    uint32_t BPB_TotSec32;//四(三)个region的总扇区数(32-bit count)
    uint32_t BPB_FATSz32;//一个FAT的扇区数(32-bit count)
    uint32_t BPB_ExtFlags:16;//????
    uint32_t BPB_FSVer:16;//版本号,必须为0
    uint32_t BPB_RootClus;//根目录的第一个cluster号
    uint32_t BPB_FSinfo:16;//????
    uint32_t BPB_BkBootSec:16;//????
    uint8_t  BPB_Reserved[12];//不重要，必须为0
    uint8_t  BS_DrvNum;//不重要
    uint8_t  BS_Reserved1;//不重要,必须为0
    uint8_t  BS_BootSig;//????
    uint32_t BS_VolID;//????
    uint8_t  BS_VolLab[11];
    uint8_t  BS_FilSysType[8];//字符串"FAT32"
    uint8_t  padding[420];//补充空位
    uint32_t signature_word:16;//0xaa55
}__attribute__((packed));

int DataClusters;//数据区的cluser数
int ClusterSize;//cluster的大小(byte)
int DataOffset;//数据区的起始

struct sdir_entry
{
  uint8_t DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t DIR_NTRes;
  uint8_t DIR_CrtTimeTenth;
  uint32_t DIR_CrtTime:16;
  uint32_t DIR_CrtDate:16;
  uint32_t DIR_LstAccDate:16;
  uint32_t DIR_FstClusHI:16;
  uint32_t DIR_WrtTime:16;
  uint32_t DIR_WrtDate:16;
  uint32_t DIR_FstClusLO:16;
  uint32_t DIR_FileSize;
}__attribute__((packed));

struct ldir_entry
{
  uint8_t LDIR_Ord;
  uint16_t LDIR_Name1[5];
  uint8_t LDIR_Attr;
  uint8_t LDIR_Type;
  uint8_t LDIR_Chksum;
  uint16_t LDIR_Name2[6];
  uint32_t LDIR_FstClusLO:16;
  uint16_t LDIR_Name3[2];
}__attribute__((packed));

int ctype[1000000];//记录cluster的type
int GetSize(char *fname);//得到文件大小
void SetBasicAttributes(const struct fat_header* header);//计算文件的一些属性
uint32_t retrieve(const void *ptr,int byte);//从ptr所指的位置取出长为byte的数据
void ScanCluster(const void* header);
void Recover(const void* header);

int main(int argc, char *argv[]) {
//printf("sizeof wchar:%d\n",(int)sizeof(wchar_t));
assert(sizeof(struct fat_header)==512);
assert(sizeof(struct sdir_entry)==32);
assert(sizeof(struct ldir_entry)==32);

char fname[128]="/home/ebata/img/M5-frecov.img";
int fsize=GetSize(fname);

int fd=open(fname,O_RDONLY);
assert(fd>=0);

const struct fat_header* fh=(struct fat_header*)mmap(NULL,fsize,
PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE,fd,0);//确认读到文件头了
assert(fh->signature_word==0xaa55);
SetBasicAttributes(fh);
ScanCluster(fh);
Recover(fh);
}

void Recover(const void* header)
{ 
void *cstart=(void *)header+DataOffset;
for(int i=0;i<DataClusters;i++)
{
  printf("Scanning cluster at %x\n",(unsigned)(cstart-header));
  if(ctype[i]!=DIRECTORY_ENTRY) continue;
  struct sdir_entry* sdir=cstart;
    if(sdir->DIR_Attr==ATTR_LONG_NAME)//变长文件头
    {
      assert(0);
      wchar_t name[1024];
      memset(name,'\0',sizeof(name));
      struct ldir_entry* ldir= (struct ldir_entry* )sdir;
      int reachend=0;
      while(ldir->LDIR_Attr==ATTR_LONG_NAME)
      {
        wchar_t lname[1024];
        int pos=0;
        for(int i=0;i<5;i++)
        {
          if(reachend) break;
          wchar_t wch=ldir->LDIR_Name1[i];
          if(wch!=0xffff)
          lname[pos++]=wch;
          else
          reachend=1;
        }
        
        for(int i=0;i<6;i++)
        {
          if(reachend) break;
          wchar_t wch=ldir->LDIR_Name2[i];
          if(wch!=0xffff)
          lname[pos++]=wch;
          else
          reachend=1;
        }
        
        for(int i=0;i<2;i++)
        {
          if(reachend) break;
          wchar_t wch=ldir->LDIR_Name3[i];
          if(wch!=0xffff)
            lname[pos++]=wch;
          else
            reachend=1;
        }
        lname[pos]=0;
        wcscat(lname,name);
        wcscpy(name,lname);
        ldir=ldir+1;
      }
    wprintf(L"long name=%s\n",name);
  }
  else
  {
    assert(0);
    char name[15];
    char name1[10];
    char name2[5];
    strncpy(name1,(void *)sdir,8);
    strncpy(name2,(void *)sdir+8,3);
    sprintf(name1,"%s.%s",name1,name2);
    printf("short name=%s\n",name);
  }
  cstart=cstart+ClusterSize;
 }

}

void ScanCluster(const void* header)
{
  void* cstart=(void *)header+DataOffset;
  for(int i=0;i<DataClusters;i++)
  {
    #ifdef _DEBUG
      printf("Cluster at offset 0x%x is labeled as %d\n",DataOffset+(i-1)*ClusterSize,ctype[i-1]);
    #endif
    void *ptr=cstart;
    int isbmphd=0; 
    for(int j=0;j<ClusterSize;j++)
    {
      char c=retrieve(ptr,1);
      if(c=='B'&&isbmphd==0) isbmphd=1;
      else if(c=='M'&&isbmphd==1) isbmphd=2;
      else if(c=='P'&&isbmphd==2) isbmphd=3;
      else isbmphd=0;
      if(isbmphd==3) break;
      ptr++;
    }

    if(isbmphd==3)
    {ctype[i]=DIRECTORY_ENTRY;
    cstart=cstart+ClusterSize;
    continue;}

    uint32_t temp=retrieve(cstart,2);
    if(temp==0x4d42)
    {ctype[i]=BMP_HEADER;
    cstart=cstart+ClusterSize;
    continue;}

    ctype[i]=UNCERTAIN;
    cstart=cstart+ClusterSize;
  }
}



uint32_t retrieve(const void *ptr,int byte)
{
    uint32_t p1,p2,p3,p4;
    switch(byte)
    {
      case 1:
        return (uint32_t)(*(unsigned char *)ptr);
      case 2:
        p1=*(unsigned char *)ptr;
        p2=*(unsigned char *)(ptr+1);
        return (uint32_t)((p2<<8)|p1);
      case 4:
        p1=*(unsigned char *)ptr;
        p2=*(unsigned char *)(ptr+1);
        p3=*(unsigned char *)(ptr+2);
        p4=*(unsigned char *)(ptr+3);
        return (uint32_t)((p4<<24)|(p3<<16)|(p2<<8)|p1);
      default:printf("bytes not aligned\n");assert(0);
    }
    return 0;
}

int GetSize(char *fname)
{
  FILE* fp=fopen(fname,"r");
  assert(fp);
  fseek(fp,0,SEEK_END);
  int ret=ftell(fp);
  rewind(fp);
  fclose(fp);
  return ret;
}

  void SetBasicAttributes(const struct fat_header* header)
  {
      uint32_t sectors = header->BPB_TotSec32-header->BPB_RsvdSecCnt-header->BPB_FATSz32*header->BPB_NumFATs;
      DataClusters = sectors/header->BPB_SecPerClus;
      ClusterSize=header->BPB_SecPerClus*header->BPB_BytePerSec;
      DataOffset=(header->BPB_RsvdSecCnt+header->BPB_NumFATs*header->BPB_FATSz32)*header->BPB_BytePerSec;
      printf("Data Region has 0x%x clusters\n",DataClusters);
      printf("Cluster Size is 0x%x bytes\n",ClusterSize);
      printf("Data Region started at 0x%x\n",DataOffset);
  }
