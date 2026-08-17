#ifndef _JDNS_H_
#define _JDNS_H_

#define JDNS_RESULT_COUNT             (3)

#ifdef __cplusplus
extern "C" {
#endif

int jdns_getaddrinfo(const char *domain, char dns_ip_list[][16], char *ip[JDNS_RESULT_COUNT]);
char *get_ip_by_domain(const char *domain, char *ip, size_t len);

#ifdef __cplusplus
}
#endif
#endif
