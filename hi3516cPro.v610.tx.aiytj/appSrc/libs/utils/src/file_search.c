/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : file_search.c
 * Created Time : 2014-03-25
 * Version      : 1.0
 * Author       : tangpengcheng
 * Description  :
 */
 
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>

#include "file_search.h"
#include "stdio.h"
#include "debug.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


typedef struct search_handle_s
{
    DIR * curdir[256];
    int dir_num;
    char path[PATH_MAX];
    int path_len;
    char  file_ext[16];
    char recordtype;
    char bnametime;
    struct tm * start_tm;
    struct tm * end_tm;
}search_handle_t;


int file_search_init(search_handle * handle, search_opt_t *opt)
{
    if(!opt->path){
        DBG("search path error\n");
        return -1;
    }

    *handle = calloc(1, sizeof(search_handle_t));
    if(!*handle) {
        DBG("calloc error\n");
        return -1;
    }
    search_handle_t* hd = (search_handle_t*)*handle;
    hd->start_tm = opt->start_time;
    hd->end_tm = opt->end_time;     

    strcpy(hd->path, opt->path);
    if(hd->path[strlen(hd->path) -1] == '/'){
        hd->path[strlen(hd->path) -1] = 0;
    }
    hd->path_len = strlen(hd->path);

    DIR * filedir = opendir(hd->path);
    if(!filedir) {      
        DBG("open dir fails, %s\n", strerror(errno));
        return -1;
    }

    hd->recordtype = opt->recordtype;
    hd->bnametime = opt->bnametime;
    
    if(opt->file_ext)
        strcpy(hd->file_ext, opt->file_ext);
    hd->curdir[hd->dir_num ++] = filedir;       
    return 0;
}

int record_file_search(search_handle handle, file_info_t *file_info, int req_num, int* ret_num)
{
    int file_no = 0;
    search_handle_t* hd = (search_handle_t*)handle;
    if(!hd) {
        DBG("handle error\n");
        return -1;
    }
    
    struct timeval tmval;
    struct timezone tz;
    gettimeofday(&tmval, &tz);

    *ret_num = 0;
    struct dirent *ent, local_entry;
    char * str;
    struct stat st;
    char filepath[PATH_MAX];

    time_t start_time = 0;
    time_t end_time = 0;

    if(hd->start_tm) {
        start_time = mktime(hd->start_tm);      
    }

    if(hd->start_tm) {
        end_time = mktime(hd->end_tm);      
    }
    
    while(hd->dir_num) {
        //DBG("enter path : %s\n", hd->path);
        while(1) {
            if (0 != readdir_r(hd->curdir[hd->dir_num - 1], &local_entry, &ent)) {
                ent = NULL;
            }
            if (NULL == ent) {
                break;
            }
            
            sprintf(filepath, "%s/%s", hd->path, ent->d_name);
            if(stat(filepath, &st) != SUCCESS) {
                DBG("stat :%s error:%s\n", filepath, strerror(errno));
                continue;
            }
            
            if(S_ISDIR(st.st_mode)) {     
                if(strcmp(ent->d_name, ".")==0 || strcmp(ent->d_name, "..") == 0)  
                    continue;

                struct tm currentTM = {0};
                int iRet = sscanf(ent->d_name, "%4d%2d%2d", &currentTM.tm_year, 
                                    &currentTM.tm_mon, &currentTM.tm_mday);
                currentTM.tm_year -= 1900;
                currentTM.tm_mon --;
                if(3 == iRet && hd->bnametime) {
                    if(hd->start_tm) {
                        struct tm TM = {0};
                        TM.tm_year = hd->start_tm->tm_year;                     
                        TM.tm_mon = hd->start_tm->tm_mon;
                        TM.tm_mday = hd->start_tm->tm_mday;

                        if(mktime(&TM) < mktime(&currentTM))
                            continue;
                    }
                    
                    if(hd->end_tm) {                        
                        struct tm TM = {0};
                        TM.tm_year = hd->end_tm->tm_year;                       
                        TM.tm_mon = hd->end_tm->tm_mon;
                        TM.tm_mday = hd->end_tm->tm_mday;
                        
                        if(mktime(&TM) > mktime(&currentTM))
                            continue;
                    }
                }               

                sprintf(hd->path + strlen(hd->path), "/%s", ent->d_name); 
                //DBG("open path:%s\n", hd->path);
                hd->curdir[hd->dir_num ++] = opendir(hd->path);
                if(!hd->curdir[hd->dir_num - 1]) {      
                    DBG("open dir fails, %s\n", strerror(errno));
                    -- hd->dir_num;
                    continue;
                }
            } else if (S_ISREG(st.st_mode)) {
                char* pStrFind = strstr(ent->d_name, ".tmp");
                if(pStrFind)
                    continue;

                char type = '\0';
                char fileExtName[4] = {0};
                struct tm currentTM;
                int iRet = sscanf(ent->d_name, "%c-%2d%2d%2d.%3s", &type, &currentTM.tm_hour, 
                                    &currentTM.tm_min, &currentTM.tm_sec, fileExtName);
                if(5 != iRet)
                    continue;

                if(0 != hd->file_ext[0] && strcmp(hd->file_ext, fileExtName))               
                    continue;

                if(hd->recordtype && tolower(type) != tolower(hd->recordtype))
                    continue;
                
                if (strstr(hd->path, "samba") != NULL) {
                    st.st_mtime += tz.tz_minuteswest;
                }
                
                if(hd->bnametime) { 
                    if(hd->start_tm && currentTM.tm_hour*3600+currentTM.tm_min*60+currentTM.tm_sec < 
                        hd->start_tm->tm_hour*3600+hd->start_tm->tm_min*60+hd->start_tm->tm_sec)
                        continue;
                    
                    if(hd->end_tm && currentTM.tm_hour*3600+currentTM.tm_min*60+currentTM.tm_sec > 
                        hd->end_tm->tm_hour*3600+hd->end_tm->tm_min*60+hd->end_tm->tm_sec)
                        continue;
                } else {

                    //if(0 != start_time && st.st_mtime < start_time)
                    //  continue;
                    
                    if(0 != end_time && st.st_mtime > end_time)
                        continue;
                }

                file_no = (*ret_num)++;
                //DBG("%d, %s, atime: %u, mtime: %u, ctime: %u, size: %u\n", 
                //  file_no, ent->d_name, st.st_atime, st.st_mtime, st.st_ctime, st.st_size);
                
                strcpy(file_info[file_no].file_name, filepath + hd->path_len);
                file_info[file_no].start_time = st.st_mtime;
                file_info[file_no].len = st.st_ctime - st.st_mtime;
                if (strstr(ent->d_name, "A")) {
                    file_info[file_no].type = 1;
                }
                if(!--req_num)
                    return 1;
            }
        }

        //DBG("exit path : %s\n", hd->path);
        closedir(hd->curdir[--hd->dir_num]);
        hd->curdir[hd->dir_num] = NULL;
        str = strrchr(hd->path, '/');
        *str = 0;
    }

    return 0;
}

