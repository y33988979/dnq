#ifndef _DNQ_SENSOR_H_
#define _DNQ_SENSOR_H_


#include "common.h"


/* sensor response data lenght */
#define SENSOR_RESPONSE_LEN       15
#define SENSOR_REQUEST_LEN        13

S32 dnq_sensor_init();
S32 dnq_sensor_deinit();

#endif /* _DNQ_SENSOR_H_ */

