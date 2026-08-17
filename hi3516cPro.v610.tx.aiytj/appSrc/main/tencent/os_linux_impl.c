/*
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright (C) 2018-2020 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <memory.h>
#include <netpacket/packet.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "iv_sys.h"

// #define SYS_MEM_STRACCE

#ifdef SYS_MEM_STRACCE
typedef struct {
    void *p;
    size_t size;
} memory_item_t;

static memory_item_t memory_item[1024] = {0};
static size_t malloc_size              = 0;
#endif

static void *os_linux_malloc(size_t size)
{
    void *p = malloc(size);

    if (p) {
        memset(p, 0, size);
    }

    return p;
}

static void os_linux_free(void *ptr)
{
    if (ptr) {
#ifdef SYS_MEM_STRACCE

        for (int i = 0; i < 1024; i++) {
            if (memory_item[i].p == ptr) {
                malloc_size -= memory_item[i].size;
                printf("free %p %zu\n", ptr, memory_item[i].size);

                memory_item[i].p    = 0;
                memory_item[i].size = 0;

                printf("->  %zu\n", malloc_size);
                break;
            }
        }
#endif

        free(ptr);
    }
}

void MallocList()
{
#ifdef SYS_MEM_STRACCE

    printf("mem report:\n");

    for (int i = 0; i < 1024; i++) {
        if (memory_item[i].p != 0) {
            printf("\tptr %p %zu\n", memory_item[i].p, memory_item[i].size);
        }
    }

#endif
}

static void os_linux_print(const char *fmt, va_list ap)
{
    vprintf(fmt, ap);
    fflush(stdout);
}

static uint32_t os_linux_gettime_ms(void)
{
    struct timeval time_val = {0};
    uint32_t time_ms;

    gettimeofday(&time_val, NULL);
    time_ms = time_val.tv_sec * 1000 + time_val.tv_usec / 1000;

    return time_ms;
}

static uint32_t os_linux_gettime_second(void)
{
    return (uint32_t)time(NULL);
}

static void os_linux_msleep(uint32_t ms)
{
    usleep(1000 * ms);
}

static void os_linux_getmac(uint8_t *mac, size_t mac_len)
{
    struct ifaddrs *ifaddr = NULL;
    struct ifaddrs *ifa    = NULL;
    size_t i               = 0;

    if (getifaddrs(&ifaddr) == -1) {
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if ((ifa->ifa_addr) && (ifa->ifa_addr->sa_family == AF_PACKET)) {
            struct sockaddr_ll *s = (struct sockaddr_ll *)ifa->ifa_addr;

            /* skip loopback device */
            if (strcasecmp(ifa->ifa_name, "lo") == 0) {
                continue;
            }

            for (i = 0; i < s->sll_halen && i < mac_len; i++) {
                mac[i] = s->sll_addr[i];
            }
            break;
        }
    }

    freeifaddrs(ifaddr);
}

static size_t os_linux_get_disksize(void)
{
    struct statvfs st;
    statvfs("/bin", &st);

    return (st.f_frsize * st.f_blocks) / 1024;
}

static size_t os_linux_get_memsize(void)
{
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (meminfo == NULL) {
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), meminfo)) {
        int ram;
        if (sscanf(line, "MemTotal: %d kB", &ram) == 1) {
            fclose(meminfo);
            return ram;
        }
    }

    fclose(meminfo);
    return 0;
}

static const char *os_linux_get_platform(void)
{
    return "Linux";
}

static int os_linux_create_file(const char *path)
{
    return open(path, O_RDWR | O_CREAT, 0777);
}

static int os_linux_open_file(const char *path)
{
    return open(path, O_RDWR);
}

static size_t os_linux_read_file(int fd, void *buf, size_t count)
{
    return read(fd, buf, count);
}

static size_t os_linux_write_file(int fd, const void *buf, size_t count)
{
    return write(fd, buf, count);
}

static int os_linux_close_file(int fd)
{
    return close(fd);
}

void *os_linux_fopen(const char *filename, const char *mode)
{
    return (void *)fopen(filename, mode);
}

size_t os_linux_fread(void *ptr, size_t size, size_t nmemb, void *fp)
{
    return fread(ptr, size, nmemb, (FILE *)fp);
}

size_t os_linux_fwite(const void *ptr, size_t size, size_t nmemb, void *fp)
{
    return fwrite(ptr, size, nmemb, (FILE *)fp);
}

static size_t os_linux_fsize(void *fp)
{
    long size = 0;

    fseek((FILE *)fp, 0, SEEK_END);
    size = ftell((FILE *)fp);
    rewind((FILE *)fp);

    return size;
}

int os_linux_flose(void *fp)
{
    return fclose((FILE *)fp);
}

static long int os_linux_random(void)
{
    return random();
}

iv_sys_os_impl_t os_impl = {.hal_os_malloc         = os_linux_malloc,
                            .hal_os_free           = os_linux_free,
                            .hal_os_printf         = os_linux_print,
                            .hal_os_gettime_ms     = os_linux_gettime_ms,
                            .hal_os_gettime_second = os_linux_gettime_second,
                            .hal_os_sleepms        = os_linux_msleep,
                            .hal_os_getmac         = os_linux_getmac,
                            .hal_os_get_disksize   = os_linux_get_disksize,
                            .hal_os_get_memsize    = os_linux_get_memsize,
                            .hal_os_get_platform   = os_linux_get_platform,
                            .hal_os_create_file    = os_linux_create_file,
                            .hal_os_read_file      = os_linux_read_file,
                            .hal_os_write_file     = os_linux_write_file,
                            .hal_os_close_file     = os_linux_close_file,
                            .hal_os_fopen          = os_linux_fopen,
                            .hal_os_fread          = os_linux_fread,
                            .hal_os_fwrite         = os_linux_fwite,
                            .hal_os_filesize       = os_linux_fsize,
                            .hal_os_fclose         = os_linux_flose,
                            .hal_os_random         = os_linux_random};
