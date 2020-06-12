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
ATTR_LONG_NAME=0xF};
enum CLUSTER_TYPE{DIRECTORY_ENTRY,BMP_HEADER,BMP_DATA,UNUSED,UNCERTAIN};
//一个cluster内可能包含多个DIRECTORY_ENTRY;
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
  uint8_t DIR_Name[11];//名字
  uint8_t DIR_Attr;//属性
  uint8_t DIR_NTRes;//大小写相关
  uint8_t DIR_CrtTimeTenth;
  uint32_t DIR_CrtTime:16;
  uint32_t DIR_CrtDate:16;
  uint32_t DIR_LstAccDate:16;
  uint32_t DIR_FstClusHI:16;//指向的块数高位
  uint32_t DIR_WrtTime:16;
  uint32_t DIR_WrtDate:16;
  uint32_t DIR_FstClusLO:16;//指向的块数低位
  uint32_t DIR_FileSize;//文件大小
}__attribute__((packed));

struct ldir_entry
{
  uint8_t LDIR_Ord;//长文件头编号
  uint8_t LDIR_Name1[10];//Name Part1
  uint8_t LDIR_Attr;//属性
  uint8_t LDIR_Type;//0
  uint8_t LDIR_Chksum;//用于校验
  uint8_t LDIR_Name2[12];//Name Part2
  uint32_t LDIR_FstClusLO:16;//0
  uint8_t LDIR_Name3[4];//Name part3
}__attribute__((packed));

struct bitmap_header//事实上它包含了信息头的一部分
{
  uint8_t bfType[2];
  uint32_t bfSize;//文件大小
  uint32_t bfReserved1:16;
  uint32_t bfReserved2:16;
  uint32_t bfOffBits;//数据的偏移量
  uint32_t biSize;
  uint32_t biWidth;//宽度
  uint32_t biHeight;//高度
  uint32_t biPlanes:16;
  uint32_t biBitCount:16;
  uint32_t biCompression;
  uint32_t biSizeImage;
  uint32_t biXPelsPexMeter;
  uint32_t biYPelsPexMeter;
  uint32_t biClrUsed;
  uint32_t BiClrImportant;
}__attribute__((packed));

int ctype[1000000];//记录cluster的type
int GetSize(char *fname);//得到文件大小
void SetBasicAttributes(const struct fat_header* header);//计算文件的一些属性
uint32_t retrieve(const void *ptr,int byte);//从ptr所指的位置取出长为byte的数据
void ScanCluster(const void* header);
void Recover(const void* header);

int main(int argc, char *argv[]) {
assert(sizeof(struct fat_header)==512);
assert(sizeof(struct sdir_entry)==32);
assert(sizeof(struct ldir_entry)==32);
assert(sizeof(struct bitmap_header)==54);
char fname[128]="/home/ebata/img/M5-frecov.img";
int fsize=GetSize(fname);
int fd=open(fname,O_RDONLY);
assert(fd>=0);

const struct fat_header* fh=(struct fat_header*)mmap((void *)0x700000000000,fsize,
PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE,fd,0);//确认读到文件头了
assert(fh->signature_word==0xaa55);
SetBasicAttributes(fh);
ScanCluster(fh);
Recover(fh);
}

uint8_t Chksum(unsigned char* pFcbName)
{
  uint16_t FcbNameLen;
  unsigned char sum;
  sum=0;
  for(int i=11;i!=0;i--)
  {
    sum=((sum&1)?0x80:0)+(sum>>1)+*pFcbName++;
  }
  return sum;
}

