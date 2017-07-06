#!/bin/sh

i=0
n=0
while(true)
do
    upgrade_pro = `pgrep dnq_upgrade`
    if [ "$upgrade_pro" == "" ]; then
        let "count=0"
        echo "dnq_upgrade is died..... cnt=$count"
        echo "restart dnq_upgrade....."
        ./dnq_upgrade
    else
        let "n=n+1"
    fi
    sleep 10
done