#! /bin/sh
#---------------------------------------------------------------------------
#          FILE: updateExt.sh
#         USAGE: ./updateExt.sh
#   DESCRIPTION: 升级分两步进行
#                1  解压升级包，并nandwrite squshfs
#                2  find + mv ，替换 web conf 等文件
#                   a) 进程对文件的引用会导致文件替换失败
#                   b) 直接rm -rf 会概率性(1/700)导致文件夹及文件的直接丢失
#        AUTHOR: zhangjian ()
#  ORGANIZATION:
#       CREATED: 2014-04-08 10:18:40 AM
#      REVISION: 1.0
#---------------------------------------------------------------------------

xt_ret() { [ "${1}" = "0" ] && return 0; echo "${2}"; return 1 ;}

fn_kill_complete()
{
	while :; do
		procId=`ps |grep "$1"|grep -v grep|awk '{print $1}'`

		if [ "${procId}" = "" ]; then
			echo "${1} exit complete"
			break
		else
			echo "${1}:${procId} is exist, kill it."
			kill -9 ${procId}
			killall ${1}
			sleep 1
		fi

	done
}

fn_kill_jco_server()
{
	while : ; do
		procId=`ps |grep "jco_server"|grep -v grep|awk '{print $1}'`

		if [ "${procId}" = "" ]; then
			echo "jco_server exit complete"
			break
		else
			echo "jco_server is exist, kill it."
			kill -9 ${procId}
			killall jco_server
			sleep 1
		fi

	done
}

fn_prog_bar_start()
{
    total_secs=25				# seconds spent by C + BASH => takes_secs

    start=`date +%s`
    xtar_secs=${1:-7}           # seconds spent by C
    let start=start-xtar_secs
    sync

    # speed of I/O write() is .33M/s, after ENCODE exit 21M/14 -> 1.5M/s
    # flush when dirty upto 1.2M or .6M, freeTotal is 60M
    set +x
    echo 3 > /proc/sys/vm/drop_caches
    echo   4 >/proc/sys/vm/dirty_ratio
    echo   2 >/proc/sys/vm/dirty_background_ratio       # 1.2M
    echo  50 >/proc/sys/vm/dirty_writeback_centisecs    # 0.7M
    echo 500 >/proc/sys/vm/dirty_expire_centisecs       # 1.8M
    set -x
}

fn_prog_bar_set()
{
    # total_secs = xtar_secs + updateExt.sh
    curr=`date +%s`
    let 'prog_bar=100*(curr-start)/total_secs'

    [ "$prog_bar" -ge 100 ] && prog_bar=99

    echo "----- ${prog_bar}% $1 ----"
    ccli update -act set -progressbar ${prog_bar}
}

fn_prog_bar_succ()
{
    curr=`date +%s`
    prog_bar=100
    echo "----- ${prog_bar} $1 ----"
    ccli update -act set -progressbar ${prog_bar}

    let takes_secs=curr-start
    echo "---- it takes $takes_secs on upgrade ----"
}

fn_prog_bar_fail()
{
    prog_bar=111
    echo "----- ${prog_bar} $1 ----"
}

fn_do_md5()
{
    [ -f "$package" ]
    xt_ret $? "upgrade file[${package}] in needed" || return $?

    md5_pack=`tail -c32 $package`
    [ -n "${md5_pack}" ]
    xt_ret $? "md5_pack" || return $?

    sed -i '$s/[0-9a-z]\{32\}$//' $package
    xt_ret $? "chop $package" || return $?

    md5_here=`md5sum $package | awk '{print $1}'`

    [ "${md5_here}" = "${md5_pack}" ]
    xt_ret $? "(here vs pack) ${md5_here} != ${md5_pack}" || return $?
    return 0
}

