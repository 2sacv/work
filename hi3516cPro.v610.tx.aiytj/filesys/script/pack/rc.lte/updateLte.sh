#!/bin/sh

LTE_UNTAR_DIR="/tmp/lte"
LTE_UPDATE_FILE="/tmp/upgrade.tgz"
LTE_UPGRADE_BINPACK="${LTE_UNTAR_DIR}/image_ec716s/at_command.hbinpkg"
LTE_USB_INITFILE="${LTE_UNTAR_DIR}/config/cfg_ec716s_usb.ini"
LTE_TOOL_DIR="${LTE_UNTAR_DIR}/uptools"
LTE_UPGRADE_TYPE="BL AP CP"
YIYUAN_TOOL_NAME="DownloadCLI"

# 需要兼容 CPU 类型时，在 lte 目录下创建对应 CPU 目录，并放入对应编译链编译的升级工具
# 不同设备可能会有下列列表的差异，需要适配时直接添加到对应的列表中即可

# 1. 增加升级包解压路径
LIST_UPGRADE_PATH="/tmp/upgrade.tgz /tmp/upgrade/upgrade.tgz /tmp/t31/upgrade.tgz"

# 2. 增加驱动所在路径 /ipc/drv
LIST_IPC_DRVS="cdc-acm.ko cdc_acm.ko usb/cdc-acm.ko usb/cdc_acm.ko"

# 3. 增加升级端口 /dev
LIST_UPGRADE_PORT="/dev/ttyACM0"

# 4. 增加 feed_dog 程序 /ipc/bin
LIST_IPC_DOG="/ipc/bin/feed /ipc/bin/feedwdt /sbin/watchdog"

xt_ret() { [ "${1}" = "0" ] && return 0; echo "error: ${2}"; return 1;}
fn_logger_ret() { [ "${1}" = "0" ] && return 0; logger "error: ${2}"; echo "error: ${2}"; return 1;}
fn_echo_succ() { echo "succ: $1" ;}
check_files() { for f in "$@"; do [ -f "$f" ] && [ -s "$f" ] || return 1; done; return 0;}

fn_kill_complete()
{
	cont=0
    while [ $cont -lt 3 ]; do
		procId=`ps | grep "$1" | grep -v grep | awk '{print $1}'`
		if [ "${procId}" = "" ]; then
			echo "${1} exit complete"; break
		else
			echo "${1} is exist, kill it."; kill -9 $procId; sleep 1
		fi
        let cont++
	done
}

fn_kill_jco_server()
{
	cont=0
    while [ $cont -lt 3 ]; do
		procId=`ps | grep "jco_server" | grep -v grep | grep -v Z | awk '{print $1}'`
		if [ "${procId}" = "" ]; then
			echo "jco_server exit complete"; break
		else
			echo "jco_server is exist, kill it."; kill -9 $procId; #killall jco_server
            sleep 1
		fi
        let cont++
	done
}

fn_reboot()
{
    mount | grep -q ' /mnt ' && umount -f /mnt
	
	fn_echo_succ "Rebooting from updateLte.sh..."

	sync; sleep 1; sync; sleep 2

    killall -9 "${dog_name}"
	reboot -f
}

fn_start_watchdog()
{
    for end_dog in ${LIST_IPC_DOG}; do
        if [ -f "${end_dog}" ]; then
            dog_name=$(echo "${end_dog}" | awk -F/ '{print $NF}')
            echo "find: ${end_dog}"; break
        fi
    done
    [ -f "${end_dog}" ]
    xt_ret $? "can not find feed dog process" || return $?
    fn_echo_succ "find feed dog process : ${dog_name}"
    "${dog_name}" &
}

fn_link_upgrade_path()
{
    # 可能存在的升级包解压路径，不同平台可能不一致但唯一
    # 统一移动到 /tmp/upgrade/upgrade.tgz 进行解压升级
    for end_upgrade_path in ${LIST_UPGRADE_PATH}; do
        if [ -f "${end_upgrade_path}" ]; then
            echo "find: ${end_upgrade_path}"; mv "${end_upgrade_path}" "${LTE_UPDATE_FILE}"; break
        fi
    done
    [ -f "${LTE_UPDATE_FILE}" ]
    xt_ret $? "can not find upgrade pack path" || return $?
    fn_echo_succ "upgrade pack path : ${LTE_UPDATE_FILE}"
    logger "upgrade pack path : ${LTE_UPDATE_FILE}"
}

