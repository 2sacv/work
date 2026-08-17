#ifndef _ID_PROTECT_H
#define _ID_PROTECT_H
#ifdef __cplusplus 
extern "C" {
#endif

#define PATH_BKUP_DEVID "/opt/conf/devid.key"
#define PATH_BKUP_MAC   "/opt/conf/mac.key"

int validate_devid();

#ifdef __cplusplus
}
#endif
#endif
