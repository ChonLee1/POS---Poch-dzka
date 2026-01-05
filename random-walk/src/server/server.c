#include "server.h"

typedef struct {
    int client_fd;
    int running;
    pthread_mutex_t mtx;

    int32_t x, y;
    uint32_t step;
    int sim_running;
    int32_t width, height;
    uint32_t k_max;
    uint32_t seed;
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
            shutdown(ctx->client_fd, SHUT_RDWR);
            break;
        }
        // Spracovanie typu správy
        if (type == MSG_QUIT) {
            printf("[server] got MSG_QUIT\n");
            set_running(ctx, 0);
            shutdown(ctx->client_fd, SHUT_RDWR);
            break;
        } else if (type == MSG_START) {
            if (len != sizeof(msg_start_t)) {
                printf("[server] invalid MSG_START length %u\n", (unsigned)len);
                continue;
            }

            msg_start_t s;
            memcpy(&s, buf, sizeof(s));

            if (s.width <= 0 || s.height <= 0 || s.k_max == 0) {
                printf("[server] invalid START params\n");
                continue;
            }

            pthread_mutex_lock(&ctx->mtx);
            ctx->width = s.width;
            ctx->height = s.height;
            ctx->k_max = s.k_max;
            ctx->seed = s.seed;

            ctx->x = s.width / 2;
            ctx->y = s.height / 2;
            ctx->step = 0;

            ctx->sim_running = 1;
            pthread_mutex_unlock(&ctx->mtx);

            printf("[server] simulation started (W=%d H=%d K=%u seed=%u)\n",
                s.width, s.height, (unsigned)s.k_max, (unsigned)s.seed);
        } else {
            // neznámy typ správy - ignoruj
            printf("[server] unknown msg type %u\n", (unsigned)type);
        }
    }
    return NULL;
}

static void* sim_thread(void* arg) {
    server_ctx_t* ctx = (server_ctx_t*)arg;

    while (get_running(ctx)) {

        // 1) ak simulácia nie je spustená (neprišiel START), len čakaj
        pthread_mutex_lock(&ctx->mtx);
        int sim = ctx->sim_running;
        pthread_mutex_unlock(&ctx->mtx);

        if (!sim) {
            nanosleep((const struct timespec[]){{0, 100000000}}, NULL); // 100 ms
            continue;
        }

        // 2) načítaj si potrebné veci + sprav 1 krok (pod mutexom)
        int32_t x, y, w, h;
        uint32_t step, k_max;
        uint32_t seed;

        pthread_mutex_lock(&ctx->mtx);

        w = ctx->width;
        h = ctx->height;
        k_max = ctx->k_max;

        // lokálna kópia seedu (rand_r potrebuje unsigned int*)
        seed = ctx->seed;

        x = ctx->x;
        y = ctx->y;
        step = ctx->step;

        // random smer 0..3
        unsigned int s = (unsigned int)seed;
        int dir = (int)(rand_r(&s) % 4);
        ctx->seed = (uint32_t)s; // ulož späť aktualizovaný seed

        // pokus o pohyb
        int32_t nx = x;
        int32_t ny = y;

        if (dir == 0) nx--;       // left
        else if (dir == 1) nx++;  // right
        else if (dir == 2) ny--;  // up
        else ny++;                // down

        // hranice sveta: ak mimo, ostaneme
        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
            x = nx;
            y = ny;
            ctx->x = x;
            ctx->y = y;
        }

        // krok sa ráta vždy (aj keď narazíš na hranicu) – najjednoduchšie
        step++;
        ctx->step = step;

        // kontrola ukončenia simulácie
        int finished = ((x == 0 && y == 0) || (step >= k_max));
        if (finished) {
            ctx->sim_running = 0;
            printf("[server] simulation reached end condition\n");
        }

        pthread_mutex_unlock(&ctx->mtx);

        // 3) pošli stav klientovi (už mimo mutexu)
        msg_state_t st = { .x = x, .y = y, .step = step };

        if (proto_send(ctx->client_fd, MSG_STATE, &st, (uint32_t)sizeof(st)) != 0) {
            fprintf(stderr, "[server] failed to send STATE\n");
            set_running(ctx, 0);
            shutdown(ctx->client_fd, SHUT_RDWR);
            break;
        }

        if (finished) {
            printf("[server] simulation finished\n");
        }

        // 4) tempo simulácie
        nanosleep((const struct timespec[]){{0, 500000000}}, NULL); // 0.5s
    }

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