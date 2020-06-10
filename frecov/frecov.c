#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<assert.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/mman.h>
#include<sys/stat.h>
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
    uint32_t signature_word:16;//0x55AA
}__attribute__((packed));

struct fat_table
{

};

struct cluster
{

};

int GetSize(char *fname);//得到文件大小
uint32_t GetDataClusters(struct fat_header* header);//得到数据区的cluster数
uint32_t retrieve(const void *ptr,int byte);//从ptr所指的位置取出长为byte的数据

int main(int argc, char *argv[]) {
assert(sizeof(struct fat_header)==512);
char fname[128]="/home/ebata/img/M5-frecov.img";
int fsize=GetSize(fname);
printf("fsize=%d\n",fsize);
int fd=open(fname,O_RDONLY);
assert(fd>=0);

struct fat_header* fh=(struct fat_header*)mmap(NULL,fsize,
PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE,fd,0);
printf("%x\n",retrieve(fh,4));
}
uint32_t retrieve(const void *ptr,int byte)
{
  uint32_t p1,p2,p3,p4;

  switch(byte)
  {
    case 1:
      return (uint32_t)(*(char *)ptr);
    case 2:
      p1=(uint32_t)(*(char *)ptr);
      p2=(uint32_t)(*(char *)(ptr+1));
      return (uint32_t)((p2<<8)|p1);
    case 4:
      p1=(uint32_t)(*(char *)ptr);
      p2=(uint32_t)(*(char *)(ptr+1));
      p3=(uint32_t)(*(char *)(ptr+2));
      p4=(uint32_t)(*(char *)(ptr+3));
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
return ret;
}

uint32_t GetDataClusters(struct fat_header* header)
{
    uint32_t sectors = header->BPB_TotSec32-header->BPB_RsvdSecCnt-header->BPB_FATSz32*header->BPB_NumFATs;
    uint32_t clusters = sectors/header->BPB_SecPerClus;
    return clusters;
}
