/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : g_log.c
 * @Created Time : 2023-03-24
 * @Version      : 1.0
 * @Author       : hul zhangj
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include "debug.h"
#include "factory_db.h"
#include "g_log.h"
#include "utils.h"
#include "system_ctrl.h"

/*
    方法一：
    ccli log -act set -rtsp 1

    方法二：
    vi /opt/etc/local.rc
    export dbg_venc=1 打开 venc 日志

    方法三：
    log venc {on|off} 或 log venc {0|1}
*/

void init_g_log(void)
{
    struct {
        long offset;
        char *key;
    } list[] = {
        {offsetof(sMod, venc   ), "dbg_venc"   },
        {offsetof(sMod, audio  ), "dbg_audio"  },
        {offsetof(sMod, record ), "dbg_record" },
        {offsetof(sMod, jcp    ), "dbg_jcp"    },
        {offsetof(sMod, sim4g  ), "dbg_sim4g"  },
        {offsetof(sMod, wifi   ), "dbg_wifi"   },
        {offsetof(sMod, rtsp   ), "dbg_rtsp"   },
        {offsetof(sMod, search ), "dbg_search" },
        {offsetof(sMod, upgrade), "dbg_upgrade"},
        {offsetof(sMod, http   ), "dbg_http"   },
        {offsetof(sMod, onvif  ), "dbg_onvif"  },
        {offsetof(sMod, tencent ), "dbg_tencent" },
        {offsetof(sMod, lamp   ), "dbg_lamp"   },
        {offsetof(sMod, alarm  ), "dbg_alarm"  },
        {offsetof(sMod, md     ), "dbg_md"     },
        {offsetof(sMod, hd     ), "dbg_hd"     },
        {offsetof(sMod, osd    ), "dbg_osd"    },
        {offsetof(sMod, ptz    ), "dbg_ptz"    },
        {offsetof(sMod, gb28181), "dbg_gb28181"},
        {offsetof(sMod, netcheck), "dbg_netcheck"},
        {offsetof(sMod, gpio    ), "dbg_gpio"    },
        {offsetof(sMod, mask    ), "dbg_mask"    },
    };

    if (is_okey("/opt/etc/local.rc")) {
        int i;
        for (i = 0; i < ARRAY_SIZE(list); i++) {
            if (getenv(list[i].key) != NULL) {
                SYSLOG("enable %s\n", list[i].key);
                __set_g_log(list[i].offset, 1);
            }
        }
    }
    if(is_okey(FACTORY_SDFIRELOG)) {
        UtilSystemCmd("telnetd -p24");
        toggle_redirect(1);
    }

    return;
}

void toggle_redirect(int tofile)
{
    char *sdlog     = NULL;
    char cmd[128]   = {0};
    char devid[16] = {0};
    static int prev = FALSE;

    if (tofile) {
        if (prev) {
            DBG("ignore repeated redirect %d\n", tofile);
            return;
        } else {
            if (is_okey("/tmp/messages.dot")) {
                UtilSystemCmd("tar -zcf /tmp/messages.dot.tgz /tmp/messages.dot");
            }

            if (is_okey(FACTORY_SDFIRELOG)) {
                system_get_dev_id(devid);
                snprintf(cmd, sizeof(cmd), "/mnt/%s", devid);
                mkdir(cmd, 0755);

                char time_str[64];
                time_t now = time(NULL);
                struct tm *p_tm = localtime(&now);  // 使用当地时间

                strftime(time_str, sizeof(time_str), "%m%d-%H%M%S", p_tm);
                snprintf(cmd, sizeof(cmd), "/mnt/%s/%s.log", devid, time_str);
                sdlog = cmd;
            } else {
                sdlog = "/tmp/messages.dot";
            }

            DBG("sdlog:%s\n", sdlog);
            UtilSystemCmd2("killall c2tty; sleep .5; c2tty -f %s &", sdlog);

            prev = TRUE;
            set_g_log(dbg);
        }
    } else if (!tofile) {
        if (!prev) {
            DBG("ignore repeated redirect %d\n", tofile);
            return;
        } else {
            UtilSystemCmd("killall c2tty");
            sync();

            SYSLOG("____________ re-redirect stdout ___________\n");
            prev = FALSE;
            clr_g_log(dbg);
        }
    }
}
