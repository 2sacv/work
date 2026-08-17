#!/bin/sh
usb_eth='/mnt/f_eth.txt'

. /ipc/bin/io.rc

LOGGER() { echo "$@"; logger "$@"; echo "$@" > /dev/kmsg;}

fn_4g_reboot()
{
    LOGGER "
    ====================
    4g err reboot
    ===================="
    
    if test -d /sys/class/net/eth0/ && ifconfig eth0 | grep -q "RUNNING"; then
        LOGGER "eth RUNNING, 4g err continue"
        return
    fi

    /ipc/bin/toggle tar
    sync; sleep .5; sync;
    killall -9 fsck.exfat
    echo 1 > /sys/class/mmc_host/mmc0/reboot
    sleep 2
}

fn_usbnet_setip()
{
    echo "eth over usb"
    sleep 2
    ifconfig usb0 up
    ifconfig usb0 192.168.1.217
    #
    if [ -f /opt/conf/config.xml ]; then
        dev_ip=`xml_shuttle -cr -k /cfg/eth/ip /opt/conf/config.xml`
        dev_ip=`echo $dev_ip| awk -F' ' '{print $3}'`
        dev_gw=`xml_shuttle -cr -k /cfg/eth/gw /opt/conf/config.xml`
        dev_gw=`echo $dev_gw| awk -F' ' '{print $3}'`
        if [ $dev_ip != "192.168.1.217" ]; then
            echo "dev_ip = $dev_ip, dev_gw = $dev_gw"
            route del default gw 0.0.0.0 dev usb0
            ifconfig usb0 $dev_ip
            route add default gw $dev_gw dev usb0 metric 1
        fi
    fi

    if [ $dev_ip = "192.168.1.217" ]; then
        udhcpc -i usb0 -R -b -T2 -A3 &
    fi
    touch /tmp/usb2eth
}

fn_ethnet_setip()
{
	echo "etho up"
	echo 200000 > /sys/class/net/eth0/queues/tx-0/byte_queue_limits/limit_min
	# default ipaddr
	if grep -q 'root=/dev/nfs' /proc/cmdline; then
		echo "run on nfs, won't set default ip!"
	else
		ETHADDR=`cat /proc/cmdline | xargs -n1 | awk -F'=' '/ethaddr/{print $2}'`
		if [ -n $ETHADDR ]; then
			ifconfig eth0 down;
			ifconfig eth0 hw ether $ETHADDR;
			ifconfig eth0 up
		fi
		ifconfig eth0 192.168.1.217;
		route add default gw 192.168.1.1 dev eth0
		ifconfig eth0 mtu 1460
		ifconfig lo up;    
	fi

	#增强网络性能
    echo 3 > /sys/class/net/eth0/queues/rx-0/rps_cpus
    echo 32768 > /proc/sys/net/core/rps_sock_flow_entries
    echo 32768 > /sys/class/net/eth0/queues/rx-0/rps_flow_cnt

    if [ -z "$1" ]; then
        udhcpc -i eth0 -R -b -T2 -A3 &
    fi
}

fn_rmmod_result_check()
{
    rmmod $1
    rmmod_ret=$?
    mod_list=`lsmod | awk '{print $1}' | grep $1`
    if [ $rmmod_ret -ne 0 ] || [ -n "${mod_list}" ]; then 
        echo "rmmod $1 failed"
    fi
}

fn_eth_insmod_result_check()
{
    insmod /ipc/drv/phy/$1
    insmod_ret=$?
    suffix=".ko"
    mod_list=`lsmod | awk '{print $1}' | grep ${1%${suffix}}`
    if [ $insmod_ret -ne 0 ] || [ "${mod_list}" == "" ]; then 
        echo "insmod /ipc/drv/phy/$1 failed"
    fi
}

fn_usb_insmod_result_check()
{
    insmod /ipc/drv/usb/$1
    insmod_ret=$?
    suffix=".ko"
    mod_list=`lsmod | awk '{print $1}' | grep ${1%${suffix}}`
    if [ $insmod_ret -ne 0 ] || [ "${mod_list}" == "" ]; then 
        echo "insmod /ipc/drv/usb/$1 failed"
    fi
}

