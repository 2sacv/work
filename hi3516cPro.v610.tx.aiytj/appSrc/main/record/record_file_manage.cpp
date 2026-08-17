/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2014-04-02
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/statfs.h>

#include "debug.h"
#include "utils.h"
#include "js_scheduler.h"
#include "record_disk.h"
#include "record_file_manage.h"
#include "jconfig.h"
#include "jevent.h"
#include "record_dirent.h"

using namespace std;

static int sIsDeletingFiles = 0;

int generate_record_dir(const char *inPath)
{
    struct stat theStatBuffer;
	int ret = 0;

	do{
		ret = stat(inPath, &theStatBuffer);
		if(ret == 0){
			//directory exists, break to return
			break;
		}
		
		//this directory doesn't exist, so let's try to create it
		ret = mkdir(inPath, S_IRWXU |S_IRGRP | S_IXGRP|S_IROTH | S_IXOTH);
	}while(0);

    return ret;
}

//  规则A：
//  IPCamera/20110429-0/A-102106.mp4
//  前缀表示
//  “A-“  前缀表示报警输入和移动侦测录像、图片
//  “M-“  前缀表示手动录像、图片
//  “S-“  前缀表示定时录像
//  “V-“  声音告警录像
//  后缀表示
//  “mp4” 后缀表示录像文件
//由外部给定 utc 时间，确保各处使用的 utc 时间统一
int generate_record_filename(eJRecType rectype, char *filename, time_t start_utc)
{
    int ret = 0;

    time_t curTime = 0;
    struct tm curTm_s = {0};
    struct tm *curTm = &curTm_s;

    char devPath[128] = {0,};
    char filePath[256] = {0,};
    char *p = filePath;
	char chtype = 0;

	do{
	    ret = storage_get_mmcpath(devPath);
		if(ret < 0){
			break;
		}

	    curTime = start_utc;
	    // curTime = (curTime%900<=3) ? curTime-(curTime%900) : curTime; ? why ?
	    localtime_r(&curTime, &curTm_s);

		switch(rectype){
			case JREC_TYPE_ALARM:
				chtype = 'S';
				break;
			case JREC_TYPE_SCHEDULE:
				chtype = 'S';
				break;
			case JREC_TYPE_MANUAL:
				chtype = 'S';
				break;
			default:
				chtype = 0;
				break;
		}

		if(chtype == 0)
			break;

	    // mkdir "/mnt/IPCamera"
	    p += sprintf(p, "%s/IPCamera", devPath);
	    ret = generate_record_dir(filePath);
	    if(ret < 0) {
	        js_log( "mkdir %s error!\n", filePath);
	        break;
	    }

	    // mkdir "/mnt/20080729"
	    p += sprintf(p, "/%d%02d%02d", curTm->tm_year + 1900, curTm->tm_mon + 1, curTm->tm_mday);
	    ret = generate_record_dir(filePath);
	    if(ret < 0) {
	        js_log( "mkdir %s error!\n", filePath);
	        break;
	    }

	    p += sprintf(p, "/%c-%02d%02d%02d%s", chtype, curTm->tm_hour, curTm->tm_min, curTm->tm_sec, TMPFILE_SUFFIX);
	    strcpy(filename, filePath);		
			
	}while(0);

    return ret;
}


int is_removing_files(void)
{
    return sIsDeletingFiles;
}

static int sort_by_datestr(string &filestr1, string &filestr2)
{
	int ret;

	//A-091500 M-091500, compare skip A-/M-
	if(filestr1.length() >= 10 && filestr2.length() >= 10){
		ret = filestr1.compare(2, 8, filestr2, 2, 8);
	}
	else{
		ret = filestr1.compare(filestr2);
	}

	if(ret < 0)
		ret = 1;
	else
		ret = 0;

	return ret;
}

