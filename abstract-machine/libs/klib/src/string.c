#include <klib.h>
#include <amdev.h>

size_t strlen(const char *s) {
  size_t len=0;
  const char*temp=s;
  while(*temp)
  {len++;
   temp++;
  }
  return len;
}

char *strcpy(char* dst,const char* src) {
  size_t size_d=strlen(dst),size_s=strlen(src);
  assert(dst+size_d<=src||src+size_s<=dst);
  for(int i=0;i<size_s;i++)
  *(dst+i)=*(src+i);
  return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
   size_t size_d=strlen(dst),size_s=strlen(src);
   assert(dst+size_d<=src||src+size_s<=dst);
   for(int i=0;i<n;i++)
   {if(i<size_s)
   *(dst+i)=*(src+i);
   else
   *dst='\0';
   }
   return dst;
}

char* strcat(char* dst, const char* src) {
  size_t size_d=strlen(dst),size_s=strlen(src);
  assert(dst+size_d<=src||src+size_s<=dst);
  for(int i=0;i<size_s;i++)
  *(dst+size_d+i)=*(src+i);
  *(dst+size_d+size_s)='\0';
  return dst;
}

int strcmp(const char* s1, const char* s2) {
  const char *s1_cp=s1;
  const char *s2_cp=s2;
  while(*s1_cp==*s2_cp&&*s1_cp&&*s2_cp)
  {s1_cp++;s2_cp++;}
  return (unsigned)*s1_cp-(unsigned)*s2_cp;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    const char *s1_cp=s1;
    const char *s2_cp=s2;
    while(*s1_cp==*s2_cp&&*s1_cp&&*s2_cp&&n)
    {s1_cp++;s2_cp++;n--;}
    return (unsigned)*s1_cp-(unsigned)*s2_cp;
}


void* memset(void* v,int c,size_t n) {
  assert(v&&n>=0);
  char* v_tp=(char*)v;
  while(n--)
  {*v_tp=c;
  v_tp++;}
  return v;
}

void* memcpy(void* out, const void* in, size_t n) {
  assert(out&&in&&n);
  char* src=(char*)in;
  char* dest=(char*)out;
  while(n--)
  {*dest=*src;
  dest++;src++;
  }
  return out;
} 

int memcmp(const void* s1, const void* s2, size_t n){
  assert(s1&&s2&&n>=0);
  char *s1_cp=(char *)s1;
  char *s2_cp=(char *)s2;
  while(n--){
  if(*s1_cp==*s2_cp)
  {if(n==0) return 0;
  s1_cp++;s2_cp++;}
  else break;
  }
  if(*s1_cp>*s2_cp) return 1;
  else if(*s1_cp<*s2_cp) return -1; 
  return 0;
}

void *memmove(void* dst, const void* src, size_t n)
{return NULL;}
char *strtok(char* s,const char* delim)
{return NULL;}
char *strstr(const char *s1, const char *s2)
{return NULL;}
char *strchr(const char *s, int c)
{return NULL;}
char *strrchr(const char *s, int c)
{return NULL;}