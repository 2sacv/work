#!/bin/sh

# Copyright (C) by Jabsco Company
# Created Time : 2012-10-15
# Version      : 1.0
# Author       : luoshunfa
# Description  :

SYSLOG() { /bin/logger -t jcommc "$@"; echo $@ > /dev/console ;}
MEDIA_ROOT_DIR=/mnt

killall c2tty # 使用 c2tty -f 后，拔卡时 SYSLOG 往 console 打日志会导致卡死，所以 umount 时先 kill c2tty

/bin/netstat -nat | /bin/grep -q 9999
if [ $? == 0 ]; then
    SYSLOG "auto_umount.sh send remove to jco_server"
	/ipc/bin/ccli sdcard -act remove -path $MEDIA_ROOT_DIR
    # don't rm, as a hint of `lzbox fat`
else
    SYSLOG "rmmove sd to avoid CONTINUED error write"
    /bin/umount -f $MEDIA_ROOT_DIR
fi

