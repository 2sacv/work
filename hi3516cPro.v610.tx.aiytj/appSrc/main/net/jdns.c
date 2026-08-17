#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "jdns.h"

/* dns header field */
#define DNS_ID_FIELD                 (0x6666)
#define DNS_CTRL_FIELD               (0x0100)
#define DNS_QUESTION_COUNT_FIELD     (0x0001)
#define DNS_ANSWER_COUNT_FIELD       (0x0000)
#define DNS_AUTHORITY_COUNT_FIELD    (0x0000)
#define DNS_ADDITIONAL_COUNT_FIELD   (0x0000)

/* dns record section */
#define DNS_QTYPE_FIELD              (0x0001)
#define DNS_QCLASS_FIELD             (0x0001)

/* dns server and resolve result */
#define DNS_SERVER_COUNT             (3)

static int jdns_domain_check(const char *domain) {
    uint32_t idx = 0;

    for (idx = 0;idx < strlen(domain);idx++) {
        if (domain[idx] == '.') {
            continue;
        }
        if (domain[idx] < 0x30 || domain[idx] > 0x39) {
            return 0;
        }
    }

    return -1;
}

static void jdns_uint2str(uint8_t *input, char *output)
{
    uint8_t idx = 0, i = 0, j = 0;
    uint8_t pos = 0;
    char temp[10] = {0};

    for (idx = 0;idx < 4;idx++) {
        i = 0;
        j = 0;
        pos = input[idx];
        memset(temp, 0, 10);
        do {
            temp[i++] = pos % 10 + '0';
        } while ((pos /= 10) > 0);

        do {
            output[--i + strlen(output)] = temp[j++];
        } while (i > 0);
        output[strlen(output)] = '.';
    }
    output[strlen(output) - 1] = 0x00;
}

static void jdns_request_message(const char *domain, uint8_t buffer[1024], uint32_t *index)
{
    uint32_t idx = 0;

    /* identification */
    buffer[idx++] = (DNS_ID_FIELD >> 8) & 0x00FF;
    buffer[idx++] = (DNS_ID_FIELD) & 0x00FF;

    /* control */
    buffer[idx++] = (DNS_CTRL_FIELD >> 8) & 0x00FF;
    buffer[idx++] = (DNS_CTRL_FIELD) & 0x00FF;

    /* question count */
    buffer[idx++] = (DNS_QUESTION_COUNT_FIELD >> 8) & 0x00FF;
    buffer[idx++] = (DNS_QUESTION_COUNT_FIELD) & 0x00FF;

    /* answer count */
    buffer[idx++] = (DNS_ANSWER_COUNT_FIELD >> 8) & 0x00FF;
    buffer[idx++] = (DNS_ANSWER_COUNT_FIELD) & 0x00FF;

    /* authority count */
    buffer[idx++] = (DNS_AUTHORITY_COUNT_FIELD >> 8) & 0x00FF;
    buffer[idx++] = (DNS_AUTHORITY_COUNT_FIELD) & 0x00FF;

    /* additional count */
    buffer[idx++] = (DNS_ADDITIONAL_COUNT_FIELD >> 8) & 0x00FF;
    buffer[idx++] = (DNS_ADDITIONAL_COUNT_FIELD) & 0x00FF;

    /* qname */
    {
        uint32_t section_start = 0, qname_idx = 0;
        do {
            if (domain[qname_idx] == '.' || qname_idx == strlen(domain)) {
                buffer[idx++] = (uint32_t)(qname_idx - section_start);
                memcpy(&buffer[idx], &domain[section_start], (qname_idx - section_start));
                idx += (qname_idx - section_start);
                section_start = qname_idx + 1;
            }

            if (qname_idx == strlen(domain)) {
                break;
            }

            qname_idx++;
        } while(1);
        buffer[idx++] = 0x00;
    }

    /* qtype */
    buffer[idx++] = (DNS_QTYPE_FIELD >> 8) & 0x00FF;
    buffer[idx++] = (DNS_QTYPE_FIELD) & 0x00FF;

    /* qclass */
    buffer[idx++] = (DNS_QCLASS_FIELD >> 8) & 0x00FF;
    buffer[idx++] = (DNS_QCLASS_FIELD) & 0x00FF;

    *index = idx;
}

