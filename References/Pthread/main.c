//
//  main.c
//  null
//
//  Created by mac on 1/31/16.
//  Copyright (c) 2016 mac. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
pthread_mutex_t mutex;
pthread_t threads[2];

void *thread(void *arg)
{
    printf("thread begining\n");
    return 0;
}

void thread_test()
{
    printf("do something\n");
    int pthread_err = pthread_create(threads, NULL, (void *)thread, NULL);
    if (pthread_err != 0)
    {
        printf("%s: Create thread Failed!\n", __func__);
    }
}

int main(int argc, const char * argv[])
{
    thread_test();
    sleep(5);
    return 0;
}

