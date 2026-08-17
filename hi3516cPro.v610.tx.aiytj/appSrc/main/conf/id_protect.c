#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "logapi.h"

#include "utils.h"
#include "jcpService.h"
#include "delay_exec.h"
#include "id_protect.h"
#include "soft_check.h"
#include "system_ctrl.h"

#define DEFAULT_DEVID   "00000000000"
#define DEFAULT_MAC     "00:00:01:02:03:04"

static int dump_bootargs_to_bakfile()
{
    int ret = 0;
    char devid[64] = {0};
    char mac[64] = {0};
    char cmdline[1024] = {0};

    ret = LoadFile("/proc/cmdline", cmdline, sizeof(cmdline) - 1);
	DBG("cmdline;%s, ret;%d\n", cmdline, ret);
    return_val_if_fail(ret > 0, FAILURE);

    ret = get_val(cmdline, "device_id=", devid);
    return_val_if_fail(ret > 0, FAILURE);

    ret = get_val(cmdline, "ethaddr=", mac);
    return_val_if_fail(ret > 0, FAILURE);

    char key_devid[64] = {0};
    char key_mac[64] = {0};

    ret = LoadFile(PATH_BKUP_DEVID, key_devid, sizeof(key_devid) - 1);
    
    if (ret > 0) {
        if (0 != strncasecmp(key_devid, devid, strlen(DEFAULT_DEVID))) {
            DumpFile(PATH_BKUP_DEVID, devid, strlen(devid));
        }
    } else {
        DumpFile(PATH_BKUP_DEVID, devid, strlen(devid));
    }

    ret = LoadFile(PATH_BKUP_MAC, key_mac, sizeof(key_mac) - 1);

    if (ret > 0) {
        if (0 != strncasecmp(key_mac, mac, strlen(DEFAULT_MAC))) {
            DumpFile(PATH_BKUP_MAC, mac, strlen(mac));
        }
    } else {
        DumpFile(PATH_BKUP_MAC, mac, strlen(mac));
    }

	return 0;
}

static int dump_bakfile_to_bootargs()
{
    int nr = 0;
    char key_devid[64] = {0};
    char key_mac[64] = {0};
    char cmd[256] = {0};
    char resp[4096] = {0};

    int ret = LoadFile(PATH_BKUP_MAC, key_mac, sizeof(key_mac) - 1);

    if (ret > 0) {
        sprintf(cmd, "ethcfg -act set -ethmac %s\r\n", key_mac);
        LOG("%s", cmd);
        jcpcmd_sendrecv(cmd, resp, sizeof(resp) - 1);
        nr++;
    }

    ret = LoadFile(PATH_BKUP_DEVID, key_devid, sizeof(key_devid) - 1);
    
    if (ret > 0) {
        sprintf(cmd, "prienv -act set -device_id %s\r\n", key_devid);
        LOG("%s", cmd);
        jcpcmd_sendrecv(cmd, resp, sizeof(resp) - 1);
        nr++;
    }

    sprintf(cmd, "prienv -act flush\r\n");
    jcpcmd_sendrecv(cmd, resp, sizeof(resp) - 1);

    return nr;
}

int validate_devid()
{
    if (system_get_security()) {
        dump_bootargs_to_bakfile();
    } else {
        ERR("system_is_security FAIL\n");

        if (is_okey(PATH_BKUP_MAC) && is_okey(PATH_BKUP_DEVID)) {
            int ret = dump_bakfile_to_bootargs();
            if (ret > 0) {
                // remove to avoid dump_bakfile_to_bootargs fail, Reboot repeatly
                DBG("dump only once\n");
                remove(PATH_BKUP_DEVID);
                remove(PATH_BKUP_MAC);
                sleep(20); // device_id delay
                DELAY_REBOOT_LINUX();
                sleep(8);
            }
        } else {
            ERR("security times FAIL\n");
        }
    }

    return 0;
}
