/* dnq upgrd Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is dnq upgrd Program.
 * Note : 
 */


#include "dnq_common.h"
#include "dnq_os.h"
#include "dnq_log.h"
#include "dnq_upgrade.h"
#include "dnq_checksum.h"
#include "ngx_palloc.h"

#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_tcp_socket.h>


#define UPGRD_CHNL_TX_ID    10
#define UPGRD_CHNL_RX_ID    11

static U8   serverip[] = "118.190.114.219";
static U32  serverport = 5672;
static U8   mac_addr[] =  "70b3d5cf4924";
static U8   username[] =  "host001";
static U8   password[] =    "123456";
static U8   g_dbg_lever = DBG_ALL;
static upgrd_msg_t  g_upgrd_msg = {0};
static sem_t  upgrd_sem;

static U8   upgrd_info_desc[] = 
    {0x47, 0x10, 0x00 ,0x02, 0x00, \
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, \
    0xFF, 0x01, 0x00, 0x01, 0x01, 0xCC, 0xCC};

static upgrd_status_e upgrd_status = UPGRD_WAIT;
static upgrd_info_t upgrd_info = {0};

amqp_connection_state_t  g_conn;
upgrd_channel_t upgrd_channel[2] = 
{
    /* host to server */
    {UPGRD_CHNL_TX_ID, "queue_cloud_callback", "exchange_cloud", "callback"},
    
    /* server to host */
    {UPGRD_CHNL_RX_ID, "hostupdate_", "exchange_host_update", ""}
    
};

upgrd_channel_t *upgrd_channel_tx = &upgrd_channel[0];
upgrd_channel_t *upgrd_channel_rx = &upgrd_channel[1];

S32 upgrd_debug(U32 lever, const char *fmt, ...)
{
    int n;
    int size = 1024;
    int current_lever;
    U8  dbg_buf[2048] = {0};
    va_list ap;

    current_lever = g_dbg_lever;

    /* print log message to console, set lever */
    if( (current_lever > 0 && current_lever >= lever) \
        || lever == DBG_ALL)
    {
        va_start(ap, fmt);
        //n = vsprintf(fmt, ap);
        vfprintf(stdout, fmt, ap);
        //n = vsnprintf(g_dbg_buf, size, fmt, ap);
        va_end(ap);
    }

    //n = printf(dbg_buf, n);
        
    return n;
}

static void upgrd_msg_reset()
{
    memset(&g_upgrd_msg, 0, sizeof(upgrd_msg_t));
    g_upgrd_msg.data = g_upgrd_msg.msg;
}
    
static void upgrd_info_reset()
{
    memset(&upgrd_info, 0, sizeof(upgrd_info_t));
}

static void get_upgrd_info(upgrd_info_t *info)
{
    if(info)
    {
        memcpy(info, &upgrd_info, sizeof(upgrd_info_t));
    }
}

static void save_upgrd_info(upgrd_info_t *info)
{
    if(info)
    {
        memcpy(&upgrd_info, info, sizeof(upgrd_info_t));
    }
}

S32 send_msg_to_server(
    amqp_connection_state_t conn,
    upgrd_channel_t *pchnl,
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
        UPGRD_INFO( "send success! chid=%d, ex=%s, key=%s",\
        pchnl->chid, pchnl->exchange, pchnl->rtkey);
    else
        UPGRD_ERROR( "send failed! chid=%d, ex=%s, key=%s",\
        pchnl->chid, pchnl->exchange, pchnl->rtkey);
    return ret;
}

S32 send_response_to_server(
    amqp_connection_state_t conn,
    upgrd_channel_t *pchnl,
    S32 ret_code)
{
    S32 ret = -1;
    U8  message[256] = {0};

    /* A simple json data */
    sprintf(message, "{type:\"upgrade\",mac:\"%s\",status:%d}", mac_addr, ret_code);
    /* send response to server */
    ret = send_msg_to_server(conn, pchnl, message);
    return ret;
}

/* call system function */
static S32 system_call(U8 *command)
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
                printf("[system_call]run shell script successfully.\n");
                return 0;
            }
            else
            {
                printf("[system_call]run shell script fail, script exit code: %d\n", WEXITSTATUS(status));
				return -1;
            }
        }
        else
        {
            printf("[system_call]exit status = [%d]\n", WEXITSTATUS(status));
			return -1;
        }
    }

    return 0;
}