static int jdns_name_parse(uint8_t *packet, int length, uint32_t *idx, char *name_out, int name_out_len)
{
    int name_end = -1;
    uint32_t j = *idx;
    int ptr_count = 0;

    char *cp = name_out;
    const char *const end = name_out + name_out_len;

    /* Normally, names are a series of length prefixed strings terminated */
    /* with a length of 0 (the lengths are u8's < 63). */
    /* However, the length can start with a pair of 1 bits and that */
    /* means that the next 14 bits are a pointer within the current */
    /* packet. */

    for (;;) {
        uint8_t label_len = packet[j++];
        if (!label_len) {
            break;
        }

        if (label_len & 0xc0) {
            uint8_t ptr_low = packet[j++];
            if (name_end < 0) {
                name_end = j;
            }
            j = (((int)label_len & 0x3f) << 8) + ptr_low;
            /* Make sure that the target offset is in-bounds. */
            if (j < 0 || j >= length) return -1;
            /* If we've jumped more times than there are characters in the
             * message, we must have a loop. */
            if (++ptr_count > length) return -1;
            continue;
        }
        if (label_len > 63) return -1;
        if (cp != name_out) {
            if (cp + 1 >= end) return -1;
            *cp++ = '.';
        }
        if (cp + label_len >= end) return -1;
        if (j + label_len > length) return -1;
        memcpy(cp, packet + j, label_len);
        cp += label_len;
        j += label_len;
    }
    if (cp >= end) return -1;
    *cp = '\0';
    if (name_end < 0)
        *idx = j;
    else
        *idx = name_end;
    return 0;
}

static int jdns_response_message(uint8_t buffer[1024], uint32_t buffer_len, char dns_ip_list[][16], char *ip[JDNS_RESULT_COUNT])
{
    uint32_t idx = 0, rd_len = 0, dns_count = 0;

    //transaction id/flags4/questions nums/answer RRS nums/Authority RRs nums/Additional RRs nums
    if (idx + 12 >= buffer_len) {
        return -1;
    }
    //ignore transaction id/flags4
    idx += 4;

    /*
     * parse length
     * */
    uint16_t questions = buffer[idx++] * 256;
    questions += buffer[idx++];
    uint16_t answer = buffer[idx++] * 256;
    answer += buffer[idx++];
    idx+= 2;//authority
    idx+=2;//additional

    //questions group
    for (uint16_t i = 0; i < questions; i++) {
        while(buffer[idx] != 0x00) {
            idx += buffer[idx] + 1;
        }

        if (idx > buffer_len) {
            return -1;
        }

        idx += 1 + 4;
    }

    memset(dns_ip_list, 0, JDNS_RESULT_COUNT * 16);

    //answers group
    for (uint16_t i = 0; i < answer; i++) {
        char tmp_name[256] = {0};
        if (jdns_name_parse((uint8_t *)buffer, buffer_len, &idx, tmp_name, 256) < 0) {
            return -1;
        }
        //type, 2 bytes
        idx += 2;
        //class, 2 bytes
        idx += 2;
        //TTL, 4bytes
        idx += 4;

        // data length
        rd_len = (buffer[idx++] * 256);
        rd_len += buffer[idx++];

        if ((idx  + rd_len) > buffer_len) {
            return -1;
        }

        if (rd_len == 4) {
            if (dns_count < JDNS_RESULT_COUNT) {
                jdns_uint2str(&buffer[idx], dns_ip_list[dns_count]);
                ip[dns_count] = dns_ip_list[dns_count];
                dns_count++;
            }else{
                break;
            }
        }

        idx += rd_len;
    }

    if (dns_count == 0) {
        return -1;
    }

    return 0;
}

