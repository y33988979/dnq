#ifndef _DNQ_CONFIG_H_
#define _DNQ_CONFIG_H_

#define DNQ_OS_MEM_POOL_SIZE       (1*1024*1024)
#define DNQ_APP_MEM_POOL_SIZE      (1*1024*1024)

#define DNQ_ROOM_CNT               16
#define DNQ_ROOM_MAX               16

#define DNQ_TEMP_ADJUST_STEP       50
#define DNQ_CORRECT_ADJUST_STEP    1

#define DNQ_SERVER_URL   "iot.wiseheater.com"
#define DNQ_SERVER_PORT     5672


#define ETH_NAME  "eth0"

#define ROOT_PATH           "/root/dnq"
#define MAIN_PROGRAM_NAME   "dnq_manage"
#define UPGRD_PROGRAM_NAME  "dnq_upgrade"

/*
*1 初始获取ip，初始获取mac
*2 动态扫描SN传感器
*3 获取房屋数量，保存全局配置
*4 rabbitmq消息返回。返回数据处理状态
*5 485调试问题
*6 RTC时间更新问题

*隔一段时间要向云端要一次时间，进行同步
*SN状态全部默认为 正常
*
*
*/

#endif /* _DNQ_CONFIG_H_ */

