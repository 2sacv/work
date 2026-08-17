#ifndef __DDNS9299_H__
#define __DDNS9299_H__
#include "ddns_common.h"


#ifdef __cplusplus
extern "C" {
#endif

    int ddns_9299_ip_check(char *ip, int timeout, char status[/*32*/]);
    int ddns_9299_update(ddns_opt_t* opt, int timeout, char status[/*32*/]);


#ifdef __cplusplus
}
#endif

#endif

