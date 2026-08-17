/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2014-06-11
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _cmdparse_H_
#define _cmdparse_H_

#ifdef __cplusplus
extern "C" {
#endif

    int cmdline_parse_argv(char *command_line, int *argcp, char ***argvp);
    int cmdline_free_argv(int argcp, char **argvp);

#ifdef __cplusplus
}
#endif
#endif

