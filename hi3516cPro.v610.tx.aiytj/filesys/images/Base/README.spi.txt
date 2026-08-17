在UBOOT中SPI FLASH共分10个部分：

名称                起始位置             大小
sf-uboot            0x000000            0x030000 = 192 *1024    (192k)
sf-env              0x030000            0x010000 = 64  *1024    (64k)
sf-kernel           0x040000            0x2C0000 = 2816*1024    (2816K)
sf-rootfs           0x300000            0x0D0000 = 832 *1024    (832K)
sf-ipcfs            0x3D0000            0x670000 = 6592*1024    (6592K)
sf-algofs           0xA40000            0x540000 = 5376*1024    (5376K)
sf-optfs            0xF80000            0x080000 = 512 *1024    (512K)
                                                    共计：16M(16384K)

wifi还差 340K，期望11月16日 爱芯提供新压缩方法节省内核空间，预计可以省500K左右

制作文件系统命令见附件，使用如下：
./mkfs.jffs2 -r miscfs -o miscfs.jffs2 -e 32KiB --pad=0xB0000 -s 0x100 -n －b 
其中--pad=0xB0000为输出的镜像的大小为0.7M

./mksquashfs filesys rootfs.sqfs -comp xz

挂载：
mount -t squashfs /dev/mtdblock4 /ipc
mount -t jffs2 /dev/mtdblock5 /opt

在文件系统中(只在拯救模式中)：
擦除：    mtd_debug erase /dev/mtd5 0 0xB0000 
烧写：    mtd_debug write /dev/mtd5 0 0xB0000 ./miscfs.jffs2

一旦分区改变，需要同步修改的地方：
1，u-boot
2, generate_firmware，镜像生成工具

uboot下烧写命令：
downboot- load u-boot.bin tftp
downkernel- load uImage tftp
downrootfs- load rootfs.sqfs tftp
downipcfs- load ipcfs.sqfs tftp
downmiscfs- load filesys.jffs2 tftp

downekernel- load big uImage tftp

uboot恢复默认环境参数
format

调整分区修改：
filesys\images\Base\README.spi.txt
uboot\patch\configs\hi3516cv610_debug_defconfig
uboot\patch\cmd\load.c
filesys\script\pack\npack.sh   (仅大小变化则不用修改)
filesys\script\pack\rc.d\generate_firmware
filesys\script\upTools\updateExt.sh  (仅大小变化则不用修改)
filesys\filesys_normal\sbin\miscmount  (仅大小变化则不用修改)
tools\generate_firmware\generate_firmware.c
