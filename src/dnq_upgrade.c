/* dnq upgrade Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is dnq upgrade Program.
 * Note : 
 */


#include "dnq_common.h"
#include "dnq_upgrade.h"
#include "ngx_palloc.h"

#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_tcp_socket.h>

static ngx_pool_t *mem_pool;

#define UPGRADE_INFO_LEN      16

#define UPGRADE_CHNL_TX_ID    10
#define UPGRADE_CHNL_RX_ID    11

channel_t upgrade_channel[2] = 
{
    /* host to server */
    {UPGRADE_CHNL_TX_ID, "queue_cloud_callback", "exchange_cloud", "callback"},
    
    /* server to host */
    {UPGRADE_CHNL_RX_ID, "hostupdate_", "exchange_host_update", ""}
    
};

channel_t *upgrade_channel_tx = &upgrade_channel[0];
channel_t *upgrade_channel_rx = &upgrade_channel[1];

/**
  * @brief  CRC16-IBM校验
	* @param 	*addr需要校验数据首地址
						 num 需要校验的数据长度
	* @retval  计算得到的数据低位在前 高位在后
  */  
#define POLY 0x8005
U16 crc16(U8 *addr, U32 num ,U32 crc)  
{  
    int i;  
    //u16 crc=0;					//CRC16-IBM初值
    U16 Over_crc=0;			//CRC16-IBM结果异或
    for (; num > 0; num--)              /* Step through bytes in memory */  
    {  
        crc = crc ^ (*addr++ << 8);     /* Fetch byte from memory, XOR into CRC top byte*/  
        for (i = 0; i < 8; i++)             /* Prepare to rotate 8 bits */  
        {  
            if (crc & 0x8000)            /* b15 is set... */  
                crc = (crc << 1) ^ POLY;    /* rotate and XOR with polynomic */  
            else                          /* b15 is clear... */  
                crc <<= 1;                  /* just rotate */  
        }                             /* Loop for 8 bits */  
        crc =crc^Over_crc;                  /* Ensure CRC remains 16-bit value */  
    }                               /* Loop until num=0 */  
    return(crc);                    /* Return updated CRC */  
}

S32 send_msg_to_server(
    amqp_connection_state_t conn,
    channel_t *pchnl,
    char *message)
{
    S32 ret = -1;
	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");
	props.delivery_mode = 2; /* persistent delivery mode */
	//snprintf(response_buffer,128, "50#%s#%s#%s",data);
	
	ret = die_on_error(amqp_basic_publish(conn,
	                                pchnl->chid,
	                                amqp_cstring_bytes(pchnl->exchange),
	                                amqp_cstring_bytes(pchnl->rtkey),
	                                0,
	                                0,
	                                &props,
	                                amqp_cstring_bytes(message)),
	             "Publishing");
    if(ret == 0)
        DNQ_INFO(DNQ_MOD_RABBITMQ, "send success! chid=%d, ex=%s, key=%s",\
        pchnl->chid, pchnl->exchange, pchnl->rtkey);
    else
        DNQ_ERROR(DNQ_MOD_RABBITMQ, "send failed! chid=%d, ex=%s, key=%s",\
        pchnl->chid, pchnl->exchange, pchnl->rtkey);
    return ret;
}

S32 send_response_to_server(
    amqp_connection_state_t conn,
    channel_t *pchnl,
    json_type_e json_type)
{
    S32 ret = -1;
    char  *json_response = NULL;
    client_response_t response;
    
    /* send response to server */
    if(json_type == JSON_TYPE_AUTHORRIZATION)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: authorization");
        
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "authorization");
        
        json_response = json_create_response(&response);
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_TEMP_POLICY)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: policy");
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "policy");
        
        json_response = json_create_response(&response);
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_TEMP_LIMIT)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: limit");
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "limit");
        
        json_response = json_create_response(&response);
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_TEMP_ERROR)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: degree error");
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "error");
        
        json_response = json_create_response(&response);
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_POWER_CONFIG)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: power config");
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "power");
        
        json_response = json_create_response(&response);
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_RESPONSE)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: response");
    }
    else if(json_type == JSON_TYPE_CORRECT)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: correct");
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "correct");
        
        json_response = json_create_response(&response);
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }
    else if(json_type == JSON_TYPE_INIT)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg type: init");
        strcpy(response.mac, MAC_ADDR);
        strcpy(response.status, "init");
        
        json_response = json_create_response(&response);
        ret = send_msg_to_server(conn, pchnl, json_response);
        dnq_free(json_response);
    }

    return ret;
}

