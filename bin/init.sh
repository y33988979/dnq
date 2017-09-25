#!/bin/sh

if [ -f /root/dnq/sn.conf ];then
    insmod /root/dnq/w1_ds18b20.ko
fi

echo "dnqV2 ready!"

