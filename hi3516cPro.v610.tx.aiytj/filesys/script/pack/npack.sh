#! /bin/bash

. rules
. rc.d/common.rc

CWD=${PWD}
STRIP=arm-v01c02-linux-musleabi-strip

function fn_agt_CMD()
{
    ${xparser} -i -c w -k /cfg/followcfg/zoom  -v ${ZOOM}  $xmlfile
    # ${xparser} -i -c w -k /cfg/faceae/enable   -v ${F4AE}  $xmlfile
    echo '1 /cfg/followcfg/zoom' >> $upgrad_mod
    # echo '1 /cfg/faceae/enable' >> $upgrad_mod
}

fn_agt_HBGK()
{
    fn_echo_succ "customize hanbang"

    echo 'devname & devtype'
    ${xparser} -i -c w -k /cfg/platforms/hbservicecfg/serverip  -v 192.168.0.101   $xmlfile && \
    ${xparser} -i -c w -k /cfg/devinfo/devname  -v 高清红外网络摄像机    $xmlfile && \
    ${xparser} -i -c w -k /cfg/audioIn/enable  -v 1                      $xmlfile && \
    ${xparser} -i -c w -k /cfg/audioIn/volumn   -v 85                  $xmlfile && \
    ${xparser} -i -c w -k /cfg/osdinfo/name    -v HBGK                   $xmlfile && \
    ${xparser} -i -c w -k /cfg/osdinfo/nameleft -v 0                     $xmlfile && \
    ${xparser} -i -c w -k /cfg/osdinfo/nametop -v 0                      $xmlfile && \
    ${xparser} -i -c w -k /cfg/osdinfo/timeleft -v 1920                  $xmlfile && \
    ${xparser} -i -c w -k /cfg/osdinfo/timetop -v 0                      $xmlfile && \
    ${xparser} -i -c w -k /cfg/osdinfo/bpsen    -v 0                     $xmlfile
    xt_ret $? "" || return $?

    # 开启人形，关闭人形框，联动报警声音
    ${xparser} -i -c w -k /cfg/humanDetect/enable           -v 1        $xmlfile && \
    ${xparser} -i -c w -k /cfg/humanDetect/screenenable     -v 0        $xmlfile && \
    ${xparser} -i -c w -k /cfg/humanDetect/mbdesc           -v 1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,  $xmlfile && \
    ${xparser} -i -c w -k /cfg/humanDetect/times            -v 0:2164260863,1:2164260863,2:2164260863,3:2164260863,4:2164260863,5:2164260863,6:2164260863,              $xmlfile && \
    ${xparser} -i -c w -k /cfg/humanDetLink/sound           -v 1        $xmlfile && \
    #${xparser} -i -c w -k /cfg/humanDetLink/soundsel        -v 2        $xmlfile
    xt_ret $? "" || return $?

    # 所有定制项，都要从 factory 删除，以使恢复出厂时，可以生效
    sed -i '/cfg.audioIn.enable/d'              ${factkep} && \
    sed -i '/cfg.devinfo.devname/d'             ${factkep} && \
    sed -i '/cfg.osdinfo.nametop/d'             ${factkep} && \
    sed -i '/cfg.osdinfo.nameleft/d'            ${factkep} && \
    sed -i '/cfg.osdinfo.name/d'                ${factkep} && \
    sed -i '/cfg.osdinfo.timeleft/d'            ${factkep} && \
    sed -i '/cfg.osdinfo.timetop/d'             ${factkep} && \
    sed -i '/cfg.osdinfo.bpsen/d'               ${factkep} && \
    sed -i '/cfg.humanDetect.enable/d'          ${factkep} && \
    sed -i '/cfg.humanDetect.screenenable/d'    ${factkep} && \
    sed -i '/cfg.humanDetect.mbdesc/d'          ${factkep} && \
    sed -i '/cfg.humanDetect.times/d'           ${factkep} && \
    sed -i '/cfg.humanDetLink.sound/d'          ${factkep} && \
    #sed -i '/cfg.humanDetLink.soundsel/d'       ${factkep}
    xt_ret $? "" || return $?

    # devvecfg above 1080P

    fn_opt_ip 192.168.0.100
    xt_ret $? "" || return $?
    #fn_opt_user_passwd admin 888888
    xt_ret $? "" || return $?

    ui_ver="HBGK"

    ${xparser} -i -c w -k /cfg/authmode/mode -v 0  $xmlfile
    xt_ret $? "" || return $?
}

