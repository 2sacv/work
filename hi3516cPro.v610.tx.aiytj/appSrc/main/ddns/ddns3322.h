#ifndef __DDNS3322_H__
#define __DDNS3322_H__
#include "ddns_common.h"

#ifdef __cplusplus
extern "C" {
#endif

    int ddns_3322_update(ddns_opt_t *opt, int timeout, char status[/*32*/]);


#ifdef __cplusplus
}
#endif

#endif