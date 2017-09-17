#!/bin/bash

export HardwareVersion=0x100
export SoftwareVersion=0x101

export UPGRADE_SWVER=0x100

export TOP_DIR=$(pwd)

export PATH=$PATH:$TOP_DIR/bin

mkdir -p $TOP_DIR/pub/image

echo "TOP_DIR=$TOP_DIR"
echo "PUB_DIR=$TOP_DIR/pub"
#echo "PATH=$PATH"
echo "set environment ok!"

