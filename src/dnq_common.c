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

static ngx_pool_t *mem_pool;

S32 dnq_init()
{
    ngx_pool_t * pool = NULL;;
    pool = dnq_mempool_init(1024*1024);
    if(!pool)
        return -1;
    mem_pool = pool;
    return 0;
}

S32 dnq_deinit()
{
    dnq_mempool_deinit(mem_pool);
    return 0;
}

