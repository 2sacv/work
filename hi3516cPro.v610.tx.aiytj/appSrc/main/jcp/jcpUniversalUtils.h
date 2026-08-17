/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jcpUniversalUtils.h
 * @Created Time : 2013-12-26
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _JCPUNIVERSALUTILS_H_
#define _JCPUNIVERSALUTILS_H_
#ifdef __cplusplus
extern "C" {
#endif


    int arg_opt_if_set(const char *optName, ArgOptS opts[]);
	int arg_opt_if_set_ex(const char *optName, ArgOptS_Expand opts[]);

    int  help_jcp_arg_msg(int argc, char **argv, char *buf, int buflen,ArgOptS opts[], HelpMsgS helps[]);

	int  help_jcp_arg_msg_ex(int argc, char **argv, char *buf, int buflen,  ArgOptS_Expand opts[], HelpMsgS helps[]);	

    int setmustListonlyRule(char *buf, ArgOptS opts[]);

    int parser_jcp_arg(int argc, char **argv, ArgOptS opts[], char *buf, void *array, int str_size);
	int parser_jcp_arg_ex(int argc, char **argv, ArgOptS_Expand opts[], char *buf, void *array, int str_size);	

    int checkListRule(ArgOptS opts[]);
	int checkListRule_ex(ArgOptS_Expand opts[]);	

    int assembleListString(char *buf, int buflen, ArgOptS opts[]);
	int assembleListString_ex(char *buf, int buflen, ArgOptS_Expand opts[]);

    int asmListCount(char *buf, ArgOptS opts[], void *arry, int gidx, int arr_size);


#ifdef __cplusplus
}
#endif
#endif

