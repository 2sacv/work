
/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : delay_exec.h
 * Created Time : 2014-04-15
 * Version      : 1.0
 * Author       : cheby
 * Description  :
 */

#ifndef DELAY_EXEC_H_
#define DELAY_EXEC_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define DELAY_REBOOT_LINUX() do {                                           \
    delay_ctrl_exec(DELAY_CMD_REBOOT, (void *)__func__, strlen(__func__));  \
} while(0)

#define DELAY_REBOOT_LINUX_CONF() do {                                           \
    delay_ctrl_exec(DELAY_CMD_REBOOTCONF, (void *)__func__, strlen(__func__));  \
} while(0)

#define DELAY_RESET_APPS() do {                                                 \
    delay_ctrl_exec(DELAY_CMD_KILL_ALL_APP, (void *)__func__, strlen(__func__));\
} while(0)

	typedef enum
    {
        DELAY_CMD_BEGIN = -1,
        DELAY_CMD_REBOOT = 0,       // reboot device
        DELAY_CMD_REBOOTCONF,       // reboot at conf
        DELAY_CMD_KILL_ALL_APP,     // reset jco_server
        DELAY_CMD_SETETHMAC,
        DELAY_CMD_SETETHIP,         // set eth ip addr
        DELAY_CMD_DEFAULT,          // 恢复默认值
        DELAY_CMD_SETDNS,           // set dns
        DELAY_CMD_SETUPDATEPORT,    // 重启服务
        DELAY_CMD_SETWIFI,          // wifi设置
        DELAY_CMD_SETWIFDNS,        // 设置wifidns
        DELAY_CMD_DELAY_WIFI,       // delay start wifi
        DELAY_CMD_DEFAULT_KEEP_NET, // 复位保留ethcfg wificfg配网信息
        DELAY_CMD_END
    }
    DELAY_CMD_E;

    typedef struct {
        DELAY_CMD_E cmd;
        void *param;
        int len;            // parameter length
    } DELAY_EXEC_S;

    /*====================================================================
     discrib: delay execute command in a thread to avoid block the caller function
     param:
        int delay_ctrl_exec(            -OUT SUCCESS/FAILURE
            DELAY_CMD_E cmd,    -IN DELAY_CMD_E
            void *param,            -IN command parameter
            int len)                -IN parameter length
    =====================================================================*/
    int delay_ctrl_exec(DELAY_CMD_E cmd, void *param, int len);
    void delay_one_minute_reboot();
    void sync_syslog();
    void now_rbeoot_linux();
    void secs_delay_reboot(int sec, const char *func);
    void exit_jco_server(void);
#ifdef __cplusplus
}

#endif

#endif /* DELAY_EXEC_H_ */