fn_get_pack_tailinfo()
{
    src_file="${LTE_UPDATE_FILE}"
    header_info_size=104
    lte_md5=$(tail -c 32 "${src_file}" | tr -d ' \t\n\r')
    lte_size=$(tail -c 40 "${src_file}" | head -c 8 | tr -d ' \t\n\r')
    lte_version=$(tail -c 104 "${src_file}" | head -c 64 | tr -d ' \t\n\r')
    fn_echo_succ "get ltepack info: ${lte_version} ${lte_size} ${lte_md5}"
    logger "get ltepack info: ${lte_version} ${lte_size} ${lte_md5}"
}

fn_check_version_mismatch()
{
    # check AT already
    cont=0
    while [ "${cont}" -lt 20 ]; do
        cur_version=$(AT ATI | awk '/Revision:/ {print $2}' | tr -d ' \t\r\n')
        if [ -n "${cur_version}" ]; then
            echo "AT already success ; get cur_version=${cur_version}"; break
        fi
        sleep 0.5; let cont++; echo "try AT ready : ${cont}"
    done
    [ -n "${cur_version}" ]
    xt_ret $? "AT already fail" || return $?

    # check version 
    [ "${cur_version}" !=  "${lte_version}" ]
    xt_ret $? "version no change, stop upgrade" || return $?
    fn_echo_succ "check version : cur_version=${cur_version}, up_version=${lte_version}"
    logger "check version : cur_version=${cur_version}, up_version=${lte_version}"
}

fn_verify_pack_integrity()
{
    # check size
    upgrade_total_size=$(stat -c %s "${src_file}" | tr -cd '0-9')
    upgrade_size=$((upgrade_total_size - header_info_size))
    [ "${upgrade_size}" -eq "${lte_size}" ]
    xt_ret $? "check upgrade size error" || return $?
    fn_echo_succ "check upgrade size success"

    # get upgrade original pack
    tmp_file="${src_file}.tmp"
    head -c "${upgrade_size}" "${src_file}" > "${tmp_file}" 2>/dev/null
    xt_ret $? "head -c ${src_file} to ${tmp_file} fail" || return $?
    mv "${tmp_file}" "${src_file}"

    # check md5
    [ $(md5sum "${src_file}" | awk '{print $1}') = "${lte_md5}" ]
    xt_ret $? "check upgrade md5 error" || return $?
    fn_echo_succ "check upgrade md5 success"
}

fn_check_upgrade_tgz()
{
    fn_get_pack_tailinfo
    fn_logger_ret $? "get upgrade info error" || return $?

    fn_verify_pack_integrity
    xt_ret $? "verify pack integrity error" || return $?

    fn_echo_succ "check upgrade.tgz success"
    logger "check upgrade.tgz success"
}

