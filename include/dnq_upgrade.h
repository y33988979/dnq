#ifndef _DNQ_UPGRADE_H_
#define _DNQ_UPGRADE_H_

#include "common.h"
#include <pthread.h>

#define UPGRADE_TAG    0x47

#define ERR_BASE       10
#define ERR_LENGHT    -(ERR_BASE+1)
#define ERR_TAG       -(ERR_BASE+2)
#define ERR_TYPE      -(ERR_BASE+3)
#define ERR_MAC       -(ERR_BASE+4)
#define ERR_HWVER     -(ERR_BASE+5)
#define ERR_SWVER1    -(ERR_BASE+6)
#define ERR_SWVER2    -(ERR_BASE+7)
#define ERR_CRC       -(ERR_BASE+8)
#define ERR_MODE      -(ERR_BASE+9)

typedef struct upgrade_info
{
    U8  tag;           /* upgrade tag, 0x47 */
    U8  type;          /* upgrade type, 0x10:app 0x11:kernel: 0x12:rootfs 0x13:data */ 
    U8  mac[6];        /* host mac addr */
    U16 hw_ver;        /* hardware version */
    U16 sw_ver;        /* software version */
    U16 crc_16;        /* checksum 校验 */
    U8  mode;          /* 1:指定版本升级 2:高版本升级 */
    U8  need_ver;      /* control=1时, 软件版本号为need_ver的主机升级 */
    
}upgrade_info_t;

typedef struct response
{
    U8        mac[16];
    U16       ret_code;
}response_t;

typedef enum upgrade_status
{
    UPGRADE_WAIT,
    UPGRADE_READY,
    UPGRADE_DATA_CHECK,
    UPGRADE_DATA_WRITE,
    UPGRADE_DECOMPRESS,
    UPGRADE_DONE,
}upgrade_status_e;


S32 dnq_upgrade_init();
S32 dnq_upgrade_deinit();


#endif /* _DNQ_UPGRADE_H_ */

