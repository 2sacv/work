#ifndef __NET_QRCODE_H__
#define __NET_QRCODE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define QRCODE_FS_CHN  1
#define QRCODE_BMP_PATH     "/ipc/web/image/qrcode.bmp"
#define WEB_QRCODE_BMP_PATH "image/qrcode.bmp"

int build_qrcode(void);

int start_qrcode_server();

int stop_qrcode_server();

int uninit_qrcode_server();

void ggwave_push_audio(uint8_t *stream, int len);
#ifdef __cplusplus
}
#endif

#endif


