#ifndef _CONFIG_H_
#define _CONFIG_H_

/* 内存池的大小 */
#define DNQ_OS_MEM_POOL_SIZE       (1*1024*1024)
#define DNQ_APP_MEM_POOL_SIZE      (1*1024*1024)

/* 电暖气当前路数，电暖气最大路数 */ 
#define DNQ_ROOM_CNT               16
#define DNQ_ROOM_MAX               16

/* LCD设定温度时，调节的步长，单位是50，=0.5度 */
#define DNQ_TEMP_ADJUST_STEP       50
/* LCD设定温度校准时，调节的步长 单位是1，=1度 */
#define DNQ_CORRECT_ADJUST_STEP    1

/* 电暖气工作的高低温极限值，实际温度值要除以100 */
#define DNQ_TEMP_MAX    3100
#define DNQ_TEMP_MIN    0

/* 云服务器参数 */
#define DNQ_SERVER_URL   "iot.wiseheater.com"
#define DNQ_SERVER_PORT     5672

/* 扫描每一路传感器的间隔时间 */
#define DNQ_SENSOR_SCAN_INTERVAL  1

/* 使用485自动收发，需要硬件支持，若不支持自动收发，注释掉 */
#define DNQ_RS485_AUTO_TXRX_SUPPORT    1

/* 网卡信息 */
#define ETH_NAME  "eth0"

/* 工作路径 */
#define ROOT_PATH           "/root/dnq"
#define MAIN_PROGRAM_NAME   "dnq_manage"
#define UPGRD_PROGRAM_NAME  "dnq_upgrade"

#define VERSION_FILE        "version.txt"

/*
*3 获取房屋数量，保存全局配置
*隔一段时间要向云端要一次时间，进行同步
*内存池释放问题
*
软件硬件版本号
mode=0 控制所有房间，需要调试下，lcd需要更新所有房间
制作rootfs，整个烧写镜像
*
*
*/

#endif /* _CONFIG_H_ */