fn_check_pkg()
{
    # do dev_type check after tar
    dev_type_pkg=$(echo `cat $xtardir/dev_type`)
    dev_type_flash=$(echo `cat /ipc/etc/dev_type`)
    sensor_type=`cat /proc/cmdline| xargs -n1 | grep sensor= | awk -F'=' '{print $2}' | tr '[a-z]' '[A-Z]'`
    available_sensor=`cat $xtardir/available_sensor | grep $sensor_type`
    cat $xtardir/available_sensor

    # 防止主控不匹配做升级
    CPUTYPE=`cat /proc/cmdline| xargs -n1 | grep cpu= | awk -F'=' '{print $2}'`
    cpu_type_pkg=`cat $xtardir/dev_type | awk -F '_' '{print $1}'`

    # 将两个变量都转换为小写后再比较
    if [ "$(echo "$CPUTYPE" | tr '[:upper:]' '[:lower:]')" != "$(echo "$cpu_type_pkg" | tr '[:upper:]' '[:lower:]')" ]; then
        xt_ret 1 "cpu type not match:${CPUTYPE}, ${cpu_type_pkg}" || return $?
    fi

    # 防止SENSOR不匹配做升级
    if [ "$sensor_type" != "$available_sensor" ]; then
        xt_ret 1 "unknow sensor_type:${sensor_type}, ${available_sensor}" || return $?
    fi

    # 防止跨force包升级
    echo dev_type from .tgz: ${dev_type_pkg} flash: ${dev_type_flash}
    if [ "${dev_type_flash:0:6}" != "${dev_type_pkg:0:6}" ]; then
        xt_ret 1 "Error: find error dev_type" || return $?
    fi

    if echo $dev_type_pkg | grep -q _force; then
        echo "force upgrade from flash:${dev_type_flash}"
        rm -rf /opt/conf/config.xml
        return 0
    fi

    # 防止产品项目不匹配升级
    if [ "${dev_type_pkg}" != "${dev_type_flash}" ]; then
        xt_ret 1 "unknow dev_type:${dev_type_pkg}, flash:${dev_type_flash}" || return $?
    fi

    # 防止非JCO包升级
    DEVINFO=`cat /proc/cmdline| xargs -n1 | grep devinfo= | awk -F'=' '{print $2}'`
    DEVINFO=${DEVINFO:0:4}
    if [ "jcox" != "${DEVINFO}" ]; then
        xt_ret 1 "unknow devinfo:${DEVINFO}" || return $?
    fi

    return 0

}

fn_up_img()
{
    #set -x
    # upgrade flash
    let skip_size-=64+$img_size
    #echo "bs=1 if=$src_file of=img skip=$skip_size count=$img_size"
    #dd bs=1 if=$src_file of=$upgradetgzdir/img skip=$skip_size count=$img_size > /dev/null 2>&1

    let tail_size=64+$img_size
    echo " ${img_type} $src_file $img_size"
    tail -c ${tail_size} $src_file  > $upgradetgzdir/img
    truncate -s $img_size $upgradetgzdir/img
    ls -l $upgradetgzdir/img

    xt_ret $? "Fail: dd uptools.tgz" || return $?
    truncate -s $skip_size $src_file

    xt_ret $? "Fail: dd uptools.tgz" || return $?
    sync

    nwrite=0
    while [ $nwrite -lt 3 ]; do
        echo "-------- do ${img_type} ---------"
        {
            #mtd_debug erase /dev/${img_type} 0 0x`grep ${img_type}: /proc/mtd | awk '{print $2}'` && \
            #mtd_debug write /dev/${img_type} 0 ${img_size} img

            #flash_eraseall /dev/${img_type}
            #echo "flash erase ${img_type} complete!"
            #flashcp img /dev/${img_type}
            #sync
            #sleep 10
            #echo "Write ${img_type} complete!"
            if [ ${img_type} == "mtd0" ]; then
                md5_dbg=`mtd_md5sum read /dev/${img_type} 0 ${img_size} occupy | awk '{print $1}'`
                if [ "${md5_dbg}" = ${img_md5} ]; then
                    echo "
                    ___ Latest $img_type u-boot, skip ___
                    "; return
                fi
				mtd_debug erase /dev/${img_type} 0 0x`grep ${img_type}: /proc/mtd | awk '{print $2}'` && \
				mtd_debug write /dev/${img_type} 0 ${img_size} img
            fi

            if [ ${img_type} == "mtd2" ]; then
                md5_dbg=`mtd_md5sum read /dev/${img_type} 0 ${img_size} occupy | awk '{print $1}'`
                if [ "${md5_dbg}" = ${img_md5} ]; then
                    echo "
                    ___ Latest $img_type uImage, skip ___
                    "; return
                fi
				mtd_debug erase /dev/${img_type} 0 0x`grep ${img_type}: /proc/mtd | awk '{print $2}'` && \
				mtd_debug write /dev/${img_type} 0 ${img_size} img
            fi

            if [ ${img_type} == "mtd3" ]; then
                md5_dbg=`mtd_md5sum read /dev/${img_type} 0 ${img_size} occupy | awk '{print $1}'`
                if [ "${md5_dbg}" = ${img_md5} ]; then
                    echo "
                    ___ Latest $img_type rootfs.sqfs, skip ___
                    "; return
                fi
				mtd_debug erase /dev/${img_type} 0 0x`grep ${img_type}: /proc/mtd | awk '{print $2}'` && \
				mtd_debug write /dev/${img_type} 0 ${img_size} img
            fi

            if [ ${img_type} == "mtd4" ]; then
                md5_dbg=`mtd_md5sum read /dev/${img_type} 0 ${img_size} occupy | awk '{print $1}'`
                if [ "${md5_dbg}" = ${img_md5} ]; then
                    echo "
                    ___ Latest $img_type ipcfs.sqfs, skip ___
                    "; return
                fi
				mtd_debug erase /dev/${img_type} 0 0x`grep ${img_type}: /proc/mtd | awk '{print $2}'` && \
				mtd_debug write /dev/${img_type} 0 ${img_size} img
            fi

            if [ ${img_type} == "mtd5" ]; then
                md5_dbg=`mtd_md5sum read /dev/${img_type} 0 ${img_size} occupy | awk '{print $1}'`
                if [ "${md5_dbg}" = ${img_md5} ]; then
                    echo "
                    ___ Latest $img_type algo.sqfs, skip ___
                    "; return
                fi
				mtd_debug erase /dev/${img_type} 0 0x`grep ${img_type}: /proc/mtd | awk '{print $2}'` && \
				mtd_debug write /dev/${img_type} 0 ${img_size} img
            fi
        } &

        bg_pid=$!
        while :; do
            kill -0 ${bg_pid} 2>/dev/null || break
            sleep 2
        done;

        sync
        #md5_dbg=`mtd_md5sum read /dev/${img_type} 0 ${img_size} occupy | awk '{print $1}'`
		nwrite=0
        echo "-------- do ${img_type} succ-----"
        rm -f img; break

        if [ "${md5_dbg}" = ${img_md5} ]; then
            nwrite=0
            echo "-------- do ${img_type} succ-----"
            rm -f img; break
        else
            echo "dbg vs org -> ${md5_dbg} != ${img_md5}"
            let nwrite++
        fi
    done
}

