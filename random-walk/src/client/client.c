#include "client.h"
#include <protocol.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>

static int get_running(client_ctx_t* ctx) {
    int r;
    pthread_mutex_lock(&ctx->mtx);
    r = ctx->running;
    pthread_mutex_unlock(&ctx->mtx);
    return r;
}

static void set_running(client_ctx_t* ctx, int value) {
    pthread_mutex_lock(&ctx->mtx);
    ctx->running = value;
    pthread_mutex_unlock(&ctx->mtx);
}

void* recv_thread(void* arg) {
   client_ctx_t* ctx = (client_ctx_t*)arg;

    while (get_running(ctx)) {
        msg_type_t t;
        uint32_t len = 0;

        // buffer pre payload: my čakáme msg_state_t
        msg_state_t st;

        if (proto_recv(ctx->fd, &t, &st, (uint32_t)sizeof(st), &len) != 0) {
            // server zavrel alebo shutdown prebudil recv
            set_running(ctx, 0);
            shutdown(ctx->fd, SHUT_RDWR);
            break;
        }

        if (t == MSG_STATE && len == sizeof(st)) {
            printf("[client] step=%u pos=(%d,%d)\n", st.step, st.x, st.y);
        } else {
            // zatiaľ ignorujeme iné správy
            // (neskôr tu môžeš spracovať napr. výsledky, menu, atď.)
        }
    }
    return NULL;
}

void* input_thread(void* arg) {
    client_ctx_t* ctx = (client_ctx_t*)arg;
    char line[64];

    while (get_running(ctx)) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            set_running(ctx, 0);
            shutdown(ctx->fd, SHUT_RDWR);
            break;
        }

        if (line[0] == 'q') {
            (void)proto_send(ctx->fd, MSG_QUIT, NULL, 0);
            set_running(ctx, 0);
            shutdown(ctx->fd, SHUT_RDWR);
            break;

        } else if (line[0] == 'n') {
            msg_start_t s;
            s.width  = 10;
            s.height = 10;
            s.k_max  = 200;
            s.seed   = (uint32_t)time(NULL);

            if (proto_send(ctx->fd, MSG_START, &s, (uint32_t)sizeof(s)) != 0) {
                fprintf(stderr, "[client] failed to send MSG_START\n");
                set_running(ctx, 0);
                shutdown(ctx->fd, SHUT_RDWR);
                break;
            }

            printf("[client] started simulation (W=%d H=%d K=%u seed=%u)\n",
                   s.width, s.height, (unsigned)s.k_max, (unsigned)s.seed);

        } else {
            printf("[client] commands: n = new simulation, q = quit\n");
        }
    }

    return NULL;
}