#!/bin/sh

if [ -f /root/dnq/SN.conf ];then
    insmod /root/dnq/w1_ds18b20.ko
fi

size=`stat -c %s /root/dnq/exit.dump`
if [ $size -gt 102400 ];then
    rm -rf /root/dnq/exit.dump
    echo "delete exit.dump"
fi

echo "dnqV2 ready!"