static S32 upgrd_data_decompress(U8 *filename)
{
    S32 ret;
    U8  command[128] = {0};
    U8  upgrade_file[64] = {0};
    upgrd_info_t info;
    
    get_upgrd_info(&info);

    if(info.file_type == FILE_TYPE_TAR_GZ)
    {
        /* step1: gunzip decompress */
        strcpy(upgrade_file, filename);
        strcat(upgrade_file, ".tar.gz");
        //linux_x64
        //sprintf(command, "gunzip -9fvk %s", upgrade_file);
        //arm_linux
        sprintf(command, "cp -fr %s ./11.tar.gz", upgrade_file);
        system_call(command);
        sprintf(command, "gunzip -f %s", upgrade_file);
        
        UPGRD_INFO("decompress file.. \"%s\"", upgrade_file);
        UPGRD_INFO("system call--> %s", command);
        ret = system_call(command);
        if(ret < 0)
        {
            UPGRD_ERROR("system exec failed! command=\"%s\"", command);
            return -1;
        }
        UPGRD_INFO("decompress done!", command);
        
        /* step2: unpack, tar xvf [filename]*/
        strcpy(upgrade_file, filename);
        strcat(upgrade_file, ".tar");
        sprintf(command, "tar xvf %s", upgrade_file);
        UPGRD_INFO("unpack file.. \"%s\"", upgrade_file);
        UPGRD_INFO("system call-->%s", command);
        ret = system_call(command);
        if(ret < 0)
        {
            UPGRD_ERROR("system exec failed! command=\"%s\"", command);
            return -1;
        }
        UPGRD_INFO("unpack done!", command);
    }
    else if(info.file_type == FILE_TYPE_TAR_BZ2)
    {
        /* step1: gunzip decompress */
        strcpy(upgrade_file, filename);
        strcat(upgrade_file, ".tar.bz2");
        sprintf(command, "bunzip2 -9fvsk %s", upgrade_file);
        UPGRD_INFO("decompress file.. \"%s\"", upgrade_file);
        UPGRD_INFO("system call-->%s", command);
        ret = system_call(command);
        if(ret < 0)
        {
            UPGRD_ERROR("system exec failed! command=\"%s\"", command);
            return -1;
        }
        UPGRD_INFO("decompress done!", command); 

        /* step2: unpack, tar xvf [filename]*/
        strcpy(upgrade_file, filename);
        strcat(upgrade_file, ".tar");
        sprintf(command, "tar xvf %s", upgrade_file);
        UPGRD_INFO("unpack file.. \"%s\"", upgrade_file);
        UPGRD_INFO("system call-->%s", command);
        ret = system_call(command);
        if(ret < 0)
        {
            UPGRD_ERROR("system exec failed! command=\"%s\"", command);
            return -1;
        }
        UPGRD_INFO("unpack done!", command);
    }
    
    return 0;
}

static S32 upgrd_data_write(U8 *filename, U8 *data, U32 data_len)
{
    S32 fd;
    S32 ret;
    S32 wlen, nwrite;
    S32 total_len = 0;
    U8  upgrade_file[64] = {0};

    upgrd_info_t info;
    
    get_upgrd_info(&info);

    strcpy(upgrade_file, filename);
    if(info.file_type == FILE_TYPE_TAR_GZ)
        strcat(upgrade_file, ".tar.gz");
    else if(info.file_type == FILE_TYPE_TAR_BZ2)
        strcat(upgrade_file, ".tar.bz2");
    else if(info.file_type == FILE_TYPE_TAR)
        strcat(upgrade_file, ".tar");
    else if(info.file_type == FILE_TYPE_ZIP)
        strcat(upgrade_file, ".zip");
    
    UPGRD_INFO( "open upgrade file \"%s\"", upgrade_file);
    fd = open(upgrade_file, O_CREAT|O_WRONLY);
    if(fd < 0)
    {
        UPGRD_ERROR("open file \"%s\" failed! err=%s", \
            upgrade_file, strerror(errno)); 
        return -1;
    }

    wlen = data_len;
    total_len = 0;
    while(total_len < data_len)
    {
        printf("data=0x%08x, len=%d\n", data, total_len);
        if(data_len-total_len > 1024)
            wlen = 1024;
        else
            wlen = data_len-total_len;
        
        nwrite = write(fd, data+total_len, wlen);
        if(nwrite < 0)
        {
            UPGRD_ERROR("write file \"%s\" failed! err=%s", \
                upgrade_file, strerror(errno)); 
            close(fd);
            return -1;
        }
        total_len += nwrite;
    }

    close(fd);

    UPGRD_INFO( "write upgrade file \"%s\" done!", upgrade_file);
    if(total_len > data_len)
        UPGRD_WARN("file write: writen total_len[%d] > data_len[%d] !",\
        total_len, data_len); 
    return 0;
}

