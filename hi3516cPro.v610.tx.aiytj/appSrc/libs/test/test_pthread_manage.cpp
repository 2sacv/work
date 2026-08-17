/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : test_pthreadManage.cpp
 * @Created Time : 2014-03-10
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "debug.h"

#include "pthread_manage.h"

static char gMainQuit = 0;
static void signalINTHand(int signum)
{
	DBG("Set quit flag!!\n");
	gMainQuit = 1;

	return ;
}

void *phtreadTest1(void *arg)
{
	DBG("Start phtreadTest1....\n");

	while(!(*(char*)arg))
	{
		usleep(100 * 1000);
	}

	DBG("Quit phtreadTest1....\n");
}

void *phtreadTest2(void *arg)
{
	DBG("Start phtreadTest2....\n");

	while(!(*(char*)arg))
	{
		usleep(100 * 1000);
	}

	DBG("Quit phtreadTest2....\n");
}

void *phtreadTest3(void *arg)
{
	DBG("Start phtreadTest3....\n");

	while(!(*(char*)arg))
	{
		usleep(100 * 1000);
	}

	DBG("Quit phtreadTest3....\n");
}

int main()
{
	struct sigaction sigAction;

	/* insure a clean shutdown if user types ctrl-c */
	sigAction.sa_handler = signalINTHand;
	sigAction.sa_flags = 0;
	sigemptyset(&sigAction.sa_mask);
	sigaddset(&sigAction.sa_mask, SIGTERM);
	sigaddset(&sigAction.sa_mask, SIGINT);
	sigaction(SIGINT, &sigAction, NULL);
	sigaction(SIGTERM, &sigAction, NULL);

	char quit1 = 0, quit2 = 0, quit3 = 0;
	pthread_t pid = -1;

	if(create_pthread("test 1", phtreadTest1, NULL, (void*)&quit1) < 0)
	{
		goto EXIT;
	}

	if(create_pthread("test 2", phtreadTest2, NULL, (void*)&quit2) < 0)
	{
		goto EXIT;
	}

	pid = create_pthread("test 3", phtreadTest3, NULL, (void*)&quit3);

	sleep(3);

	print_all_pthread_info();
	quit3 = 1;
	join_pthread(pid);
	print_all_pthread_info();

	while(!gMainQuit)
	{
		sleep(1);
	}

EXIT:
	quit1 = 1;
	quit2 = 1;
	quit_all_pthread();
	
	return 0;
}