static int remove_by_files(const char *dirPath)
{
    DIR *pDir = NULL;
    struct dirent *pDirent = NULL;
    struct dirent dlocal = {0};
    struct stat st = {0};
    char fileName[256] = {0};
    int count = 0;

    vector<string> strvec;
    string strtmp;
    int ret = -1;

    js_log("______ remove_by_files @dir: %s\n", dirPath);

    do {
        pDir = opendir(dirPath);
        if (pDir == NULL) {
            js_log("opendir %s error: %s\n", dirPath, strerror(errno));
            break;
        }

        while(1) {
            if (0 != readdir_r(pDir, &dlocal, &pDirent)) {
                pDirent = NULL;
            }

            if (NULL == pDirent) {
                break;
            }

            if ('.' == pDirent->d_name[0]) {
                continue;
            }

            js_log("del %s/%s\n", dirPath, pDirent->d_name);
            snprintf(fileName, sizeof(fileName) - 1, "%s/%s", dirPath, pDirent->d_name);
            if (stat(fileName, &st) != 0) {
                continue;
            }

            if (S_ISDIR(st.st_mode)) {
                js_log("remove dir: %s in remove_by_files\n", fileName);
                remove_by_files(fileName);
            } else {
                strtmp = pDirent->d_name;
                strvec.push_back(strtmp);
            }
        }

        sort(strvec.begin(), strvec.end(), sort_by_datestr);
        vector<string>::iterator it = strvec.begin();
        for (; it != strvec.end(); ++it) {
            strtmp = *it;
            sprintf(fileName, "%s/%s", dirPath, strtmp.c_str());
            ret = remove(fileName);
            if (ret == -1) {
                SYSLOG("delete mp4 error: %s\n", fileName);
                //set_g_stat(record, SD_ERR_WRITE);
                //goto errout;
                continue;
            }

            js_log("remove file:%s ret:%d\n", fileName, ret);

            if ((count%10) == 0) {
                usleep(1000*1000);  // 防止 cpu 高，单目删双目的卡时，还是有点勉强
            }

            if (++count >= 20) {  //删 20 个录像判断一次
                sync();
                usleep(100*1000); // 100ms 让文件系统更新元数据
                if(is_storage_devpath_space_enough()) {
                    js_log("storage is enough\n");
                    break;
                }
                count = 0;
            }
        }

        if (it == strvec.end()) {
            ret = rmdir_recursive(dirPath);
            if (ret == -1) {
                char p[256] = {0};
                strcpy(p, dirPath);
                replace_str(p, "IPCamera/2", "IPCamera/err_2");
                ret = rename(dirPath, p);
                SYSLOG("rename dir %s to %s, ret:%d\n", dirPath, p, ret);
                ret = 0;
            } else {
                js_log("remove dir:%s ret:%d\n", dirPath, ret);
            }
        }

        vector<string>().swap(strvec);
    }while(0);

//errout:
    if(NULL != pDir) {
        closedir(pDir);
        pDir = NULL;
    }
	
    return ret;
}

int remove_dir_quick(const char *dir)
{
    sCache1File *f_mp4s = (sCache1File *)system_malloc(sizeof(sCache1File)*MAX_RECS_OF_DAY);
    sCache1File *p_mp4s[MAX_RECS_OF_DAY] = {NULL};

    int ret = 0;
    int num = 0;
    char abspath[256] = {0};

    do {
        for (int ii = 0; ii < ARRAY_SIZE(p_mp4s); ii++) {
            p_mp4s[ii] = &f_mp4s[ii];
        }

        int del_succ = 0;
        num = 0;
        lookupdir(dir, p_mp4s, MAX_RECS_OF_DAY, &num, REC_FILE_ALL);

        int i = 0;
        for (i = 0; num > 0 && i < num; i++) {
            snprintf(abspath, sizeof(abspath)-1, "%s/%s", dir, p_mp4s[i]->name);
            if (-1 == remove(abspath)) {
                SYSLOG("delete mp4 error: %s\n", abspath);
                continue;
            }

            if ((del_succ%10) == 0) {
                usleep(1000*1000); // 防止 cpu 高，单目删双目的卡时，还是有点勉强
            }

            if (++del_succ >= 20) {  //删 20 个录像判断一次
                printf("remove %s to %s total 20\n", p_mp4s[i-19]->name, abspath);
                if(is_storage_devpath_space_enough()) {
                    js_log("storage is enough\n");
                    goto __enough;
                }
                del_succ = 0;
            }
        }

        if (num > 0 && i == num) {
            printf("remove[%d] %s is %s ending\n", num-1, p_mp4s[num-1]->name, dir);
        }
    } while (num == MAX_RECS_OF_DAY);

    // 若 enough, 必走了上面的 goto
    ret = remove_by_files(dir);

__enough:
    if (f_mp4s) {
        free(f_mp4s);
    }
    return ret;
}

void remove_oldest_dir(void *data)
{
    char removeDir[256] = {0,};
    char dirPath[256] = {0,};
    int yyyymmdd = 0;

    DIR *pDir = NULL;
    struct dirent *pDirent = NULL;
    struct dirent dlocal = {0};
    int  err = -1;

    vector<string> strvec;
    string strtmp;
    char mmcpath[128] = {0};

    sIsDeletingFiles = 1;

	do{
	    err = storage_get_mmcpath(mmcpath);
		if(err < 0){
			break;
		}

		sprintf(dirPath, "%s/%s", mmcpath, "IPCamera");
	    pDir = opendir(dirPath);
	    if(pDir == NULL) {
	        js_log("opendir :%s error\n", dirPath);
			break;
		}

	    while(1) {
	        if(0 != readdir_r(pDir, &dlocal, &pDirent)) {
	            pDirent = NULL;
	        }
			
	        if(NULL == pDirent) {
	            break;
	        }
			
	        if('.' == pDirent->d_name[0]) {
	            continue;
	        }
			
	        if (strlen("20080808"/*format*/) != strlen(pDirent->d_name)
	                || (pDirent->d_name[0] != '1' && pDirent->d_name[0] != '2')) {
	            continue;
	        }

			strtmp = pDirent->d_name;
			strvec.push_back(strtmp);
	        usleep(10);
	    }		

        sort(strvec.begin(), strvec.end());
        vector<string>::iterator it = strvec.begin();
        for (; it != strvec.end(); ++it) {
            strtmp = *it;
            snprintf(removeDir, sizeof(removeDir) - 1, "%s/%s", dirPath, strtmp.c_str());
            js_log("goto removeDir:%s\n", removeDir);
            err = remove_dir_quick(removeDir);
            if (err == -1) {
                goto errout;
            }
            yyyymmdd = atoi(strtmp.c_str());
            send_event_chn(JEvent_AlarmRMRcord, yyyymmdd);

			if (is_storage_devpath_space_enough()) {
				js_log("storage is enough\n");
				break;
			}
		}
		
		if (it == strvec.end()) {
			err = rmdir_recursive(dirPath);
			js_log("remove dir:%s ret:%d\n", dirPath, err);
		}
		
		vector<string>().swap(strvec);
	}while(0);

errout:
    if (NULL != pDir) {
        closedir(pDir);
        pDir = NULL;
    }

	sIsDeletingFiles = 0;
	
}


