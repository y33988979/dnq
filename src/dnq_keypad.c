/* dnq keypad Program
 * 
 *  Copyright (c) 2017 yuchen
 *  Copyright 2017-2017 jiuzhoutech Inc.
 *  yuchen  <yuchen@jiuzhoutech.com, 382347665@qq.com>
 * 
 *  this is a keypad interface API, for app.
 * Note : 
 */


#include <sys/ioctl.h>
#include <linux/input.h>

#include "dnq_common.h"
#include "dnq_os.h"
#include "dnq_log.h"
#include "dnq_keypad.h"

#define DEVINPUT "/dev/input/event0"
#define NUM 1

static int keypad_fd;

static dnq_appinfo_t *keypad_appinfo ;

extern S32 send_msg_to_lcd(dnq_msg_t *msg);
static void keypad_default_callback(U32 key, U32 status)
{
    dnq_msg_t sendmsg;
    switch (key)
    {
        case DNQ_KEY_UP:
            DNQ_INFO(DNQ_MOD_KEYPAD, "key [up]!", status);
            break;
        case DNQ_KEY_DOWN:
            DNQ_INFO(DNQ_MOD_KEYPAD, "key [down]!", status);
        break;
        case DNQ_KEY_LEFT:
            DNQ_INFO(DNQ_MOD_KEYPAD, "key [left]!", status);
        break;
        case DNQ_KEY_RIGHT:
            DNQ_INFO(DNQ_MOD_KEYPAD, "key [right]!", status);
        break;
        case DNQ_KEY_MENU:
            DNQ_INFO(DNQ_MOD_KEYPAD, "key [menu]!", status);
        break;
        case DNQ_KEY_SCAN:
            DNQ_INFO(DNQ_MOD_KEYPAD, "key [scan]!", status);
        break;
        case DNQ_KEY_EXIT:
            DNQ_INFO(DNQ_MOD_KEYPAD, "key [exit]!", status);
        break;
        case DNQ_KEY_OK:
            DNQ_INFO(DNQ_MOD_KEYPAD, "key [ok]!", status);
        break;
        case DNQ_KEY_SELF_CHECK:
            DNQ_INFO(DNQ_MOD_KEYPAD, "key [self_check]!", status);
        break;
        default:
            DNQ_INFO(DNQ_MOD_KEYPAD, "unknown key!! val=%d.", key);
        break;
    }

    sendmsg.Class = MSG_CLASS_KEYPAD;
    sendmsg.code = key;  /* key */
    sendmsg.payload = (void*)status; /* status */
    send_msg_to_lcd(&sendmsg);

}

KeypadCallback  fKeypadCallback = keypad_default_callback;
void dnq_keypad_callback_enable(KeypadCallback callback)
{
    if(callback == NULL)
        callback = keypad_default_callback;
    fKeypadCallback = callback;
}

void* keypad_task(void *args)
{	
    int fb = keypad_fd;
    ssize_t rb;

    struct input_event ev[NUM];	
    int yalv;

    while (1) 
    {
        if((rb = read(fb, ev, sizeof(struct input_event) * NUM))>0)
        {
            for (yalv = 0; yalv < (int) (rb / sizeof(struct input_event));yalv++) 
            {
                switch (ev[yalv].type) 
                {
                    case EV_SYN:
                    DNQ_DEBUG(DNQ_MOD_KEYPAD, "EV_TYPE = EV_SYN");
                    break;
                    
                    case EV_KEY:					
                    switch(ev[yalv].code)
                    {
                        case BTN_TOUCH:
                        DNQ_INFO(DNQ_MOD_KEYPAD, "EV_KEY : BTN_TOUCH value %d ",ev[yalv].value);
                        break;
                        default:
                            /* callback when key release */
                            if(ev[yalv].value == DNQ_KEY_RELEASE) 
                                fKeypadCallback(ev[yalv].code, ev[yalv].value);
                        //DNQ_INFO(DNQ_MOD_KEYPAD, "EV_KEY : code %d value %d \n", ev[yalv].code,ev[yalv].value);
                        break;
                    }						
                    break;
                    
                    case EV_LED:
                    DNQ_INFO(DNQ_MOD_KEYPAD, "EV_TYPE = EV_LED");

                    break;
                    
                    case EV_REP:
                    DNQ_INFO(DNQ_MOD_KEYPAD, "EV_TYPE = EV_REP");

                    break;
                    
                    case EV_ABS:					
                    switch(ev[yalv].type)
                    {
                        case ABS_X:
                        DNQ_INFO(DNQ_MOD_KEYPAD, "EV_ABS : ABS_X value %d ",ev[yalv].value);
                        break;
                        
                        case ABS_Y:
                        DNQ_INFO(DNQ_MOD_KEYPAD, "EV_ABS : ABS_Y value %d ",ev[yalv].value);
                        break;
                        
                        default:
                        DNQ_INFO(DNQ_MOD_KEYPAD, "EV_ABS : code %d value %d ",ev[yalv].code,ev[yalv].value);
                        break;
                    }
                    break;
                    
                    default:
                    DNQ_DEBUG(DNQ_MOD_KEYPAD, "EV_TYPE = %x", ev[yalv].type);
                    break;
                }
            //printf("time= %1d.%061d \n", ev[yalv].time.tv_sec,ev[yalv].time.tv_usec);		    		    
            }	 					
        }
    }
    return 0;
}

S32 dnq_keypad_init()
{
    dnq_task_t  *task = NULL;
    dnq_queue_t *queue = NULL;
    dnq_appinfo_t **appinfo = NULL;

    appinfo = &keypad_appinfo;

	if ((keypad_fd = open(DEVINPUT, O_RDONLY | O_NDELAY)) < 0) 
    {
		DNQ_ERROR(DNQ_MOD_KEYPAD, "open \"%s\" error! %s",\
          DEVINPUT, strerror(errno));
		return -1;
    }

    *appinfo = dnq_app_task_create("keypad", 2048*16, \
        QUEUE_MSG_SIZE, 5, keypad_task, NULL);
    if(!*appinfo)
    {
        close(keypad_fd);
        DNQ_ERROR(DNQ_MOD_KEYPAD, "dnq_keypad_init error!");
        return -1;
    }

    keypad_appinfo = *appinfo;
    DNQ_INFO(DNQ_MOD_KEYPAD, "dnq_keypad_init ok!");
    return 0;
}

S32 dnq_keypad_deinit()
{
    S32 ret;
    
    if(!keypad_appinfo)
        return -1;
    ret = dnq_app_task_exit(keypad_appinfo);
    if(ret < 0)
    {
        DNQ_ERROR(DNQ_MOD_KEYPAD, "keypad_task exit error!");
        return -1;
    }
    close(keypad_fd);
    
    DNQ_INFO(DNQ_MOD_KEYPAD, "keypad_deinit ok!");
    return ret;
}