fn_usb_load()
{
    vbus=`cat /sys/class/gpio/gpio59/value`
    if [ "$vbus" -eq 0 ]; then
        echo out > /sys/class/gpio/gpio59/direction
        echo 1 > /sys/class/gpio/gpio59/value
    fi

    fn_usb_insmod_result_check mii.ko
    fn_usb_insmod_result_check xhci_hcd.ko
    fn_usb_insmod_result_check xhci_plat_hcd.ko
    fn_usb_insmod_result_check dwc3.ko
    sleep .5

    if [ -f ${usb_eth} ]; then
        fn_usb_insmod_result_check dwc3.ko
        fn_usb_insmod_result_check usbnet.ko
        fn_usb_insmod_result_check asix.ko
        fn_usbnet_setip
    else
        fn_usb_insmod_result_check usbnet.ko
        fn_usb_insmod_result_check cdc_ether.ko
        fn_usb_insmod_result_check rndis_host.ko
        fn_usb_insmod_result_check cdc_acm.ko
        fn_usb_insmod_result_check usb_wwan.ko
        fn_usb_insmod_result_check option.ko
        let count=0
        USB_IDVENDOR="/sys/bus/usb/devices/1-1/idVendor"
        USB_TTY="/dev/ttyUSB2"

        
        while [ ! -f $USB_IDVENDOR ] && [ $count -lt 50 ]; do
            let count++;
            sleep 0.1
            echo "wait @partion_adding $count"
        done

        count=0
        while [ ! -e $USB_TTY ] && [ $count -lt 5 ]; do
            let count++;
            sleep 0.1
            echo "wait @/dev/ttyUSB $count"
        done

        SIM4G_AT_TXBYUGA="/sys/class/net/usb0/statistics/tx_bytes"
        if [ ! -c ${USB_TTY} ] || [ ! -f ${SIM4G_AT_TXBYUGA} ]; then
            echo "===================================================="
            ls /sys/bus/usb/drivers/cdc_ether
            ls /sys/class/net/usb0/statistics
            ls /dev/ttyUSB*
            echo "===================================================="
            fn_4g_reboot
        fi

        ls /dev/ttyUSB*
        DEV_USB1=$(basename /sys/bus/usb/devices/1-1:1.3/tty*)
        DEV_USB2=$(basename /sys/bus/usb/devices/1-1:1.4/tty*)

        if [ "$DEV_USB1" != "ttyUSB1" ]; then                 
            echo "usb $DEV_USB1"                              
            ln -sf /dev/"$DEV_USB1" /dev/ttyUSB1              
        fi                                                    

        if [ "$DEV_USB2" != "ttyUSB2" ]; then                 
            echo "usb $DEV_USB2"                              
            ln -sf /dev/"$DEV_USB2" /dev/ttyUSB2              
        fi                                                    

        ln -sf /dev/ttyUSB2 /dev/tty4G

        mkdir -p /tmp/4g/ /opt/conf/4g/
    fi

    if [ $? -ne 0 ]; then
        echo "4g ko init failed"
        exit 1
    fi

    sleep .1
    /ipc/bin/4g LTE at_init | tee -a /tmp/messages
}

fn_eth_load()
{
	if [ -e /mnt/ethenable.txt ] ; then
		fn_eth_insmod_result_check mdio_bsp_femac.ko
		fn_eth_insmod_result_check hl_eth.ko

		fn_ethnet_setip

		if [ $? -ne 0 ]; then
			echo "eth ko load failed"
			return 0;
		fi
	fi
}

fn_eth_unload()
{
	if [ -e /mnt/ethenable.txt ] ; then
		if ifconfig eth0 > /dev/null 2>&1; then
			ifconfig eth0 down
		fi

		fn_rmmod_result_check hl_eth.ko
		fn_rmmod_result_check mdio_bsp_femac.ko

		if [ $? -ne 0 ]; then
			echo "eth ko unload failed"
			return 1;
		fi
	fi
}

fn_usb_unload()
{
    ps | awk "/[u]dhcpc.*usb0/{print \$1}" | while read PID; do kill -USR2 $PID; kill -9 $PID; done

    echo 1 > /sys/bus/usb/devices/1-1/remove
    usleep 500000

    fn_rmmod_result_check option.ko
    fn_rmmod_result_check usb_wwan.ko
    fn_rmmod_result_check cdc_acm.ko
    fn_rmmod_result_check rndis_host.ko
    fn_rmmod_result_check cdc_ether.ko
    fn_rmmod_result_check usbnet.ko
	fn_rmmod_result_check dwc3.ko
	fn_rmmod_result_check xhci_plat_hcd.ko
	fn_rmmod_result_check xhci_hcd.ko
	fn_rmmod_result_check mii.ko

    vbus=`cat /sys/class/gpio/gpio59/value`
    if [ "$vbus" -eq 1 ]; then
        echo out > /sys/class/gpio/gpio59/direction
        echo 0 > /sys/class/gpio/gpio59/value
    fi

    if [ $? -ne 0 ]; then
        echo "rmmod 4G ko failed"
        exit 1
    else
        echo "rmmod 4g ko success"
    fi
}

fn_usbinit()
{
    if [ -f /ipc/bin/4g ]; then
        fn_gpio 4g on
        fn_gpio vbus on
        echo  4g > /tmp/usb_dev
        mkdir -p /tmp/4g/ /opt/conf/4g/
	    sleep 1
        fn_usb_load
    elif [ -f /ipc/bin/wifi ]; then
        echo wifi > /tmp/usb_dev
        mkdir -p /tmp/wifi/ /opt/conf/wifi/
        /ipc/drv/airlink/wifi_load.sh
        lsmod > /tmp/lsmod
        grep -q "ble_soc" /tmp/lsmod
        [ "${?}" -eq 0 ] && touch /tmp/ble
    fi
}

fn_main()
{
    case $1 in
    init)
        fn_usbinit;
        ;;
	init_eth)
        fn_eth_load;
        ;;
    load)
        fn_usb_load;
        ;;
    unload)
        fn_usb_unload;
        ;;
    *)
        echo "UNKNOW operate"
        ;;
    esac
}

fn_main $@
