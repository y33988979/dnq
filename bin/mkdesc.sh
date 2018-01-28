#!/bin/sh

input_file=$1
output_file=$2

echo $1 $2

tag="47"
upgrade_type="10"
file_type="01"
mode="01"
need_ver="0000"
mac="70b3d5cf4924"
hw_ver=0`echo $HardwareVersion |cut -c 3-6`
sw_ver=0`echo $SoftwareVersion |cut -c 3-6`
crc_32=`crc32 $input_file | tr [a-z] [A-Z]`

echo $tag$upgrade_type$file_type$mode$need_ver$mac$hw_ver$sw_ver$crc_32 > $output_file
desc=`cat $output_file`
echo "upgrade descriptor: $desc"

