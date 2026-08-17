/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : test_time_call.cpp
 * Created Time : 2014-02-26
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "time_call.h"


void timecall1(void *data)
{
	printf("time call 1\n");
}

void timecall2(void *data)
{
	printf("time call 2\n");
}

void timecall3(void *data)
{
	printf("time call 3\n");
}

int main(int argc, char *argv[])
{
	JTimeEngine engine1, engine2;

	create_time_engine(&engine1);
	create_time_engine(&engine2);

	sleep(3);

	register_time_func(engine1, "time1", 1, timecall1, NULL);
	register_time_func(engine1, "time2", 2, timecall2, NULL);
	register_time_func(engine2, "time3", 3, timecall3, NULL);

	sleep(120);

	release_time_engine(&engine1);
	release_time_engine(&engine2);	
}

