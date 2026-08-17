#! /bin/sh

. /ipc/bin/io.rc

fn_main()
{
    fn_gpio wifi off; sleep 3
    fn_gpio wifi on ; sleep 3

    set -x
    USB_IDVENDOR="/sys/bus/usb/devices/1-1/idVendor"
    for i in 2 1 2 1; do test -f ${USB_IDVENDOR} && break; echo sleep-$i; sleep $i; done

    idVendor=`cat /sys/bus/usb/devices/1-1/idVendor`
    test -z "$idVendor" && idVendor=`cat /sys/bus/usb/devices/usb1/idVendor`
    idProduct=`cat /sys/bus/usb/devices/1-1/idProduct`
    test -z "$idProduct" && idProduct=`cat /sys/bus/usb/devices/usb1/idProduct`

    echo "USB DEV - idVendor:${idVendor} idProduct:${idProduct}"

    [ "${1}" = show ] && exit

    case ${idVendor}.${idProduct} in
    ffff.3733)
        insmod /ipc/drv/airlink/plat_soc.ko
        insmod /ipc/drv/airlink/wifi_soc.ko
        insmod /ipc/drv/airlink/ble_soc.ko
        ;;
    0bda.f179|1d6b.0002)
        insmod /ipc/drv/airlink/8188fu.ko
        ;;
    0bda.0179)
        insmod /ipc/drv/airlink/8188eu.ko
        ;;
    esac

    ifconfig wlan0 mtu 1460 up
    
}

fn_main 2>&1 | tee -a /tmp/messages 
