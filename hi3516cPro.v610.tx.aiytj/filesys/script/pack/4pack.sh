#! /bin/bash

. rules
. rc.d/common.rc

Usage="
    ./4pack.sh ARG1 ARG2
    
    4g_version_ex = EC801ECNCGR07A01M02_BNH

    ARG1 version
        -----------------------------------------
        R03 : R03A01M02_BNH
        R07 : R07A01M02_BNH

    ARG2 list 默认 CNCG

    ARG3 list 默认 EC801E
"

fn_do_make()
{
    case $1 in
    R03) version="R03A01M02_BNH" ;;
    R07) version="R07A01M02_BNH" ;;
    *) echo "$Usage" && exit ;;
    esac

    type=$2
    case ${type:-CNCG} in
    CNCG) type="CNCG" ;;
    esac

    Model=$3
    case ${Model:-EC801E} in
    EC801E) Model="EC801E" ;;
    esac

    firmware_binpkg=${firmware_dir}/${Model}${type}${version}/"at_command.hbinpkg"

    [ -f "${firmware_binpkg}" ]
    xt_ret $? "firmware_binpkg not exist" || return $?
}

function fn_do_prepare()
{
    date_pack=`strings ${CWD}/ipc/app/jco_server | awk '/verdttm/{print $2;exit}'`
    xt_ret $? "get date fail" || return $?

    timestamp=$(echo "$date_pack" | sed 's/\././; s/\(.*\)\(..\)\(..\)/\1:\2:\3/')
    xt_ret $? "sed date fail" || return $?
}

function fn_do_copyfile()
{
    cp ${firmware_binpkg} ${local_binpkg}
    xt_ret $? "cp firmware_binpkg fail" || return $?

    for upgrade_file in "${local_lte}"/*; do
        if [ "${upgrade_file}" != "${firmware_dir}" ]; then
            cp -a "${upgrade_file}" "${dir_lte}"
        fi
    done
}

function fn_do_ltepkg()
{
    # cp upgrade file
    echo "clean ${dir_lte}"
    rm -rf ${dir_lte} ${tar_ltepack}
    mkdir -p ${dir_lte}

    echo "clean do_ltepack"

    fn_do_copyfile
    
    cd ${dir_lte} && tar -zcf ${tar_ltepack} .

    # append upgrade info to tar_ltepack, include : version ,size ,md5sum
    up_lte_version=`strings ${tar_binpkg} | awk -F: '/QUEC_TOOLS_VER_CHECK: /{print $2}' | awk '{sub(/^[[:blank:]]*/,"",$1);print $1}'`
    [ -n "$up_lte_version" ]
    xt_ret $? "$up_lte_version" || return $?

    printf "%64s%8d%32s" "${up_lte_version}" `stat -c %s "${tar_ltepack}"` `md5sum "${tar_ltepack}" | awk '{print $1}' ` >> ${tar_ltepack}
    printf "%-64s%-8d%-4s%-32s\n" "${up_lte_version}" `stat -c %s "${tar_ltepack}"` "" `md5sum  "${tar_ltepack}" | awk '{print $1}'`

    # append updateExt.sh
    cat ${upLte} >> ${tar_ltepack} && \
    cat ${upLte} | wc -c | xargs printf "%06d" >> ${tar_ltepack}
    xt_ret $? "updateLte.sh append fail" || return $?

    tar_tgz_ltepack="${tar_pkgtype}_${up_lte_version}.tgz"
    tar_ffw_ltepack="${tar_pkgtype}_${up_lte_version}.ffw"
    
    # append md5
    md5sum ${tar_ltepack} | awk '{printf $1}' >> ${tar_ltepack}
    xt_ret $? "md5sum ltepack fail" || return $?

    # encrypt
    ${jco_crypt} enc ${tar_ltepack}
    xt_ret $? "crypt ltepack fail" || return $?

    cp ${tar_ltepack} ${dir_tar}/${tar_tgz_ltepack}
    fn_echo_succ "succ ltepack:\n ${tar_tgz_ltepack}"
    echo -n forceforce >> ${tar_ltepack}
    echo -n forceforce | wc -c | xargs printf "%02d" >> ${tar_ltepack}    # lzbox nr_dev_type
    xt_ret $? "append dev_type fail" || return $?

    echo -n ${timestamp}>> ${tar_ltepack}
    mv ${tar_ltepack} ${dir_tar}/${tar_ffw_ltepack}
    fn_echo_succ "succ ffw ltepack:\n ${tar_tgz_ltepack}"
}

function fn_main()
{
    CWD=${PWD}/
    dir_release=${CWD}release
    dir_uptools=${CWD}uptools
    dir_tar=${dir_release}/tar
    jco_crypt="${CWD}/rc.d/jco_crypt"
    local_lte="${TOPDIR}/rc.lte"
    local_binpkg="${local_lte}/image_ec716s"
    firmware_dir="${local_lte}/firmware"
    dir_lte="${TOPDIR}/lte"
    tar_ltepack="${dir_lte}/../lte.tgz"
    tar_binpkg="${dir_lte}/image_ec716s/at_command.hbinpkg"
    upLte="${dir_lte}/updateLte.sh"
    tar_pkgtype="4gfirmware"

    fn_do_make $@
    xt_ret $? "do make fail" || return $?

    fn_do_prepare
    xt_ret $? "do prepare fail" || return $?

    fn_do_ltepkg
    xt_ret $? "do 4g upgrade fail" || return $?

    echo "====================================================================="
}

fn_main $@