static void upgrd_info_print(upgrd_info_t *info)
{
    UPGRD_INFO( "tag:\t0x%x", info->tag);
    UPGRD_INFO( "upgrade_type:\t%d", info->upgrade_type);
    UPGRD_INFO( "file_type:\t%d", info->file_type);
    UPGRD_INFO( "mac:\t%02X:%02X:%02X:%02X:%02X:%02X", \
        info->mac[0],info->mac[1],info->mac[2],\
        info->mac[3],info->mac[4],info->mac[5]);
    UPGRD_INFO( "hw_ver:\t0x%04x", info->hw_ver);
    UPGRD_INFO( "sw_ver:\t0x%04x", info->sw_ver);
    UPGRD_INFO( "crc_32:\t0x%08x", info->crc_32);
    UPGRD_INFO( "mode:\t%d", info->mode);
    UPGRD_INFO( "need_ver:\t%d", info->need_ver);
}

static S32 upgrd_data_check(U8 *data, U32 len)
{
    U32 calc_crc;
    upgrd_info_t info;
    
    get_upgrd_info(&info);
    
    calc_crc = crc32(0xFFFFFFFF, data, len);
    if(calc_crc != info.crc_32)
    {
        UPGRD_ERROR("crc32 error! calc_crc=0x%08x, upgrd_crc=0x%08x",\
            calc_crc, info.crc_32);
        //return ERR_CRC;
    }

    return 1;
}

static S32 upgrd_info_check(upgrd_info_t *info, U32 len)
{
    U32 upgrd_mode;
    U32 upgrd_type;
    U8  host_mac[8] = {0};

    memcpy(host_mac, mac_addr, 6);
    upgrd_info_print(info);

    printf("sizeof(upgrd_info_t) == %d\n", sizeof(upgrd_info_t));
    /* check data lenght */
    if(len != UPGRD_INFO_LEN)
    {
        UPGRD_ERROR( "upgrd_info lenght[%d] error! lenght should %d", \
            len, UPGRD_INFO_LEN);
        return ERR_LENGHT;
    }

    /* check upgrd tag */
    if(info->tag != UPGRD_TAG)
    {
        UPGRD_ERROR( "upgrd_tag should be 0x%02x!", 0x47);
        return ERR_TAG;
    }

    /* check upgrade type */
    if(info->upgrade_type >= UPGRD_TYPE_MAX)
    {
        UPGRD_ERROR( "upgrade_type should be 0~%d!", UPGRD_TYPE_MAX-1);
        return ERR_TYPE;
    }

    if(info->file_type >= FILE_TYPE_MAX)
    {
        UPGRD_ERROR( "file_type should be 0~%d!", FILE_TYPE_MAX-1);
        return ERR_TYPE;
    }
    
    /* check hardware version */
    if(info->hw_ver != HWVER)
    {
        UPGRD_ERROR( "the hw_version is not match! \
            host HWVER=0x%08x, upgrd HWVER=0x%08x", HWVER, info->hw_ver);
        return ERR_HWVER;
    }

    /* check mac addr */
    if(strncmp(host_mac, info->mac, 6) != 0)
    {
        UPGRD_ERROR( "the mac_addr is not match!");
        UPGRD_ERROR( "current mac=%02X:%02X:%02X:%02X:%02X:%02X", \
            host_mac[0],host_mac[1],host_mac[2],host_mac[3],host_mac[4],host_mac[5]); 
        UPGRD_ERROR( "upgrd mac=%02X:%02X:%02X:%02X:%02X:%02X", \
            info->mac[0],info->mac[1],info->mac[2],info->mac[3],info->mac[4],info->mac[5]);
        //return ERR_MAC;
    }

    upgrd_mode = info->mode;
    upgrd_type = info->upgrade_type;
    
    /* check software version */
    if(upgrd_mode == 1)
    {
        /* 遇到高版本升级 */
        if(info->sw_ver <= SWVER)
        {
            UPGRD_ERROR( "upgrd_mode:%d, Not need upgrd! \
                current SWVER=0x%08x, upgrd SWVER=0x%08x", upgrd_mode, SWVER, info->sw_ver);
            return ERR_SWVER;
        }
    }
    else if(upgrd_mode == 2)
    {
        /* 指定版本号升级 */
        if(info->need_ver != SWVER)
        {
            UPGRD_ERROR( "upgrd_mode:%d, Not need upgrd! \
                current SWVER=0x%08x, upgrd SWVER=0x%08x", upgrd_mode, SWVER, info->sw_ver);
            return ERR_SWVER;
        }
    }
    else
    {
        UPGRD_ERROR( "upgrd_mode error! mode=%d", upgrd_mode);
        return ERR_MODE;
    }
    
    /* need upgrd */
    UPGRD_INFO( "upgrd_mode:%d,CRC:0x%08x  need upgrd soft!! \
                current SWVER=0x%08x, upgrd SWVER=0x%08x",\
                upgrd_mode, info->crc_32, SWVER, info->sw_ver);

    /* save info */
    save_upgrd_info(info);
                
    return upgrd_type;
}

