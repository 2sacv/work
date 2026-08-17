/*
 *       Filename:  aging8h.h
 *    Description:  
 *        Version:  1.0
 *        Created:  12/30/2025 09:34:08 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xuyx (), 
 *   Organization:  
 */
#ifndef __AGING8H_H
#define __AGING8H_H
#ifdef __cplusplus
extern "C" {
#endif

#define BURNED_SIGN  (10)

int get_aging8h(void);
int get_aging8h_pass(void);
void set_aging8h(void);

#ifdef __cplusplus
}
#endif
#endif
