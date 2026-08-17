#ifndef _TUNNEL_H
#define _TUNNEL_H
#ifdef __cplusplus 
extern "C" {
#endif

int tunn_process(const char *cmd, int len, char *resp, int resp_len, int *fget);

#ifdef __cplusplus
}
#endif
#endif
