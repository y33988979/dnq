#ifndef _DNQ_MANAGE_H_
#define _DNQ_MANAGE_H_

#include "common.h"
#include "dnq_os.h"

#define DNQ_CONFIG_UPDATE   0x10

S32 dnq_manage_init();
S32 dnq_manage_deinit();
S32 send_msg_to_manage(dnq_msg_t *msg);


#endif /* _DNQ_MANAGE_H_ */

