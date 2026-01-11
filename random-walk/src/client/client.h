#pragma once
#include "net.h"
#include "protocol.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef struct {
    int fd;
    int running;

    pthread_mutex_t mtx;

    const char* host;
    uint16_t port;

    int simulation_done;   // <- NOVÉ: 1 = prišlo MSG_DONE
} client_ctx_t;

void* recv_thread(void* arg);
void* input_thread(void* arg);

/* "Akcie" z main menu */
int client_connect_only(client_ctx_t* ctx);                 // connect + handshake
int client_start_simulation(client_ctx_t* ctx, int spawn,   // spawn? 1/0
                            int32_t w, int32_t h,
                            uint32_t k, uint32_t reps, uint32_t seed,
                            uint8_t p_up, uint8_t p_down, uint8_t p_left, uint8_t p_right);
int client_quit_server_and_close(client_ctx_t* ctx);       