static void upgrade_info_print(upgrade_info_t *info)
{
    DNQ_INFO(DNQ_MOD_UPGRADE, "tag:\t0x%x", info->tag);
    DNQ_INFO(DNQ_MOD_UPGRADE, "type:\t%d", info->type);
    DNQ_INFO(DNQ_MOD_UPGRADE, "mac:\t%02X:%02X:%02X:%02X:%02X:%02X", \
        info->mac[0],info->mac[1],info->mac[2],\
        info->mac[3],info->mac[4],info->mac[5]);
    DNQ_INFO(DNQ_MOD_UPGRADE, "hw_ver:\t0x%04x", info->hw_ver);
    DNQ_INFO(DNQ_MOD_UPGRADE, "sw_ver:\t0x%04x", info->sw_ver);
    DNQ_INFO(DNQ_MOD_UPGRADE, "crc_16:\t0x%08x", info->crc_16);
    DNQ_INFO(DNQ_MOD_UPGRADE, "mode:\t%d", info->mode);
    DNQ_INFO(DNQ_MOD_UPGRADE, "need_ver:\t%d", info->need_ver);
}

static S32 upgrade_info_check(upgrade_info_t *info, U32 len)
{
    U32 upgrade_mode;
    U32 upgrade_type;
    U8  host_mac[8] = {0};

    upgrade_info_print(info);

    /* check data lenght */
    if(len != UPGRADE_INFO_LEN)
    {
        DNQ_ERROR(DNQ_MOD_UPGRADE, "upgrade_info lenght[%d] error! lenght should %d", \
            len, UPGRADE_INFO_LEN);
        return ERR_LENGHT;
    }

    /* check upgrade tag */
    if(info->tag != 0x47)
    {
        DNQ_ERROR(DNQ_MOD_UPGRADE, "upgrade_tag should be 0x%02x!", 0x47);
        return ERR_TAG;
    }
    
    /* check hardware version */
    if(info->hw_ver != HWVER)
    {
        DNQ_ERROR(DNQ_MOD_UPGRADE, "the hw_version is not match! \
            host HWVER=0x%08x, upgrade HWVER=0x%08x", HWVER, info->hw_ver);
        return ERR_HWVER;
    }

    /* check mac addr */
    if(strncmp(host_mac, info->mac, 6) != 0)
    {
        DNQ_ERROR(DNQ_MOD_UPGRADE, "the mac_addr is not match!");
        DNQ_ERROR(DNQ_MOD_UPGRADE, "current mac=0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X", \
            host_mac[0],host_mac[1],host_mac[2],host_mac[3],host_mac[4],host_mac[5]); 
        DNQ_ERROR(DNQ_MOD_UPGRADE, "upgrade mac=0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X", \
            info->mac[0],info->mac[1],info->mac[2],info->mac[3],info->mac[4],info->mac[5]);
        return ERR_MAC;
    }

    upgrade_mode = info->mode;
    upgrade_type = info->type;
    
    /* check software version */
    if(upgrade_mode == 1)
    {
        /* 遇到高版本升级 */
        if(info->sw_ver <= SWVER)
        {
            DNQ_ERROR(DNQ_MOD_UPGRADE, "uprade_mode:%d, Not need uprade! \
                current SWVER=0x%08x, upgrade SWVER=0x%08x", upgrade_mode, SWVER, info->sw_ver);
            return ERR_SWVER1;
        }
    }
    else if(upgrade_mode == 2)
    {
        /* 指定版本号升级 */
        if(info->need_ver != SWVER)
        {
            DNQ_ERROR(DNQ_MOD_UPGRADE, "uprade_mode:%d, Not need uprade! \
                current SWVER=0x%08x, upgrade SWVER=0x%08x", upgrade_mode, SWVER, info->sw_ver);
            return ERR_SWVER2;
        }
    }

    /* need upgrade */
    DNQ_INFO(DNQ_MOD_UPGRADE, "uprade_mode:%d, need uprade soft!! \
                current SWVER=0x%08x, upgrade SWVER=0x%08x", upgrade_mode, SWVER, info->sw_ver);
                
    return upgrade_type;
}

static U32 upgrade_msg_process(amqp_envelope_t *penve, amqp_connection_state_t conn)
{
    U32 ret = -1;
    char  *json_msg = NULL;
    char   cjson_struct[3072] = {0};
    char  *json_response = NULL;
    json_type_e  json_type = 0;
    channel_t   *pchnl = NULL;
    dnq_msg_t  sendmsg;
    U32    json_len;

    pchnl = upgrade_channel_tx; /* response  */
    json_msg = penve->message.body.bytes;
    json_len = penve->message.body.len;

    ret = upgrade_info_check(penve->message.body.bytes, penve->message.body.len);
    if(ret < 0)
        return -1;

    

    /* send response to server */
    send_response_to_server(conn, pchnl, json_type);    

    return 0;
}

