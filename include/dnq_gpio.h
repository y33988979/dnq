#ifndef _DNQ_GPIO_H_
#define _DNQ_GPIO_H_

#include "common.h"

/* direction */
#define GPIO_IN      0
#define GPIO_OUT     1

/* value */
#define GPIO_LOW     0
#define GPIO_HIGH    1

#define GPIO_PH11    0xEB

typedef enum _gpio
{
    PH01 = 0xE0,
    PH11 = 0xEB,
}gpio_e;


#endif /* _DNQ_GPIO_H_ */