fn_agt_YJS()
{
    fn_echo_succ "customize yujunshi"
    [ "${NIC}" == "4g" ] && s_product=${P_TYPE}

    # 开启人形，开启人形框，联动报警声音
    ${xparser} -i -c w -k /cfg/humanDetect/enable           -v 1        $xmlfile && \
    ${xparser} -i -c w -k /cfg/humanDetect/screenenable     -v 1        $xmlfile && \
    ${xparser} -i -c w -k /cfg/motorcfg/waittime            -v 30       $xmlfile && \
    ${xparser} -i -c w -k /cfg/motorcfg/seqtimes            -v 1000000  $xmlfile && \

    ${xparser} -i -c w -k /cfg/humanDetect/mbdesc           -v 1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,1111111111111111111111,  $xmlfile && \
    ${xparser} -i -c w -k /cfg/humanDetect/times            -v 0:2164260863,1:2164260863,2:2164260863,3:2164260863,4:2164260863,5:2164260863,6:2164260863,              $xmlfile && \
    ${xparser} -i -c w -k /cfg/humanDetLink/sound           -v 1        $xmlfile
    xt_ret $? "" || return $?
    fn_heredoc "
	1 /cfg/devinfo/devname 高清无线网络摄像机
	1 /cfg/devinfo/devtype ${s_product}
	1 /cfg/devinfo/custom_ui ${AGENT}
	" > ${custom_add}
    # 所有定制项，都要从 factory 删除，以使恢复出厂时，可以生效
    sed -i '/cfg.audioIn.enable/d'              ${factkep} && \
    sed -i '/cfg.devinfo.devname/d'             ${factkep} && \
    sed -i '/cfg.osdinfo.nametop/d'             ${factkep} && \
    sed -i '/cfg.osdinfo.nameleft/d'            ${factkep} && \
    sed -i '/cfg.osdinfo.name/d'                ${factkep} && \
    sed -i '/cfg.osdinfo.timeleft/d'            ${factkep} && \
    sed -i '/cfg.osdinfo.timetop/d'             ${factkep} && \
    sed -i '/cfg.osdinfo.bpsen/d'               ${factkep} && \
    sed -i '/cfg.humanDetect.enable/d'          ${factkep} && \
    sed -i '/cfg.humanDetect.screenenable/d'    ${factkep} && \
    sed -i '/cfg.humanDetect.mbdesc/d'          ${factkep} && \
    sed -i '/cfg.humanDetect.times/d'           ${factkep} && \
    sed -i '/cfg.humanDetLink.sound/d'          ${factkep} && \
    #sed -i '/cfg.humanDetLink.soundsel/d'       ${factkep}
    xt_ret $? "" || return $?

    # devvecfg above 1080P

    fn_opt_ip 192.168.0.100
    xt_ret $? "" || return $?
    #fn_opt_user_passwd admin 888888
    xt_ret $? "" || return $?

    ui_post=""
    ui_ver="YJS"

    ${xparser} -i -c w -k /cfg/authmode/mode -v 0  $xmlfile
    xt_ret $? "" || return $?
}

function fn_opt_ip()
{
    test -z "${1}" && { echo "Usage: ip mustnot be NULL"; return 1; }

    VIP=${1:-192.168.2.44}
    MASK=${2:-255.255.255.0}
    GATEWAY=${3:-${VIP%.*}.1}

    ${xparser} -i -c w -k /cfg/eth/ip   -v ${VIP}       $xmlfile && \
    ${xparser} -i -c w -k /cfg/eth/mask -v ${MASK}      $xmlfile && \
    ${xparser} -i -c w -k /cfg/eth/gw   -v ${GATEWAY}   $xmlfile
    xt_ret $? "" || return $?

    (cd $dir_webpage;
     find -name '*.js' -type f | xargs sed -i "s/192.168.1.217/${VIP}/g";)
     xt_ret $? "" || return $?
}

