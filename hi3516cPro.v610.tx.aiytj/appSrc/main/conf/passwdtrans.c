/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : passwdtrans.c
 * @Created Time : 2014-04-23
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

#include "passwdtrans.h"

static const char *cst = "rt5u1b3q0e4a6c8b9yu0vimn";

int passwd_trans_encode(char *dst, char *src, int len)
{
	if(NULL == dst || NULL == src || len < 0 || len > 36)
	{
		ERR("paramter error!\n");
		return FAILURE;
	}

	int i = 0, j = 0;
	
	for(i = 0, j = 0; i < len; i++, j++)
	{
		if((i > 0) && (i % 3 == 0))
		{
			dst[j] = cst[i % strlen(cst)];
			j++;
		}
		
		dst[j] = src[i];
	}
	dst[j] = '\0';

	//DBG("encode dst : %s\n", dst);

	return SUCCESS;
}

int passwd_trans_decode(char *dst, char *src, int len)
{
	if(NULL == dst || NULL == src || len < 0 || len > 48)
	{
		ERR("paramter error!\n");
		return FAILURE;
	}

	int i = 0, j = 0;

	for(i = 0, j = 0; i < len; i++)
	{
		if((i > 0) && (i % 4 == 3)) // 3 7 11 15
		{
			continue;	
		}
		
		dst[j] = src[i];	
		j++;
	}
	dst[j] = '\0';

	//DBG("decode dst : %s\n", dst);
	
	return SUCCESS;	
}

