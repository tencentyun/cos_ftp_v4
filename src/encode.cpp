#include <iconv.h>  
#include <stdlib.h>
#include <string.h>
#include "encode.h"

/************************************************************************ 
功能说明：判断一个字符串是不是UTF-8编码 
输入参数： 
  参数1: 源字符串 
   
返回：false, 字符串不是UTF-8编码 true, 字符串是UTF-8编码 
**************************************************************************/  
bool IsUtf8(unsigned char *pszStr)  
{  
    int dwLen = 0;  
    if(pszStr == NULL || (dwLen = strlen((char*)pszStr)) == 0)  
    {  
        return false;  
    }     
    unsigned int udwUtf8Len = 0; 
	unsigned int udwUnkouwnLen = 0; 
      
    int i;  
    for(i = 0; i < dwLen;)  
    {  
        unsigned char cChar = pszStr[i];  
          
        if(cChar < 0x7f)  
        {  
            //udwUtf8Len++;  
            i++;  
            continue;  
        }  
          
        if(cChar < 0xc0 || cChar > 0xfd)  
        {  
            i++;  
			udwUnkouwnLen++;
            continue;  
        }  
          
        int dwCount;  
        if(cChar >= 0xfc)  
        {  
            dwCount = 5;  
        }  
        else if(cChar >= 0xf8)  
        {  
            dwCount = 4;  
        }  
        else if(cChar >= 0xf0)  
        {  
            dwCount = 3;  
        }  
        else if(cChar >= 0xe0)  
        {  
            dwCount = 2;  
        }  
        else   
        {  
            dwCount = 1;  
        }  
          
        if(dwCount + i > dwLen)  
        {  
            i++;  
			udwUnkouwnLen++;
            continue;  
        }  
          
        i++;  
          
        int j;  
        for(j = 0; j < dwCount; j++)  
        {  
            cChar = pszStr[i];  
            if(cChar < 0x80 || cChar > 0xbf)  
            {  
				udwUnkouwnLen += 2;
                break;                
            }  
            i++;  
        }  
        if(j >= dwCount)  
        {  
            udwUtf8Len += dwCount + 1;            
        }  
    }     
      
    //统计字符串中满足utf8编码格式的字符个数(asc码[1,127]也算满足)，如果个数大于字符串长度的0.85，则认为是utf8编码  
    //if(udwUtf8Len > dwLen * 0.9)  
	if(udwUtf8Len >= (udwUnkouwnLen + udwUtf8Len) * 0.9)
    {  
        return true;  
    }  
    else  
    {  
        return false;  
    }  
}  
  
/************************************************************************ 
功能说明：判断一个字符串是不是gb2312的常用字 
输入参数： 
  参数1: 源字符串 
   
返回：false, 字符串不是gb2312的常用字  true, 字符串是gb2312的常用字 
**************************************************************************/  
bool IsGBKChin(unsigned char *pszStr)  
{  
    int dwLen = 0;  
    if(pszStr == NULL || (dwLen = strlen((char*)pszStr)) == 0)  
    {  
        return 0;  
    }  
  
    int i;  
    for(i = 0; i < dwLen;)  
    {  
        unsigned char cChar = pszStr[i];  
        if(cChar < 0x7f)  
        {             
            i++;  
            continue;  
        }  
  
        if(i >= dwLen - 1)  
        {  
            return false;  
        }  
          
        unsigned char c1, c2;  
        c1 = (unsigned char)pszStr[i];  
        c2 = (unsigned char)pszStr[i+1];          
          
        if((c1 >= 0xB0 && c1 <= 0xF7) && (c2 >= 0xA1 && c2 <= 0xFE))  
        {  
            i += 2;  
            continue;  
        }  
        else  
        {  
            return false;         
        }  
    }  
  
    return true;  
}  
  
  
/************************************************************************ 
功能说明：编码转换函数 
输入参数： 
  参数1: 源字符串编码格式 "GBK",
  参数2: 目标字符串编码格式 "UTF-8"
  参数3 源字符串 
  参数4 源字符串长度 
  参数5 目标字符串的内存空间 
  参数6 目标字符串最大允许长度 
备注： 
  需要包括头文件 #include <iconv.h> 
  
返回：0, 函数完成 -1 ，函数失败 
**************************************************************************/  
int CodeConvert(const char *from_charset, const char *to_charset, const char *inbuf, size_t inlen, char *outbuf, size_t outlen)  
  
{  
     iconv_t cd;  
     int rc;  
     char **pin = (char**)&inbuf;  
     char **pout = &outbuf;  
        
     cd = iconv_open(to_charset,from_charset);  
     if (cd == 0)  
     {  
      return -1;  
     }  
       
     memset(outbuf, '\0', outlen);  
     rc = iconv(cd,pin,&inlen,pout,&outlen);  
     while((rc == -1) && (inlen > 0) && (outlen > 0))  
     {  
      inbuf++;  
      inlen--;  
      pin = (char**)&inbuf;  
      pout = &outbuf;  
      rc = iconv(cd,pin,&inlen,pout,&outlen);  
     }  
       
     iconv_close(cd);  
     return 0;  
}  