function fn_opt_user_passwd()
{
    test ${#} -eq 2
    xt_ret $? "[$*] must be format like [opt_user_passwd user passwd]" || return $?

    user=$1
    pass=$2

    env
    pass_md5=`${ingenic_crypt} md5 ${user}:${pass}`
    xt_ret $? "" || return $?
    pass_base=`${ingenic_crypt} base ${pass}`
    xt_ret $? "" || return $?
    pass_onvif=`${ingenic_crypt} onvif ${pass}`
    xt_ret $? "" || return $?

    # mode 0:不验证  1:basic  2: digest
    ${xparser} -i -c w -k /cfg/authmode/mode                    -v 1  $xmlfile && \
    ${xparser} -i -c w -k /cfg/sysUser/userlist/user/username    -v ${user} $xmlfile && \
    ${xparser} -i -c w -k /cfg/sysUser/userlist/user/cryptpasswd -v ${pass_base} $xmlfile && \
    ${xparser} -i -c w -k /cfg/sysUser/userlist/user/digestpasswd -v ${pass_md5} $xmlfile && \
    ${xparser} -i -c w -k /cfg/sysUser/userlist/user/onvifpasswd  -v ${pass_onvif} $xmlfile
    xt_ret $? "opt_user_passwd" || return $?
}

function fn_clean_filesys()
{
    # /ipc/app
    ls ${dir_filesys}/ipc/app/* | grep -E -v "(jco_server|p2p)" | xargs rm -rf
    find ${CWD} -name '.nfs*' | xargs rm -f
    find ${CWD} -name '.log'  | xargs rm -f
    rm -rf ${dir_filesys}/opt/conf/config.xml   # del non-svn and redundent files

    rm -rf ${dir_filesys}/ipc/sensor/custom
    rm -rf ${dir_filesys}/ipc/sensor/beta
    rm -f ${dir_filesys}/ipc/web/image/logo_*.png
    rm -f ${dir_filesys}/ipc/web/image/login/login_hbgk.png
    rm -f ${dir_filesys}/ipc/web/image/login/login_wsd.png
    rm -f ${dir_filesys}/ipc/web/image/login/login_vcam.png
    rm -rf ${dir_filesys}/ipc/web/zlsj
    rm -rf ${dir_filesys}/ipc/web/wsd
    rm -rf ${dir_filesys}/ipc/web/SecurityExpert
    rm -rf ${dir_filesys}/ipc/web/canavis
    rm -rf ${dir_filesys}/ipc/web/multi-language
    rm -rf ${dir_filesys}/ipc/web/icu*
    rm -rf ${dir_filesys}/ipc/web/Knowledge
    rm -rf ${dir_filesys}/ipc/web/hbcfPro
    rm -rf ${dir_filesys}/ipc/web/nologo
    rm -rf ${dir_filesys}/ipc/web/mhk
    rm -rf ${dir_filesys}/ipc/web/vcam*
    rm -rf ${dir_filesys}/ipc/web/sunell
    rm -rf ${dir_filesys}/ipc/web/ykdz
    rm -rf $dir_webpage/COSTOMIZE
    rm -rf ${dir_filesys}/opt/custom/eng
    rm -rf ${dir_webpage}/js_eth
    rm -rf ${dir_filesys}/ipc/webs

    case $NIC in
    e|eth)
        rm -rf ${dir_filesys}/ipc/bin/hostapd
        rm -rf ${dir_filesys}/ipc/bin/wpa_supplicant
        rm -rf ${dir_filesys}/ipc/drv/airlink/8188fu.ko
        rm -rf ${dir_filesys}/ipc/bin/4g
        rm -rf ${dir_filesys}/ipc/bin/udhcpc.script.4g
        rm -rf ${dir_filesys}/ipc/etc/amr/33.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/33.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/34.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/34.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/35.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/35.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/36.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/36.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/37.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/37.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/38.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/38.amr
        rm -rf ${dir_filesys}/ipc/drv/usb
        ;;
    4|4g)
        rm -rf ${dir_filesys}/ipc/web
        rm -rf ${dir_filesys}/ipc/bin/hostapd
        rm -rf ${dir_filesys}/ipc/bin/hostapd_cli
        rm -rf ${dir_filesys}/ipc/bin/iwconfig
        rm -rf ${dir_filesys}/ipc/bin/iwgetid
        rm -rf ${dir_filesys}/ipc/bin/iwlist
        rm -rf ${dir_filesys}/ipc/lib/libiw.so.29
        rm -rf ${dir_filesys}/ipc/bin/iwpriv
        rm -rf ${dir_filesys}/ipc/bin/wifi
        rm -rf ${dir_filesys}/ipc/bin/wpa_cli
        rm -rf ${dir_filesys}/ipc/bin/wpa_passphrase
        rm -rf ${dir_filesys}/ipc/bin/wpa_supplicant
        rm -rf ${dir_filesys}/ipc/drv/airlink
        mv ${dir_filesys}/ipc/bin/udhcpc.script.4g ${dir_filesys}/ipc/bin/udhcpc.script
        rm -rf ${dir_filesys}/opt/conf/airlink
        rm -rf ${dir_filesys}/ipc/etc/amr/04.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/04.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/06.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/06.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/15.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/15.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/16.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/16.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/17.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/17.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/con_fail.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/con_fail.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/recv.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/recv.amr
        rm -rf ${dir_filesys}/ipc/etc/ffw/12.amr
        rm -rf ${dir_filesys}/ipc/etc/ffw/13.amr
        rm -rf ${dir_filesys}/ipc/etc/ffw/14.amr
        rm -rf ${dir_filesys}/ipc/etc/ffw/18.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/cut2double.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/cut2infrared.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/cut2white.amr

        ;;
    w|wifi|ap)
        rm -rf ${dir_filesys}/ipc/web
        rm -rf ${dir_filesys}/ipc/bin/4g
        rm -rf ${dir_filesys}/ipc/bin/udhcpc.script.4g
        rm -rf ${dir_filesys}/ipc/etc/amr/33.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/33.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/34.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/34.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/35.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/35.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/36.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/36.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/37.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/37.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/38.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/41.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/42.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/43.amr
        rm -rf ${dir_filesys}/ipc/etc/amr_en/38.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/csq_0.amr
        rm -rf ${dir_filesys}/ipc/etc/amr/csq_1.amr

        #  4g  relative files
		#rm -rf ${dir_filesys}/ipc/drv/usb
        rm -rf ${dir_filesys}/ipc/drv/usb/option.ko
        rm -rf ${dir_filesys}/ipc/bin/AT
        rm -rf ${dir_filesys}/ipc/bin/at.*
        rm -rf ${dir_filesys}/ipc/bin/impdbg
        rm -rf ${dir_filesys}/ipc/bin/Wget
        ;;
    *)
        echo "
        Usage: ZOPT={e|w|4} npack.sh ...
        "
        exit
        ;;
    esac
}

function fn_customize()
{
    # do before fn_prepare_config, in case to customize config.xml
    ui_post=".gen"
    ui_ver="GEN"
    s_product=${P_TYPE}
    
    # 定制设备型号 devtype 修改 s_devtype 
    case ${NIC} in
    ap)
        s_devtype=${s_product}     # wifi 不带后缀 
        s_product=${s_product}-BLE
        ;;
    4g)
        s_product=${s_product}-4G
        s_devtype=${s_product} 
        ;;
    esac

    devtype=`fn_lowercace ${s_product}`

    # MAXHEIGHT 在 zpack.sh 中根据产品型号赋值，默认 300W
    echo ${MAXHEIGHT:-1296} > ${dir_filesys}/ipc/etc/maxheight

    [ "${LANGUAGE}" = en ] && fn_opt_user_english

    fn_agt_CMD
    xt_ret $? "" || return $?

    echo "AGENT = $AGENT"
    case $AGENT in
    HBGK)   fn_agt_HBGK
            xt_ret $? "" || return $? 
            ui_ver=$AGENT; ui_post=".${AGENT}" ;;  
    YJS)    fn_agt_YJS
            xt_ret $? "" || return $?
            ui_ver=$AGENT; ui_post=".${AGENT}" ;;
    esac

    #date_pack=`date +'%F.%H%M%S'`
    date_pack=`strings ${dir_filesys}/ipc/app/jco_server | awk '/verdttm/{print $2;exit}'`
    #timestamp=`date +%F.%T`;
    #把 date +'%F.%H%M%S' 格式转成 date +%F.%T
    timestamp=$(echo "$date_pack" | sed 's/\././; s/\(.*\)\(..\)\(..\)/\1:\2:\3/')
    echo ${timestamp} > ${dir_filesys}/ipc/etc/timestamp
    echo "hostname ${devtype}" >> ${dir_filesys}/ipc/etc/profile

    # 版本号增加UI定制后缀
    ${xparser} -i -c w -k /cfg/devinfo/devtype   -v ${s_product} ${xmlfile} &&\
    ${xparser} -i -c w -k /cfg/devinfo/platform  -v ${PLATFORM}  ${xmlfile} &&\
    ${xparser} -i -c w -k /cfg/devinfo/custom_ui -v ${ui_ver}    ${xmlfile}
    xt_ret $? "" || return $?

    fn_prepare_config
    xt_ret $? "" || return $?
}

function fn_imgsize()
{
    img_size=`ls -l ${dir_com}/$1 | awk '{print $5}'`
    xt_ret $? "do ls get size" || return $?
    flash_size=`grep -m1 "^$2" ${fstab} | awk '{print $3}'`
    xt_ret $? "do grep get size" || return $?
    let flash_size+=0	# transform hex to decimal
    if [ $img_size -gt $flash_size ]; then
        fn_echo_fail "$1 size($img_size) bigger than $2 size($flash_size)!"
        return 1
    else
        let rest_size=flash_size-img_size
        echo "$2 rest $rest_size than $1!"
    fi
}

function fn_do_images()
{
    cd ${dir_filesys}/

    # opt
    miscfs_size=`grep sf-optfs ${fstab} | awk '{print $3}'`
    mkfs.jffs2 -r opt -o ${dir_com}/filesys.jffs2 -e 32KiB --pad=$miscfs_size -s 0x100 -n
    xt_ret $? "mkfs" || return $?
    fn_imgsize filesys.jffs2 sf-optfs
    xt_ret $? "do images" || return $?

    find algo -exec touch -t 202507111200 {} +  # 固定文件夹以及文件的修改时间，防止因为修改时间不同导致 algo 分区 md5 不同
    mksquashfs algo ${dir_com}/algofs.sqfs -comp xz -fstime 1710000000 -noappend  > /dev/null 2>&1 # fstime 固定构建时间，防止文件内容没变，打出来的包 md5 不同
    xt_ret $? "mksquashfs" || return $?
    mv ${dir_filesys}/algo ${dir_filesys}/../
    mkdir -p ${dir_filesys}/algo

    # /ipc
    get_type=$(echo `cat ipc/etc/dev_type`)
    fn_echo_succ "do ipcfs from ipc for dev_type:$get_type"
    if [ "${P_TYPE}" = TW36 ]; then
        rm -rf ${dir_filesys}/ipc/web
        mv ${dir_filesys}/ipc/sensor/sensor_sc235/config_product_scene_color_F1.0.ini ${dir_filesys}/ipc/sensor/sensor_sc235/config_product_scene_color.ini
        mv ${dir_filesys}/ipc/sensor/sensor_sc235/config_product_scene_linear_F1.0.ini ${dir_filesys}/ipc/sensor/sensor_sc235/config_product_scene_linear.ini
    else
        rm -rf ${dir_filesys}/ipc/sensor/sensor_sc235/config_product_scene_color_F1.0.ini
        rm -rf ${dir_filesys}/ipc/sensor/sensor_sc235/config_product_scene_linear_F1.0.ini
    fi
    mksquashfs ipc ${dir_com}/ipcfs.sqfs -comp xz -b 32k > /dev/null 2>&1
    xt_ret $? "mksquashfs" || return $?
    mv ${dir_filesys}/ipc ${dir_filesys}/../
    mkdir -p ${dir_filesys}/ipc

    # rootfs
    rm -rf ${dir_filesys}/opt/*
    mksquashfs ${dir_filesys} ${dir_com}/rootfs.sqfs -comp xz  > /dev/null 2>&1 # -no-xattrs
    xt_ret $? "mksquashfs" || return $?

    # cp u-boot kernel
    cd ${local_image}
    cp ${fstab} ${dir_com}
    if [ "${P_TYPE}" = TW36 ]; then
        cp ${local_image}/boot_image.bin.TW36 ${dir_com}/boot_image.bin
    elif [ "${P_TYPE}" = TY313 ]; then
        cp ${local_image}/boot_image.bin ${dir_com}/boot_image.bin
    else
        echo "unkown P_TYPE:${P_TYPE}"
        false
    fi
    xt_ret $? "" || return $?
    [ "${NIC}" = 4g ] && cp uImage.4g ${dir_com}/uImage || cp uImage ${dir_com}/uImage
    xt_ret $? "" || return $?

    # judge image size
    fn_imgsize boot_image.bin sf-uboot
    xt_ret $? "do images" || return $?
    fn_imgsize uImage sf-kernel
    xt_ret $? "do images" || return $?
    fn_imgsize rootfs.sqfs sf-rootfs
    xt_ret $? "do images" || return $?
    fn_imgsize ipcfs.sqfs sf-ipcfs
    xt_ret $? "do images" || return $?
    fn_imgsize algofs.sqfs sf-algofs    #添加 algo 分区大小检查
    xt_ret $? "do images" || return $?

    cd ${dir_com}
    fn_echo_succ "\ngen 0 for factory:"
    echo =====================================================================
    generate_firmware 1 output_sf_fireware_${date_pack}_${s_product}.bin
    cp output_sf_fireware_${date_pack}_${s_product}.bin output_sf_fireware.bin
    echo =====================================================================
    xt_ret $? "" || return $?

    return 0
}

function fn_makerinfo()
{
    local pack_date=`date +'%F.%T'`

    echo "pakcmd $pakcmd @$pack_date"
    echo "Packer `cat ~/.subversion/auth/svn.simple/* | grep -m1 -A2 username | tail -1`"
    svn info ${CWD}
}

function fn_do_tar_bin()
{
    ffw_package="${dir_tar}/${CHIP}.${devtype}${ui_post}.${LANGUAGE}.$date_pack.ffw"
    tar_package="${dir_tar}/${CHIP}.${devtype}${ui_post}.${LANGUAGE}.$date_pack.tgz"
    tar_uptools="${dir_tar}/${CHIP}.uptools.tgz"

	cd ${dir_uptools}
    tar -zcf ${tar_uptools} *
    xt_ret $? "tar" || return $?

    fn_do_available_sensor
	# append header(32B type, 32B size, 32B MD5)
    printf "%16s%8d%8s%32s" "opt.tgz" `stat -c %s ${tar_package}` "resv" `md5sum ${tar_package} | awk '{print $1}'` >> ${tar_package}

    # append image
    #fn_echo_succ "-- - - spi maps -- - "
    maps=(
        mtd0 boot_image.bin    # uboot
        mtd2 uImage            # kernel
        mtd3 rootfs.sqfs       # rootfs, 因为无法解决出错问题所以不升级rootfs
        mtd4 ipcfs.sqfs        # ipcfs
        mtd5 algofs.sqfs       # algo
    )

    [ "${ROOTFS}" = 1 ] && ROOT_SQFS=rootfs.sqfs
    [ "${ALGO:-1}" = 1 ] && ALGO_SQFS=algofs.sqfs

    img_files_pkg="${ROOT_SQFS} ${ALGO_SQFS} ipcfs.sqfs uImage boot_image.bin"  # copy sequence is end to begin

    fn_echo_succ "load ${img_files_pkg} to package"

    for imgfile in ${img_files_pkg}; do
        local i j
        for (( i=0,j=1; i<${#maps[@]}; j+=2,i+=2 )); do
            n=${maps[${i}]}
            f=${maps[${j}]}
            if [ "$f" == $imgfile ]; then
                test -e ${dir_com}/${f}
                xt_ret $? "image $f is no exist" > /dev/stderr || continue

                cat ${dir_com}/${f} >> ${tar_package}
                printf "%16s%8d%8s%32s" ${n} `stat -c %s ${dir_com}/${f}` "resv" `md5sum ${dir_com}/${f} | awk '{print $1}'` >> ${tar_package}
            fi
        done
    done

    # append upgrade tools
    cat ${tar_uptools} >> ${tar_package}
    printf "%16s%8d%8s%32s" "uptools.tgz" `stat -c %s ${tar_uptools}` "resv" `md5sum ${tar_uptools} | awk '{print $1}'` >> ${tar_package}
    rm -f ${tar_uptools}

    # append truncate
    cat ${local_uptools}/truncate >> ${tar_package}
    printf "%16s%8d%8s%32s" "truncate" `stat -c %s ${local_uptools}/truncate` "resv" `md5sum ${local_uptools}/truncate | awk '{print $1}'` >> ${tar_package}

    # append updateExt.sh
    cat ${local_uptools}/updateExt.sh >> ${tar_package} && \
    cat ${local_uptools}/updateExt.sh | wc -c | xargs printf "%06d" >> ${tar_package}
    xt_ret $? "" || return $?

    md5sum ${tar_package} | awk '{printf $1}' >> ${tar_package}
    xt_ret $? "md5sum" || return $?

    ${jco_crypt} enc ${tar_package}
    xt_ret $? "crypt" || return $?

    cp ${tar_package} ${ffw_package}
    xt_ret $? "crypt" || return $?
    echo -n ${DEV_TYPE} >> ${ffw_package}
    echo -n ${DEV_TYPE} | wc -c | xargs printf "%02d" >> ${ffw_package}    # lzbox nr_dev_type
    xt_ret $? "crypt" || return $?
    # -f -d1 不可以共用，要后打.ffw包
    echo -n ${timestamp}>> ${ffw_package}
    fn_echo_succ "succ ffw:\n ${ffw_package}"
}

function fn_do_available_sensor()
{
    cd ${dir_package}; ls

    typeset -u available_sensor
    available_sensor=`ls ${CWD}ipc/sensor/ | grep sensor | awk -F '.' '{print $1}' | awk -F 'sensor_' '{print $2}'`
    echo "$available_sensor" > available_sensor
    available_sensor=`sort -u available_sensor | uniq`
    echo "Support sensor: "$available_sensor
    echo "$available_sensor" > available_sensor

	tar -zcf ${tar_package} *
	cd - > /dev/null
}

function fn_do_tar()
{
    cpu_type_dev=hi3516cv608
    cd ${dir_package}
    echo "${DEV_TYPE}_force" > dev_type
    echo ${cpu_type_dev} > cpu_type
    fn_do_tar_bin
    xt_ret $? "tar_bin" || return $?

    mv ${tar_package} ${tar_package//.tgz/.force.tgz}

    cd ${dir_package}
    echo ${DEV_TYPE} > dev_type
    echo ${cpu_type_dev} > cpu_type

    fn_do_tar_bin
    xt_ret $? "tar_bin" || return $?
}

function fn_opt_user_english()
{
    ${xparser} -i -c w -k /cfg/osdinfo/osdlanguage -v 1             $xmlfile  && \
    ${xparser} -i -c w -k /cfg/sysCustomize/webdeflang   -v 1       $xmlfile
    xt_ret $? "" || return $?
    sed -i '/cfg.sysCustomize.webdeflang/d' ${factkep}
    mv ${dir_filesys}/opt/custom/eng/* ${dir_filesys}/opt/custom
}

function fn_prepare_config()
{
    cd ${dir_filesys}/opt/conf

    sed -i "s/__RELEASE_TIME__/`date +'%F.%T'`/g" config.org
    xt_ret $? "" || return $?

    # 根据SVN的修改记录自动更新网页的日期
    cd $CWD
    svn_web=`svn info | grep "^URL:"|grep -v "\^/"|awk '{print $2}'|awk '{split($0,a,"script");print a[1]}'`"filesys_normal/ipc/web"
    cd -

    if [ -n "${Svn}" ]; then
        svn_date='2018-06-08'
    else
        svn log -l2 --xml $svn_web > tmp_log
        svn_date=`${xparser} -c r -k /log/logentry/date tmp_log | awk '{print $3}' | awk -F 'T' '{print $1}'`
    fi

    rm -f tmp_log
    date_year=`echo $svn_date | awk -F '-' '{print $1}'`
    date_month=`echo $svn_date | awk -F '-' '{print $2}'`
    date_day=`echo $svn_date | awk -F '-' '{print $3}'`
    date_rebuld="$date_year""$date_month""$date_day"
    cfg_date=`grep "<webver>" config.org | sed 's/<webver>//' | sed 's/<\/webver>//' | awk -F '-' '{print $2}'`
    sed -i "/<webver>/s/$cfg_date/$date_rebuld/" config.org

    ${xparser} -c r -f upgrade.add.list config.org > upgrade.add.pair
    xt_ret $? "" || return $?

    ${xparser} -c r -f upgrade.mod.list config.org > upgrade.mod.pair
    xt_ret $? "" || return $?
}

function fn_make_files()
{
    #
    DEV_TYPE=${CHIP}_${s_product}
    echo ${DEV_TYPE} > ${dir_filesys}/ipc/etc/dev_type

    # for .tar
    fn_makerinfo >> ${p_RELEASE}
    xt_ret $? "" || return $?

    cp -a ${p_RELEASE} ${p_ROOTRLS}
    xt_ret $? "" || return $?

    # 转移xml
    mv ${xmlfile} ${dir_filesys}/ipc/etc

    cd ${dir_filesys}/opt/
    cp -a * ${dir_package}
    xt_ret $? "cp to package" || return $?

    # for upgrade
    cp -a ${local_uptools} ${dir_uptools}
    xt_ret $? "upTool not exist" || return $?
}

function fn_strip()
{
    # g_run g_stat process
    mkdir -p ${dir_filesys}/ipc/bin/.c/
    cp $TOPDIR/../../../appSrc/main/include/{g_sys.h,g_run.h,g_stat.h} ${dir_filesys}/ipc/bin/.c/

    # clean app
    cd ${dir_filesys}/ipc/app
    nm jco_server | grep -E -w "(_ZN3p2p3net10Connection7synRecvEPNS0_3BusEh|_ZN3p2p3net10ForwardBus7receiveEPKhj)" | awk '{print $1}'|xargs > xp2p_addr
    word=`cat xp2p_addr | wc -w`
    [ $word -eq 2 ]
    xt_ret $? "" || return $?           
    $STRIP jco_server && chmod +x jco_*
    xt_ret $? "${FUNCNAME}" || return $?

    # strip
    local dir files
    for dir in /bin /sbin /usr/bin /usr/sbin /ipc/bin /ipc/lib /lib; do
        files=`file ${dir_filesys}${dir}/* | awk -F: '/ELF 32-bit/ {print $1}'`
        if [ -z "$files" ]; then
            continue
        fi
        file ${dir_filesys}${dir}/* | awk -F: '/ELF 32-bit/ {print $1}' | xargs $STRIP
        xt_ret $? "" || return $?
    done

    for dir in /drivers /ipc/drv; do
        files=`file ${dir_filesys}${dir}/* | awk -F: '/ELF 32-bit/ {print $1}'`
        if [ -z "$files" ]; then
            continue
        fi
        file ${dir_filesys}${dir}/* | awk -F: '/ELF 32-bit/ {print $1}' | xargs $STRIP -S
        xt_ret $? "" || return $?
    done
}

function fn_main()
{
    CWD=${PWD}/
    dir_release=${CWD}release
    dir_package=${CWD}package
    dir_uptools=${CWD}uptools
    dir_filesys=${CWD}filesys
    dir_webpage=${CWD}filesys/ipc/web
    dir_com=${dir_release}/com
    dir_tar=${dir_release}/tar
    xmlfile=${dir_filesys}/opt/conf/config.org
    factkep=${dir_filesys}/opt/conf/factory.kep.list
    upgrad_mod=${dir_filesys}/opt/conf/upgrade.mod.list
    custom_add=${dir_filesys}/opt/conf/customcof.add.pair
    vec_1080p=${dir_filesys}/opt/conf/factory.kep.pair.1080p
    vec_1296p=${dir_filesys}/opt/conf/factory.kep.pair.1296p
    vec_1440p=${dir_filesys}/opt/conf/factory.kep.pair.1440p
    fstab=${local_image}/README.spi.txt

    xparser="${CWD}/rc.d/xml_shuttle"
    ingenic_crypt="${CWD}/rc.d/ingenic_crypt"
    jco_crypt="${CWD}/rc.d/jco_crypt"
    p_RELEASE="${dir_filesys}/ipc/etc/RELEASE"
    p_ROOTRLS="${dir_filesys}/etc/ROOT_RELEASE"

    ls | grep -E -v "(rc.*|rules|.sh$|.txt)" | xargs rm -rf {}

    mkdir -p $dir_package
    mkdir -p $dir_uptools
    mkdir -p $dir_filesys
    mkdir -p $dir_release
    mkdir -p $dir_com
    mkdir -p $dir_tar

    set -x
    PATH=${CWD}rc.d/:$PATH
    set +x

    # copy
    [ -d "$local_filesys" ] && cp -a ${local_filesys}/* ${dir_filesys}
    xt_ret $? "Error: use $local_filesys : $local_webpage please" || return $?

    fn_strip
    xt_ret $? "prepare failed" || return $?

    # customize type 和 devtype 等
    fn_customize
    xt_ret $? "${FUNCNAME}" || return $?

    fn_clean_filesys
    xt_ret $? "ZOPT" || return $?

    fn_make_files
    xt_ret $? "fail to prepare" || return $?

    # do images b4 fn_do_tar() for rootfs.sqfs
    fn_do_images
    xt_ret $? "do images" || return $?

    fn_do_tar
    xt_ret $? "tar" || return $?

    fn_echo_succ "
    cd ${tar_package%/*};
    time curl -F \"file=@${tar_package//*release/release};\" 192.168.9.111/webs/updateCfg

    KB`du -sk ${tar_package}`
    KB`du -sk ${tar_package//.tgz/.force.tgz}`
    Get release at ${dir_release}
    enjoy on Hisilicon!
    " | tee ${CWD}/.banner
    date

    sizeK=`du -sk ${dir_com}/rootfs.sqfs | awk '{print $1}'`

    if [ "${sizeK}" -ge 6144 ]; then
        fn_echo_fail "
        ------------------------------------------------
        SPI-FLASH sqfs size[${sizeK}] must less than 6144K
        "
        exit 1
    fi
    cp -rf  ${local_image}/tftpd32.exe  ${dir_com}/
    # cp -rf  ${CWD}/rc.d/box ${dir_com}/
    chmod 777 -R  ${dir_com}
}

fn_main $@
