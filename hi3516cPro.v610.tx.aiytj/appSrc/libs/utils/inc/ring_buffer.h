/*
 *       Filename:  ring_buffer.h
 *    Description:  
 *        Version:  1.0
 *        Created:  01/13/2026 08:50:45 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H
#ifdef __cplusplus 
extern "C" {
#endif

//request dst_total >= bytes_to_copy
int dst_ringbuffer_memcpy(char *dst_data, size_t *dst_used, size_t dst_total,
                          char *src_data, size_t bytes_to_copy);

//request src_total >= bytes_to_copy
int src_ringbuffer_memcpy(char *dst_data, char *src_data, size_t *src_used,
                          size_t src_total, size_t bytes_to_copy);

#ifdef __cplusplus
}
#endif
#endif