static S32 upgrade_wait_message(amqp_connection_state_t conn)
{
    amqp_frame_t frame;
    amqp_rpc_reply_t ret;
    amqp_envelope_t envelope;
    //uint64_t start_time = now_microseconds();
    int received = 0;
    int error_count = 0;
    int connected = 0;

    while(1)
    {
    #if 0
        now = now_microseconds();
        if (now > next_summary_time) 
        {
            int countOverInterval = received - previous_received;
            double intervalRate = countOverInterval / ((now - previous_report_time) / 1000000.0);
            printf("%d ms: Received %d - %d since last report (%d Hz)\n",
            (int)(now - start_time) / 1000, received, countOverInterval, (int) intervalRate);

            previous_received = received;
            previous_report_time = now;
            next_summary_time += SUMMARY_EVERY_US;
        }
    #endif
        amqp_maybe_release_buffers(conn);

        struct timeval timeout;
        timeout.tv_sec  = 3;
        timeout.tv_usec = 0;
        ret = amqp_consume_message(conn, &envelope, &timeout, 0);
        if(ret.library_error != AMQP_STATUS_TIMEOUT)
        DNQ_DEBUG(DNQ_MOD_RABBITMQ, "recv msg! type=%d,%d, errno=%d", \
            ret.reply_type, frame.frame_type, ret.library_error);

        if (AMQP_RESPONSE_NORMAL != ret.reply_type)
        {
            if (AMQP_RESPONSE_LIBRARY_EXCEPTION == ret.reply_type &&
            AMQP_STATUS_UNEXPECTED_STATE == ret.library_error)
            {
                DNQ_ERROR(DNQ_MOD_RABBITMQ, "reply exception!");
                if (AMQP_STATUS_OK != amqp_simple_wait_frame(conn, &frame))
                {
                    DNQ_ERROR(DNQ_MOD_RABBITMQ, "test2!");
                    return;
                }
                if (AMQP_FRAME_METHOD == frame.frame_type)
                {
                    switch (frame.payload.method.id)
                    {
                        case AMQP_BASIC_ACK_METHOD:
                        /* if we've turned publisher confirms on, and we've published a message
                        * here is a message being confirmed
                        */
                        DNQ_ERROR(DNQ_MOD_RABBITMQ, "Received AMQP_BASIC_ACK_METHOD");
                        break;
                        case AMQP_BASIC_RETURN_METHOD:
                        /* if a published message couldn't be routed and the mandatory flag was set
                        * this is what would be returned. The message then needs to be read.
                        */
                        {
                            DNQ_ERROR(DNQ_MOD_RABBITMQ, "Received AMQP_BASIC_RETURN_METHOD");
                            amqp_message_t message;
                            ret = amqp_read_message(conn, frame.channel, &message, 0);
                            if (AMQP_RESPONSE_NORMAL != ret.reply_type)
                            {
                                return;
                            }

                            amqp_destroy_message(&message);
                        }

                        break;

                        case AMQP_CHANNEL_CLOSE_METHOD:
                        /* a channel.close method happens when a channel exception occurs, this
                        * can happen by publishing to an exchange that doesn't exist for example
                        *
                        * In this case you would need to open another channel redeclare any queues
                        * that were declared auto-delete, and restart any consumers that were attached
                        * to the previous channel
                        */

                        DNQ_ERROR(DNQ_MOD_RABBITMQ, "Received AMQP_CHANNEL_CLOSE_METHOD");
                        return;

                        case AMQP_CONNECTION_CLOSE_METHOD:
                        /* a connection.close method happens when a connection exception occurs,
                        * this can happen by trying to use a channel that isn't open for example.
                        *
                        * In this case the whole connection must be restarted.
                        */

                        DNQ_ERROR(DNQ_MOD_RABBITMQ, "Received  AMQP_CONNECTION_CLOSE_METHOD");
                        return;

                        default:
                        DNQ_ERROR(DNQ_MOD_RABBITMQ, "An unexpected method was received %d\n", frame.payload.method.id);
                        fprintf(stderr ,"An unexpected method was received %d\n", frame.payload.method.id);
                        return;
                    }
                }
            }     
            else
            {
                //DNQ_DEBUG(DNQ_MOD_RABBITMQ, "error %d %d",\
                //    ret.reply_type,ret.library_error);
                if(ret.library_error != AMQP_STATUS_TIMEOUT)
                {
                    connected = 0;
                    error_count++;
                }
                else
                {
                    connected = 1;
                    error_count = 0;
                }
                if(error_count == 50)
                    return -1;
            }
        }
        else
        {
            DNQ_INFO(DNQ_MOD_RABBITMQ, "recv content:");
            amqp_dump(envelope.message.body.bytes, envelope.message.body.len);
            upgrade_msg_process(&envelope, conn);
            amqp_destroy_envelope(&envelope);
        }
        received++;
        usleep(200*1000);
    }
}

