#pragma once
#include <pthread.h>

typedef struct {
    int fd;
    int running;
    pthread_mutex_t mtx;
} client_ctx_t;

void* recv_thread(void* arg);
void* input_thread(void* arg);
