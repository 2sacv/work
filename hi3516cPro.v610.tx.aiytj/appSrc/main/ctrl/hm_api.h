#if 0
/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : hm_api.h
 * Created Time : 2012-10-15
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _hm_api_H_
#define _hm_api_H_
#ifdef __cplusplus
extern "C" {
#endif

    int init_hm_service(void *data);
    void uninit_hm_service(void);

    void handle_hm_msg_storage(char *action, char *path);
    void handle_hm_msg_usb(char *action, char *devpath);
    void handle_hm_msg_net(char *action, char *interface);
    void handle_hm_msg_dhcp(char *action, char *interface);

#ifdef __cplusplus
}
#endif
#endif
#endif
