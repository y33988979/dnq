#ifndef _DNQ_CHECKSUM_H_
#define _DNQ_CHECKSUM_H_

#include "common.h"

#define POLY 0x8005

U16 crc16(U8 *addr, U32 num ,U32 crc_initial);
U32 crc32(U32 crc, U8 *buffer, U32 size) ;

S32 calc_img_crc(const U8 *file_name, U32 *img_crc);

void dnq_checksum_init();

#endif /* _DNQ_CHECKSUM_H_ */

