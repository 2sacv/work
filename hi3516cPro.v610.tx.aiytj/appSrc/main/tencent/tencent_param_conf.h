#ifdef PLATFORM_TENCENT
#ifndef _TENCENT_PARAM_CONF_H_
#define _TENCENT_PARAM_CONF_H_

#ifdef __cplusplus
extern "C" {
#endif

int tencent_load_triple_info(const char* file_name, TripleInfoS* info);
int tencent_uboot_triple_repair(const char *original);
int tencent_get_conf_info(char *info_buf, int buf_size);
int tencent_get_key_secret(char *pt_key, char* dev_name, char* dev_secret, char* pt_secret);

#ifdef __cplusplus
}
#endif

#endif //_TENCENT_PARAM_CONF_H_
#endif //PLATFORM_TENCENT

