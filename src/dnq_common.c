/* dnq common Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a common interface API, for app.
 * Note : 
 */


#include "dnq_common.h"
#include "ngx_palloc.h"
#include "cJSON.h"

static ngx_pool_t *mem_pool;


/**
  * @brief  CRC16-IBM校验
	* @param 	*addr需要校验数据首地址
						 num 需要校验的数据长度
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

S32 dnq_init()
{
    ngx_pool_t * pool = NULL;
    
    if(mem_pool)  /* already inited */
        return 0; 
    
    pool = dnq_mempool_init(1024*1024);
    if(!pool)
        return -1;
    
    cJSON_Hooks hooks = {dnq_malloc, dnq_free};
    cJSON_InitHooks(&hooks);
    
    mem_pool = pool;
    return 0;
}

S32 dnq_deinit()
{
    dnq_mempool_deinit(mem_pool);
    return 0;
}