int get_lastest_record_date(char *buf, int bufsize)
{
	int ret = -1;
	
    char dirPath[256] = {0,};

    DIR *pDir = NULL;
    struct dirent *pDirent = NULL;
    struct dirent dlocal = {0};
    int  err = -1;

	vector<string> strvec;
	string strtmp;
    char mmcpath[128] = {0};

	do{
		if(buf == NULL || bufsize < 8)
			break;
		
	    err = storage_get_mmcpath(mmcpath);
		if(err < 0){
			break;
		}

		sprintf(dirPath, "%s/%s", mmcpath, "IPCamera");
	    pDir = opendir(dirPath);
	    if(pDir == NULL) {
	        js_log("opendir :%s error\n", dirPath);
			break;
		}

	    while(1) {
	        if(0 != readdir_r(pDir, &dlocal, &pDirent)) {
	            pDirent = NULL;
	        }
			
	        if(NULL == pDirent) {
	            break;
	        }
			
	        if('.' == pDirent->d_name[0]) {
	            continue;
	        }
			
	        if (strlen("20080808"/*format*/) != strlen(pDirent->d_name)
	                || (pDirent->d_name[0] != '1' && pDirent->d_name[0] != '2')) {
	            continue;
	        }

			strtmp = pDirent->d_name;
			strvec.push_back(strtmp);
	        usleep(10);
	    }		

		sort(strvec.begin(), strvec.end());
		if(strvec.size()){
			strtmp = strvec.at(strvec.size()-1);
			strcpy(buf, strtmp.c_str());
			js_log("get record date:%s\n", buf);
			ret = 0;
		}else{
			ret = -1;
		}
		
		vector<string>().swap(strvec);

	}while(0);

    if(NULL != pDir) {
        closedir(pDir);
        pDir = NULL;
    }

	return ret;
}


int get_lastest_record_datetime(char *buf, int bufsize)
{
	char datetime[16] = {0};
	int ret = -1;

    DIR *pDir = NULL;
    struct dirent *pDirent = NULL;
    struct dirent dlocal = {0};
    struct stat st = {0};

	char mmcpath[128] = {0};
	char dirPath[256] = {0};
    char fileName[256] = {0};
	vector<string> strvec;
	string strtmp;
	string filetime;

	do{
		if(buf == NULL || bufsize < 20)
			break;
		
		ret = get_lastest_record_date(datetime, sizeof(datetime));
		if(ret < 0){
			js_log("get_lastest_record_date error!\n");
			break;
		}

	    ret = storage_get_mmcpath(mmcpath);
		if(ret < 0){
			break;
		}

		sprintf(dirPath, "%s/%s/%s", mmcpath, "IPCamera", datetime);
	    pDir = opendir(dirPath);
	    if(pDir == NULL) {
	        js_log("opendir :%s error\n", dirPath);
			break;
		}
		
		while(1) {
			if (0 != readdir_r(pDir, &dlocal, &pDirent)) {
				pDirent = NULL;
			}
			
			if (NULL == pDirent) {
				break;
			}
		
			if('.' == pDirent->d_name[0]) {
				continue;
			}
		
			snprintf(fileName, sizeof(fileName) - 1, "%s/%s", dirPath, pDirent->d_name);
			if(stat(fileName, &st) != 0) {
				continue;
			}
		
			if(S_ISDIR(st.st_mode)) {
				continue;
			} 
			else if(pDirent->d_name[1] == '-' && pDirent->d_name[8] == '-'  &&
				strlen(pDirent->d_name) == 17
			){
				//S-171054-0831.mp4
				strtmp = pDirent->d_name;
				strvec.push_back(strtmp);				
			}
			
			usleep(10);
		}		

		sort(strvec.begin(), strvec.end(), sort_by_datestr);	
		if(strvec.size()){
			strtmp = strvec.at(strvec.size()-1);
			filetime = strtmp.substr(1, 12);
			sprintf(buf, "%s%s", datetime, filetime.c_str());
			js_log("get record date time:%s\n", buf);
			ret = 0;
		} else{
			ret = -1;
		}
		
		vector<string>().swap(strvec);
	}while(0);


    if(NULL != pDir) {
        closedir(pDir);
        pDir = NULL;
    }
	
	return ret;
}


