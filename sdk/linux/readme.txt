
nuc972_config_dnqV2  为项目最终使用的内核.config文件。

记录一下之前kernel调试的过程：

如下是拿到linux软件包后，在电暖气硬件平台上，kernel编译和适配的过程。
1、先使用nuc972_defconfig默认配置。
make nuc972_defconfig
2、添加硬件板的内核驱动配置
make menuconfig
2.1 添加内核的.config support，proc/config.gz支持。
选择General setup  ---> 
    <*> Kernel .config support
    [*]   Enable access to .config through /proc/config.gz 

.config中对应的配置项如下：
CONFIG_IKCONFIG=y
CONFIG_IKCONFIG_PROC=y
功能说明：生成的kernel，在proc目录下存放config.gz，就是当前内核的.config配置文件，以gzip压缩形式存放。

2.2 取消内核initramfs/initrd support，取消下面两项
选择General setup  ---> 
[ ] Initial RAM filesystem and RAM disk (initramfs/initrd) support
()    Initramfs source file(s) (NEW)
.config中对应的配置项如下：
CONFIG_BLK_DEV_INITRD=y
CONFIG_INITRAMFS_SOURCE="../rootfs"
CONFIG_INITRAMFS_ROOT_UID=0
CONFIG_INITRAMFS_ROOT_GID=0
CONFIG_RD_GZIP=y
CONFIG_RD_BZIP2=y
CONFIG_RD_LZMA=y
CONFIG_RD_XZ=y
CONFIG_RD_LZO=y
CONFIG_INITRAMFS_COMPRESSION_NONE=y
# CONFIG_INITRAMFS_COMPRESSION_GZIP is not set
# CONFIG_INITRAMFS_COMPRESSION_BZIP2 is not set
# CONFIG_INITRAMFS_COMPRESSION_LZMA is not set
# CONFIG_INITRAMFS_COMPRESSION_XZ is not set
# CONFIG_INITRAMFS_COMPRESSION_LZO is not set
# CONFIG_CC_OPTIMIZE_FOR_SIZE is not set
功能说明：生成的独立的kernel，kernel不自带rootfs。需要mount外部的rootfs文件系统。

2.3 取消内核无线的支持，因为项目没有无线的相关功能。取消CONFIG_WIRELESS选项。
选择[*] Networking support  --->
        [ ]   Wireless  ---> 
   
内核的.config将关掉如下选项：
CONFIG_WIRELESS=y
# CONFIG_NVT_EXT_WIFI is not set
CONFIG_WEXT_CORE=y
CONFIG_WEXT_PROC=y
CONFIG_CFG80211=y
# CONFIG_NL80211_TESTMODE is not set
# CONFIG_CFG80211_DEVELOPER_WARNINGS is not set
# CONFIG_CFG80211_REG_DEBUG is not set
CONFIG_CFG80211_DEFAULT_PS=y
CONFIG_CFG80211_DEBUGFS=y
# CONFIG_CFG80211_INTERNAL_REGDB is not set
CONFIG_CFG80211_WEXT=y
# CONFIG_LIB80211 is not set
# CONFIG_MAC80211 is not set
功能说明：去掉内核中的无线功能。

2.4 添加spi总线支持，因为项目使用了spi nor flash硬件，需要打开spi相关选项。
选择Device Drivers  --->
    [*] SPI support  --->
    <*>   Nuvoton NUC970 Series SPI Port 0
        SPI0 pin selection by transfer mode (Quad mode)  --->
         ( ) Normal mode
         (X) Quad mode

内核的.config将加入如下选项：
#
# SPI Master Controller Drivers
#
# CONFIG_SPI_ALTERA is not set
CONFIG_SPI_BITBANG=y
# CONFIG_SPI_GPIO is not set
# CONFIG_SPI_OC_TINY is not set
# CONFIG_SPI_PXA2XX_PCI is not set
# CONFIG_SPI_SC18IS602 is not set
# CONFIG_SPI_XCOMM is not set
# CONFIG_SPI_XILINX is not set
CONFIG_SPI_NUC970_P0=y
# CONFIG_SPI_NUC970_P0_NORMAL is not set
CONFIG_SPI_NUC970_P0_QUAD=y
# CONFIG_SPI_NUC970_P0_SS1 is not set
# CONFIG_SPI_NUC970_P1 is not set
# CONFIG_SPI_DESIGNWARE is not set

#
# SPI Protocol Masters
#
# CONFIG_SPI_SPIDEV is not set
# CONFIG_SPI_TLE62X0 is not set
功能说明：添加spi总线支持，芯塘nuc972有2个spi，我们使用了port0，接了spi nor flash，所以要打开支持。
    
