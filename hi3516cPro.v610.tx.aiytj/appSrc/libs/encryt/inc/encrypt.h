/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2014-03-19
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _encrypt_H_
#define _encrypt_H_
#ifdef __cplusplus
extern "C" {
#endif

    int encrypt_file(char* filename);
    int de_encrypt_file(const char *filename);
    int en_de_crypt_file(char *filename);
    int check_id_file(char *id, char *filename);

#ifdef __cplusplus
}
#endif
#endif