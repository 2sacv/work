#ifndef __DDNS_COMMON_H__
#define __DDNS_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct ddns_opt_s {
        char* user_name;
        char* password;
        char* sdomain;  //domain for access
        char* wlan_ip;
        int wlan_port;  //upnp map port
        int work_mode;  //just for 9299.org
        char* mac_addr; //just for 9299.org
    } ddns_opt_t;


#ifdef __cplusplus
}
#endif

#endif

