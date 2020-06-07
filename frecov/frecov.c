#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<assert.h>
typedef FAT_HEADER fat_header
struct FAT_HEADER
{
uint8_t BS_jmpBoot[3];//跳转到boot code
uint8_t BS_OEMName[8];//代工厂名字?
uint32_t BPB_BytePerSec:16;//每一个扇区的字节数
uint8_t BPB_SecPerClus;//每一个簇的扇区数
uint32_t BPB_RsvdSecCnt:16;
uint8_t BPB_NumFATs;
uint32_t BPB_RootEntCnt:16;
uint32_t BPB_TotSec16:16;
uint8_t BPB_Media;
uint32_t BPB_FATSz16:16;
uint32_t BPB_SecPerTrk:16;
uint32_t BPB_NumHeads:16;
uint32_t BPB_HiddSec;
uint32_t BPB_TotSec32;
uint32_t BPB_FATSz32;
uint32_t BPB_ExtFlags:16;
uint32_t BPB_FSVer:16;
uint32_t BPB_RootClus;
uint32_t BPB_FSinfo:16;
uint32_t BPB_BkBootSec:16;
uint8_t BPB_Reserved[12];
uint8_t BS_DrvNum;
uint8_t BS_Reserved1;
uint8_t BS_BootSig;
uint32_t BS_VolID;
uint8_t BS_VolLab[11];
uint8_t BS_FilSysType[8];//字符串"FAT32"
uint8_t padding[420];//补充空位
uint32_t signature_word:16;//0x55AA
}__attribute__((packed)) ;

int main(int argc, char *argv[]) {
    printf("%d\n",(int)sizeof(fat_header));
assert(sizeof(fat_header)==512);

}