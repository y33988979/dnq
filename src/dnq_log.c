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
#include "dnq_common.h"
#include "dnq_log.h"
#include "dnq_os.h"

static U8 g_dnq_dbg_lever[DNQ_MOD_CNT] = {0};
static U8 g_dbg_buf[2048] = {0};
static U32 debug_inited = 0;

typedef struct debug_module
{
    U16  id;
    U8   name_desc[32];
}debug_module_t;

static S8 *debug_level_info[] = 
{
    "don't print any info",
    "print error info",
    "print warn info",
    "print info info",
    "print debug info",
    "print verbose info"
};

static debug_module_t g_dbg_modules[DNQ_MOD_CNT] =
{
    DNQ_MOD_NONE,    "none",
    DNQ_MOD_ALL,     "All Module",
    DNQ_MOD_OS,      "OS     Module",
    DNQ_MOD_UART,    "Uart   Module",
    DNQ_MOD_LCD,     "LCD    Module",
    DNQ_MOD_MCU,     "MCU    Module",
    DNQ_MOD_SENSOR,  "SENSOR Module",
    DNQ_MOD_RABBITMQ,"RABBITMQ Module",
    DNQ_MOD_CONFIG,  "CONFIG Module",    
    DNQ_MOD_MANAGE,  "MANAGE Module",
    DNQ_MOD_KEYPAD,  "Keypad Module",
    DNQ_MOD_GPIO,    "GPIO Module",
    DNQ_MOD_NETWORK, "NETWORK Module",
    DNQ_MOD_UPGRADE, "UPGRADE Module",
    //DNQ_MOD_CNT,     "UNKNOW Module"
    
};

int dnq_debug(U32 module_id, U32 lever, const char *fmt, ...)
{
    S32 n;
    U32 size = 2048;
    U32 current_module_lever;
    va_list ap;

    if(module_id <= 0 || module_id >= DNQ_MOD_CNT \
        || lever < DNQ_DBG_NONE || lever > DNQ_DBG_ALL)
    {
        printf("[ERROR]dnq_debug: invalid param! module_id=%d, lever=%d", module_id, lever);
        return -1;
    }

    current_module_lever = g_dnq_dbg_lever[module_id];
    if(lever > current_module_lever && lever != DNQ_DBG_ALL)
        return -1;

    /* print log message to console */
    {
        va_start(ap, fmt);
        //n = vsprintf(fmt, ap);
        n = vfprintf(stdout, fmt, ap);
        //n = vsnprintf(g_dbg_buf, size, fmt, ap);
        va_end(ap);
        //fflush(stdout);
    }
    //n = printf(g_dbg_buf, n); 
    return n;
}

/*
* debug lever:
* 0:NONE 1:ERROR, 2:WARN, 3:INFO, 4:DEBUG 5:ALL
*/
U32 dnq_debug_setlever(U32 module_id, U32 lever)
{
    
    U32 i;
    if(module_id <= 0 || module_id >= DNQ_MOD_CNT)
    {
        printf("[ERROR]%s module id is invalid! id = %d\n", __FUNCTION__, module_id);
        return -1;
    }
    if(lever < DNQ_DBG_NONE || lever > DNQ_DBG_VERBOSE)
    {
        printf("[ERROR]%s level id is invalid! level = %d\n", __FUNCTION__, lever);
        return -1;
    }

    /* set all modules */
    if(module_id == DNQ_MOD_ALL)
    {
        for(i=0; i<DNQ_MOD_CNT; i++)
        {
            g_dnq_dbg_lever[i] = lever;
        }
    }
    else
    {
        g_dnq_dbg_lever[module_id] = lever;
    }
    
    return lever;
}

U32 dnq_debug_test(int lever)
{
    dnq_debug_setlever(1, lever);
    DNQ_INFO(DNQ_MOD_RABBITMQ, "INFO: test!");
    DNQ_WARN(DNQ_MOD_RABBITMQ, "WARN: test!");
    DNQ_DEBUG(DNQ_MOD_RABBITMQ, "DEBUG: test!");
    DNQ_ERROR(DNQ_MOD_RABBITMQ, "ERROR: test!");
    DNQ_PRINT(DNQ_MOD_RABBITMQ, "PRINT: test!");
}
void debug_help()
{
    U32 i;
    printf("=====================================================\n");
    printf("usage:   debugset [module_id] [lever]\n");
    printf("lever: 0:none 1:error 2:warn 3:info 4:debug 5:all\n");
    printf("example: debugset 1 2\n\n");
    printf("Module No	Descriptions\n");
    for(i=0; i<DNQ_MOD_CNT; i++)
        printf("%d: %s\n", g_dbg_modules[i].id, g_dbg_modules[i].name_desc);
    printf("=====================================================\n");
    printf("Print level     Descriptions\n");
    for(i=0; i<DNQ_DBG_ALL; i++)
    {
        printf("%4d		%s\n", i, debug_level_info[i]);
    }
    printf("=====================================================\n");
}

void dnq_debug_control()
{
    U32  module_id;
    U32  dbg_lever;
    S32  len;
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
            dnq_msleep(200);
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

        printf("len=%d\n", len);
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
    if(debug_inited)
        return 0;
    
    dnq_debug_setlever(1, 3);
    if(dnq_task_create("debug_ctrl", 32*2048, dnq_debug_control, NULL) == NULL)
    {
        DNQ_PRINT(DNQ_MOD_ALL, "debug_ctrl task create error: %s", strerror(errno));
        return -1;
    }
    debug_inited = 1;
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