2.5 添加mtd设备层支持，因为项目使用了spi nor flash硬件，需要通过mtd层进行flash的读写访问。
选择Device Drivers  --->
    <*> Memory Technology Device (MTD) support  --->
         <*>   Command line partition table parsing
         -*-   Common interface to block layer for MTD 'translation layers'
         <*>   Caching block device access to MTD devices
            Self-contained MTD device drivers  --->
            <*> Support most SPI Flash chips (AT26DF, M25P, W25X, ...) 
            [*]   Use FAST_READ OPCode allowing SPI CLK >= 50MHz (NEW)

内核的.config将加入如下选项：
CONFIG_MTD=y
# CONFIG_MTD_TESTS is not set
# CONFIG_MTD_REDBOOT_PARTS is not set
CONFIG_MTD_CMDLINE_PARTS=y
# CONFIG_MTD_AFS_PARTS is not set
# CONFIG_MTD_AR7_PARTS is not set

#
# User Modules And Translation Layers
#
CONFIG_MTD_BLKDEVS=y
CONFIG_MTD_BLOCK=y
# CONFIG_FTL is not set
# CONFIG_NFTL is not set
# CONFIG_INFTL is not set
# CONFIG_RFD_FTL is not set
# CONFIG_SSFDC is not set
# CONFIG_SM_FTL is not set
# CONFIG_MTD_OOPS is not set
# CONFIG_MTD_SWAP is not set

#
# RAM/ROM/Flash chip drivers
#
# CONFIG_MTD_CFI is not set
# CONFIG_MTD_JEDECPROBE is not set
CONFIG_MTD_MAP_BANK_WIDTH_1=y
CONFIG_MTD_MAP_BANK_WIDTH_2=y
CONFIG_MTD_MAP_BANK_WIDTH_4=y
# CONFIG_MTD_MAP_BANK_WIDTH_8 is not set
# CONFIG_MTD_MAP_BANK_WIDTH_16 is not set
# CONFIG_MTD_MAP_BANK_WIDTH_32 is not set
CONFIG_MTD_CFI_I1=y
CONFIG_MTD_CFI_I2=y
# CONFIG_MTD_CFI_I4 is not set
# CONFIG_MTD_CFI_I8 is not set
# CONFIG_MTD_RAM is not set
# CONFIG_MTD_ROM is not set
# CONFIG_MTD_ABSENT is not set

#
# Mapping drivers for chip access
#
# CONFIG_MTD_COMPLEX_MAPPINGS is not set
# CONFIG_MTD_PLATRAM is not set

#
# Self-contained MTD device drivers
#
# CONFIG_MTD_DATAFLASH is not set
CONFIG_MTD_M25P80=y
CONFIG_M25PXX_USE_FAST_READ=y
# CONFIG_MTD_SST25L is not set
# CONFIG_MTD_SLRAM is not set
# CONFIG_MTD_PHRAM is not set
# CONFIG_MTD_MTDRAM is not set
# CONFIG_MTD_BLOCK2MTD is not set

#
# Disk-On-Chip Device Drivers
#
# CONFIG_MTD_DOCG3 is not set
# CONFIG_MTD_NAND is not set
# CONFIG_MTD_ONENAND is not set

#
# LPDDR flash memory drivers
#
# CONFIG_MTD_LPDDR is not set
# CONFIG_MTD_UBI is not set
功能说明：添加mtd层支持，及芯塘的spi flash驱动，用于读写flash，及通过mtdblock挂载等。

2.6 添加I2C支持，主板上有一颗rtc芯片，ds1312sn，需要打开i2c功能对其读写，及rtc驱动。
（实际并没有用到I2C，可以不打开I2C，因为采用了单片机和rtc通信，cpu直接通过单片机来获取实时时间数据）
选择Device Drivers  --->
    <*> I2C support  --->
        I2C Hardware Bus support  --->
        <*> NUC970 I2C Driver for Port 0
2.7 添加RTC支持，（由于RTC芯片交给单片机来控制，RTC的选项不配置也行，如果要通过cpu来进行控制，则必须添加支持）
选择Device Drivers  --->
    [*] Real Time Clock  --->
        <*>   Dallas/Maxim DS1305/DS1306

