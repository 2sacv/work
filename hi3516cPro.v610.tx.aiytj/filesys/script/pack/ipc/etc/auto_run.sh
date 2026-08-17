#!/bin/sh

LOGGER() { echo "$@"; logger "$@"; echo "$@" > /dev/kmsg;}

fn_insmod_modules()
{
    sensor_type=`cat /proc/cmdline | xargs -n1 | awk -F'=' '/sensor/{print $2}'`
    sensor_type=$(echo "$sensor_type" | awk '{print tolower($0)}')
    os_mem_str=`cat /proc/cmdline | xargs -n1 | awk -F'=' '/mem/{print $2}'`
    let os_mem_size=`echo ${os_mem_str} | awk -F "K" '{printf $1}'`
    mmz_mem_size=$((65536-$os_mem_size))
    mmz_mem_str="${mmz_mem_size}K"
    mmz_start_size=$((0x40000000+$os_mem_size*1024))
    mmz_start_str=$(echo "$mmz_start_size" | awk '{printf("%x\n", $1)}')
    mmz_start_str="0x${mmz_start_str}"
    echo "insmod params sensor:${sensor_type} mmz_size:${mmz_mem_str} mmz_start:${mmz_start_str}"
    cd /ipc/drv
    ./load3516cv608_debug -i -sensor ${sensor_type} -mmz_size ${mmz_mem_str} -mmz_start ${mmz_start_str}
    cd -
}

CUSTOM_CONF="/opt/conf/custom_conf"
CONFIG_XML="/opt/conf/config.xml"
fn_xml_getval()
{
    xml_shuttle -cr -k $1 "$CONFIG_XML" 2>/dev/null | awk '{print $3}'
}

fn_xml_check()
{
    set -x
    # 取 custom_appid / release_time 的值
    CUST_APPID=$(awk '/custom_appid/{a=$3} END{print a}' "$CUSTOM_CONF")
    CUST_TIME=$(awk '/release_time/{t=$3} END{print t}' "$CUSTOM_CONF")
    XML_APPID=$(fn_xml_getval /cfg/devinfo/custom_appid)
    XML_TIME=$(fn_xml_getval /cfg/release_time)
    CUST_APPID=${CUST_APPID:-$XML_APPID}
    CUST_TIME=${CUST_TIME:-$XML_TIME}

    if [ "$CUST_APPID" = "$XML_APPID" ] && [ "$CUST_TIME" = "$XML_TIME" ]; then
        set +x
		return;
	fi
    set +x

	/ipc/bin/reset2factory
}

fn_mount_lang()
{
    lang=`cat /proc/cmdline | xargs -n1 | awk -F'=' '/lang/{print $2}'`
    case ${lang:=0} in
    #1) mount --bind /ipc/etc/pcm/01.amr /ipc/etc/pcm/01.amr ;;
    2) mount --bind /ipc/etc/amr_en/ /ipc/etc/amr           ;;
    9) mount --bind /ipc/etc/amr_ru/ /ipc/etc/amr           ;;
    *) ;;
    esac
}

fn_get_vc_custom()
{
    if [ -f "/mnt/get_vc_custom.txt" ] && [ -f "/opt/conf/customcof.add.pair" ]; then
        for f in /opt/custom/login_main.png /opt/custom/logo.png; do
            test -f $f && cp -f $f /mnt/
        done
        cp -f /opt/conf/customcof.add.pair /mnt/sd_custom.txt
        mv /mnt/get_vc_custom.txt /mnt/get_vc_custom.done
        echo "get_vc_custom.done"
    else
        echo "skip get_vc_custom"
    fi
}

