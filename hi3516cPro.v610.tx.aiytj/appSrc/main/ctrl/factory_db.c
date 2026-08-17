/*
 *       Filename:  db.c
 *    Description:
 *        Version:  1.0
 *        Created:  03/16/2017 11:41:35 AM
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
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "factory_db.h"
#include "debug.h"
#include "our_md5.h"
#include "base64.h"
#include "utils.h"

#define MD5_DIGEST_LENGTH 16
struct db_info {
    int total;
    int used;
    FILE *fp;
};

static struct db_info db = {0};

int init_sdcard()
{
    struct stat st;

    if (-1 == stat(FACTORY_DB_DEV, &st)) {
        DBG("%s not exist\n", FACTORY_DB_DEV);
        return -1;
    }

    if (-1 == stat(FACTORY_DB_PATH, &st)) {
        DBG("%s not exist\n", FACTORY_DB_PATH);
        return -1;
    }

    if (db.fp == NULL) {
        db.fp = fopen(FACTORY_DB_FILE, "w+");
    }

    return 0;
}

int quit_sdcard()
{
    if (db.fp) {
        fclose(db.fp);
        db.fp = 0;
    }
    return 0;
}

int stat_sdcard(int *total, int *used)
{
    int ret;

    if (db.fp == NULL) {
        db.fp = fopen(FACTORY_DB_FILE, "rb+");

        if (db.fp == NULL) {
            DBG("open %s fail\n", FACTORY_DB_FILE);
            return -1;
        }
    }

    if (db.total == 0 && db.used == 0) {
        fseek(db.fp, 0, SEEK_SET);
        ret = fread(&db, 8, 1, db.fp);
        if (ret != 1) {
            DBG("fread error\n");
            return -1;
        }
    }

    *total = db.total;
    *used  = db.used;

    DBG("total: %d used: %d\n", db.total, db.used);

    fseek(db.fp, 8+sizeof(struct record)*db.used, SEEK_SET);

    return 0;
}

int index_sdcard(const char *index)
{
    if (db.fp == NULL) {
        db.fp = fopen(FACTORY_DB_FILE, "rb+");

        if (db.fp == NULL) {
            DBG("open %s fail\n", FACTORY_DB_FILE);
            return -1;
        }
    }

    sscanf(index, "%d", &db.used);
    printf("reset used.index to %s\n", index);

    return pack_sdcard();
}

int pack_sdcard()
{
    int ret;

    fseek(db.fp, 0, SEEK_SET);
    ret = fwrite(&db, 8, 1, db.fp);
    if (ret == 1) {
        DBG("pack_sdcard succ @total:%d @used:%d\n", db.total, db.used);
        return 0;
    }

    DBG("pack_sdcard fail\n");
    return -1;
}

int fetch_record(struct record *rec)
{
    int ret;

    if (!db.fp) {
        DBG("fp is NULL\n");
    }

    ret = fread(rec, sizeof(*rec)-sizeof(rec->wifimac), 1, db.fp);
        if (ret == 1) {
        rec->product_key[PRODUCT_KEY_LENGTH-1]       = '\0';
        rec->device_name[DEVICE_NAME_LENGTH-1]       = '\0';
        rec->device_secret[DEVICE_SECRET_LENGTH-1]   = '\0';
        rec->product_secret[PRODUCT_SECRET_LENGTH-1] = '\0';
        rec->aliid_md5sum[LEN_MD5]                   = '\0';
        rec->devid[LEN_ID]                           = '\0';
        return 0;
    }

    DBG("fetch_record error\n");
    return -1;
}

int use_record(const char *wifimac)
{
    int ret;
    struct record rec;

    //printf("_______________skip ++ \n");
    db.used++;

    // depend on last read
    ret = fwrite(wifimac, sizeof(rec.wifimac), 1, db.fp);
    if (ret == 1) {
        DBG("use_record with hwadd:%s succ\n", wifimac);
    } else {
        return -1;
    }

    return pack_sdcard();
}

#if 0
int decrypt_record(const char *key, struct record *rec, struct record *rec2)
{
    int  i                = 0;
    int  ret              = 0;
    int  ali_len          = 0;
    char bzero_32[72]     = {0};
    char md5sum[64]       = {0};
    char id_buf[256]      = {0};
    char ali_conf[1024]   = {0};
    unsigned char md[MD5_DIGEST_LENGTH] = {0};
    MD5_CTX_OUR ctx;

    if ((NULL == key)||(NULL == rec)||(NULL == rec2))
    {
        DBG("decrypt_record param error\n");
        return -1;
    }
    if((strlen(rec->product_key)<=0) || (strlen(rec->device_name)<=0)
        || (strlen(rec->device_secret)<=0) || (strlen(rec->product_secret)<=0)){
        DBG("one of the size of four paramS is 0 error !\n");
        return -1;
    }
    /*the size of product_secret may be 0! only the size is bigger than 0 ,then decrypt it!*/


    str_decrypt_tfid(rec2->product_key,    rec->product_key,    strlen(rec->product_key),    key);
    str_decrypt_tfid(rec2->device_name,    rec->device_name,    strlen(rec->device_name),    key);
    str_decrypt_tfid(rec2->device_secret,  rec->device_secret,  strlen(rec->device_secret),  key);
    str_decrypt_tfid(rec2->product_secret, rec->product_secret, strlen(rec->product_secret), key);

    str_decrypt_tfid(rec2->aliid_md5sum,   rec->aliid_md5sum, LEN_MD5, key);
    str_decrypt_tfid(rec2->devid,          rec->devid,        LEN_ID,  key);

    sprintf(id_buf, "%s%s%s%s", rec2->product_key, rec2->device_name, rec2->device_secret,rec2->product_secret);
    DBG("id_buf %s  %d",  id_buf,strlen(id_buf));
    base64decode(ali_conf, &ali_len, id_buf,strlen(id_buf));

    our_MD5Init(&ctx);
    ourMD5Update(&ctx, (unsigned char *)ali_conf, (unsigned long)ali_len);
    our_MD5Final(md, &ctx);

    for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(md5sum+i*2, "%02x", md[i]);
    }


    ret = strncmp(rec2->aliid_md5sum, md5sum, 32);
    if (ret != 0) {
        SYSLOG("md5 check fail, aliid_md5sum is %s, check is %s \n",rec2->aliid_md5sum, md5sum);
        LOG("md5 fail, aliid_md5sum: %s, check: %s \n", rec2->aliid_md5sum, md5sum);
        return -1;
    }

    memset(bzero_32,'0',strlen(rec2->product_secret));
    if(!strncmp(rec2->product_secret, bzero_32, strlen(rec2->product_secret))){
        memset(rec2->product_secret,0,sizeof(rec2->product_secret));
        DBG("the product_key is NULL!");
    }


    return ret;
}
#endif

int get_key(char *key)
{
    if (access("/tmp/cid.txt", F_OK) != 0) {
        system("/ipc/bin/lzbox cid /tmp/cid.txt");
        system("ifconfig wlan0 up");
        sleep(1);
    }

    int ret;
    FILE *f = NULL;

    f = fopen(F_CID_TEMP, "r+");
    if (NULL == f) {
        DBG("fopen %s fail\n", F_CID_TEMP);
        return -1;
    }

    ret = fread(key, LEN_MD5, 1, f);
    if (ret != 1) {
        DBG("fread error\n");
        fclose(f);
        return -1;
    }

    key[LEN_MD5] = '\0';
    DBG("succ read key: %s\n", key);
    fclose(f);
    return 0;
}

