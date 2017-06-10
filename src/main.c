/* dnq main Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  application for nuc970.
 * Note : 
 */


#include "dnq_common.h"
#include "dnq_config.h"
#include "ngx_palloc.h"
#include "dnq_keypad.h"
#include "dnq_rabbitmq.h"
#include "dnq_network.h"
#include "dnq_uart.h"
#include "dnq_log.h"
#include "dnq_mcu.h"
#include "dnq_sensor.h"
#include "dnq_lcd.h"
#include "dnq_os.h"

#define MAIN_CHECK(func) \
    do\
    {\
        if(func < 0)\
        {\
            DNQ_ERROR(DNQ_MOD_ALL, "main check error!");\
            return -1;\
        }\
    }while(0);


void *send_test(void *args)
{
    int n = 0;
    int cnt = 1;
    dnq_queue_t *queue;
    dnq_msg_t send_msg;

    queue = (dnq_queue_t *)args;

    

    printf("stack itemaddr0=0x%08x\n", &n);
    printf("stack itemaddr1=0x%08x\n", &cnt);
    printf("stack itemaddr2=0x%08x\n", &queue);
    printf("stack itemaddr3=0x%08x\n", &send_msg);
    printf("test_queue1=0x%08x\n", queue);
    printf("msg size=0x%08x, %d\n", sizeof(dnq_msg_t), sizeof(dnq_msg_t));
    printf("queue size=0x%08x, %d\n", sizeof(dnq_queue_t),sizeof(dnq_queue_t));
    while(1)
    {
    
        send_msg.Class = n+1;
        send_msg.code = n+2;
        send_msg.lenght = n+3;
        strcpy(send_msg.data, "this is test message!");

        printf("send test msg!! queue->msg addr=0x%08x\n", queue->msg);
        dnq_msg_send(queue, &send_msg);
        
        dnq_msleep(100);
        n += 10;
        if(cnt++ % 7 == 0)
            sleep(300);
    }
}

void dump_sem(sem_t* sem)
{
    S32 i,*p = (S32*)sem;
    printf("semval:");
    for(i=0;i<sizeof(sem_t);i+=4)
    {
       
        printf("%08x ", *p++);
    }
    printf("\n");
}

int main()
{
    S32 len;
    U8 buffer[1024];
    dnq_queue_t *queue = NULL;
    dnq_msg_t recv_msg;
    sem_t sem1 = {0};
    //dnq_debug_setlever(1,5); 

    extern S32 dnq_config_init();    

    MAIN_CHECK( dnq_init() );
    MAIN_CHECK( dnq_config_init() );  
    MAIN_CHECK( dnq_debug_init() );
    MAIN_CHECK( dnq_uart_init() );
    MAIN_CHECK( dnq_network_init() );
    MAIN_CHECK( dnq_lcd_init() );
    MAIN_CHECK( dnq_mcu_init() );
    MAIN_CHECK( dnq_sensor_init() );
    MAIN_CHECK( dnq_keypad_init() );
    MAIN_CHECK( dnq_manage_init() );
    MAIN_CHECK( dnq_rabbitmq_init());

    queue = dnq_queue_create(QUEUE_SIZE_MAX);
    if(queue == NULL)
        goto exit;
    
    sem1 = queue->sem;
    dump_sem(&sem1);
    printf("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
    
    printf("test_queue=0x%08x!\n",queue );
    dnq_task_create("send", 32*2048, send_test, (void*)queue);


    while(1)
    {
        if(memcmp(&sem1, &queue->sem, sizeof(sem_t)) != 0 )
            dump_sem(&queue->sem);
        if(dnq_msg_recv_timeout(queue, &recv_msg, 300) < 0)
            continue;
        
        DNQ_PRINT(DNQ_MOD_ALL, "recv mgs:");
        DNQ_PRINT(DNQ_MOD_ALL,"type=%d, code=%d, len=%d, data=%s\n",\
            recv_msg.Class, recv_msg.code, recv_msg.lenght, recv_msg.data);
    }
    
exit:
    dnq_keypad_deinit();
    dnq_lcd_deinit();
    dnq_mcu_deinit();
    dnq_uart_deinit();
    dnq_deinit();

    return 0;
}

