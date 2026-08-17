/* 
 *       Filename:  ring_buffer.c
 *    Description:  
 *        Version:  1.0
 *        Created:  01/13/2026 08:50:33 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */
#include <string.h>
#include "debug.h"

#define RING_BUFFER_ADD(val, num, size_of_buffer) do { \
    val %= (size_of_buffer);                           \
    val += (num);                                      \
    if (val > (size_of_buffer)) {                      \
        val %= (size_of_buffer);                       \
    }                                                  \
} while(0)

#define RING_BUFFER_SUB(val, num, size_of_buffer) do { \
    if (val - (num) >= 0) {                            \
        val -= (num);                                  \
    } else {                                           \
        val = (size_of_buffer) - ((num) - val);        \
    }                                                  \
} while(0)

int dst_ringbuffer_memcpy(char *dst_data, size_t *dst_used, size_t dst_total,
                          char *src_data, size_t bytes_to_copy)
{
    size_t dst_left = dst_total - *dst_used;
    size_t copy_left = 0;
    int ret = 0;

    goto_exit_if_fail(NULL != dst_data, exit, ret = -1, "dst_data is null\n");
    goto_exit_if_fail(NULL != dst_used, exit, ret = -1, "dst_used is null\n");
    goto_exit_if_fail(dst_total > 0, exit, ret = -1, "dst_total is <= 0\n");
    goto_exit_if_fail(NULL != src_data, exit, ret = -1, "src_data is null\n");
    goto_exit_if_fail(bytes_to_copy > 0, exit, ret = -1, "bytes_to_copy is <= 0\n");

    if (dst_left >= bytes_to_copy) {
        memcpy(&dst_data[*dst_used], src_data, bytes_to_copy);
    } else {
        if (dst_left > 0) {
            memcpy(&dst_data[*dst_used], src_data, dst_left);
        }
    
        copy_left = bytes_to_copy - dst_left;
        memcpy(dst_data, &src_data[dst_left], copy_left);
    }

    RING_BUFFER_ADD(*dst_used, bytes_to_copy, dst_total);
    ret = 0;

exit:

    return ret;
}

int src_ringbuffer_memcpy(char *dst_data, char *src_data, size_t *src_used,
                          size_t src_total, size_t bytes_to_copy)
{
    size_t src_left = src_total - *src_used;
    size_t copy_left = 0;
    int ret = 0;

    goto_exit_if_fail(NULL != dst_data, exit, ret = -1, "dst_data is null\n");
    goto_exit_if_fail(NULL != src_data, exit, ret = -1, "src_data is null\n");
    goto_exit_if_fail(NULL != src_used, exit, ret = -1, "src_used is null\n");
    goto_exit_if_fail(src_total > 0, exit, ret = -1, "src_total is <= 0\n");
    goto_exit_if_fail(bytes_to_copy > 0, exit, ret = -1, "bytes_to_copy is <= 0\n");

    if (src_left >= bytes_to_copy) {
        memcpy(dst_data, &src_data[*src_used], bytes_to_copy);
    } else {
        if (src_left > 0) {
            memcpy(dst_data, &src_data[*src_used], src_left);
        }
    
        copy_left = bytes_to_copy - src_left;
        memcpy(&dst_data[src_left], src_data, copy_left);
    }

    RING_BUFFER_ADD(*src_used, bytes_to_copy, src_total);
    ret = 0;

exit:

    return ret;
}
