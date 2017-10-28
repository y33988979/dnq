#!/bin/bash

export HardwareVersion=0x100
export SoftwareVersion=0x102
##1.00 初始版本
##1.01 BUG修复，添加一代传感器支持，更新dnq_config_t结构，上传软硬件版本到服务器，等。
##1.02 添加升级后清理脚本upgrade_done.sh，代码中添加rootfs调整功能，等。


export UPGRADE_SWVER=0x100

export TOP_DIR=$(pwd)

export PATH=$PATH:$TOP_DIR/bin

mkdir -p $TOP_DIR/pub/image

echo "TOP_DIR=$TOP_DIR"
echo "PUB_DIR=$TOP_DIR/pub"
#echo "PATH=$PATH"
echo "set environment ok!"

