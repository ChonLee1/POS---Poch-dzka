#include "server.h"
#include <net.h>
#include <protocol.h>

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int client_fd;
    int running;
    pthread_mutex_t mtx;

    int32_t x, y;
    uint32_t step;
} server_ctx_t;

static int get_running(server_ctx_t* ctx) {
    int r;
    pthread_mutex_lock(&ctx->mtx);
    r = ctx->running;
    pthread_mutex_unlock(&ctx->mtx);
    return r;
}

static void set_running(server_ctx_t* ctx, int value) {
    pthread_mutex_lock(&ctx->mtx);
    ctx->running = value;
    pthread_mutex_unlock(&ctx->mtx);
}

static void* net_thread(void* arg) {
    server_ctx_t* ctx = (server_ctx_t*)arg;

    msg_type_t type;
    uint32_t len = 0;

    // buffer zatiaľ nepotrebujeme (QUIT nemá payload),
    // ale proto_recv chce payload_buf, ak len > 0.
    unsigned char buf[64];

    while (get_running(ctx)) {

        // Čakaj na ďalšiu správu od klienta (blokuje)
        if (proto_recv(ctx->client_fd, &type, buf, (uint32_t)sizeof(buf), &len) != 0) {
            // klient sa odpojil alebo chyba v komunikácii
            fprintf(stderr, "[server] proto_recv failed -> stopping\n");
            set_running(ctx, 0);
            break;
        }

        // Spracovanie typu správy
        if (type == MSG_QUIT) {
            printf("[server] got MSG_QUIT\n");
            set_running(ctx, 0);
            break;
        } else {
            // zatiaľ len log (neskôr tu budú START/LOAD/NEW...)
            printf("[server] got msg type=%u len=%u (ignored)\n",
                   (unsigned)type, (unsigned)len);
        }
    }

    return NULL;
}

static void* sim_thread(void* arg) {
    server_ctx_t* ctx = (server_ctx_t*)arg;

    while (get_running(ctx)) {
        // simulácia práce servera (napr. aktualizácia stavu hry)
        
        // aktualizuj stav (len príklad)
        int32_t x, y;
        uint32_t step;
        pthread_mutex_lock(&ctx->mtx);
        ctx->x += 1;
        ctx->y += 1;
        ctx->step += 1;
        x = ctx->x;
        y = ctx->y;
        step = ctx->step;
        pthread_mutex_unlock(&ctx->mtx);
        
        msg_state_t st = { .x = x, .y = y, .step = step };
        
        if (proto_send(ctx->client_fd, MSG_STATE, &st, sizeof(st)) != 0) {
            fprintf(stderr, "[server] failed to send STATE\n");
            set_running(ctx, 0);
            break;
        }
    }
    
    sleep(200 * 1000); // 200 ms
    
    return NULL;
}


int server_run(uint16_t port) {
    // 1) listen
    int lfd = net_listen(port, 8);
    if (lfd < 0) {
        perror("net_listen");
        return 1;
    }
    printf("[server] listening on %u...\n", (unsigned)port);

    // 2) accept
    int cfd = net_accept(lfd);
    if (cfd < 0) {
        perror("net_accept");
        close(lfd);
        return 1;
    }
    printf("[server] client connected\n");

    // 3) handshake: očakávame MSG_HELLO
    msg_type_t t;
    char payload[64];
    uint32_t len = 0;

    if (proto_recv(cfd, &t, payload, (uint32_t)sizeof(payload), &len) != 0 || t != MSG_HELLO) {
        fprintf(stderr, "[server] expected HELLO\n");
        close(cfd);
        close(lfd);
        return 1;
    }

    // ukončenie stringu pre bezpečný výpis
    payload[(len < sizeof(payload)) ? len : (sizeof(payload) - 1)] = 0;
    printf("[server] HELLO payload: '%s'\n", payload);

    if (proto_send(cfd, MSG_HELLO_ACK, NULL, 0) != 0) {
        fprintf(stderr, "[server] failed to send HELLO_ACK\n");
        close(cfd);
        close(lfd);
        return 1;
    }
    printf("[server] handshake OK\n");

    // 4) init ctx
    server_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.client_fd = cfd;
    ctx.running = 1;
    pthread_mutex_init(&ctx.mtx, NULL);

    // 5) spusti net_thread (blokuje na prijímaní)
    pthread_t tnet;
    pthread_t tsim;
    pthread_create(&tnet, NULL, net_thread, &ctx);
    pthread_create(&tsim, NULL, sim_thread, &ctx);
    
    
    // 6) čakaj, kým net_thread neskončí (napr. QUIT)
    pthread_join(tnet, NULL);
    pthread_join(tsim, NULL);
    
    // 7) cleanup
    set_running(&ctx, 0); // pre istotu
    pthread_mutex_destroy(&ctx.mtx);
    close(cfd);
    close(lfd);
    printf("[server] shutdown\n");
    return 0;
}


