#include <klib.h>
#include <amdev.h>

char minstr[4][50]={"-128","-32768","-2147483648","-9223372036854775808"};
long long llmin=-9223372036854775807LL-1LL;
char NIL[10]="(nil)";
//base为输出的进制,dtype为输入的数据类型,依次为char(0),unsigned char(1),short(2),unsigned short(3),int(4),unsigned(5),long(6),unsigned long(7),long long(8),unsigned long long(9)
//up为是否大写,0小写,1大写
//prefix16进制是否需要前缀
void itoa(char *dest,long long src,int base,int dtype,int up,int prefix,int is_zero_extend,int is_width_limit,int width);

int printf(const char *fmt, ...) {
char out[1024]={'\0'};  
va_list ap;
va_start(ap,fmt);
int len=vsprintf(out,fmt,ap);
for(int i=0;i<len;i++)
_putc(out[i]);
va_end(ap);
return len;
}


int vsprintf(char *out, const char *fmt, va_list ap) {
   char *str=out;
   uint32_t width=0;//限制输出宽度
   uint8_t is_width_limit=0;
   uint8_t is_zero_extend=0;
   int8_t c;
   uint8_t uc;
   int16_t sd;
   uint16_t usd;
   int32_t d;
   uint32_t ud;
   int64_t ld;
   uint64_t uld;
   char *s;
   char d2s[50];
   for(;*fmt;fmt++)
   {
     if((*fmt!='%')&&(*fmt!=0x5c))
     {*str=*fmt;str++;
      continue;
     }//如果没匹配则直接到下一个字符
     else if(*fmt==0x5c)//匹配到格式符
     {fmt++;
      switch (*fmt)
      {case 'n':*str=(char)0xa;str++; break;
       default:{}
      }
      continue;
     }
     assert(*fmt=='%');
     width=0;//匹配到限制符
     is_width_limit=0;
     is_zero_extend=0;
     fmt++;
     while(*fmt==' ') fmt++;
     if(*fmt=='0') 
     {is_zero_extend=1;
     fmt++;}
     while((*fmt)>='0'&&(*fmt)<='9')
     {is_width_limit=1;
     width=width*10+(*fmt-'0');
     fmt++;}
     
     
     switch(*fmt)
     {case 'c':uc=va_arg(ap,int);
               *str++=(char)uc;
               *str='\0';break;
      case 'd': d=va_arg(ap,int);
                if(d==0) {*str='0';str++;break;}
                if(d==-2147483648){
                  strcat(out,minstr[2]);
                  while(*str) str++;
                  break;}
                if(d<0)  {*str='-';str++;d=-d;}
                itoa(d2s,d,10,4,0,0,is_zero_extend,is_width_limit,width);          
                *str='\0';
                strcat(out,d2s);
                while(*str)str++;
                break;
      case 'u': ud=va_arg(ap,unsigned);
                if(ud==0) {*str='0';str++;break;}
                itoa(d2s,ud,10,5,0,0,is_zero_extend,is_width_limit,width);          
                *str='\0';
                strcat(out,d2s);
                while(*str)str++;
                break;
      case 'x':ud=va_arg(ap,unsigned);
               if(ud==0) {*str='0';str++;break;}
               itoa(d2s,ud,16,5,0,0,is_zero_extend,is_width_limit,width);
               *str='\0';
               strcat(out,d2s);
               while(*str)str++;
               break;
      case 'X':ud=va_arg(ap,unsigned);
               if(ud==0) {*str='0';str++;break;}
               itoa(d2s,ud,16,5,1,0,is_zero_extend,is_width_limit,width);
               *str='\0';
               strcat(out,d2s);
               while(*str)str++;
               break;
      case 'o':ud=va_arg(ap,unsigned);
               if(ud==0) {*str='0';str++;break;}
               itoa(d2s,ud,8,5,1,0,is_zero_extend,is_width_limit,width);
               *str='\0';
               strcat(out,d2s);
               while(*str)str++;
               break;
      case 'p':uld=va_arg(ap,unsigned long long);
               if(uld==0){
                  strcat(out,NIL);
                  while(*str) str++;
                  break;}
               itoa(d2s,uld,16,9,0,1,is_zero_extend,is_width_limit,width);
               *str='\0';
               strcat(out,d2s);
               while(*str)str++;
               break;
      case 's':s=va_arg(ap,char*);
               if(*s)
               {*str='\0';
               strcat(out,s);
               }
               while(*str) str++;
               break;
      case 'h':fmt++;
              switch(*fmt){
                  case 'd':sd=va_arg(ap,int);
                          if(sd==0) {*str='0';str++;break;}
                          if(sd==-32768){
                          strcat(out,minstr[1]);
                          while(*str) str++;
                          break;}
                          if(sd<0)  {*str='-';str++;sd=-sd;}
                          itoa(d2s,sd,10,2,0,0,is_zero_extend,is_width_limit,width);          
                          *str='\0';
                          strcat(out,d2s);
                          while(*str)str++;
                          break;
                  case 'u':usd=va_arg(ap,unsigned);
                           if(usd==0) {*str='0';str++;break;}
                           itoa(d2s,usd,10,3,0,0,is_zero_extend,is_width_limit,width);          
                           *str='\0';
                           strcat(out,d2s);
                           while(*str)str++;
                           break;
                  case 'h':fmt++;
                          if(*fmt=='d')
                          {c=va_arg(ap,unsigned);
                          if(c==0) {*str='0';str++;break;}
                          if(c==-128){
                          strcat(out,minstr[0]);
                          while(*str) str++;
                          break;}
                          if(c<0)  {*str='-';str++;c=-c;}
                          itoa(d2s,c,10,0,0,0,is_zero_extend,is_width_limit,width);          
                          *str='\0';
                          strcat(out,d2s);
                          while(*str)str++;
                          }
                          else if(*fmt=='u')
                          {uc=va_arg(ap,int);
                           if(uc==0) {*str='0';str++;break;}
                           itoa(d2s,uc,10,3,0,0,is_zero_extend,is_width_limit,width);          
                           *str='\0';
                           strcat(out,d2s);
                           while(*str)str++;
                          }break;
                  default:break;}
              break;
      case 'l':fmt++;
              switch(*fmt){
                  case 'd':ld=va_arg(ap,long);
                          if(ld==0) {*str='0';str++;break;}
                          if(ld==llmin){
                          strcat(out,minstr[3]);
                          while(*str) str++;
                          break;}
                          if(ld<0)  {*str='-';str++;ld=-ld;}
                          itoa(d2s,ld,10,6,0,0,is_zero_extend,is_width_limit,width);          
                          *str='\0';
                          strcat(out,d2s);
                          while(*str)str++;
                          break;
                  case 'u':uld=va_arg(ap,unsigned long);
                           if(uld==0) {*str='0';str++;break;}
                           itoa(d2s,uld,10,7,0,0,is_zero_extend,is_width_limit,width);          
                           *str='\0';
                           strcat(out,d2s);
                           while(*str)str++;
                           break;
                  case 'l':fmt++;
                          if(*fmt=='d')
                          {ld=va_arg(ap,long long);
                          if(ld==0) {*str='0';str++;break;}
                          if(ld==llmin){
                          strcat(out,minstr[3]);
                          while(*str) str++;
                          break;}
                          if(ld<0)  {*str='-';str++;ld=-ld;}
                          itoa(d2s,ld,10,8,0,0,is_zero_extend,is_width_limit,width);          
                          *str='\0';
                          strcat(out,d2s);
                          while(*str)str++;
                          }
                          else if(*fmt=='u')
                          {uld=va_arg(ap,unsigned long long);
                           if(uld==0) {*str='0';str++;break;}
                           itoa(d2s,uld,10,9,0,0,is_zero_extend,is_width_limit,width);          
                           *str='\0';
                           strcat(out,d2s);
                           while(*str)str++;
                          }break;
                  default:break;}
              break;
      default:break;
     }
   }
   *str='\0';
 return str-out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap,fmt);
  vsprintf(out,fmt,ap);
  va_end(ap);
  return strlen(out);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap,fmt);
  vsprintf(out,fmt,ap);
  va_end(ap);
  int len=strlen(out);
  if(len>n)
  {for(int i=n;i<len;i++)
   *(out+n)='\0';
  }
  return len;
}
void itoa(char *dest,long long src,int base,int dtype,int up,int prefix,int is_zero_extend,int is_width_limit,int width)
{
char temp[50];
if(dtype==0)
{char msrc=(char)src;
char mbase=(char)base;
for(int i=0;msrc;msrc=msrc/mbase,i++)
{char bit=msrc%mbase;
if(bit>=0&&bit<=9) temp[i]=(char)('0'+bit);
else if(bit>=10&&bit<=15) 
{ if(up)
  temp[i]=(char)('A'+bit-10);
  else
  temp[i]=(char)('a'+bit-10);
}
if((msrc/mbase)==0)temp[i+1]='\0';
}}
if(dtype==1)
{unsigned char msrc=(unsigned char)src;
unsigned char mbase=(unsigned char)base;
for(int i=0;msrc;msrc=msrc/mbase,i++)
{unsigned char bit=msrc%mbase;
if(bit>=0&&bit<=9) temp[i]=(char)('0'+bit);
else if(bit>=10&&bit<=15) 
{ if(up)
  temp[i]=(char)('A'+bit-10);
  else
  temp[i]=(char)('a'+bit-10);
}
if((msrc/mbase)==0)temp[i+1]='\0';
}}
if(dtype==2)
{short msrc=(short)src;
short mbase=(short)base;
for(int i=0;msrc;msrc=msrc/mbase,i++)
{short bit=msrc%mbase;
if(bit>=0&&bit<=9) temp[i]=(char)('0'+bit);
else if(bit>=10&&bit<=15) 
{ if(up)
  temp[i]=(char)('A'+bit-10);
  else
  temp[i]=(char)('a'+bit-10);
}
if((msrc/mbase)==0)temp[i+1]='\0';
}}
if(dtype==3)
{unsigned short msrc=(unsigned short)src;
unsigned short mbase=(unsigned short)base;
for(int i=0;msrc;msrc=msrc/mbase,i++)
{unsigned short bit=msrc%mbase;
if(bit>=0&&bit<=9) temp[i]=(char)('0'+bit);
else if(bit>=10&&bit<=15) 
{ if(up)
  temp[i]=(char)('A'+bit-10);
  else
  temp[i]=(char)('a'+bit-10);
}
if((msrc/mbase)==0)temp[i+1]='\0';
}}
if(dtype==4)
{int msrc=(int)src;
int mbase=(int)base;
for(int i=0;msrc;msrc=msrc/mbase,i++)
{int bit=msrc%mbase;
if(bit>=0&&bit<=9) temp[i]=(char)('0'+bit);
else if(bit>=10&&bit<=15) 
{ if(up)
  temp[i]=(char)('A'+bit-10);
  else
  temp[i]=(char)('a'+bit-10);
}
if((msrc/mbase)==0)temp[i+1]='\0';
}}
if(dtype==5)
{
unsigned msrc=(unsigned)src;
unsigned mbase=(unsigned)base;
for(int i=0;msrc;msrc=msrc/mbase,i++)
{unsigned bit=msrc%mbase;
if(bit>=0&&bit<=9) temp[i]=(char)('0'+bit);
else if(bit>=10&&bit<=15) 
{ if(up)
  temp[i]=(char)('A'+bit-10);
  else
  temp[i]=(char)('a'+bit-10);
}
if((msrc/mbase)==0)temp[i+1]='\0';
}}
if(dtype==6)
{long msrc=(long)src;
long mbase=(long)base;
for(int i=0;msrc;msrc=msrc/mbase,i++)
{long bit=msrc%mbase;
if(bit>=0&&bit<=9) temp[i]=(char)('0'+bit);
else if(bit>=10&&bit<=15) 
{ if(up)
  temp[i]=(char)('A'+bit-10);
  else
  temp[i]=(char)('a'+bit-10);
}
if((msrc/mbase)==0)temp[i+1]='\0';
}}
if(dtype==7)
{unsigned long msrc=(unsigned long)src;
unsigned long mbase=(unsigned long)base;
for(int i=0;msrc;msrc=msrc/mbase,i++)
{unsigned long bit=msrc%mbase;
if(bit>=0&&bit<=9) temp[i]=(char)('0'+bit);
else if(bit>=10&&bit<=15) 
{ if(up)
  temp[i]=(char)('A'+bit-10);
  else
  temp[i]=(char)('a'+bit-10);
}
if((msrc/mbase)==0)temp[i+1]='\0';
}}
if(dtype==8)
{long long msrc=(long long)src;
long long mbase=(long long)base;
for(int i=0;msrc;msrc=msrc/mbase,i++)
{long long bit=msrc%mbase;
if(bit>=0&&bit<=9) temp[i]=(char)('0'+bit);
else if(bit>=10&&bit<=15) 
{ if(up)
  temp[i]=(char)('A'+bit-10);
  else
  temp[i]=(char)('a'+bit-10);
}
if((msrc/mbase)==0)temp[i+1]='\0';
}}
if(dtype==9)
{unsigned long long msrc=(unsigned long long)src;
unsigned long long mbase=(unsigned long long)base;
for(int i=0;msrc;msrc=msrc/mbase,i++)
{unsigned long long bit=msrc%mbase;
if(bit>=0&&bit<=9) temp[i]=(char)('0'+bit);
else if(bit>=10&&bit<=15) 
{ if(up)
  temp[i]=(char)('A'+bit-10);
  else
  temp[i]=(char)('a'+bit-10);
}
if((msrc/mbase)==0)temp[i+1]='\0';
}}

if(base==16&&prefix) 
{*dest='0';
*(dest+1)='x';
dest=dest+2;}
int len=strlen(temp);
if(len>width&&is_width_limit) len=width;
if(is_zero_extend&&is_width_limit)
{for(int i=0;i<width;i++)
{
if(i<width-len) dest[i]='0';
else dest[i]=temp[width-i-1];
}
}
else
{
for(int i=0;i<len;i++)
dest[i]=temp[len-i-1];
}
dest[len]='\0';

}