/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : file_search.h
 * Created Time : 2014-03-25
 * Version      : 1.0
 * Author       : tangpengcheng
 * Description  :
 */

#ifndef __FILE_SEARCH__H__
#define __FILE_SEARCH__H__


#ifdef __cplusplus
extern "C" {
#endif

#ifndef MP4FILE_NAME_MAX
#define MP4FILE_NAME_MAX 128
#endif

typedef void * search_handle;

typedef struct search_opt_s
{
	char * path;
	char * file_ext;
	char recordtype; //etc. 's' --shedule record; 'm'-- manul record 
 	char bnametime; // if 0, not handle ; if 1 will handle according file name
	char reserve[2];
	struct tm * start_time;
	struct tm * end_time;
}search_opt_t;

typedef struct {
	int len;
	int type;
	time_t start_time;
	char file_name[MP4FILE_NAME_MAX];
}file_info_t;
/*
return value: 0 -- success 
		    -1 -- error
*/
int file_search_init(search_handle * handle, search_opt_t *opt);

/*
return value: 0 -- success and end search 
		     1 -- success but have not complete search , need to resume
		    -1 -- error
 NAME_MAX: 255
*/
int file_search(search_handle handle, char file_data[][MP4FILE_NAME_MAX], int req_num, int* ret_num);

/*
return value: 0 -- success and end search 
		     1 -- success but have not complete search , need to resume
		    -1 -- error
 NAME_MAX: 255
*/
int record_file_search(search_handle handle, file_info_t *file_info, int req_num, int* ret_num);

/*
return value: 0 -- success 
		    -1 -- error
*/
int file_search_release(search_handle * handle);


#ifdef __cplusplus
}
#endif


#endif

