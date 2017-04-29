/* dnq log Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a log interface API, for app.
 * Note : 
 */
 

#include <stdarg.h>
#include "common.h"
#include "dnq_log.h"


static U8 g_dnq_dbg_module[DNQ_MOD_CNT] = {0};
static U8 g_dnq_dbg_lever[DNQ_MOD_CNT] = {0};
static U8 g_dbg_buf[2048] = {0};

int dnq_debug(U32 module_id, U32 lever, const char *fmt, ...)
{
    int n;
    int size = 1024;
    int all_module_lever;
    int current_module_lever;
    va_list ap;

    all_module_lever = g_dnq_dbg_lever[DNQ_MOD_ALL];
    current_module_lever = g_dnq_dbg_lever[module_id];

    /* print log message to console, set lever */
    if((all_module_lever > 0 && all_module_lever >= lever)\
        || (current_module_lever > 0 && current_module_lever >= lever)
        || module_id == DNQ_MOD_ALL \
        || lever == DNQ_DBG_ALL)
    {
        va_start(ap, fmt);
        //n = vsprintf(fmt, ap);
        vfprintf(stdout, fmt, ap);
        //n = vsnprintf(g_dbg_buf, size, fmt, ap);
        va_end(ap);
    }

    n = printf(g_dbg_buf, n);
        
    return n;
}

#if 0
dnq_log_t* dnq_log_init()
{
    dnq_log_t *log;

    log = (dnq_log_t)malloc(sizeof(dnq_log_t));
}
int dnq_log_deinit()
{
    free(log);
}

int dnq_log_open(U8 *filename)
{
    FILE *fp = NULL;
    fp = fopen(filename, "w+");
    if(pf == NULL)
    {
        DNQ_PRINT(DNQ_MOD_ALL, "fopen error: %s", strerror(errno));
    }
    return fp;
}

int dnq_log_close(FILE *fp)
{
    fclose(fp);
}

int dnq_log_write(char *buffer)
{
    int len ;
    len = fwrite(buffer, 1, strlen(buffer), fp);
    if(len < 0)
        DNQ_PRINT(DNQ_MOD_ALL, "fwrite error: %s", strerror(errno));
    return len;
}
#endif


int dnq_debug_setlever(U32 module_id, U32 lever)
{
    /*
    * 0:NONE 1:ERROR, 2:WARN, 3:INFO, 4:DEBUG 5:ALL
    */
    g_dnq_dbg_lever[module_id] = lever;
    return lever;
}

int dnq_debug_test(int lever)
{
    dnq_debug_setlever(1, lever);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "INFO: test!");
    DNQ_WARN(DNQ_MOD_RABBITMQ, "WARN: test!");
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "DEBUG: test!");
    DNQ_ERROR(DNQ_MOD_RABBITMQ, "ERROR: test!");
    DNQ_PRINT(DNQ_MOD_RABBITMQ, "PRINT: test!\n");
}