fn_choose_uptool()
{
    # get cpu type to choose uptool
    cur_cpu_type=`cat /proc/cmdline| xargs -n1 | grep cpu= | awk -F'=' '{print $2}'`
    [ -n "${cur_cpu_type}" ]
    xt_ret $? "get cpu fail" || return $?

    QDOWNLOAD_CLI_DIR=$(ls -d ${LTE_TOOL_DIR}/*/ | grep -i "${LTE_TOOL_DIR}/${cur_cpu_type}" | head -n 1)
    xt_ret $? "find uptool dir fail" || return $?

    QDOWNLOAD_CLI_TOOL="${QDOWNLOAD_CLI_DIR}/${YIYUAN_TOOL_NAME}"
    [ -f "${QDOWNLOAD_CLI_TOOL}" ]
    fn_logger_ret $? "can not find uptool" || return $?

    fn_echo_succ "choose ${QDOWNLOAD_CLI_TOOL}"
    logger "choose ${QDOWNLOAD_CLI_TOOL}"
}

fn_choose_driver()
{
    # Check driver file
    for end_driver in ${LIST_IPC_DRVS}; do
        if [ -f "/ipc/drv/${end_driver}" ]; then
            echo "find: ${end_driver}"; break
        fi
    done
    [ -f "/ipc/drv/${end_driver}" ]
    fn_logger_ret $? "can not find driver" || return $?
    fn_echo_succ "choose ${end_driver}"; logger "choose ${end_driver}"
}

fn_package_untar()
{
    rm -rf "${LTE_UNTAR_DIR}" && mkdir -p "${LTE_UNTAR_DIR}" && tar zxf "${LTE_UPDATE_FILE}" -C "${LTE_UNTAR_DIR}"
    fn_logger_ret $? "tar fail" || return $?
    fn_echo_succ "untar success"
}

fn_check_upfile()
{
    # check upgrade file、tool
    check_files "${LTE_UPGRADE_BINPACK}" "${LTE_USB_INITFILE}" "${QDOWNLOAD_CLI_TOOL}"
    fn_logger_ret $? "check upgrade file、tool fail" || return $?
    fn_echo_succ "check upgrade file、tool success"; logger "check upgrade file、tool success"
}

fn_uptool_unpack()
{
    # run uptool to unpack
    "${QDOWNLOAD_CLI_TOOL}" -c "${LTE_USB_INITFILE}" -S
    fn_logger_ret $? "uptool unpack fail" || return $?
    fn_echo_succ "unpack success"; logger "unpack success"
}

fn_upgrade_prepare()
{
    fn_choose_driver
    xt_ret $? "choose driver fail" || return $?

    fn_package_untar
    xt_ret $? "package untar fail" || return $?

    fn_choose_uptool
    xt_ret $? "choose uptool fail" || return $?

    fn_check_upfile
    xt_ret $? "check upfile fail" || return $?

    fn_uptool_unpack
    xt_ret $? "uptool unpack fail" || return $?
}

fn_upgrade0()
{
    sleep 3

    # insmod driver
    insmod "/ipc/drv/${end_driver}"

    # check driver loaded
    cont=0
    while [ "${cont}" -lt 10 ]; do
        if lsmod | grep -q "cdc_acm"; then
            echo "find cdc_acm"; break
        fi
        sleep 1; let cont++; echo "try find cdc_acm : ${cont}"
    done
    lsmod | grep -q "cdc_acm"
    fn_logger_ret $? "cdc_acm load fail" || return $?
    fn_echo_succ "cdc_acm load succ"

    # AT AT+ECRST=delay,599 to reset
    AT_RETURN=$(AT AT+ECRST=delay,599 2>&1)
    echo "${AT_RETURN}" | grep -q "OK"
    echo "AT_RETURN=${AT_RETURN}"
    fn_logger_ret $? "AT AT+ECRST=delay,599 fail" || return $?
    
    # check upgrade port
    cont=0
    while [ "${cont}" -lt 10 ]; do
        for end_upgrade_port in ${LIST_UPGRADE_PORT}; do
            if [ -c "${end_upgrade_port}" ]; then
                echo "find: ${end_upgrade_port}"; break 2
            fi
        done
        sleep 1; let cont++; echo "try find upgrade port : ${cont}";
    done
    [ -c "${end_upgrade_port}" ]
    fn_logger_ret $? "can not find upgrade port" || return $?
    logger "${end_upgrade_port} already"

    "${QDOWNLOAD_CLI_TOOL}" -p "${end_upgrade_port}" -c "${LTE_USB_INITFILE}" -B "${LTE_UPGRADE_TYPE}" -r
    fn_logger_ret $? "uptool upgrade failed" || return $?
}

fn_check_upgrade_success()
{
    cont=0
    while [ "${cont}" -lt 20 ]; do
        new_version=$(AT ATI | awk '/Revision:/ {print $2}' | tr -d ' \t\r\n')
        if [ -n "${new_version}" ]; then
            break
        fi
        sleep 1; let cont++; echo "try AT ATI : ${cont}"
    done
    [ -n "${new_version}" ]
    xt_ret $? "AT ATI failed" || return $?

    fn_echo_succ "cur_version=${new_version}, up_version=${lte_version}"
    logger "cur_version=${new_version}, up_version=${lte_version}"
    [ "${new_version}" = "${lte_version}" ]
    fn_logger_ret $? "upgrade complete, version failed" || return $?

    # 播报升级成功语音
    touch /opt/conf/upgrade_win
    sync

    fn_echo_succ "==== 4g module upgrade succ ===="
    logger "==== 4g module upgrade succ ===="
}

fn_upgrade()
{
    fn_kill_complete auto_run.sh
	fn_kill_jco_server
    fn_start_watchdog

    fn_link_upgrade_path
    xt_ret $? "find ltepack fail" || return $?

    fn_check_upgrade_tgz
    xt_ret $? "unpack fail" || return $?

    fn_check_version_mismatch
    fn_logger_ret $? "check version fail" || return $? 

    fn_upgrade_prepare
    xt_ret $? "upgrade preparation fail" || return $?

    fn_upgrade0
    xt_ret $? "upgrade fail" || return $?

    fn_check_upgrade_success
    xt_ret $? "4g moudle upgrade failed" || return $?
}

fn_main()
{
    fn_upgrade

    fn_reboot
}

fn_main $@