#! /bin/bash

BUILD_DIR=$PWD
KERNEL_DIR=linux-5.10.221
INSTALL_PATH=../../filesys/images/Base
CROSS=arm-v01c02-linux-musleabi-
MKIMAGE_DIR=$BUILD_DIR/_mkimage
TOOLS_DIR=$BUILD_DIR/tools
ITS_FILE=linux_image.its
USB_PATH=../../filesys/filesys_normal/ipc/drv/usb
MMC_PATH=../../filesys/filesys_normal/ipc/drv/mmc
KERNEL_NAME=uImage

tag=$1

function xt_ret()
{
    [ "${1}" = "0" ] && return 0
    printf "${BASH_SOURCE[1]##*/}%-6s" "|${BASH_LINENO[0]}|"    # no FUNCNAME print
    FG=31 BG=40
    echo -e "\E[${FG};${BG}m${@:2}"
    echo -ne "\E[0m"
    return 1
}

function fn_bkup_vmlinux()
{
    test -f vmlinux || return
    p_vm='../../filesys/images/vm/'
    test -d ${p_vm} || { mkdir -p ${p_vm}; svn add ${p_vm} ;}
    stamp=`stat -c %Y vmlinux`
    stamp=`date +%Y%m%d.%H%M%S -d @${stamp}`
    cp -a vmlinux ${p_vm}/vmlinux.${stamp}.${tag}
#    svn add ${p_vm}/vmlinux.${stamp}.${tag}
    echo ${tag} > ../.banner
}

if [ ! -d $KERNEL_DIR ]; then
    tar -xf $KERNEL_DIR.tar.gz
	echo "patch ${KERNEL_DIR} ..."
	patch -d $KERNEL_DIR -Np1 < $KERNEL_DIR.patch
	patch -d $KERNEL_DIR -Np1 < bspwdt_linux.patch
fi

