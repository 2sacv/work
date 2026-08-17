#ifndef _CONF_SPI_H
#define _CONF_SPI_H
#ifdef __cplusplus
extern "C" {
#endif

    /* macro */

    /* typedef */

    /* declaration */

    int SetBootargs_spi(ArgOptS opts[]);
    int uboot_set_bootargs_param_nor(char *name, char *value);
    int config_uboot_env_spi(char* action, ArgOptS opts[], char *msgbuf);

#ifdef __cplusplus
}
#endif
#endif
