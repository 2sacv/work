/*
 * File Name    :
 * Created Time : 2024-05-11
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#if defined(__OPENSSL__)

#ifndef _OPENSSL_H_
#define _OPENSSL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "openssl/ssl.h"

SSL_CTX* js_ssl_ctx_new(char* ca_file, char* ca_path, char* crt_file, char *key_file);
void js_ssl_ctx_free(SSL_CTX* ssl_ctx);
SSL* js_ssl_new(SSL_CTX* ssl_ctx, int fd);
void js_ssl_free(SSL * ssl);
int js_ssl_accept(SSL * ssl);
int js_ssl_connect(SSL * ssl);
int js_ssl_read(SSL * ssl, void* buf, int len);
int js_ssl_write(SSL * ssl, const void* buf, int len);
int js_ssl_close(SSL * ssl);

#ifdef __cplusplus
}
#endif

#endif

#endif

