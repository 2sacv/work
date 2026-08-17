/* 
 *       Filename:  tunnel.c
 *    Description:  
 *        Version:  1.0
 *        Created:  12/16/2017 05:19:32 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (), 
 *   Organization:  
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* file */
#include <fcntl.h>
#include <sys/file.h>

/* socket() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.h"
#include "tunnel.h"
#include "jcpService.h"
#include "system_ctrl.h"

#define TAG_sh      "sh "
#define TAG_jcp     "jcp "
#define TAG_fget    "fget "

/* 
 * cmd   :  in , command
 * len   :  in , command len
 * resp  :  out, resp buf
 * RETVAL:  strlen(resp) and a append '\0' when not TAG_fget
 * */
int tunn_process(const char *cmd, int len, char *resp, int resp_len, int *fget)
{
    int nr = -1;
    *fget = FALSE;

    if (0 == memcmp(cmd, TAG_sh, strlen(TAG_sh))) {
        DBG("----------\n");
        nr = ReadCmdResult(cmd+strlen(TAG_sh), resp, resp_len-1);
        if (nr == FAILURE) {
            nr = snprintf(resp, resp_len-1, "[%s] vpopen fail\n", cmd+strlen(TAG_sh));
        }
		DBG("req_cmd:%s\n", cmd+strlen(TAG_sh));
        resp[nr] = '\0';
        return (nr+1);
    } else if (0 == memcmp(cmd, TAG_jcp, strlen(TAG_jcp))) {
        DBG("----------\n");
        jcpcmd_sendrecv((char *)(cmd+strlen(TAG_jcp)), resp, resp_len);
        nr = strlen(resp);
        resp[nr] = '\0';
        return (nr+1);
    } else if (0 == memcmp(cmd, TAG_fget, strlen(TAG_fget))) {
        DBG("----------\n");
        char file[128] = {0}; 
        int bytes; 

        sscanf(cmd+strlen(TAG_fget), "%100s", file);
        bytes = bytes_of_file(file);

        if (bytes < 0) {
            return 1+sprintf(resp, "UNKNOW file[%s]\n", cmd+strlen(TAG_fget));
        }

		if (bytes > resp_len) {
			return 1+sprintf(resp, "NOT BUF TO RESP [%s]\n", cmd+strlen(TAG_fget));
		}
		
        if (LoadFile(file, resp, bytes) < 0) {
            return 1+sprintf(resp, "UNKNOW file[%s]\n", cmd+strlen(TAG_fget));
        }
        
        *fget = TRUE;
        return bytes;
    } else {
        DBG("----------\n");
        return 1+sprintf(resp, "UNKNOW cmd[%s]\n"
                             "Usage:\n" 
                             "\t{sh|jcp} command\n"
                             "\tfget filepath\n", 
                             cmd);
    }
}

/**
 * strnstr - Find the first substring in a %NUL terminated string
 * @s1: The string to be searched
 * @s2: The string to search for
 * @n : 在s1的前n字符中查找
 * add by zhangj
 **/
char* strnstr(char* s1, char* s2, int n)
{
    int len1, len2;

    len2 = strlen(s2);
    if (!len2) {
        return (char *)s1;
    }

    len1 = strlen(s1);
    if (len2 > len1) {
        return NULL;
    }

    while (len1 >= len2) {
        if (!memcmp(s1, s2, len2))
            return (char *)s1;
        s1++;
        len1--;
    }
    return NULL;
}