fn_do_sd_custom()
{
    # 检查是否存在多个sd_custom开头的文件
    # 多个时，按 ascii 字典排序
    custom_file_count=$(ls -1 /mnt/sd_custom* 2>/dev/null | wc -l)
    if [ ${custom_file_count:=0} -ne 1 ]; then
        echo "custom num: ${custom_file_count}, skip"
        return 1
    fi

    mkdir -p /opt/custom
    sd_custom=/opt/conf/sd_custom.txt

    for custom_file in /mnt/sd_custom*; do
        touch ${sd_custom}
        if ! diff ${custom_file} ${sd_custom}; then
            cp -f $custom_file ${sd_custom}
            /ipc/bin/reset2factory
        fi
        break
    done

    for login_main in /mnt/login_main*; do
        diff -q $login_main /opt/custom/login_main.png || \
        cp -f $login_main /opt/custom/login_main.png
        break
    done

    for logo in /mnt/logo*; do
        diff -q $logo /opt/custom/logo.png || \
        cp -f $logo /opt/custom/logo.png
        break
    done

    sync
}

fn_rm_sdlog()
{
    MNT_ROOT="/mnt"
    PATTERN="????-??????.log"
    MAX_FILES=100
    TO_DELETE=10

    digit_dirs=$(find "$MNT_ROOT" -maxdepth 1 -type d -name "[0-9]*" 2>/dev/null)

    if [ -z "$digit_dirs" ]; then
        echo " $MNT_ROOT have no number dir"
        return 1
    fi

    valid_dirs=""
    for d in $digit_dirs; do
        basename_d=$(basename "$d")
        case $basename_d in
            ''|*[!0-9]*) continue ;;
            *) valid_dirs="$valid_dirs $d" ;;
        esac
    done

    if [ -z "$valid_dirs" ]; then
        echo "no find number dir"
        return 1
    fi

    total_cleaned=0

    for LOG_DIR in $valid_dirs; do
        echo ">>> handle dir: $LOG_DIR"
        
        # 获取匹配的日志文件（使用绝对路径）
        log_files=$(ls "$LOG_DIR"/$PATTERN 2>/dev/null | sort)
        if [ -z "$log_files" ]; then
            echo "no match logfile"
            continue
        fi

        count=$(echo "$log_files" | wc -l)
        echo "find $count logfile"

        if [ $count -le $MAX_FILES ]; then
            echo " ≤ $MAX_FILES，no need clean"
            continue
        fi

        # 删除最早的 TO_DELETE 个
        to_remove=$(echo "$log_files" | head -n $TO_DELETE)
        echo "delete the oldest $TO_DELETE logfile"
        echo "$to_remove" | while IFS= read -r f; do

        full_path="$LOG_DIR/$f"
        echo "delete: $full_path"
        rm -f "$full_path"
        done
    done
    sync
}