int file_search(search_handle handle, char file_data[][MP4FILE_NAME_MAX], int req_num, int* ret_num)
{
    search_handle_t* hd = (search_handle_t*)handle;
    if(!hd) {
        DBG("handle error\n");
        return -1;
    }
    
    struct timeval tmval;
    struct timezone tz;
    gettimeofday(&tmval, &tz);

    *ret_num = 0;
    struct dirent *ent, local_entry;
    char * str;
    struct stat st;
    char filepath[PATH_MAX];

    time_t start_time = 0;
    time_t end_time = 0;

    if(hd->start_tm) {
        start_time = mktime(hd->start_tm);      
    }

    if(hd->start_tm) {
        end_time = mktime(hd->end_tm);      
    }

    while(hd->dir_num) {
        //DBG("enter path : %s\n", hd->path);
        while(1) {
            if (0 != readdir_r(hd->curdir[hd->dir_num - 1], &local_entry, &ent)) {
                ent = NULL;
            }
            if (NULL == ent) {
                break;
            }
            
            sprintf(filepath, "%s/%s", hd->path, ent->d_name);
            if(stat(filepath, &st) != SUCCESS) {
                DBG("stat :%s error:%s\n", filepath, strerror(errno));
                continue;
            }
            
            if(S_ISDIR(st.st_mode)) {     
                if(strcmp(ent->d_name, ".")==0 || strcmp(ent->d_name, "..") == 0)  
                    continue; 

                sprintf(hd->path + strlen(hd->path), "/%s", ent->d_name); 
                //DBG("open path:%s\n", hd->path);
                hd->curdir[hd->dir_num ++] = opendir(hd->path);
                if(!hd->curdir[hd->dir_num - 1]) {      
                    DBG("open dir fails, %s\n", strerror(errno));
                    -- hd->dir_num;
                    continue;
                }
            } else if (S_ISREG(st.st_mode)) {
                char* pStrFind = strstr(ent->d_name, ".tmp");
                if(pStrFind)
                    continue;

                if(hd->file_ext[0] != 0) {                  
                    pStrFind = strstr(ent->d_name, hd->file_ext);
                    if(!pStrFind)
                        continue;
                }
                
                if (strstr(hd->path, "samba") != NULL) {
                    st.st_mtime += tz.tz_minuteswest;
                }
                
                if(0 != start_time && st.st_mtime < start_time)
                    continue;

                if(0 != end_time && st.st_mtime > end_time)
                    continue;
                
                //DBG("%s\n", ent->d_name);
                strcpy(file_data[(*ret_num)++], filepath + hd->path_len);
                if(!--req_num)
                    return 1;
            }
        }

        //DBG("exit path : %s\n", hd->path);
        closedir(hd->curdir[--hd->dir_num]);
        hd->curdir[hd->dir_num] = NULL;
        str = strrchr(hd->path, '/');
        *str = 0;
    }

    return 0;

}


int file_search_release(search_handle * handle)
{
    search_handle_t* hd = (search_handle_t*)*handle;
    if(!hd) {
        DBG("handle error\n");
        return -1;
    }

    int i = 0;
    for(; i < hd->dir_num; ++i) {
        if(hd->curdir[i])
            closedir(hd->curdir[i]);
    }
    if (*handle)
        free(*handle);
    *handle = NULL;
        
    return 0;
}


