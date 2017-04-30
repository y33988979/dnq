#ifndef _DNQ_KEYPAD_H_
#define _DNQ_KEYPAD_H_

#include "common.h"

/* key event status */
#define DNQ_KEY_RELEASE    0
#define DNQ_KEY_PRESSE     1
#define DNQ_KEY_HOLD       2

/* keypad for pins are 4*8 matrix PH pin */
#define DNQ_KEY_UP         46
#define DNQ_KEY_DOWN       48
#define DNQ_KEY_LEFT       30
#define DNQ_KEY_RIGHT      16 
#define DNQ_KEY_MENU       36
#define DNQ_KEY_SCAN       37
#define DNQ_KEY_EXIT       31
#define DNQ_KEY_OK         19
#define DNQ_KEY_SELF_CHECK 23


typedef void (*KeypadCallback)(U32 key, U32 status);
void dnq_keypad_callback_enable(KeypadCallback callback);

S32 dnq_keypad_init();
S32 dnq_keypad_deinit();

#endif /* _DNQ_KEYPAD_H_ */

