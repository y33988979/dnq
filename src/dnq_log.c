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
#include "dnq_os.h"

static U8 g_dnq_dbg_module[DNQ_MOD_CNT] = {0};
static U8 g_dnq_dbg_lever[DNQ_MOD_CNT] = {0};
static U8 g_dbg_buf[2048] = {0};

typedef struct debug_module
{
    U32  id;
    U8   name_desc[32];
}debug_module_t;

static debug_module_t g_dbg_modules[DNQ_MOD_CNT] =
{
    0,               "none",
    DNQ_MOD_ALL,     "All Module",
    DNQ_MOD_KEYPAD,  "Keypad Module",
    DNQ_MOD_UART,    "Uart   Module",
    DNQ_MOD_LCD,     "LCD    Module",
    DNQ_MOD_MCU,     "MCU    Module",
    DNQ_MOD_RABBITMQ,"RABBITMQ Module",
    DNQ_MOD_OS,      "OS     Module",
    DNQ_MOD_RTC,     "RTC    Module",
    DNQ_MOD_NETWORK, "NETWORK Module",
};

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
void debug_help()
{
    U32 i;
    printf("==========debug help:============\n");
    printf("usage:   debugset [module_id] [lever]\n");
    printf("lever: 0:none 1:error 2:warn 3:info 4:debug 5:all\n");
    printf("example: debugset 1 2\n");
    for(i=0; i<DNQ_MOD_CNT; i++)
        printf("%d: %s\n", g_dbg_modules[i].id, g_dbg_modules[i].name_desc);
    printf("=================================\n");
}

void dnq_debug_control()
{
    U32  module_id;
    U32  dbg_lever;
    U32  len;
    char *ptr = NULL;
    char cmdline[512];

    sleep(1);
    printf("DNQ_V2 > ");
    fflush(stdout);
    while(1)
    {
        memset(cmdline, 0, sizeof(cmdline));
        ptr = fgets(cmdline, sizeof(cmdline), stdin);
        if(ptr == NULL)
        {
            usleep(200*1000);
            continue;
        }

        //printf("input=%s", cmdline);
        if(strncmp(cmdline, "help", 4) == 0)
        {
            debug_help();
            continue;
        }
        if(strncmp(cmdline, "debugset ", strlen("debugset ")) != 0)
        {
            printf("DNQ_V2 > ");
            fflush(stdout);
            continue;
        }

        len = strlen(cmdline);
        while(len--)
        {
            if(cmdline[len] == ' ' || cmdline[len] == '\n')
                cmdline[len] = '\0';
        }

        ptr = cmdline + strlen("debugset ");
        len = 5;
        while(len-- && *ptr == '\0')
            ptr++;

        if(len < 0) {debug_help();continue;}
        module_id = atoi(ptr);

        ptr += strlen(ptr);
        len = 5;
        while(len-- && *ptr == '\0')
            ptr++;
        
        if(len < 0) {debug_help();continue;}
        dbg_lever = atoi(ptr);

        /* set debug lever */
        dnq_debug_setlever(module_id, dbg_lever);
        DNQ_PRINT(DNQ_MOD_ALL, "debugset: [%s] lever[%d]!\n",\
            g_dbg_modules[module_id].name_desc, dbg_lever);
    }
}


S32 dnq_debug_init()
{
    dnq_debug_setlever(1, 6);
    if(dnq_task_create("debug_ctrl", 32*2048, dnq_debug_control, NULL) == NULL)
    {
        DNQ_PRINT(DNQ_MOD_ALL, "debug_ctrl task create error: %s", strerror(errno));
        return -1;
    }
    DNQ_PRINT(DNQ_MOD_ALL, "debug_ctrl task create success!\n");
    return 0;
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


