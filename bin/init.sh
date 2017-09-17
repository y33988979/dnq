#!/bin/sh

if [ test -e /root/dnq/sn.config ];then
    insmod w1_ds18b20.ko
fi

echo "dnqV2 ready!"