static S32 uprade_rabbitmq_init(char *serverip, int port, amqp_connection_state_t *pconn)
{
    int i = 0;
    int status;
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;
    amqp_queue_declare_ok_t *r = NULL;
    channel_t  *pchnl = NULL;
    
    conn = amqp_new_connection();
    *pconn = conn;
    if(!conn)
        dnq_error(-1, "amqp_new_connection error!");

    socket = amqp_tcp_socket_new(conn);
    if(!socket)
        dnq_error(-1, "amqp_tcp_socket_new error!");
   
    status = amqp_socket_open(socket, SERVER_IPADDR, SERVER_PORT);
    if(status < 0)
        dnq_error(status, "amqp_socket_open error!");

    DNQ_INFO(DNQ_MOD_RABBITMQ, "create new connecting! socket=%d, ip=%s, port=%d", \
        status, SERVER_IPADDR, SERVER_PORT);

    die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 30, AMQP_SASL_METHOD_PLAIN, \
    username, password), "Logging in");

    DNQ_INFO(DNQ_MOD_RABBITMQ, "Login success! user=%s, passwd=%s", username, password);


    pchnl = upgrade_channel;

    for(i=0; i<2; i++)
    {
        pchnl = &upgrade_channel[i];
        //create channel
        amqp_channel_open(conn, pchnl->chid);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
        DNQ_INFO(DNQ_MOD_RABBITMQ, "Opening channel %d success!", pchnl->chid);

        if(pchnl->chid == UPGRADE_CHNL_TX_ID)
        {
        //exchange declare
        amqp_exchange_declare(conn, /* 连接 */
                            pchnl->chid, 
                            amqp_cstring_bytes(pchnl->exchange),
                            amqp_cstring_bytes("direct"),
                            0, /* passive */
                            1, /* durable 持久化 */
                            0, /* auto delete */
                            0, /* internal */
                            amqp_empty_table);
        DNQ_INFO(DNQ_MOD_RABBITMQ, "exchange declare %s!", pchnl->exchange);

        //queue declare
        strcat(pchnl->qname, MAC_ADDR);
        r = amqp_queue_declare(conn, 
                            pchnl->chid,
                            amqp_cstring_bytes(pchnl->qname),
                            0, /* passive */
                            0, /* durable 持久化 */
                            0, /* exclusive */
                            1, /* auto delete */
                            amqp_empty_table);

        die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
        DNQ_INFO(DNQ_MOD_RABBITMQ, "queue declare %s!", pchnl->qname);

        
        //bind
        strcpy(pchnl->rtkey, MAC_ADDR);
        amqp_queue_bind(conn, 
                        pchnl->chid, 
                        amqp_cstring_bytes(pchnl->qname), 
                        amqp_cstring_bytes(pchnl->exchange),
                        amqp_cstring_bytes(pchnl->rtkey), 
                        amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");
        DNQ_INFO(DNQ_MOD_RABBITMQ, "queue %s bind on exchange=%s, rtkey=%s!", \
            pchnl->qname, pchnl->exchange, pchnl->rtkey);
        }
    }

    pchnl = &upgrade_channel[1];
    //consume
    amqp_basic_consume(conn, 
                        pchnl->chid, 
                        amqp_cstring_bytes(pchnl->qname), 
                        amqp_empty_bytes, /* consumer_tag */
                        0,  /* no local */
                        1,  /* no ack */
                        0,  /* exclusive */
                        amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

    //purge
    amqp_queue_purge(conn, pchnl->chid, amqp_cstring_bytes(pchnl->qname));

    upgrade_wait_message(conn);

    /* error close the channel */
    for(i=0; i<2; i++)
    {
        pchnl = &upgrade_channel[i];
        die_on_amqp_error(amqp_channel_close(conn, pchnl->chid, AMQP_REPLY_SUCCESS), "Closing channel");
    }
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");

    DNQ_ERROR(DNQ_MOD_RABBITMQ, "rabbitmq_init exit...");
    return 0;
}


S32 uprade_rabbitmq_task()
{
    while(1)
    {
        DNQ_INFO(DNQ_MOD_RABBITMQ, "msg_thread start!");
        uprade_rabbitmq_init(serverip, serverport, &g_conn);
        sleep(5);
    }
}

S32 dnq_upgrade_init()
{
    ngx_pool_t * pool = NULL;
    
    if(mem_pool)  /* already inited */
        return 0; 
    
    pool = dnq_mempool_init(1024*1024);
    if(!pool)
        return -1;
    
    cJSON_Hooks hooks = {dnq_malloc, dnq_free};
    cJSON_InitHooks(&hooks);
    
    mem_pool = pool;
    return 0;
}

S32 dnq_upgrade_deinit()
{
    dnq_mempool_deinit(mem_pool);
    return 0;
}

