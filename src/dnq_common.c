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
#include "dnq_checksum.h"
#include "dnq_log.h"
#include "cJSON.h"

#include <sys/time.h>
#include <sys/types.h>  
#include <sys/wait.h>

static U32 g_dnq_init = 0;

static U32 init_time_ms = 0;

S32 dnq_time_init()
{
    struct timeval   tv;

    if (gettimeofday(&tv, NULL) == 0)
    {
        init_time_ms =  tv.tv_sec*1000 + tv.tv_usec/1000;
    }
    else
    {
        perror("gettimeofday falied");
        return -1;
    }
    return 0;
}

U32 dnq_time_now()
{
    struct timeval  tv;
    U32 Clk = 1;	   /* Default value */

    if (gettimeofday(&tv, NULL) == 0)
    {
        Clk =  tv.tv_sec*1000 + tv.tv_usec/1000;
    }
    else
    {
        perror("gettimeofday falied");
    }

    return Clk - init_time_ms;
}

U32 dnq_time_now_us()
{
    struct timeval   tv;
    U32  Clk = 1;	  /* Default value */

    if (gettimeofday(&tv, NULL) == 0)
    {
        Clk =  tv.tv_sec*1000*1000 + tv.tv_usec;
    }
    else
    {
        perror("gettimeofday falied");
    }

    return Clk - init_time_ms*1000;
}

S32 dnq_system_call(U8 *command)
{
    S32 status;

    status = system(command);
    
    if (-1 == status)
    {
        printf(" system run error! err=%s\n", strerror(errno));
		return -1;
    }
    else
    {
        //printf("[dnq_system_call] exit status value = [0x%x]\n", status);
        if (WIFEXITED(status))
        {
            if (0 == WEXITSTATUS(status))
            {
                printf("[dnq_system_call]run shell script successfully.\n");
                return 0;
            }
            else
            {
                printf("[dnq_system_call]run shell script fail, script exit code: %d\n", WEXITSTATUS(status));
				return -1;
            }
        }
        else
        {
            printf("[dnq_system_call]exit status = [%d]\n", WEXITSTATUS(status));
			return -1;
        }
    }

    return 0;
}

S32 dnq_rootfs_init()
{
    S8 cmd[128] = {0};
    
    sprintf(cmd, "sed -i 's/.*sleep .*$/sleep 300/g' /etc/profile");
    dnq_system_call(cmd);
}

S32 dnq_reboot()
{
    DNQ_INFO(DNQ_MOD_ALL, "dnqV2 reboot...\n");
    fflush(stdout);
    sync();
    reboot(0x1234567);
}

S32 dnq_init()
{
    S32 ret;
    if(g_dnq_init)
        return 0;

    DNQ_PRINT(DNQ_MOD_OS, "================dnq_mange info==================");
    DNQ_INFO(DNQ_MOD_OS, "software=0x%x, hardware=0x%x", SWVER, HWVER);
    DNQ_INFO(DNQ_MOD_OS, "Build Time: %s %s", __DATE__ , __TIME__  );
    DNQ_INFO(DNQ_MOD_OS, "================================================");
    
    dnq_time_init();
    dnq_checksum_init();
    
    ret = dnq_mempool_init();
    if(ret < 0)
        return -1;
    
    g_dnq_init = 1;
    return 0;
}

S32 dnq_deinit()
{
    g_dnq_init = 0;
    dnq_mempool_deinit();
    return 0;
}

