#ifndef _DNQ_LOG_H_
#define _DNQ_LOG_H_

#include "common.h"
#include <errno.h>


typedef enum dnq_debug_module
{
    DNQ_MOD_ALL = 0x01,
    DNQ_MOD_KEYPAD,
    DNQ_MOD_UART,
    DNQ_MOD_LCD,
    DNQ_MOD_MCU,
    DNQ_MOD_RABBITMQ,
    DNQ_MOD_OS,
    DNQ_MOD_RTC,
    DNQ_MOD_NETWORK,
    DNQ_MOD_MANAGE,
    DNQ_MOD_GPIO,
    DNQ_MOD_CNT
}dnq_module_e;


typedef enum dnq_debug_lever
{
    DNQ_DBG_NONE,
    DNQ_DBG_ERROR,
    DNQ_DBG_WARN,
    DNQ_DBG_INFO,
    DNQ_DBG_DEBUG,
    DNQ_DBG_ALL

}dnq_dbg_lever_e;

#define tt printf("[ychen]:%s: line:%d  __test__!\n", __func__,__LINE__)

#define dnq_error_en(en, msg) \
    do { errno = en; perror(msg); return (en); } while (0)

#define dnq_error(ret, msg) \
    do { perror(msg); return (ret); } while (0)


#define DNQ_PRINT(mod, msg...)   dnq_debug(mod, DNQ_DBG_ALL, msg)
#define DNQ_PRINT2(mod, msg...)   dnq_debug(mod, DNQ_DBG_DEBUG, msg)

#define DNQ_ERROR(mod, msg,...)    dnq_debug(mod, DNQ_DBG_ERROR, "[ERROR]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)
#define DNQ_WARN(mod, msg,...)   dnq_debug(mod, DNQ_DBG_WARN, "[WARN]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)
#define DNQ_DEBUG(mod, msg,...)   dnq_debug(mod, DNQ_DBG_DEBUG, "[DEBUG]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)
#define DNQ_INFO(mod, msg,...)    dnq_debug(mod, DNQ_DBG_INFO, "[INFO]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)

//#define Dbc_Print_Err(format,...)  Dbc_Print(DBC_PRINT,"[DBC ERROR:][%d][%s][%d][%s]:" format "\n",(int)DRV_OS_TimeNow_MS(),__FILE__,__LINE__,__func__,## __VA_ARGS__)
//#define Dbc_Print_level(level,format,...)  Dbc_Print(level,"[DBC INFO:][%d][%s][%d][%s]:" format "\n",(int)DRV_OS_TimeNow_MS(),__FILE__,__LINE__,__func__,## __VA_ARGS__)

S32 dnq_debug_init();

#endif /* _DNQ_LOG_H_ */

