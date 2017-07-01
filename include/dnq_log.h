#ifndef _DNQ_LOG_H_
#define _DNQ_LOG_H_

#include "common.h"
#include <errno.h>


typedef enum dnq_debug_module
{
    DNQ_MOD_NONE = 0,
    DNQ_MOD_ALL = 1,
    DNQ_MOD_OS = 2,
    DNQ_MOD_UART = 3,
    DNQ_MOD_LCD = 4,
    DNQ_MOD_MCU = 5,
    DNQ_MOD_SENSOR = 6,
    DNQ_MOD_RABBITMQ = 7,
    DNQ_MOD_CONFIG = 8,
    DNQ_MOD_MANAGE = 9,
    DNQ_MOD_KEYPAD = 10,
    DNQ_MOD_GPIO = 11,
    DNQ_MOD_NETWORK = 12,
    DNQ_MOD_UPGRADE = 13,
    DNQ_MOD_CNT
}dnq_module_id_e;


typedef enum dnq_debug_lever
{
    DNQ_DBG_NONE    = 0,
    DNQ_DBG_ERROR   = 1,
    DNQ_DBG_WARN    = 2,
    DNQ_DBG_INFO    = 3,
    DNQ_DBG_DEBUG   = 4,
    DNQ_DBG_VERBOSE = 5,
    DNQ_DBG_ALL = 6

}dnq_dbg_lever_e;

#define tt printf("[ychen]:%s: line:%d  __test__!\n", __func__,__LINE__)

#define dnq_error_en(en, msg) \
    do { errno = en; perror(msg); return (en); } while (0)

#define dnq_error(ret, msg) \
    do { perror(msg); return (ret); } while (0)


#define DNQ_PRINT(mod, msg...)   dnq_debug(mod, DNQ_DBG_ALL, msg)
#define DNQ_PRINT2(mod, msg...)   dnq_debug(mod, DNQ_DBG_DEBUG, msg)
#define DNQ_PRINT3(mod, msg,...)   dnq_debug(mod, DNQ_DBG_ALL, msg"\n", ## __VA_ARGS__)

#define DNQ_ERROR(mod, msg,...)    dnq_debug(mod, DNQ_DBG_ERROR, "[ERROR]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)
#define DNQ_WARN(mod, msg,...)   dnq_debug(mod, DNQ_DBG_WARN, "[WARN]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)
#define DNQ_INFO(mod, msg,...)    dnq_debug(mod, DNQ_DBG_INFO, "[INFO]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)
#define DNQ_DEBUG(mod, msg,...)   dnq_debug(mod, DNQ_DBG_DEBUG, "[DEBUG]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)

//#define Dbc_Print_Err(format,...)  Dbc_Print(DBC_PRINT,"[DBC ERROR:][%d][%s][%d][%s]:" format "\n",(int)DRV_OS_TimeNow_MS(),__FILE__,__LINE__,__func__,## __VA_ARGS__)
//#define Dbc_Print_level(level,format,...)  Dbc_Print(level,"[DBC INFO:][%d][%s][%d][%s]:" format "\n",(int)DRV_OS_TimeNow_MS(),__FILE__,__LINE__,__func__,## __VA_ARGS__)

S32 dnq_debug_init();
U32 dnq_debug_setlever(U32 module_id, U32 lever);

#endif /* _DNQ_LOG_H_ */

