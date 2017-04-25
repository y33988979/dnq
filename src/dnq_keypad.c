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

#include "common.h"
#include "dnq_log.h"
#include "dnq_keypad.h"

#define DEVINPUT "/dev/input/event1"

/*
* =============keypad 4*8================
* key up      = 46  0x2e
* key down    = 48  0x30
* key left    = 30  0x1e
* key right   = 16  0x10
* 
* key 1       = 36  0x22     //44
* key 2       = 37  0x23     //2
* key 3       = 23  0x17     //21
* key 4       = 31  0x1F     //2
* key 5(ok)   = 19  0x13     //44
*
*/

/*
* =============keypad 4*4================
* key up      = 46  0x2e     //2
* key down    = 48  0x30     //44,19
* key left    = 30  0x1e     //21,16
* key right   = 16  0x10
* 
* key 1       = 36  0x22     //44
* key 2       = 37  0x23     //2
* key 3       = 23  0x17     //21
* key 4       = 31  0x1F     //2
* key 5(ok)   = 19  0x13     //44
*
*/

/*
* =============keypad 4*2================
* key up      = none
* key down    = 48  0x30
* key left    = 30  0x1e
* key right   = 16  0x10
* 
* key 1       = 36  0x22     //44
* key 2       = none
* key 3       = none
* key 4       = 31  0x1F     //2
* key 5(ok)   = 19  0x13     //44
*
*/

#define NUM 1
void* dnq_keypad_thread(void *args)
{	
	int fb = -1;
	ssize_t rb;
    char device[64] = {0};
	
	struct input_event ev[NUM];	
	int yalv;
	
    strcpy(device, DEVINPUT);
    if(argc > 1)
        strcpy(device, argv[1]);
	
	
	if ((fb = open(device, O_RDONLY | O_NDELAY)) < 0) {
		printf("cannot open %s\n", DEVINPUT);
		exit(0);
    }
    
	 while (1) {
	 	if((rb = read(fb, ev, sizeof(struct input_event) * NUM))>0)
	 	{
	 		for (yalv = 0; yalv < (int) (rb / sizeof(struct input_event));yalv++) {
	 			switch (ev[yalv].type) {
		    case EV_SYN:
				printf("EV_TYPE = EV_SYN\n");
				break;
		    case EV_KEY:					
			    switch(ev[yalv].code)
			    {
			    	case BTN_TOUCH:
			    		printf("EV_KEY : BTN_TOUCH value %d \n",ev[yalv].value);
			    	break;
			    	default:
			    		printf("EV_KEY : code %d value %d \n", ev[yalv].code,ev[yalv].value);
			    	break;
			    }						
				break;
		    case EV_LED:
					printf("EV_TYPE = EV_LED\n");
				break;
		    case EV_REP:
					printf("EV_TYPE = EV_REP\n");
				break;
		    case EV_ABS:					
			    switch(ev[yalv].type)
			    {
			    	case ABS_X:
			    		printf("EV_ABS : ABS_X value %d \n",ev[yalv].value);
			    	break;
			    	case ABS_Y:
			    		printf("EV_ABS : ABS_Y value %d \n",ev[yalv].value);
			    	break;		    	
			    	default:
			    		printf("EV_ABS : code %d value %d \n",ev[yalv].code,ev[yalv].value);
			    	break;
			    }					
				break;					
		    default:
					printf("EV_TYPE = %x\n", ev[yalv].type);
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
    int keypad_fd;
    dnq_task_t  *task = NULL;
    task = dnq_os_task_create("keypad", 2048, 0, dnq_keypad_thread, NULL);
    if(!task)
    {
        DNQ_ERROR(DNQ_MOD_KEYPAD, "dnq_keypad_init error!");
        return -1;
    }
    
    DNQ_INFO(DNQ_MOD_KEYPAD, "dnq_keypad_init ok!");
    return 0;
}

S32 dnq_keypad_deinit()
{
    task = dnq_os_task_exit(task);
    if(!task)
    {
        DNQ_ERROR(DNQ_MOD_KEYPAD, "dnq_keypad_deinit error!");
        return -1;
    }
    
    DNQ_INFO(DNQ_MOD_KEYPAD, "dnq_keypad_deinit ok!");
    return 0;
}