void call_null()
{
  printf("NMSL");
}
void Recover(const void* header)
{ 
for(int i=0;i<DataClusters;i++)
{
  if(ctype[i]!=DIRECTORY_ENTRY) continue;
  char *cptr=(char *)header+DataOffset+i*ClusterSize;//当前块的起始
    printf("Locating Directory_Entry at %x\n",(unsigned)(cptr-(char*)header));
  //先定位到BMP字符串
  while(1)
  {
          if(cptr>=(char *)header+DataOffset+(i+1)*ClusterSize) break;//偏移量超出,退出
          if(!((*cptr=='B')&&(*(cptr+1)=='M')&&(*(cptr+2)=='P')))
          {cptr++;
          continue;}
          printf("Located bmp at %x\n",(unsigned)(cptr-(char*)header));
          char name[1024];
          memset(name,'\0',sizeof(name));
          struct sdir_entry* sdir=(struct sdir_entry* )(cptr-8);
          struct ldir_entry* ldir=(struct ldir_entry* )(cptr-40);

          if(Chksum((unsigned char*)sdir)!=ldir->LDIR_Chksum) {cptr++;continue;}
          //匹配失败,不管了
          int no=1;
          //不管文件名长短都会有长目录项,只从长目录项里读名字;
          while(1)//提取每一个长文件目录项里的文件名Part
          {
            if((void *)ldir<(void *)header+DataOffset+i*ClusterSize) break;//向下越界，不读了
            int reachend=0;
            if(ldir->LDIR_Ord==(no|0x40)) reachend=1;
            char name_tp[25];
            int pos=0;
            for(int j=0;j<5;j++)
            {
              if(ldir->LDIR_Name1[2*j]!=0xFF)
                name_tp[pos++]=(char)ldir->LDIR_Name1[2*j];
              else reachend=1;
            }

            for(int j=0;j<6;j++)
            {
              if(ldir->LDIR_Name2[2*j]!=0xFF)
              name_tp[pos++]=(char)ldir->LDIR_Name2[2*j];
              else reachend=1;
            }

            for(int j=0;j<2;j++)
            {
              if(ldir->LDIR_Name3[2*j]!=0xFF)
              name_tp[pos++]=(char)ldir->LDIR_Name3[2*j];
              else reachend=1;
            }

            strcat(name,name_tp);
            if(reachend) break;
            ldir=ldir-1;
            no=no+1;
          }
          printf("%s\n",name);

          //this field:recover data
          uint32_t cid=((sdir->DIR_FstClusHI<<16)|sdir->DIR_FstClusLO)-2;//虽然不知道为什么要减2
          struct bitmap_header* bheader=(struct bitmap_header* )(void *)(header+ClusterSize*cid+DataOffset);
          printf("bheader at %p Offset:%x\n",bheader,ClusterSize*cid+DataOffset);
          if(ctype[cid]==BMP_HEADER)//定位到BMP头才进行恢复
          {
            uint32_t bmpsize=bheader->bfSize;
            uint32_t bmpoffset=bheader->bfOffBits;
            uint32_t height=bheader->biHeight;
            uint32_t width=bheader->biWidth;
            printf("BMP Data Offset:%x\n",bheader->bfOffBits);
          }
          //this field:recover data
    cptr++;
  }
}
}

void ScanCluster(const void* header)
{
  void* cstart=(void *)header+DataOffset;
  for(int i=0;i<DataClusters;i++,cstart=cstart+ClusterSize)
  {
    void *ptr=cstart;
    int isbmphd=0; 
    int ct=0;
    for(int j=0;j<ClusterSize;j++)
    {
      char c=retrieve(ptr,1);
      if(c=='B'&&isbmphd==0) isbmphd=1;
      else if(c=='M'&&isbmphd==1) isbmphd=2;
      else if(c=='P'&&isbmphd==2) isbmphd=3;
      else isbmphd=0;
      if(isbmphd==3) 
      {
        ct++;isbmphd=0;
      }
      ptr++;
    }

    if(ct>=5)//认为BMP至少出现五次才是目录项
    {ctype[i]=DIRECTORY_ENTRY;
    continue;}

    uint32_t temp=retrieve(cstart,2);
    if(temp==0x4d42)
    {ctype[i]=BMP_HEADER;
    continue;}

    ctype[i]=UNCERTAIN;
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
  printf("FileSize=%x\n",ret);
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
      printf("FAT size=%x\n",header->BPB_FATSz32*header->BPB_BytePerSec);
  }
