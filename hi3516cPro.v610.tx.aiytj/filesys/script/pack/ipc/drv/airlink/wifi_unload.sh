#! /bin/sh

idVendor=`cat /sys/bus/usb/devices/1-1/idVendor`
if [ $? != 0 ]; then
	idVendor=`cat /sys/bus/usb/devices/usb1/idVendor`
fi
idProduct=`cat /sys/bus/usb/devices/1-1/idProduct`
if [ $? != 0 ]; then
	idProduct=`cat /sys/bus/usb/devices/usb1/idProduct`
fi
echo "USB DEV - idVendor:${idVendor} idProduct:${idProduct}"
drv_default=1

if [ "${idVendor}" = "ffff" ] && [ "${idProduct}" = "3733" ] ; then
        ifconfig wlan0 down
	rmmod ble_soc
        rmmod wifi_soc
	rmmod plat_soc
	drv_default=0
fi

if [ "${idVendor}" = "148f" ] && [ "${idProduct}" = "7601" ] ; then
	ifconfig wlan0 down
	rmmod mt7601Usta
	drv_default=0
fi

if [ "${idVendor}" = "0bda" ] && [ "${idProduct}" = "8179" ] ; then
	ifconfig wlan0 down
	rmmod 8188eu
	drv_default=0
fi

if [ "${idVendor}" = "0bda" ] && [ "${idProduct}" = "f179" ] ; then
	ifconfig wlan0 down
	rmmod 8188fu
	drv_default=0
fi

if [ "${idVendor}" = "1d6b" ] && [ "${idProduct}" = "0002" ] ; then
	ifconfig wlan0 down
	rmmod 8188fu
	drv_default=0
fi

if [ "${drv_default}" = "1" ] ; then
	ifconfig wlan0 down
	rmmod 8188fu
	drv_default=0
fi
