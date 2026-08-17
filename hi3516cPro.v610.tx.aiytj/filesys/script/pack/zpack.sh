#! /bin/bash

. rules
. rc.d/common.rc

CWD=${PWD}

Usage="
    R=0 C=ls
    ./zpack.sh ARG1 [ARG2] [ARG3]

    ARG1 list 默认都支持有线
        -----------------------------------------
        w: wifi 产品，支持蓝牙功能，使用星闪模块
        4: 4g 产品
        e: 纯有线产品

    ARG2 list 默认 ty313
        -----------------------------------------
        ty313 : SC235 # 默认开人形联动音频 
        tw36 : SC235 # 启用NPU 
   
    ARG3 定制项 默认 GEN
        -----------------------------------------
        HBSD            # 鸿邦时代

    Env:                #
        Z=1             # rar 压缩 release output
        ROOTFS=1        # 打包 rootfs 分区
        P_TYPE          # 产品型号
        LANGRAGE        # 默认中文 cn|en
        PLATFORM        # TX:腾讯不包含国标和ONVIF，GB:国标，GBPtP:腾讯+国标
        HDFILTER=1      # 开自研人形静态过滤算法, 默认1开
        --------        # 
        ZOOM=1          # 跟踪变倍, 默认1开
        F4AE=1          # 开启人脸收光, 默认0关

    Example: ./zpack.sh 4
"

export C=${C} R=${R}
export ROOTFS=${ROOTFS:=0} ALGO=${ALGO:=1}
export HDFILTER=${HDFILTER:=1} ZOOM=${ZOOM:=1} F4AE=${F4AE:=0}

export pakcmd="C=${C} R=${R} ROOTFS=${ROOTFS} ALGO=${ALGO} HDFILTER=${HDFILTER} ZOOM=${ZOOM} F4AE=${F4AE} $0 $@"

fn_cflags()
{
    CWD=${PWD}
    CAR_PERSON=1
    dir_filesys=../../filesys_normal/
    dir_app=${dir_filesys}ipc/app/jco_server
	
    export CHIP=${CHIP:-HI3516CV608}
}

fn_make()
{
	LANGUAGE=cn
	PLATFORM=TX  # 不能为空

    case $1 in
    w) NIC=ap WITHRTSP=WITHRTSP GFLAGS="${GFLAGS} -D__PK__=RBNWVQRIQZ -D__BURN_DEV__=SPC_WIFI -D__WIFI__" ;;
    4) NIC=4g GFLAGS="${GFLAGS} -D__PK__=I1MV8WXACH -D__BURN_DEV__=SPC_4G -D__SIM4G__" ;;
    e) NIC=eth PLATFORM=GBPtP WITH_ONVIF=NEW WITHHTTP=WITHHTTP WITHRTSP=WITHRTSP;;
    *) echo "$Usage" && exit ;;
    esac

    case ${2:-ty313} in
    ty313)
        MAXHEIGHT=1296
        P_TYPE=${P_TYPE:-TY313}
        SNS_TYPE=sc235
        GFLAGS="${GFLAGS} -DSNS_SC235"
        ;;
    tw36)
        MAXHEIGHT=1296
        P_TYPE=${P_TYPE:-TW36}
        SNS_TYPE=sc235
        GFLAGS="${GFLAGS} -DSNS_SC235 -D__TW36__ -DSTEPLESS_PWM=1"
        ;;
    *)
        echo "$Usage" && exit
        ;;
    esac

    AGENT=$3
    case ${AGENT:-GEN} in
    HBGK) 
        echo 
        ;;
    esac

    [ "${V}" = 1 ] && GFLAGS="${GFLAGS} -DV"
    [ "${HDFILTER}" == 0 ] && GFLAGS="${GFLAGS} -DCLOSE_FILTER_NOISE"

	export NIC MAXHEIGHT PLATFORM P_TYPE LANGUAGE ROOTFS

    if [ "${R:-1}" = 0 ]; then
        cd ../../../appSrc/ && make install && cd -
        xt_ret $? "" || return $?
        echo -- skip rebuild, only make -- && return
    else
        echo -- make $1 --
    fi

    cd ../../../appSrc/ && rm -f main/.smake && ${C:-make clean} && \
    make IFLAGS="$GFLAGS" CUST_IVS_PERSON=${cust_ivs_person} PLATFORM=${PLATFORM} WITH_ONVIF=${WITH_ONVIF} WITHHTTP=${WITHHTTP} WITHRTSP=${WITHRTSP}
    xt_ret $? "" || return $?

    make i
    xt_ret $? "" || return $?
}

fn_npack()
{
    # 环境变量进行定制 -o 可读性较差, env_v 可同时传递给 make 引用
    #
    # NIC       网络类型    : w 4 e
    # P_TYPE    产品型号    : TY313 互升版本隔离 
    # AGENT     定制厂家    : HBGK
    # SNS_TYPE  sensor 类型 : sc235
	cd ${CWD}
    echo "================================ 中性LOGO"
    AGENT=${AGENT}          \
    P_TYPE=${P_TYPE}        \
    SNS_TYPE=${SNS_TYPE}    \
    ./npack.sh 
    return $?
}

fn_install()
{
    [ -z "${Z}" ] && return;

    if [ ${NIC} == "4g" ]; then
        DUAL="DUAL4G-"
    fi

    #打包备份
    DTTM=`strings ${CWD}/ipc/app/jco_server | awk '/verdttm/{print $2;exit}'`
	TICK=p.${P_TYPE}-${NIC}-${DTTM}-${DUAL}${SNS_TYPE:0:4}-${AGENT:-GEN}
    release_image_dir=/winc/Export/$CHIP"_"${P_TYPE}
    copy_path=${release_image_dir}/${TICK}

    mkdir -m 775 -p $copy_path
    cp -a release/tar ${copy_path}
    cp $dir_app $copy_path/jco_server.${DTTM}
    cp -a release/com ${copy_path}

    if [ -f /usr/bin/rar ] && [ -n "${Z}" ]; then
        cd ${release_image_dir}
        echo "rar packing..."
        rar a ${TICK}.rar ${TICK} >& /dev/null
        RAR_PATH=`lnxpath.sh ${release_image_dir}`
        Godir ${release_image_dir}
        cd ${CWD}
        echo ___ enjoy rar: ${RAR_PATH}
        sleep ${QK:-0}
    fi
}

function fn_main()
{
    fn_cflags $1
    xt_ret $? "" || return $?

    fn_make $@
    xt_ret $? "" || return $?

    fn_npack $@
    xt_ret $? "" || return $?

    fn_install
    xt_ret $? "" || return $?
      
    return $?
}

fn_main $@
