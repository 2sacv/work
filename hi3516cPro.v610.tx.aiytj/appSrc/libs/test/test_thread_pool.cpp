/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : threadPoolDemo.cpp
 * @Created Time : 2014-03-10
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "debug.h"

#include "thread_pool.h"

static char gMainQuit = 0;
static void signalINTHandler(int signum)
{
	DBG("Set quit flag!!\n");
	gMainQuit = 1;

	return ;
}

void *threadTest1(void *arg)
{
	DBG("[%s] is worked in [%d] thread!\n", (char*)arg, (int)syscall(SYS_gettid));
	
	sleep(5);

	return NULL;
}

void *threadTest2(void *arg)
{
	DBG("[%s] is worked in [%d] thread!\n", (char*)arg, (int)syscall(SYS_gettid));
	
	sleep(5);

	return NULL;
}

int main()
{
	struct sigaction sigAction;

	/* insure a clean shutdown if user types ctrl-c */
	sigAction.sa_handler = signalINTHandler;
	sigAction.sa_flags = 0;
	sigemptyset(&sigAction.sa_mask);
	sigaddset(&sigAction.sa_mask, SIGTERM);
	sigaddset(&sigAction.sa_mask, SIGINT);
	sigaction(SIGINT, &sigAction, NULL);
	sigaction(SIGTERM, &sigAction, NULL);

	const char *str1 = "hello world!";
	const char *str2 = "hello shenzhen!";
	
	if(thread_pool_init(5) < 0)
	{
		ERR("thread_pool_init error!\n");
		goto EXIT;
	}

	sleep(5);
	
	if(pool_add_worker(threadTest1, (void *)str1) < 0)
	{
		ERR("Add worker failed!\n");
		goto EXIT;
	}

	if(pool_add_worker(threadTest2, (void *)str2) < 0)
	{
		ERR("Add worker failed!\n");
		goto EXIT;
	}

	while(!gMainQuit)
	{
		sleep(1);
	}

EXIT:
	pool_destroy();
	
	DBG("Quit....\n");
	
	return 0;
}

