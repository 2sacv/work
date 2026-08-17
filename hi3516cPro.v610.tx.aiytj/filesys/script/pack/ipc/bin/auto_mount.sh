#!/bin/sh

# Copyright (C) by Jabsco Company
# Created Time : 2012-10-15 2023-07-06 22:29:30
# Version      : 2.0
# Author       : luoshunfa zhangj
# Description  : do parti format and mount

MEDIA_ROOT_DIR="/mnt"
MEDIA_LINK_NAME="/opt/media/mmcblk0p1"
F_P2P_conf="/opt/conf/tencent.conf"
alias  LOGGER=echo
export PATH="/ipc/bin:/sbin:/bin:$PATH"
export LD_LIBRARY_PATH="/ipc/lib:/lib:${LD_LIBRARY_PATH}"

GSTAT()  { /bin/netstat -nat | /bin/grep -q 9999 && gstat $@ ;}
JCPCMD() { LOGGER "do $@"; /bin/netstat -nat | /bin/grep -q 9999 && ccli $@ ;}
xt_ret() { [ "${1}" = "0" ] && return 0; echo "${2}"; return 1 ;}

fn_auto_promote_exfat()
{
    dev_mmcblk=$1
    grep -q 'mmcblk0.*exfat' /proc/mounts && return 0

    # validate FACTORY mode
    ! test -f $F_P2P_conf && echo "skip FACTORY mode" && return 0
    device_id=`cat /proc/cmdline | xargs -n1 | awk -F= '/device_id/{print $2}'`
    test 00000000000 = ${device_id:-'00000000000'} && echo "id FACTORY mode" && return 0
    test -f ${MEDIA_ROOT_DIR}/factest.txt && echo "skip FACTORY mode" && return 0
    ! test -f ${MEDIA_ROOT_DIR}/ssid.txt && echo "no auto promote" && return 1

    # ip.txt ffw.txt ssid.txt factest.txt sd.fire.log etc.
    nr_conf=`ls /mnt/*.log /mnt/*.txt 2>/dev/null | wc -l`

    if [ ! -e '/mnt/ffw.txt' ] && [ ${nr_conf} -gt 0 ]; then
        tar -zcvf /tmp/sdbak.tgz /mnt/*.log /mnt/*.txt
    fi

    # 1024MB = sizeof-120min-1080P-mp4
    if [ -e /tmp/sdbak.tgz ]; then
        #JCPCMD sdcard -act remove -path ${MEDIA_ROOT_DIR}; sleep .5
        umount -l $dev_mmcblk && \
        mkfs.exfat $dev_mmcblk && \
        mount -o noatime,nodiratime $dev_mmcblk ${MEDIA_ROOT_DIR}
        if [ "${?}" -ne 0 ]; then
            LOGGER "Do promote to exfat FAIL" && return 1
        else
            [ -e /tmp/sdbak.tgz ] && { tar -zxf /tmp/sdbak.tgz -C /; rm -f /tmp/sdbak.tgz ;} && echo ${device_id} > ${MEDIA_ROOT_DIR}/.devid
            LOGGER "Do promote to exfat succ" && return 0
        fi

    else
        LOGGER "ignore promote exfat bcz nr_conf:${nr_conf}"
    fi

    return 0
}

fn_safe_echo()
{
    out=${2:-/tmp/out}
    if [ -f "$1" ]; then
        tail -100 $1 >> $out
    else
        echo $1 >> $out
    fi

    tail -c ${KK:-8000} $out > /tmp/.safe_echo ; sync
    mv /tmp/.safe_echo $out  ; sync
}

fn_check_devid()
{
    dev_mmcblk=$1

    # validate FACTORY mode
    ! test -f $F_P2P_conf && echo "skip FACTORY mode" && return 0
    device_id=`cat /proc/cmdline | xargs -n1 | awk -F= '/device_id/{print $2}'`
    test 00000000000 = ${device_id:-'00000000000'} && echo "id FACTORY mode" && return 0
    test -f ${MEDIA_ROOT_DIR}/factest.txt && echo "skip FACTORY mode" && return 0
    test -f ${MEDIA_ROOT_DIR}/ssid.txt && echo "skip FACTORY mode" && return 0

    used_MB=`df -m $dev_mmcblk | awk '/mmcblk/{ printf "%d\n", $3 }'`

    # create devid
    if [ -f /tmp/format ] && [ ! -f ${MEDIA_ROOT_DIR}/.devid ]; then
        format_cid=`cat /tmp/format`
        cid=`cat /sys/block/mmcblk0/device/cid`
        rm -rf /tmp/format
        [ "$format_cid" = "$cid" ] && echo "devid:${device_id} cid:${cid}" > ${MEDIA_ROOT_DIR}/.devid && sync &&return 0
    fi

    # check devid
    if [ -f ${MEDIA_ROOT_DIR}/.devid ]; then
        echo ".devid is valid" && return 0
        #sd_devid=`cat ${MEDIA_ROOT_DIR}/.devid`
        #[ "${device_id}" = "${sd_devid:-00000000000}" ] && return 0
    fi

    return 1
}

# fsck.fat is easy malloc() failure
# fsck.fat -a $1 # malloc() large memory, jz_codec() allocate failure
fn_mount2()
{
    test -e /dev/mmcblk0; xt_ret $? "chk.2A" || exit $?
    content=$(timeout 10s head -c16 "$1" 2>/dev/null)
    # Set is_exfat to true if: command timed out, OR content is empty, OR content contains "EXFAT" 
    [ $? -eq 124 ] || [ -z "$content" ] || echo "$content" | grep -q -i EXFAT && is_exfat=true
    if ${is_exfat:=false}; then
         ii=0
         LOGGER "do fsck.exfat until 5 or SUCC"
         while ! fsck.exfat -a $1 >& /tmp/fsck.log; do
             fn_safe_echo /tmp/fsck.log /opt/log/fsck.log
             tail -20 /tmp/fsck.log   # 上电时候出文本一万行左右会导致设备重启
             LOGGER "do fsck.exfat ERROR"
             files=`awk -F\' '/ERROR.*file/{print $2}' /tmp/fsck.log  | sort -u`
             [ -z "${files}" ] && break

             if mount -o noatime,nodiratime $1 ${MEDIA_ROOT_DIR}; then
                 local ff=
                 for ff in $files; do
                    find /mnt/ -name $ff | xargs rm -f
                 done
                 sync; sleep .5; sync; sleep .5;
                 umount -l /mnt
             else
                 LOGGER "do repair mount ERROR"; break
             fi
             let ii++
             [ "${ii}" -gt 5 ] && break
         done
         rm -f /tmp/fsck.log
    fi

    if mount -o noatime,nodiratime $1 ${MEDIA_ROOT_DIR}; then
        if ${is_exfat}; then
            if fn_check_devid $1 || [ -d ${MEDIA_ROOT_DIR}/IPCamera ]; then
                JCPCMD sdcard -act add -path ${MEDIA_ROOT_DIR}
            else
                GSTAT record 2 on
            fi
            return 0
        else
            LOGGER "Try promote vfat to exfat"
            if fn_auto_promote_exfat $1; then
                JCPCMD sdcard -act add -path ${MEDIA_ROOT_DIR}; return 0
            else
                GSTAT record 2 on # 非 exfat 或者非工厂环境 发送未认证错误
                return 0
            fi

        fi
    else
        LOGGER "mount FAILURE"
        if ! ${is_exfat:=false}; then
            GSTAT record 2 on
        else
            GSTAT record 4 on
        fi
        return 1
    fi

    # some FORMAT are not fat _but_ Dos
    head -c16 $1 | grep -q -i fat || ${is_exfat:-false} || return 2
}

fn_mount3()
{
    cat /proc/uptime
    LOGGER "___ partition and format not JCO style mmc [$1 $2] ___"

    # del without HEAD boottag & TAIL 55AA
    test -e /dev/mmcblk0; xt_ret $? "chk.3A" || return $?
    mountpoint /mnt && {
        fuser -m /mnt | xargs -n1 lsof -p | grep /mnt
        fuser -m /mnt -k; sleep .5; umount -f /mnt ;
    }
    dd bs=1 seek=2 count=508 if=/dev/zero of=/dev/mmcblk0

    # exist but input/output error, may be 512KB detect only
    if [ "${?}" -ne 0 ]; then
        LOGGER "hot plug-in or RO error"
        touch /tmp/fmterror.txt
        sync
        return 1
    fi

    {
        sleep .1; echo w
    } | fdisk /dev/mmcblk0
    sleep .3

    # new
    test -e /dev/mmcblk0; xt_ret $? "chk.3B" || return $?
    {
    echo n
    sleep .2; echo p
    sleep .2; echo 1
    sleep .2; echo                # default start
    sleep .2; echo       
    sleep .2; echo t
    sleep .2; echo 7              # default end
    sleep .2; echo w
    sleep .2
    } | fdisk /dev/mmcblk0
    # No need to mount, hotplug event will come out after fdisk

    let i=0
    while [ $i -lt 10 ]; do
        let i++
        sleep .2
        LOGGER "waiting $i"
        ls /dev/mmcblk0p*
        test -e /dev/mmcblk0p1 && mkfs.exfat /dev/mmcblk0p1 && cat /sys/block/mmcblk0/device/cid > /tmp/format && return
    done

    LOGGER "/dev/mmcblk0p1 not exist or mkfs FAILURE"
}

# no_autommc: mount /dev/mmcblk0p1 /mnt; JCPCMD sdcard -act add -path /mnt
fn_precheck()
{
    [ "${1}" = fmt ] && { fn_mount3 mmcblk0 0; exit 1 ;}
    [ "${1}" = devid ] && { fn_check_devid $2 || JCPCMD gstat record 2 on; exit 1 ;}
    [ "${1}" = mmcblk0 ] && { LOGGER "fn_precheck %k[$1] %n[$2] not parter no need idle"; exit 0 ;}
    test -f /tmp/no_autommc && LOGGER "no_autommc $@" && { . /tmp/no_autommc; exit 0 ;}
    LOGGER "fn_precheck %k[$1] %n[$2] `cat /sys/block/mmcblk0/device/cid /proc/uptime`"

    ! test -d $MEDIA_ROOT_DIR && mkdir -p $MEDIA_ROOT_DIR
}

fn_exfat_load()
{
    if test -f /ipc/drv/exfat.ko; then
        for drv in exfat; do
            if grep $drv /proc/modules; then
                LOGGER "_______ rmmod $drv ________"
                rmmod $drv && continue
                lsmod; LOGGER "_______ rmmod $drv fail!!!!! ________"
            fi
        done
        sleep 1
        #check
        for drv in exfat; do
            if grep $drv /proc/modules; then
                LOGGER "_______ check rmmod $drv fail!!!!! try again ________"
                rmmod $drv || return 1 ;
            fi
        done

        insmod /ipc/drv/exfat.ko || \
        { LOGGER "_______ insmod fat vfat exfat fail!!! ________"; return 1 ;}
    fi

}

fn_main()
{
    set -x
    fn_precheck $@
    fn_exfat_load

    fn_mount2 /dev/$1
    case $? in
    0)  return 0 ;;
    1)  LOGGER "try remount"; fn_mount2 /dev/$1 && return 0 ;;
    *)  LOGGER "try format" ;;
    esac

    # mount2 fail, to format
    # fn_mount3 $@
}

fn_main $@