fn_scan_tmp_mp4()
{
    if [ -e /mnt/factest.txt ]; then
        return
    fi

    SYS_CID="/sys/block/mmcblk0/device/cid"
    [ ! -e "${SYS_CID}" ] && { echo "no sd card"; return ;} 

    test -f $p_sd/kill_dog.txt || echo "100" > /dev/watchdog
    echo "________ scan after sd card mount ________"
    sd_path="`df -h | awk '/mmcblk/{print $6}'`/IPCamera"
    [ -d "${sd_path}" ] || return
    test -f $p_sd/kill_dog.txt || echo "100" > /dev/watchdog

    cd ${sd_path}
    sd_latest=`/bin/ls -d 20* | tail -1`
    [ -d "${sd_latest}" ] || return

    cd ${sd_latest}

    dirmp4=${PWD}
    rec_list=/var/run/reclist.tmp
    > ${rec_list}

    for f in *.mp4.tmp; do
        test -f "${f}" || { echo "not exsit $f" ; continue ;}
        # Attention: [  echo 00 | sed 's/\<0//g'  ] is empty
        set -- `/bin/ls -le -c $f | awk '{print $9}'| sed 's/:/ /g'`
        e_hh=${1}
        e_mm=${2}
        e_ss=${3}

        hhmmoo=${f##[ASM]-}
        hhmmoo=${hhmmoo%%.mp4.tmp}
        s_hh=${hhmmoo:0:2}
        s_mm=${hhmmoo:2:2}
        s_ss=${hhmmoo:4:2}

        let end=${e_hh#0}*60*60+${e_mm#0}*60+${e_ss#0}
        let sta=${s_hh#0}*60*60+${s_mm#0}*60+${s_ss#0}
        let len=${end}-${sta}

        [ "${len}" -gt 900 ] && len=900
        len=`printf "%04d" ${len}`

        echo "${dirmp4}/$f ${dirmp4}/${f:0:1}-${hhmmoo}-${len}.mp4" >> ${rec_list}
        LOGGER "autorun mv $f ${f:0:1}-${hhmmss}-${len}.mp4"
    done

    if [ -f /opt/etc/rec_blacklist ]; then
        grep -v -f /opt/etc/rec_blacklist ${rec_list} > ${rec_list}.filter
        cp ${rec_list}.filter ${rec_list}
    fi
    
    return $?
}

fn_echo_applog()
{
    time=$(date "+%Y-%m-%d %H:%M:%S")
    echo "${time} |alarm   |        |warning |system  | $1" >> /opt/log/applog
}

fn_del_opt_log()
{
    rm /opt/log/upgrade*
    rm /opt/log/messages*
    rm /opt/log/alarmlog
    fn_echo_applog "Opt file is too large, fixed"
}

fn_opt_detect()
{
    # 800K is 100KB .tgz, config.xml is 40KB
    find /opt -type f -size  +110k | xargs  rm -f '{}'    
    used=`df -h | grep 'opt$' | xargs -n1 |  grep % | awk -F'%' '{print $1}'`
    maxsize=25600
    maxlimit=80
    if [ "$used" -gt "$maxlimit" ] ; then
        find /opt -type f -size  +80k | xargs  rm -f '{}'
        fn_del_opt_log
    fi
    
    if [ -f  /opt/custom/simhei.ttf ]; then
        ttf_size=`ls -l /opt/custom/simhei.ttf | awk '{print $5}'`
        echo "ttf_size = $ttf_size"
        if [ "$ttf_size" -gt "$maxsize" ] || [ "$ttf_size" -lt 0 ] ; then
            rm /opt/custom/simhei.ttf
            echo "simhei.ttf size too big or ttf_size = $ttf_size, rm simhei.ttf"
        fi
    fi

    mkdir -p /opt/log /opt/conf

    for file in /opt/log/alarmlog /opt/log/applog; do
        if [ ! -f "${file}" ]; then
            touch $file
            fn_echo_applog "file $file lost, fixed"
        else
            # head: alarmlog: Input/output error
            head -c1 ${file} || { rm -f ${file}; touch ${file}; sync ;}
        fi
    done
}

fn_setip()
{
    mkdir -p /var/network/
    if grep -q 'root=/dev/nfs' /proc/cmdline; then
        echo "run on nfs, no flash mount!"
        return 0
    else
        if ! ps | grep '[t]elnetd'; then
            telnetd -p9527
            (sleep 300; ps|awk '/[p]9527/ {print $1}' | xargs kill -9) &
        fi
    fi

    touch /var/lib/misc/udhcpd.leases
    if [ -f /opt/conf/config.xml ]; then
        dev_ip=`xml_shuttle -cr -k /cfg/eth/ip /opt/conf/config.xml`
        dev_ip=`echo $dev_ip| awk -F' ' '{print $3}'`
        ifconfig eth0 $dev_ip
    fi

    mkdir -p /var/network/
    ifconfig eth0 mtu 1460

    gw=${dev_ip%.*}.1
    route add default gw $gw dev eth0
}

fn_hwclock()
{
    mkdir -p /var/run
    # hwclock -s >/dev/null 2>&1
    echo 1 > /var/run/hwclock
}

fn_reboot()
{
    # tmp repair post-process
    # repair_record_all mp4 fopen() make SEGV, bad .mp4 may make OOM
    # /tmp/rec_segv OOM make jco_server no chance to DELETE, rec_segv is obsolete
    rec_list='/var/run/reclist.tmp'
    blk_list='/opt/etc/rec_blacklist'
    tmp_list='/tmp/tmp_list'

    touch ${blk_list} ${rec_list}
    cp ${blk_list} ${tmp_list}
    cat ${rec_list} ${tmp_list} | awk '{print $1}' | sort -u |
    while read file; do
        rm -f $file
        test -f $file && echo $file
    done > ${blk_list}

    LOGGER "
    ------------------------
    reboot as jco_server exit
    ------------------------"
    sleep 10

    /ipc/bin/toggle tar
    sync; sleep .5; sync; sleep 1
    killall -9 fsck.exfat
    echo 1 > /sys/class/mmc_host/mmc0/reboot
    reboot -f
}

fn_load_drv()
{
    . /ipc/bin/io.rc
    /ipc/etc/net_drv.sh init
}

fn_del_opt_space()
{
    cd /opt/log
    rm -f alarmlog.tgz applog.tgz

    cd /opt/conf
    find -name 'factory.kep.pair.*' | grep -v -f /ipc/etc/maxheight | xargs rm -f
    rm -f upgrade.add.list upgrade.mod.list
    rm -f ptz.conf reset_flag sd_cid sdstat
    rm -f /opt/custom/vg.amr /opt/custom/motion.amr /opt/custom/eng/motion.amr
    rm -f feature.pair.*
    test -f /ipc/etc/RELEASE && rm -f /opt/conf/RELEASE
    test -f /opt/log/upgrade.log.1 && tar -zcvf /opt/log/upgrade.log.1.tgz /opt/log/upgrade.log.1 && rm /opt/log/upgrade.log.1
    cd -
}

fn_main()
{
    echo "run auto_run.sh"
    sleep 1
    if [ -z '/opt/etc/TZ' ]; then
        export TZ=$(cat /opt/etc/TZ)
    else
        export TZ='CST-08:00'
    fi

    if test -f /opt/conf/reboot.epoch ; then
        reboot_len=`cat /opt/conf/reboot.epoch | wc -c`
        if [ $reboot_len -eq 17 ]; then
            source  /opt/conf/reboot.epoch
        fi
    fi
    #test -s /opt/conf/reboot.epoch && >       /opt/conf/reboot.epoch

    fn_del_opt_space
    fn_opt_detect $@

    ps | grep "[s]yslogd" | awk '{print $1}' | while read PID; do kill -9 $PID; done;
    syslogd -t -s 200 -b 1 -S -O /tmp/messages

    a=$(devmem 0x11020360); b=$(devmem 0x11021f08); c=$(devmem 0x11021f0c)
    boot_reason=$(( a == 0 ? 0 : (c == a - b ? 1 : 2) ))
    echo ${boot_reason} > /tmp/this_boot
    LOGGER "this_boot: ${boot_reason}, 0 powon 1 soft 2 wdt"
    # init mac
    # macmac=`ifconfig eth7 | grep "inet addr" | cut -d : -f 2 | cut -d ' ' -f 1`
    # macmac=`echo $macmac | sed -e 's/^192/8/g' -e 's/\.//g'  | xargs printf "%012d"`
    # echo $macmac | sed 's/../&-/g' | sed 's/-$//g'
    # /sbin/ifconfig eth0 down
    # /sbin/ifconfig eth0 hw ether `/usr/bin/fw_printenv -n ethaddr`

    #Pull down the SDcard pin on w312
    fn_mount_lang

    # init ip when no bootip
    bootip=`ifconfig eth0 | grep "inet addr" | cut -d : -f 2 | cut -d ' ' -f 1`
    [ -z "${bootip}" ] && { ifconfig eth0 192.168.1.217; ifconfig lo up; }

    echo "PATH: ${PATH}"
    echo "LD_LIBRARY_PATH: ${LD_LIBRARY_PATH}"

    echo   5 >/proc/sys/vm/dirty_ratio
    echo   3 >/proc/sys/vm/dirty_background_ratio       # 1.8M
    #cho 300 >/proc/sys/vm/dirty_writeback_centisecs    # 0.9M default 500
    #cho 500 >/proc/sys/vm/dirty_expire_centisecs       # 1.8M default 3000
    echo 1 > /proc/sys/vm/overcommit_memory             # 取消线程创建时对虚拟内存的检测，实际内存是够的

    #增强网络性能
    echo 3 > /sys/class/net/eth0/queues/rx-0/rps_cpus
    echo 32768 > /proc/sys/net/core/rps_sock_flow_entries
    echo 32768 > /sys/class/net/eth0/queues/rx-0/rps_flow_cnt

    # load dr
    LOGGER "autorun: start load drivers"
    touch /tmp/smac0

    /ipc/etc/load && echo load succ || { echo "load fail"; exit 0 ;}
    # mkdir /dev/shm    ##inittab中已经创建，此处无需重复创建

    mkdir /tmp/alisave                                          # 创建alisave用于保存grun数据

    randommin=$(($RANDOM%60))
    initime="2026-04-01 12:${randommin}:00"
    echo "${initime}"
    test -f /opt/conf/reboot.epoch && . /opt/conf/reboot.epoch || date -s "${initime}"                   # 2026-04-01 12:${randommin}:00
    test -e /dev/mmcblk0 && { for i in 1 2 2; do mountpoint /mnt && break; sleep $i; done ;}

    # custom
    fn_do_sd_custom
    fn_get_vc_custom
    fn_xml_check $@
    fn_rm_sdlog

    #/sbin/udevadm trigger
    fn_hwclock                                                  # 不要修改下列3个脚本的顺序
    fn_setip                                                    # 1 ifconfig static-ip
    fn_load_drv                                                 # 2 4G,wifi模组上电加载

    #coredump dynamicly
    ulimit -c unlimited
    echo 1 > /proc/sys/kernel/core_uses_pid;
    echo "|/ipc/bin/zip_piping" > /proc/sys/kernel/core_pattern

    p_sd='/mnt'
    if [ -f $p_sd/kill_dog.txt ]; then
        mkdir -p $p_sd/log; test -f $p_sd/dmesg.txt && mv $p_sd/dmesg.txt $p_sd/log
        cat /proc/kmsg >> $p_sd/dmesg.txt &
    fi

    test -e /opt/etc/local.rc && source /opt/etc/local.rc       # 3 mount depends on fn_setip
    test -f /mnt/up.rc && cp /mnt/up.rc /tmp/up.rc && chmod +x /tmp/up.rc && /tmp/up.rc&

    F_SD_REPORT="/opt/log/sd_report"
    sd_cnt=$(awk -F'count:' '{print $2}' $F_SD_REPORT | awk '{print $1}')
    new_sd_cnt=$((sd_cnt + 1))
    sed -i "s/count:$sd_cnt/count:$new_sd_cnt/" $F_SD_REPORT

    echo 1 > /sys/block/zram0/reset
    echo lz4 > /sys/block/zram0/comp_algorithm              # 压缩方式     
    echo 10M > /sys/block/zram0/mem_limit                   # 物理内存容量
    echo 15M > /sys/block/zram0/disksize                    # 虚拟内存容量 
    test -e /usr/bin/mkswap && /usr/bin/mkswap /dev/zram0
    test -e /usr/bin/swapon && /usr/bin/swapon -p 5 /dev/zram0

    echo "
    +------------------------------------------------+
    Hello Server World @`date +%F.%T`!!
    +------------------------------------------------+
    "
    LOGGER "autorun: start jco_server @${count:-0}"
    /ipc/app/jco_server &
    SVR_PID=$!
    echo "/ipc/app/jco_server & PID:$SVR_PID"
    LOGGER "auto_run: server pid $SVR_PID"
    while kill -0 $SVR_PID; do
        sleep 5;
    done
    ps; lsmod; sleep 4; 
    ps && fn_reboot
    echo 1 > /sys/class/mmc_host/mmc0/reboot
}

fn_insmod_modules $@
fn_main $@
