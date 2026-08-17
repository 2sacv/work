/*
 * File Name    :
 * Created Time : 2024-02-29
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _JS_HTTP_CLIENT_H_
#define _JS_HTTP_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct jhttp_client_t jhttp_client_t;

typedef void (*JHCOnHeaderFunc)(void *userdata, const char *header, int headersize);
typedef void (*JHCOnBodyDataFunc)(void *userdata, const char *body, int bodysize, int isfinal);

jhttp_client_t* jhttp_client_create(const char* scheme, char* ip, int port);
void jhttp_client_destroy(jhttp_client_t *httpclient);

int jhttp_client_set_header(jhttp_client_t* httpclient, const char* key, const char* value);
int jhttp_client_set_Param(jhttp_client_t* httpclient, const char* key, const char* value);
int jhttp_client_set_formdata(jhttp_client_t* httpclient, const char* name, const char* filename, char* value, int valuelen);
int jhttp_client_set_formfile(jhttp_client_t* httpclient, const char* name, const char* filepath);

int jhttp_client_get(jhttp_client_t* httpclient, const char* uri,
                     JHCOnHeaderFunc onheader, JHCOnBodyDataFunc onbody, 
                     void* userdata, int timeoutms);

int jhttp_client_post(jhttp_client_t* httpclient, const char* uri,
                      char* postmsg, int msgbytes,
                      JHCOnHeaderFunc onheader, JHCOnBodyDataFunc onbody, 
                      void* userdata, int timeoutms);



#ifdef __cplusplus
}
#endif

#endif