fn_up_single_files()
{
    #
    # Attention: sub-directory etc/ppp conf/isp,
    # for mv overwrite, they must before their papa,
    # EVEV try rm -rf etc/ppp conf/isp, but __FAIL__ sometimes, so mv is more safe
    #

    # exclude: web log meida
    pathes='conf'

    local i=
    for i in ${pathes}; do
        echo "up ${i}"
        mkdir -p /opt/${i}
        find ${i} -maxdepth 1 -type f -exec mv -f {} /opt/${i}/ \;
        find ${i} -maxdepth 1 -type l -exec cp -a {} /opt/${i}/ \;   # mv -f failed
        xt_ret $? "mv" || return $?
        sync
    done

    mkdir -p /opt/custom
    pathes=`find custom -maxdepth 1 -type f`
    for i in ${pathes}; do
        file_name=${i##*/}
		if [ -f "/opt/conf/customfile.add.list" ] ; then
			file_exit=$(echo `grep $file_name /opt/conf/customfile.add.list`)
		else
			file_exit=""
		fi

        if [ -n "$file_exit" ] ; then
            continue
        fi
        echo "up ${i}"

        mv -f $i /opt/custom/
        if [ -f $i ] ; then
            cp -a $i /opt/custom/
        fi
        sync
    done

    pathes='etc'
    local i=
    for i in ${pathes}; do
        #
        # once sd_custom.txt, GEN upgrade skip copy
        #
        if [ $i = 'custom/logo.png' ] || [ $i = 'custom/login_main.png' ]; then
            [ -f /opt/conf/sd_custom.txt ] && continue
        fi

        echo "up ${i}"
        mkdir -p /opt/${i}
        find ${i} -maxdepth 1 -type f -exec mv -f {} /opt/${i}/ \;
        find ${i} -maxdepth 1 -type l -exec cp -a {} /opt/${i}/ \;   # mv -f failed
        xt_ret $? "mv" || return $?
        sync
    done

    return 0
}

fn_unpack_img()
{
	set +x
	let skip_size-=64
	dd bs=1 if=$src_file of=hdr_type skip=$skip_size count=16 > /dev/null 2>&1
	let skip_size+=16
	dd bs=1 if=$src_file of=hdr_size skip=$skip_size count=8 > /dev/null 2>&1
	let skip_size+=8
	dd bs=1 if=$src_file of=hdr_resv skip=$skip_size count=8 > /dev/null 2>&1
	let skip_size+=8
	dd bs=1 if=$src_file of=hdr_md5 skip=$skip_size count=32 > /dev/null 2>&1
	let skip_size+=32

	img_type=$(echo `cat hdr_type`)
	img_size=$(echo `cat hdr_size`)
	img_md5=$(echo `cat hdr_md5`)
	echo "get header info: $img_type $img_size $img_md5"
}

fn_unpack_opt()
{
	set +x
	retryopt=10
	while [ $retryopt -ge 0 ]; do
		fn_unpack_img
		if [ $img_size -lt 1 ]; then
			break
		elif [ "$img_type" == "opt.tgz" ]; then
			let skip_size-=64+$img_size
			dd bs=1 if=$src_file of=opt.tgz skip=$skip_size count=$img_size > /dev/null 2>&1
			xt_ret $? "Fail: dd opt.tgz" || return $?

			rm -rf $xtardir/*
            #cp -rf opt.tgz /opt
			tar -xvf opt.tgz -C ${xtardir}
		    xt_ret $? "untar opt.tgz fail" || return $?
		    rm -f opt.tgz
		    sync
		    fn_prog_bar_set __1
			break
		fi
		let skip_size-=64+$img_size
        let retryopt--
	done
}

fn_unpack_rest()
{
	cd $upgradetgzdir
	retryrest=11

	while [ $retryrest -ge 0 ]; do
		fn_unpack_img
		if [ $img_size -lt 1 ]; then
			break
		elif [ "$img_type" == "truncate" ]; then
			let skip_size-=64+$img_size
			dd bs=1 if=$src_file of=truncate skip=$skip_size count=$img_size > /dev/null 2>&1
			xt_ret $? "Fail: dd truncate" || return $?

			mkdir -p $upgradetgzdir/bin
			export PATH="$upgradetgzdir/bin:$PATH"
		    echo PATH:${PATH}
			chmod +x truncate
			mv truncate $upgradetgzdir/bin
			truncate -s $skip_size $src_file
			xt_ret $? "Fail: truncate off truncate" || return $?
		elif [ "$img_type" == "uptools.tgz" ]; then
			let skip_size-=64+$img_size
			dd bs=1 if=$src_file of=uptools.tgz skip=$skip_size count=$img_size > /dev/null 2>&1
			xt_ret $? "Fail: dd uptools.tgz" || return $?

			tar -xvf uptools.tgz -C ${xtardir}
		    xt_ret $? "untar uptools.tgz fail" || return $?
		    rm -f uptools.tgz
		    truncate -s $skip_size $src_file
		    sync
		    #fn_prog_bar_set __2

		    export PATH="$xtardir/upTools:$PATH"
		    echo PATH:${PATH}
		    which nc mv mtd_debug mtd_md5sum
			chmod +x $xtardir/upTools/feedwdt
		    chmod +x $xtardir/upTools/mtd_debug
            chmod +x $xtardir/upTools/mtd_md5sum

            #升级同时定制
            if test -f $xtardir/up_custom/upc.rc; then
                sh -x $xtardir/up_custom/upc.rc  $xtardir/up_custom
            fi

            rm -rf /tmp/applog
		elif [ "$img_type" == "mtd0" ]; then
			fn_up_img
			xt_ret $? "Fail: fn_up_img $img_type" || return $?
		elif [ "$img_type" == "mtd2" ]; then
			fn_up_img
			xt_ret $? "Fail: fn_up_img $img_type" || return $?
		elif [ "$img_type" == "mtd3" ]; then
			fn_up_img
			xt_ret $? "Fail: fn_up_img $img_type" || return $?
		elif [ "$img_type" == "mtd4" ]; then
			fn_up_img
			xt_ret $? "Fail: fn_up_img $img_type" || return $?
		elif [ "$img_type" == "mtd5" ]; then
		    fn_up_img
		    xt_ret $? "Fail: fn_up_img $img_type" || return $?
		elif [ "$img_type" == "opt.tgz" ]; then
			break
		fi
		let retryrest--
	done

	    touch /opt/conf/upgrade_win

	# watchdog, background program will let tee run forever
    killall feedwdt
    sleep 1
    set -x
}

fn_check_xml()
{
    if [ ! -f /opt/conf/config.xml ] ; then
        reset2factory
    fi
}

fn_upgrade0()
{
    fn_prog_bar_set __3

	cd $upgradetgzdir
	src_file=$package
	tgz_size=`ls -l $src_file | awk '{print $ 5}'`

	# process opt.tgz
	let skip_size=tgz_size
	fn_unpack_opt

    echo "UPGRADE updateExt.sh exec _end_@`date`"
	echo "sleep 5s to keep the 100% status for webpage";
	fn_prog_bar_succ __1; sleep 4; ps

	# watchdog
	feedwdt &

	# kill auto_run.sh jco_server
	fn_kill_complete auto_run.sh
	fn_kill_jco_server
    if test -f /ipc/bin/wifi; then
        killall wpa_supplicant
        killall hostapd
    fi
	killall udhcpc
	killall udhcpd
	killall do_upgrade_listenning

	set +x
    cd $xtardir
	echo "UPGRADE updateExt.sh mv begin@`date`"

	fn_check_pkg
	xt_ret $? "Fail: check package" || return $?

    fn_up_single_files
    xt_ret $? "Fail: mv files" || return $?

    cat /ipc/etc/RELEASE
    fn_banner_succ

    echo "UPGRADE updateExt.sh mv _end_@`date`"
    set -x
    sync

    fn_check_xml
    cd /opt/conf
    xml_shuttle -i -c a -f upgrade.add.pair config.xml
    xt_ret $? "config" || return $?
    xml_shuttle -i -c w -f upgrade.mod.pair config.xml
    xt_ret $? "config" || return $?
    sync

	# process other part
	let skip_size=tgz_size
	fn_unpack_rest

    dev_type_name=$(echo `cat $xtardir/dev_type`)
    if echo $dev_type_name | grep -q _force; then
        echo "force upgrade rm config.xml:${dev_type_flash}"
        rm -rf /opt/conf/config.xml
    fi

    rm -f $package
    sync
}

fn_banner_succ()
{
    echo "
    +-----------------------------------------------------------+
    |                                                           |
    |              UPGRADE SUCCESSFUL, ENJOY IT!                |
    |                                                           |
    +-----------------------------------------------------------+
    "
}

fn_banner_fail()
{
    echo "
    +-----------------------------------------------------------+
    |                                                           |
    |              UPGRADE __FAILURE__, CHECK IT.               |
    |                                                           |
    +-----------------------------------------------------------+
    "
}

fn_reboot()
{
    logger "Rebooting from updateExt.sh..."
    #mv /tmp/messages /opt/log
    sync

    epoch=`date +%s`
    echo "date @$(($epoch+16))" > /opt/conf/reboot.epoch
    sync; sleep 1; sync; sleep 2

    # echo 1 > /dev/watchdog;
    #sleep 1
    #echo 1 > /proc/jz/watchdog/cmd
    reboot
}

unload_module()
{
    echo "unload_module!"
	du -sh /tmp/; free
	fn_prog_bar_set __4
}

# $1 is the fullpath of upgrade package
fn_upgrade()
{
    ccli update -act set -type 1

    package=$1
    PATH=$xtardir/upTools:$PATH

    fn_prog_bar_start
    unload_module

    echo "UPGRADE updateExt.sh exec begin@`date`"

    fn_upgrade0 || {
        fn_prog_bar_fail __1; fn_banner_fail;
        rm -rf ${xtardir}; killall feedwdt; return 1;
    }

    return 0
}

fn_main()
{
    mount -t jffs2 -o remount,rw /dev/mtdblock6 /opt
    # 校验/opt/log大小
    used=`df -h | grep opt | xargs -n1 |  grep % | awk -F'%' '{print $1}'`
    maxlimit=75
    if [ "$used" -gt "$maxlimit" ] ; then
        rm /opt/log/upgrade*
        rm /opt/log/messages*
        rm /opt/log/alarmlog
        rm /opt/log/kmsg.tgz
        rm /opt/log/msg0.tgz

        time=$(date "+%Y-%m-%d %H:%M:%S")
        echo "${time} |alarm   |        |warning |system  | Opt file is too large, upgrade" >> /opt/log/applog
    fi

    up_log=/opt/log/upgrade.log
	xtardir=/tmp/upgrade/upgdir
    upgradetgzdir=/tmp/upgrade

    mkdir -p ${up_log%/*}
    mkdir -p $xtardir
    cd ${xtardir}

    set -x
    cp ${up_log} ${up_log}.1
    grep UPGRADE ${up_log}.1 | grep -v echo | tail -200 > ${up_log}
    fn_upgrade $@ 2>&1 | tee -a ${up_log}
    killall feedwdt;
    fn_reboot&
}

fn_main $@