static U32 upgrd_msg_process(amqp_envelope_t *penve, amqp_connection_state_t conn)
{
    U32  ret = -1;
    U8  *msg = NULL;
    U32  msg_len = NULL;
    upgrd_channel_t  *pchnl = upgrd_channel_tx; /* response channel */

    msg = penve->message.body.bytes;
    msg_len = penve->message.body.len;

    switch(upgrd_status)
    {
        case UPGRD_WAIT:
        case UPGRD_READY:
            /* send data to upgrade_task */
            ret = send_msg_to_upgrade(msg, msg_len);
            break;
        case UPGRD_DATA_CHECK:
        case UPGRD_DATA_WRITE:
        case UPGRD_DECOMPRESS:
        case UPGRD_DONE:
            /* soft is updating , return upgrade status to server */
            ret = send_response_to_server(conn, pchnl, upgrd_status);
            UPGRD_ERROR("soft is updating... message discard! len=%d.", msg_len);
            break;
        default:
        break;        
    }
    
    return ret;
}

static S32 upgrd_wait_message(amqp_connection_state_t conn)
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
            UPGRD_DEBUG( "recv msg! type=%d,%d, errno=%d", \
                ret.reply_type, frame.frame_type, ret.library_error);

        if (AMQP_RESPONSE_NORMAL != ret.reply_type)
        {
            if (AMQP_RESPONSE_LIBRARY_EXCEPTION == ret.reply_type &&
            AMQP_STATUS_UNEXPECTED_STATE == ret.library_error)
            {
                UPGRD_ERROR( "reply exception!");
                if (AMQP_STATUS_OK != amqp_simple_wait_frame(conn, &frame))
                {
                    UPGRD_ERROR( "test2!");
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
                        UPGRD_ERROR( "Received AMQP_BASIC_ACK_METHOD");
                        break;
                        case AMQP_BASIC_RETURN_METHOD:
                        /* if a published message couldn't be routed and the mandatory flag was set
                        * this is what would be returned. The message then needs to be read.
                        */
                        {
                            UPGRD_ERROR( "Received AMQP_BASIC_RETURN_METHOD");
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

                        UPGRD_ERROR( "Received AMQP_CHANNEL_CLOSE_METHOD");
                        return;

                        case AMQP_CONNECTION_CLOSE_METHOD:
                        /* a connection.close method happens when a connection exception occurs,
                        * this can happen by trying to use a channel that isn't open for example.
                        *
                        * In this case the whole connection must be restarted.
                        */

                        UPGRD_ERROR( "Received  AMQP_CONNECTION_CLOSE_METHOD");
                        return;

                        default:
                        UPGRD_ERROR( "An unexpected method was received %d\n", frame.payload.method.id);
                        fprintf(stderr ,"An unexpected method was received %d\n", frame.payload.method.id);
                        return;
                    }
                }
            }     
            else
            {
                //UPGRD_DEBUG( "error %d %d",\
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
            UPGRD_INFO( "recv content:");
            amqp_dump(envelope.message.body.bytes, envelope.message.body.len);
            upgrd_msg_process(&envelope, conn);
            amqp_destroy_envelope(&envelope);
        }
        received++;
        usleep(200*1000);
    }
}