rm $MKIMAGE_DIR -rf
mkdir $MKIMAGE_DIR
cd $KERNEL_DIR
cp -a ../patch/* .


case $1 in
b|build)
	echo "-----build kernel opt b"
	case ${2:-4} in
	4|4g)
		echo "-----4g config--------"
		cp -rf  ../config_jco_4g .config #config_jco 参考 hi3516cv608_debug_defconfig
		KERNEL_NAME=uImage.4g
		;;
	w|wifi)
		echo "-----wifi config--------"
		cp -rf  ../config_jco .config #config_jco 参考 hi3516cv608_debug_defconfig
		KERNEL_NAME=uImage
		;;
	*)
		echo "no find config"
		exit
		;;
	esac
	make  ARCH=arm LLVM= LLVM_IAS=  CROSS_COMPILE=$CROSS zImage -j$(nproc) &&
	make  ARCH=arm LLVM= LLVM_IAS=  CROSS_COMPILE=$CROSS dtbs -j$(nproc) &&
    xt_ret $? "" || return $?
	#fn_bkup_vmlinux
	cp arch/arm/boot/zImage $MKIMAGE_DIR/
	cp arch/arm/boot/dts/hi3516cv608-demb-flash.dtb $MKIMAGE_DIR/devicetree.dtb
	cp $TOOLS_DIR/linux_image.its $MKIMAGE_DIR/	
	pushd $MKIMAGE_DIR;mkimage -f $ITS_FILE uImage; popd
	mv $MKIMAGE_DIR/uImage $INSTALL_PATH/$KERNEL_NAME
	rm $MKIMAGE_DIR -r
	echo "-----build kernel modules"
	make  ARCH=arm LLVM= LLVM_IAS=  CROSS_COMPILE=$CROSS modules -j$(nproc)
	for i in                            	\
	./fs/exfat/exfat.ko                 	\
	./drivers/net/mii.ko                	\
	./drivers/net/usb/cdc_ether.ko      	\
	./drivers/net/usb/usbnet.ko         	\
	./drivers/net/usb/rndis_host.ko     	\
	./drivers/net/usb/asix.ko           	\
	./drivers/usb/serial/usb_wwan.ko    	\
	./drivers/usb/serial/option.ko      	\
	./drivers/usb/class/cdc-acm.ko      	\
	./drivers/usb/dwc3/dwc3.ko				\
	./drivers/usb/host/xhci-hcd.ko			\
	./drivers/usb/host/xhci-plat-hcd.ko;
	do
        if [ -f $USB_PATH ]; then
		    cp $i $USB_PATH
        else
            echo "$USB_PATH not exist"
        fi
	done
	
	for j in                               \
	./drivers/mmc/core/mmc_block.ko        \
	./drivers/mmc/core/mmc_core.ko         \
	./drivers/mmc/core/pwrseq_simple.ko    \
	./drivers/mmc/host/sdhci_complement.ko \
	./drivers/mmc/host/sdhci-pltfm.ko      \
	./drivers/mmc/host/sdhci.ko            \
	./drivers/vendor/mmc/sdhcinebula.ko; 
	do
        if [ -f $MMC_PATH ]; then
		    cp $j $MMC_PATH
        else
            echo "$MMC_PATH not exist"
        fi
	done
	mv $USB_PATH/cdc-acm.ko $USB_PATH/cdc_acm.ko
	mv $USB_PATH/xhci-plat-hcd.ko $USB_PATH/xhci_plat_hcd.ko
	${CROSS}strip --strip-debug $USB_PATH/*
	mv $USB_PATH/exfat.ko $USB_PATH/../exfat.ko
	mv $MMC_PATH/sdhci-pltfm.ko $MMC_PATH/sdhci_pltfm.ko
    ;;
r|rebuild)
	echo "-----build kernel opt r"
    make ARCH=arm LLVM= LLVM_IAS=  CROSS_COMPILE=$CROSS distclean
	case ${2:-4} in
	4|4g)
		echo "-----4g config--------"
		cp -rf  ../config_jco_4g .config #config_jco 参考 hi3516cv608_debug_defconfig
		KERNEL_NAME=uImage.4g
		;;
	w|wifi)
		echo "-----wifi config--------"
		cp -rf  ../config_jco .config #config_jco 参考 hi3516cv608_debug_defconfig
		KERNEL_NAME=uImage
		;;
	*)
		echo "no find config"
		exit
		;;
	esac
	make  ARCH=arm LLVM= LLVM_IAS=  CROSS_COMPILE=$CROSS zImage -j$(nproc) &&
	make  ARCH=arm LLVM= LLVM_IAS=  CROSS_COMPILE=$CROSS dtbs -j$(nproc)
    xt_ret $? "" || return $?
	#fn_bkup_vmlinux
	cp arch/arm/boot/zImage $MKIMAGE_DIR/
	cp arch/arm/boot/dts/hi3516cv608-demb-flash.dtb $MKIMAGE_DIR/devicetree.dtb
	cp $TOOLS_DIR/linux_image.its $MKIMAGE_DIR/	
	pushd $MKIMAGE_DIR;mkimage -f $ITS_FILE uImage; popd
	mv $MKIMAGE_DIR/uImage $INSTALL_PATH/$KERNEL_NAME
	rm $MKIMAGE_DIR -r
	echo "-----build kernel modules"
	make  ARCH=arm LLVM= LLVM_IAS=  CROSS_COMPILE=$CROSS modules -j$(nproc) 
    xt_ret $? "" || return $?

	for i in                            	\
	./fs/exfat/exfat.ko                 	\
	./drivers/net/mii.ko               		\
	./drivers/net/usb/cdc_ether.ko      	\
	./drivers/net/usb/usbnet.ko         	\
	./drivers/net/usb/rndis_host.ko     	\
	./drivers/net/usb/asix.ko           	\
	./drivers/usb/serial/usb_wwan.ko    	\
	./drivers/usb/serial/option.ko      	\
	./drivers/usb/class/cdc-acm.ko      	\
	./drivers/usb/dwc3/dwc3.ko				\
	./drivers/usb/host/xhci-hcd.ko			\
	./drivers/usb/host/xhci-plat-hcd.ko;
	do
		cp $i $USB_PATH || break
	done
	
	for j in                               \
	./drivers/mmc/core/mmc_block.ko        \
	./drivers/mmc/core/mmc_core.ko         \
	./drivers/mmc/core/pwrseq_simple.ko    \
	./drivers/mmc/host/sdhci_complement.ko \
	./drivers/mmc/host/sdhci-pltfm.ko      \
	./drivers/mmc/host/sdhci.ko            \
	./drivers/vendor/mmc/sdhcinebula.ko; 
	do
		cp $j $MMC_PATH || break
	done

	mv $USB_PATH/cdc-acm.ko $USB_PATH/cdc_acm.ko
	mv $USB_PATH/xhci-plat-hcd.ko $USB_PATH/xhci_plat_hcd.ko
	mv $USB_PATH/xhci-hcd.ko $USB_PATH/xhci_hcd.ko
	${CROSS}strip --strip-debug $USB_PATH/*
    mv $USB_PATH/exfat.ko $USB_PATH/../exfat.ko
	mv $MMC_PATH/sdhci-pltfm.ko $MMC_PATH/sdhci_pltfm.ko
    ;;
m|menuconfig)
    make menuconfig ARCH=arm CC=arm-v01c02-linux-musleabi-gcc
    ;;
c|clean)
    make ARCH=arm LLVM= LLVM_IAS=  CROSS_COMPILE=$CROSS distclean
    ;;
*)
    echo Usage:
    echo "  $0 b|build       {[4]|w}    # build, default 4G"
    echo "  $0 r|rebuild     {[4]|w}    # config and re-build"
    echo "  $0 m|menuconfig             # configuration"
    ;;
esac