2.8 添加keyboard支持，芯塘具有一个keypad模块，支持外部4*2，4*4，4*8矩阵键盘。需要打开驱动支持。
选择Device Drivers  --->
    Input device support  --->
        <*>   Event interface
        [*]   Keyboards  --->
            <*>   NUC970 Matrix Keypad support
            < >     NUC970 KEYPAD wake-up support (NEW)
            <*>     Pull-up NUC970 Matrix Keypad Pin (NEW)
                    NUC970 matrix keypad pin selection (Keypad pins are 4x8 matrix PH pin)  --->
                    ( ) Keypad pins are 4x2 matrix PA pin
                    ( ) Keypad pins are 4x4 matrix PA pin
                    ( ) Keypad pins are 4x8 matrix PA pin
                    ( ) Keypad pins are 4x2 matrix PH pin
                    ( ) Keypad pins are 4x4 matrix PH pin
                    (X) Keypad pins are 4x8 matrix PH pin

打开后，内核.config会有很多KEYBOARD选项。

2.9 添加串口支持，芯塘NUC972支持11路串口，可以根据实际情况进行配置及使用。
    电暖气项目使用了4个串口：
    uart1：consolse debug串口
    uart2：底板单片机(MCU)通信串口
    uart6：LCD 屏幕通信串口
    uart8：传感器探头（温度传观器）中间使用RS232-->RS485转换芯片
选择Device Drivers  --->
    Character devices  --->
        Serial drivers  --->
            [*] NUC970 serial support
            [*]   NUC970 UART1 support                                                                | |  
            [ ]     Enable UART1 CTS wake-up function (NEW)                                           | |  
                    NUC970 UART1 pin selection (Tx:PE2, Rx:PE3)  --->                                 | |  
            [*]   NUC970 UART2 support                                                                | |  
            [ ]     Enable UART2 CTS wake-up function (NEW)                                           | |  
                    NUC970 UART2 pin selection (Tx:PF11, Rx:PF12)  --->                               | |  
            [ ]   NUC970 UART3 support                                                                | |  
            [ ]   NUC970 UART4 support                                                                | |  
            [ ]   NUC970 UART5 support                                                                | |  
            [*]   NUC970 UART6 support                                                                | |  
            [ ]     Enable UART6 CTS wake-up function (NEW)                                           | |  
                    NUC970 UART6 pin selection (Tx:PB2, Rx:PB3)  --->                                 | |  
            [ ]   NUC970 UART7 support                                  
                  NUC970 UART7 support                                                                | |  
            [*]   NUC970 UART8 support                                                                | |  
            [ ]     Enable UART8 CTS wake-up function (NEW)                                           | |  
                    NUC970 UART8 pin selection (Tx:PH12, Rx:PH13)  --->                               | |  
            [ ]   NUC970 UART9 support                                                                | |  
            [ ]   NUC970 UART10 support                                                               | |  
            [*]   Console on NUC970 serial port   
2.10 添加GPIO管脚控制功能，由于使用了RS485与温度传感器通信，需要对485的ctrl_pin进行拉高拉低操作。
选择Device Drivers  --->
    -*- GPIO Support  --->
        [*]   /sys/class/gpio/... (sysfs interface)
功能说明：打开支持后，可以通过读写/sys/class/gpio/中的节点来对gpio管脚进行控制。

2.11 添加jffs2等文件系统支持
选择File systems  --->
    [*] Miscellaneous filesystems  --->
        <*>   Journalling Flash File System v2 (JFFS2) support
        (0)     JFFS2 debugging verbosity (0 = quiet, 2 = noisy) (NEW)
        [*]     JFFS2 write-buffering support (NEW)
        
        [ ]       Verify JFFS2 write-buffer reads (NEW)
        [ ]     JFFS2 summary support (NEW)
        [ ]     JFFS2 XATTR support (NEW)
        [ ]     Advanced compression options for JFFS2 (NEW)
        
        <*>   SquashFS 4.0 - Squashed file system support
        [ ]     Squashfs XATTR support (NEW)
        [*]     Include support for ZLIB compressed file systems (NEW)
        [ ]     Include support for LZO compressed file systems (NEW) 
        [ ]     Include support for XZ compressed file systems (NEW)
        [ ]     Use 4K device block size? (NEW)  
        [ ]     Additional option for memory-constrained systems (NEW) 
    
        ## 如果不使用，可以去掉
        < >   ROM file system support
        
        ##网络挂载支持
        [*] Network File Systems  --->

2.11 .config指定cmdline，用于kernel解析：
CONFIG_CMDLINE="root=/dev/mtdblock2 rw rootfstype=jffs2 console=ttyS0,115200n8 rdinit=/sbin/init mem=64M"