static S32 upgrd_rabbitmq_init(char *serverip, int port, amqp_connection_state_t *pconn)
{
    int i = 0;
    int status;
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;
    amqp_queue_declare_ok_t *r = NULL;
    upgrd_channel_t  *pchnl = NULL;
    
    conn = amqp_new_connection();
    *pconn = conn;
    if(!conn)
        dnq_error(-1, "amqp_new_connection error!");

    socket = amqp_tcp_socket_new(conn);
    if(!socket)
        dnq_error(-1, "amqp_tcp_socket_new error!");
   
    status = amqp_socket_open(socket, serverip, port);
    if(status < 0)
        dnq_error(status, "amqp_socket_open error!");

    UPGRD_INFO( "create new connecting! socket=%d, ip=%s, port=%d", \
        status, serverip, port);

    die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 30, AMQP_SASL_METHOD_PLAIN, \
    username, password), "Logging in");

    UPGRD_INFO( "Login success! user=%s, passwd=%s", username, password);


    pchnl = upgrd_channel;

    for(i=0; i<2; i++)
    {
        pchnl = &upgrd_channel[i];
        //create channel
        amqp_channel_open(conn, pchnl->chid);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
        UPGRD_INFO( "Opening channel %d success!", pchnl->chid);

        if(pchnl->chid == UPGRD_CHNL_RX_ID)
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
        UPGRD_INFO( "exchange declare %s!", pchnl->exchange);

        printf("mac_addr=%s\n", mac_addr);
        printf("pchnl->qname=%s\n", pchnl->qname);
        //queue declare
        strcat(pchnl->qname, mac_addr);
        r = amqp_queue_declare(conn, 
                            pchnl->chid,
                            amqp_cstring_bytes(pchnl->qname),
                            0, /* passive */
                            0, /* durable 持久化 */
                            0, /* exclusive */
                            1, /* auto delete */
                            amqp_empty_table);

        die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
        UPGRD_INFO( "queue declare %s!", pchnl->qname);

        //bind
        strcpy(pchnl->rtkey, mac_addr);
        amqp_queue_bind(conn, 
                        pchnl->chid, 
                        amqp_cstring_bytes(pchnl->qname), 
                        amqp_cstring_bytes(pchnl->exchange),
                        amqp_cstring_bytes(pchnl->rtkey), 
                        amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");
        UPGRD_INFO( "queue %s bind on exchange=%s, rtkey=%s!", \
            pchnl->qname, pchnl->exchange, pchnl->rtkey);
        }
    }

    pchnl = &upgrd_channel[1];
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

    upgrd_wait_message(conn);

    /* error close the channel */
    for(i=0; i<2; i++)
    {
        pchnl = &upgrd_channel[i];
        die_on_amqp_error(amqp_channel_close(conn, pchnl->chid, AMQP_REPLY_SUCCESS), "Closing channel");
    }
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");

    UPGRD_ERROR( "rabbitmq_init exit...");
    return 0;
}


S32 rabbitmq_task()
{   
    while(1)
    {
        UPGRD_INFO( "msg_thread start!");
        upgrd_rabbitmq_init(serverip, serverport, &g_conn);
        sleep(15);
    }
}

S32 send_msg_to_upgrade(U8 *msg, U32 len)
{
    S32 ret;
    upgrd_msg_t *upgrd_msg = &g_upgrd_msg; /* used global buffer */

    /* small buffer */
    if(len < UPGRD_MSG_LEN_MAX)
    {
        upgrd_msg->data_len = len;
        upgrd_msg->data = upgrd_msg->msg;
        memcpy(upgrd_msg->msg, msg, len);
    }
    else /* large buffer */
    {
        upgrd_msg->data = malloc(len+1);
        if(upgrd_msg->data == NULL)
        {
            UPGRD_ERROR("malloc failed: err=%s\n", strerror(errno));
            return -1;
        }
        memcpy(upgrd_msg->data, msg, len);
        upgrd_msg->data_len = len;
    }

    if(sem_post(&upgrd_sem) < 0)
    {
        UPGRD_ERROR("sem_post error: err=%s\n", strerror(errno));
        return -1;
    }

    return 0;
}

S32 recv_msg_timeout(U8 **msg, U32 timeout_ms)
{
    struct timespec ts;
    struct timeval  tv;
	U32 sec, msec, time;
    U32 ret;
    
    time = timeout_ms;
    gettimeofday(&tv, NULL);
    sec = time / 1000;
    msec = time - (sec * 1000);
    tv.tv_sec += sec;
    tv.tv_usec += msec * 1000;
    sec = tv.tv_usec / 1000000;
    tv.tv_usec = tv.tv_usec - (sec * 1000000);
    tv.tv_sec += sec;
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = ((long)tv.tv_usec*1000);
    
    if((ret=sem_timedwait(&upgrd_sem, &ts)) != 0)
    {
        if(errno != ETIMEDOUT)
            UPGRD_ERROR("sem_timedwait error: err=%s\n", strerror(errno));
        return -1;
    }

    *msg = g_upgrd_msg.data;

    return g_upgrd_msg.data_len;
}