static int jdns_resolve(char dns[16], const char *domain, char dns_ip_list[][16], char *ip[JDNS_RESULT_COUNT])
{
    static int tick = 0;
    int res = 0, sock_fd = 0;
    struct sockaddr_in dest;
    uint32_t dest_len = 0;
    uint8_t send_message[1024] = {0}, recv_message[1024] = {0};
    uint32_t idx = 0;
    fd_set send_recv_sets;
    struct timeval timeselect;

    res = jdns_domain_check(domain);
    if (res < 0) {
        if (strlen(domain) >= 16) {
            return -1;
        }
        memcpy(dns_ip_list[0], domain, strlen(domain));
        ip[0] = dns_ip_list[0];
        return 0;
    }

    sock_fd = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
    if (sock_fd < 0) {
        perror("dns socket: ");
        return -1;
    }

    dest.sin_family = AF_INET;
	dest.sin_port = htons(53);
	dest.sin_addr.s_addr = inet_addr(dns);

    /* set select timeout */
    timeselect.tv_sec = 1 + (++tick) % 3;
    timeselect.tv_usec = 0;

    /* dns request message */
    jdns_request_message(domain, send_message, &idx);

    /* send to dns server */
    FD_ZERO(&send_recv_sets);
    FD_SET(sock_fd, &send_recv_sets);
    res = select(sock_fd + 1, NULL, &send_recv_sets, NULL, &timeselect);
    if (res <= 0) {
        close(sock_fd);
        return -1;
    }
    if (FD_ISSET(sock_fd, &send_recv_sets)) {
        if ((res = sendto(sock_fd, (void *)send_message, (size_t)idx, 0, (struct sockaddr*)&dest, sizeof(dest))) < 0) {
            perror("send dns request message failed: ");
            close(sock_fd);
            return -1;
        }

        if (res != idx) {
            close(sock_fd);
            return -1;
        }
    }

    /* recv from dns server */
    FD_ZERO(&send_recv_sets);
    FD_SET(sock_fd, &send_recv_sets);
    res = select(sock_fd + 1, &send_recv_sets, NULL, NULL, &timeselect);
    if (res <= 0) {
        close(sock_fd);
        return -1;
    }
    if (FD_ISSET(sock_fd, &send_recv_sets)) {
        dest_len = sizeof(dest);
        if ((res = recvfrom(sock_fd, (void *)recv_message, 1024, 0, (struct sockaddr*)&dest, &dest_len)) < 0) {
            perror("send dns request message failed: ");
            close(sock_fd);
            return -1;
        }
    }

    close(sock_fd);

    return jdns_response_message(recv_message, res, dns_ip_list, ip);
}

/*
 * 原版使用了静态变量 g_dns_ip_list，修改为 dns_ip_list 可重入版本
 **/
int jdns_getaddrinfo(const char *domain, char dns_ip_list[][16], char *ip[JDNS_RESULT_COUNT])
{
    static char *g_dns_server_list[DNS_SERVER_COUNT] = {
        "223.5.5.5",
        "223.6.6.6",
        "8.8.8.8"
    };

    int res = 0;
    uint8_t idx = 0;

    for (idx = 0; idx < DNS_SERVER_COUNT; idx++) {
        res = jdns_resolve(g_dns_server_list[idx], domain, dns_ip_list, ip);
        if (res < 0) {
            printf("send dns[%d] request message failed: Network is unreachable %s\n", idx, domain);
            continue;
        } else {
            printf("[prt] dns server: %s\n", g_dns_server_list[idx]);
            return 0;
        }
    }
    return -1;
}

/*
 * RETURN
 * 成功: 输入的参数 ip
 * 失败: NULL
 **/
char *get_ip_by_domain(const char *domain, char *ip, size_t len)
{
    char dns_ip_list[JDNS_RESULT_COUNT][16] = {{0}};
    char *ips[JDNS_RESULT_COUNT] = {0};

    jdns_getaddrinfo(domain, dns_ip_list, ips);

    if (ips[0] != NULL) {
        strncpy(ip, ips[0], len - 1);
        return ip;
    } else {
        return NULL;
    }
}
