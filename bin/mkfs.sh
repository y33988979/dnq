#!/bin/bash

if [ $# -lt 1 ]
then
    echo "Usage: ./mkfs.sh rootfs"
    exit
fi
rootfs_dir=$1
rootfs=rootfs.jffs2

#./mkfs.jffs2 -s 0x1000 -e 0x10000 --pad=0x400000 -d  $rootfs_dir -o $rootfs -n -l
./mkfs.jffs2 -s 0x1000 -e 0x10000 --pad 0x700000 -d  $rootfs_dir -o $rootfs.nopad -n -l
./mkfs.jffs2 -s 0x1000 -e 0x10000 --pad=0x700000 -d  $rootfs_dir -o $rootfs -n -l
./mkfs.jffs2 -s 0x1000 -e 0x10000 --pad=0x380000 -d  data -o data.jffs2 -n -l

ls -l $rootfs
ls -lh $rootfs
cp $rootfs image