S32 upgrade_task()
{
    S32  ret;
    S32  err_code = 0;
    U8  *msg = NULL;
    S32  len;
    upgrd_channel_t *pchnl;
    
    pchnl = upgrd_channel_tx; /* response channel */

    while(1)
    {
        len = recv_msg_timeout(&msg, 1000);
        if(len < 0)
        {
            continue;
        }

        
        /* process upgrade message */
        switch(upgrd_status)
        {
            /* check upgrd info */
            case UPGRD_WAIT:
                #if 1
                ret = upgrd_info_check((upgrd_info_t*)msg, len);
                if(ret < 0)
                {
                    err_code = ret;
                    break;
                }
                    
                #endif
                /* upgrade info correct, waiting upgrade data */
                upgrd_status = UPGRD_READY;
                err_code = UPGRD_READY;
                break;

            /* check upgrade data */
            case UPGRD_READY:
            case UPGRD_DATA_WRITE:
            case UPGRD_DECOMPRESS:
            case UPGRD_DONE:
                ret = upgrd_data_check(msg, len);
                if(ret < 0)
                {
                    err_code = ERR_CRC;
                    break;
                }
                
                /* data correct, start write data to flash */
                upgrd_status = UPGRD_DATA_WRITE;
                printf("msg= 0x%08x, len=%d\n", msg, len);
                ret = upgrd_data_write(UPGRD_FILE, msg, len);
                if(ret < 0)
                {
                    err_code = ERR_WRITE;
                    break;
                }

                /* write done, decompress and Override data.. */
                upgrd_status = UPGRD_DECOMPRESS;
                ret = upgrd_data_decompress(UPGRD_FILE);
                if(ret < 0)
                {
                    err_code = ERR_DECOMPRESS;
                    break;
                }

                /* upgrade done! */
                upgrd_status = UPGRD_DONE;
                err_code = 0;
                upgrd_status = UPGRD_WAIT;

            break;
            default:
                UPGRD_ERROR("unknown upgrade status! status=%d.", upgrd_status);
                break;
        }

        if(err_code < 0)
        {
            upgrd_info_reset();
            upgrd_status = UPGRD_WAIT;
            UPGRD_ERROR("upgrade error! err_code=%d !", err_code);
        }

        upgrd_msg_reset();
        UPGRD_INFO("upgrade success! err_code=%d !", err_code);
        /* send response to server */
        send_response_to_server(g_conn, pchnl, err_code); 
    }
}

S32 create_task(U8 *name, U32 stack_size, void *func, void *param)
{
    S32 ret;
    pthread_t tid;
    pthread_attr_t attr;
    
    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        UPGRD_ERROR("pthread_attr_init error: %s\n", strerror(errno));
        return -1;
    }

    if (stack_size > 0)
    {
        ret = pthread_attr_setstacksize(&attr, stack_size);
        if(ret != 0)
        {
            UPGRD_ERROR("pthread_attr_setstacksize error: %s\n", strerror(errno));
            return -1;
        }
    }
    
    ret = pthread_create(&tid, &attr, func, param);
    if(ret < 0)
    {
        UPGRD_ERROR("pthread_create error: %s\n", strerror(errno));
        return -1;
    }

    ret = pthread_attr_destroy(&attr);
    if(ret != 0)
    {
        UPGRD_ERROR("pthread_attr_destroy error: %s\n", strerror(errno));
    }
    UPGRD_INFO("Create task %s success!", name);
    return 0;
}

S32 net_status_check()
{
    S32 ret;
    
    return ret;
}

ngx_pool_t *mem_pool = NULL;

S32 dnq_upgrd_init()
{
    S32  ret;

    if(!mem_pool)
    {
        mem_pool = dnq_mempool_init(1024*1024);
        if(!mem_pool)
            return -1;
    }
        
    if (sem_init(&upgrd_sem, 0, 0) == -1)
    {
        UPGRD_ERROR("sem_init ");
        return -1;
    }

    net_status_check();

    dnq_app_task_create("rabbitmq_task", 4*1024*1024,\
        QUEUE_MSG_SIZE, 5, rabbitmq_task, NULL);
    create_task("upgrade_task", 4*1024*1024, upgrade_task, NULL);

    
    while(1)
    {
        sleep(1);
    }
    
    return 0;
}

S32 dnq_upgrd_deinit()
{
    return 0;
}

int main()
{
    dnq_checksum_init();
    dnq_upgrd_init();
}

