#ifndef _DNQ_UPGRADE_H_
#define _DNQ_UPGRADE_H_

#include "common.h"
#include <pthread.h>


#define UPGRD_INFO_LEN      20

#define UPGRD_FILE    "upgrade_file"
#define UPGRD_TAG     0x47

#define ERR_BASE        10             /* 错误码起始值 */
#define ERR_LENGHT     -(ERR_BASE+1)   /* 错误的数据长度 */
#define ERR_TAG        -(ERR_BASE+2)   /* 错误的TAG */
#define ERR_TYPE       -(ERR_BASE+3)   /* 错误的类型 */
#define ERR_MAC        -(ERR_BASE+4)   /* MAC地址不匹配 */
#define ERR_HWVER      -(ERR_BASE+5)   /* 硬件版本不匹配 */
#define ERR_SWVER      -(ERR_BASE+6)   /* 软件版本不匹配 */
#define ERR_CRC        -(ERR_BASE+7)   /* 错误的校验值CRC32 */
#define ERR_MODE       -(ERR_BASE+8)   /* 错误的升级模式 */

#define ERR_WRITE      -(ERR_BASE+10)   /* 写flash出错 */
#define ERR_DECOMPRESS -(ERR_BASE+11)   /* 解压出错 */


#define DBG_NONE      0
#define DBG_ERROR     1
#define DBG_WARN      2
#define DBG_DEBUG     3
#define DBG_INFO      4
#define DBG_ALL       5

S32 upgrd_debug(U32 lever, const char *fmt, ...);
#define UPGRD_ERROR( msg,...)  upgrd_debug(DBG_ERROR, "[ERROR]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)
#define UPGRD_WARN( msg,...)   upgrd_debug(DBG_WARN, "[WARN]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)
#define UPGRD_DEBUG( msg,...)  upgrd_debug(DBG_DEBUG, "[DEBUG]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)
#define UPGRD_INFO( msg,...)   upgrd_debug(DBG_DEBUG, "[INFO]%s:%d: " msg "\n",__func__,__LINE__, ## __VA_ARGS__)
#define UPGRD_PRINT( msg,...)  upgrd_debug(DBG_ALL, msg)


typedef struct _upgrd_info
{
    U8  tag;           /* upgrd tag, 0x47 */
    U8  upgrade_type;  /* upgrd type, 0x10:app 0x11:kernel: 0x12:rootfs 0x13:data */
    U8  file_type;     /* compress type, 0:tar  1:tar.gz  2:tar.bz2  3:zip*/
    U8  mode;          /* 1:指定版本升级 2:高版本升级 */
    U16 need_ver;      /* control=1时, 软件版本号为need_ver的主机升级 */
    U8  mac[6];        /* host mac addr */
    U16 hw_ver;        /* hardware version */
    U16 sw_ver;        /* software version */
    U32 crc_32;        /* checksum 校验 */
}upgrd_info_t;

typedef enum _upgrd_type
{
    UPGRD_TYPE_BOOT,
    UPGRD_TYPE_ENV,
    UPGRD_TYPE_KERNEL,
    //UPGRD_TYPE_ROOTFS,  /* Unsupport */
    UPGRD_TYPE_APP = 0x10,
    UPGRD_TYPE_DNQ_MANAGE,
    UPGRD_TYPE_DNQ_CONFIG,
    UPGRD_TYPE_DNQ_UPGRADE,
    UPGRD_TYPE_MAX,
}upgrd_type_e;

typedef enum _file_type
{
    FILE_TYPE_TAR,
    FILE_TYPE_TAR_GZ,
    FILE_TYPE_TAR_BZ2,
    FILE_TYPE_ZIP,
    FILE_TYPE_MAX
}file_type_e;

typedef enum _msg_type_e
{
    UPGRD_MSG_TYPE_DESC,
    UPGRD_MSG_TYPE_DATA

}msg_type_e;

typedef struct _upgrd_msg
{
    U32 type;
    U8  msg[64];
    U32 msg_len;
    U8 *uparge_data;
    U32 data_len;
}upgrd_msg_t;

typedef struct _response
{
    U8        mac[16];
    U16       ret_code;
}response_t;

typedef enum _upgrd_status
{
    UPGRD_WAIT = 0x0,  /* 等待升级 */
    UPGRD_READY,       /* 升级描述符正确，等待接收升级数据 */
    UPGRD_DATA_CHECK,  /* 已接收升级数据，正在校验数据 */
    UPGRD_DATA_WRITE,  /* 已接收升级数据，正在写数据到flash */
    UPGRD_DECOMPRESS,  /* 已接收升级数据，正在解压文件 */
    UPGRD_DONE,        /* 升级完成 */
}upgrd_status_e;


typedef struct _upgrd_channel{
	int chid;
	char qname[32];
	char exchange[32];
	char rtkey[32];
}upgrd_channel_t;

S32 send_msg_to_upgrade(U8 *msg, U32 len);
S32 recv_msg_timeout(U8 **msg, U32 timeout_ms);
S32 dnq_upgrd_init();
S32 dnq_upgrd_deinit();


#endif /* _DNQ_UPGRADE_H_ */

