/* dnq data checksum Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  dnq data checksum, for app.
 * Note : 
 */


#include "dnq_checksum.h"

#define BUFSIZE     1024*4 

static U32 crc_table[256]; 

static void init_crc_table(void)    
{    
    U32 c;    
    U32 i, j;    
  
    for (i = 0; i < 256; i++)   
    {    
        c = (U32)i;    
  
        for (j = 0; j < 8; j++)   
        {    
            if (c & 1)    
                c = 0xedb88320L ^ (c >> 1);    
            else    
                c = c >> 1;    
        }    
  
        crc_table[i] = c;    
    }    
} 

/**
* @brief  CRC16-IBM校验
* @param   *addr需要校验数据首地址
*           num 需要校验的数据长度
* @retval  计算得到的数据低位在前 高位在后
*/  

U16 crc16(U8 *addr, U32 num ,U32 crc)
{  
    int i;  
    //u16 crc=0;					//CRC16-IBM初值
    U16 Over_crc=0;			//CRC16-IBM结果异或
    for (; num > 0; num--)              /* Step through bytes in memory */  
    {  
        crc = crc ^ (*addr++ << 8);     /* Fetch byte from memory, XOR into CRC top byte*/  
        for (i = 0; i < 8; i++)             /* Prepare to rotate 8 bits */  
        {  
            if (crc & 0x8000)            /* b15 is set... */  
                crc = (crc << 1) ^ POLY;    /* rotate and XOR with polynomic */  
            else                          /* b15 is clear... */  
                crc <<= 1;                  /* just rotate */  
        }                             /* Loop for 8 bits */  
        crc =crc^Over_crc;                  /* Ensure CRC remains 16-bit value */  
    }                               /* Loop until num=0 */  
    return(crc);                    /* Return updated CRC */  
}

/* 计算buffer的crc校验码 */    
U32 crc32(U32 crc, U8 *buffer, U32 size)    
{    
    U32 i;    

    crc = crc ^ 0xffffffff;
    for (i = 0; i < size; i++)   
    {    
        crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);    
    }    
  
    return crc ^ 0xffffffff ;    
}

/* 计算buffer的crc校验码 */    
U32 _crc32(U32 crc, U8 *buffer, U32 size)    
{    
    U32 i;    

    for (i = 0; i < size; i++)   
    {    
        crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);    
    }    
  
    return crc ;    
} 


/*  
* 计算大文件的CRC校验码:crc32函数,是对一个buffer进行处理,  
* 但如果一个文件相对较大,显然不能直接读取到内存当中  
* 所以只能将文件分段读取出来进行crc校验,  
* 然后循环将上一次的crc校验码再传递给新的buffer校验函数,  
* 到最后，生成的crc校验码就是该文件的crc校验码.(经过测试)  
*/    
S32 calc_img_crc(const U8 *file_name, U32 *img_crc)
{    
    int fd;    
    int nread;    
    int ret;    
    U8 buf[BUFSIZE];    

    /* 第一次传入的值需要固定,如果发送端使用该值计算crc校验码, 那么接收端也同样需要使用该值进行计算 */    
    U32 crc = 0xffffffff;     

    fd = open(file_name, O_RDONLY);    
    if (fd < 0)   
    {    
        printf("%d:open %s.\n", __LINE__, strerror(errno));    
        return -1;    
    }    

    while ((nread = read(fd, buf, BUFSIZE)) > 0)   
    {    
        crc = crc32(crc, buf, nread);    
    }    

    *img_crc = crc;    

    close(fd);    

    if (nread < 0)   
    {    
        printf("%d:read %s.\n", __LINE__, strerror(errno));    
        return -1;    
    }    

    return 0;    
}  

void dnq_checksum_init()
{
    init_crc_table();
}

int crc32_test(int argc, char **argv)    
{    
    int ret;    
    unsigned int img_crc;    
    const char *file_name = argv[1];    
  
    if (argc < 2)   
    {    
        fprintf(stderr, "Usage: %s input_file\n", "crc32");  
        exit(1);    
    }    
  
    init_crc_table();    
  
    ret = calc_img_crc(file_name, &img_crc);    
    if (ret < 0)   
    {    
        exit(1);    
    }    
  
    printf("The crc of %s is:%u\n", file_name, img_crc);    
  
    return 0;    
}    



