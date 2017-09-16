#!/bin/bash

./mkimage -A arm -O linux -T kernel -a 0x7fc0 -e 8000 -d 970image 970image.ub

# please open macro --> define CONFIG_NUC970_HW_CHECKSUM
./mkimage -A arm -O linux -T kernel -S sha1 -a 0x7fc0 -e 8000 -d 970image 970image.ub

# please close macro --> #define CONFIG_NUC970_HW_CHECKSUM
./mkimage -A arm -O linux -T kernel -S crc32 -a 0x7fc0 -e 8000 -d 970image 970image.ub

# please 
./mkimage -A arm -O linux -T kernel -E AES -K key.dat -a 0x7fc0 -e 8000 -d 970image 970image.ub

