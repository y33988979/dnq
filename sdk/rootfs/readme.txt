
rootfs_48.tar.gz ：sdk完整的rootfs文件系统目录 （gcc4.8）
rootfs_48strip.tar.bz2 ：项目使用的rootfs文件系统目录（裁剪过后的）

制作jffs2文件系统：

使用mkfs.jffs2工具来制作。命令如下：

rootfs_dir=$1
rootfs=rootfs.jffs2
不指定文件系统的大小，生成jffs2镜像（--pad参数可以指定文件系统生成的大小，但--pad和值之间不能有空格，否则不生效，也可以使用=号）
./mkfs.jffs2 -s 0x1000 -e 0x10000 --pad 0x700000 -d  $rootfs_dir -o $rootfs.nopad -n -l
指定文件系统的大小，生成jffs2镜像
./mkfs.jffs2 -s 0x1000 -e 0x10000 --pad=0x700000 -d  $rootfs_dir -o $rootfs -n -l

可以参考mkfs.sh脚本。（tools/linux/bin）
