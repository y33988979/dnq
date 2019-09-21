#!/bin/bash

export HardwareVersion=0x100
export SoftwareVersion=0x105
##1.00 初始版本
##1.01 BUG修复，添加一代传感器支持，更新dnq_config_t结构，上传软硬件版本到服务器，等。
##1.02 添加升级后清理脚本upgrade_done.sh，代码中添加rootfs调整功能，等。
##1.03 如果温度传感器获取不到温度值，或者温度传感器坏了，那么让电暖气一直工作。LCD屏幕显示软硬件版本信息。
##1.04 修复当温控策略条目过时导致死机问题。
##1.05 如果温度传感器获取不到温度值，或者温度传感器坏了,那么让电暖气一直停止。
       #LCD屏幕显示固定标题："尚诺碳晶-欢迎使用"
       #升级描述符以666666开头，云端可以对终端进行重启。


export UPGRADE_SWVER=0x100

export TOP_DIR=$(pwd)

export PATH=$PATH:$TOP_DIR/bin

mkdir -p $TOP_DIR/pub/image

echo "TOP_DIR=$TOP_DIR"
echo "PUB_DIR=$TOP_DIR/pub"
#echo "PATH=$PATH"
echo "set environment ok!"

