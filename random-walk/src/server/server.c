#include "server.h"

typedef struct {
    int listen_fd;
    int client_fd;           // -1 ak ziadny klient
    int running;             // server bezi
    int session_active;      // je aktivny klient session?
    int sim_running;         // bezi simulacia?

    pthread_mutex_t mtx;

    int32_t width, height;
    uint32_t k_max;
    uint32_t reps;
    uint32_t seed;

    uint8_t p_up, p_down, p_left, p_right; // <<< DOPLŇENÉ

    /* stav pre aktualnu replikaciu */
    uint32_t cur_rep;
    uint32_t step;
    int32_t x, y;
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

static int wrap_i32(int v, int maxv) {
    if (maxv <= 0) return 0;
    v %= maxv;
    if (v < 0) v += maxv;
    return v;
}

// vráti 0=UP 1=DOWN 2=LEFT 3=RIGHT
static int pick_dir_percent(uint32_t* rng, uint8_t p_up, uint8_t p_down, uint8_t p_left, uint8_t p_right) {
    unsigned r = (unsigned)(rand_r(rng) % 100); // 0..99

    unsigned a = (unsigned)p_up;
    unsigned b = a + (unsigned)p_down;
    unsigned c = b + (unsigned)p_left;
    unsigned d = c + (unsigned)p_right; // malo by byt 100

    // pre istotu, keby prišlo niečo zlé (aj keď server to validuje)
    if (d == 0) return 3;

    if (r < a) return 0;      // UP
    if (r < b) return 1;      // DOWN
    if (r < c) return 2;      // LEFT
    return 3;                 // RIGHT
}

/* random smer: 0=up 1=down 2=left 3=right */
static void step_random(server_ctx_t* ctx) {
    int d = pick_dir_percent(&ctx->seed, ctx->p_up, ctx->p_down, ctx->p_left, ctx->p_right);

    int32_t x = ctx->x;
    int32_t y = ctx->y;

    if (d == 0) y -= 1;        // UP
    else if (d == 1) y += 1;   // DOWN
    else if (d == 2) x -= 1;   // LEFT
    else x += 1;               // RIGHT

    x = wrap_i32(x, ctx->width);
    y = wrap_i32(y, ctx->height);

    ctx->x = x;
    ctx->y = y;
}


static void* net_thread(void* arg) {
    server_ctx_t* ctx = (server_ctx_t*)arg;

    unsigned char buf[256];

    while (get_running(ctx)) {
        int fd;
        pthread_mutex_lock(&ctx->mtx);
        fd = ctx->client_fd;
        pthread_mutex_unlock(&ctx->mtx);

        if (fd < 0) {
            nanosleep((const struct timespec[]){{0, 100000000}}, NULL); // 100ms
            continue;
        }

        msg_type_t type;
        uint32_t len = 0;

        if (proto_recv(fd, &type, buf, (uint32_t)sizeof(buf), &len) != 0) {
            fprintf(stderr, "[server] client disconnected\n");
            pthread_mutex_lock(&ctx->mtx);
            ctx->session_active = 0;
            ctx->sim_running = 0;
            shutdown(ctx->client_fd, SHUT_RDWR);
            close(ctx->client_fd);
            ctx->client_fd = -1;
            pthread_mutex_unlock(&ctx->mtx);
            continue;
        }

        if (type == MSG_QUIT) {
            printf("[server] got MSG_QUIT -> shutdown server\n");
            set_running(ctx, 0);
            pthread_mutex_lock(&ctx->mtx);
            ctx->sim_running = 0;
            if (ctx->client_fd >= 0) shutdown(ctx->client_fd, SHUT_RDWR);
            /* Shutdown listen socket aby accept() prestal blokovat */
            if (ctx->listen_fd >= 0) {
                shutdown(ctx->listen_fd, SHUT_RDWR);
                close(ctx->listen_fd);
            }
            pthread_mutex_unlock(&ctx->mtx);
            break;
        }

        if (type == MSG_START) {
            if (len != sizeof(msg_start_t)) {
                printf("[server] invalid MSG_START len=%u\n", (unsigned)len);
                continue;
            }

            msg_start_t s;
            memcpy(&s, buf, sizeof(s));

            if (s.width < 2 || s.height < 2 || s.k_max == 0 || s.reps == 0) {
                printf("[server] invalid START params\n");
                continue;
            }

            unsigned psum = (unsigned)s.p_up + (unsigned)s.p_down + (unsigned)s.p_left + (unsigned)s.p_right;
            if (psum != 100) {
                printf("[server] invalid START percents sum=%u (must be 100)\n", psum);
                continue;
            }

            pthread_mutex_lock(&ctx->mtx);
            ctx->width = s.width;
            ctx->height = s.height;
            ctx->k_max = s.k_max;
            ctx->reps = s.reps;

            ctx->p_up = s.p_up;
            ctx->p_down = s.p_down;
            ctx->p_left = s.p_left;
            ctx->p_right = s.p_right;

            if (s.seed == 0) ctx->seed = (uint32_t)time(NULL);
            else ctx->seed = s.seed;

            ctx->cur_rep = 0;
            ctx->step = 0;
            ctx->x = 0;
            ctx->y = 0;

            ctx->sim_running = 1;
            pthread_mutex_unlock(&ctx->mtx);

            printf("[server] simulation started (W=%d H=%d K=%u reps=%u seed=%u) percents U=%u D=%u L=%u R=%u\n", 
                s.width, s.height, (unsigned)s.k_max, (unsigned)s.reps, (unsigned)ctx->seed,
                (unsigned)s.p_up, (unsigned)s.p_down, (unsigned)s.p_left, (unsigned)s.p_right);
        }
    }

    return NULL;
}

static void* sim_thread(void* arg) {
    server_ctx_t* ctx = (server_ctx_t*)arg;

    while (get_running(ctx)) {
        int active, sim;
        int fd;
        int32_t w, h;
        uint32_t kmax, reps;

        pthread_mutex_lock(&ctx->mtx);
        active = ctx->session_active;
        sim = ctx->sim_running;
        fd = ctx->client_fd;
        w = ctx->width;
        h = ctx->height;
        kmax = ctx->k_max;
        reps = ctx->reps;
        pthread_mutex_unlock(&ctx->mtx);

        if (!active || !sim || fd < 0) {
            nanosleep((const struct timespec[]){{0, 100000000}}, NULL); // 100ms
            continue;
        }

        /* sprav reps replikacii */
        for (uint32_t rep = 1; rep <= reps && get_running(ctx); rep++) {
            pthread_mutex_lock(&ctx->mtx);
            if (!ctx->sim_running || ctx->client_fd < 0) {
                pthread_mutex_unlock(&ctx->mtx);
                break;
            }
            ctx->cur_rep = rep;
            ctx->step = 0;

            /* start pozicia – napr. pravy dolny roh */
            ctx->x = w - 1;
            ctx->y = h - 1;
            pthread_mutex_unlock(&ctx->mtx);

            /* max kmax krokov */
            for (uint32_t step = 1; step <= kmax && get_running(ctx); step++) {
                pthread_mutex_lock(&ctx->mtx);
                if (!ctx->sim_running || ctx->client_fd < 0) {
                    pthread_mutex_unlock(&ctx->mtx);
                    break;
                }

                ctx->step = step;

                // TU: pohyb podľa percent
                step_random(ctx);

                msg_state_t st;
                st.rep = rep;
                st.reps_total = reps;
                st.step = step;
                st.x = ctx->x;
                st.y = ctx->y;

                int cfd = ctx->client_fd;
                pthread_mutex_unlock(&ctx->mtx);

                if (proto_send(cfd, MSG_STATE, &st, (uint32_t)sizeof(st)) != 0) {
                    fprintf(stderr, "[server] failed to send STATE\n");
                    pthread_mutex_lock(&ctx->mtx);
                    ctx->sim_running = 0;
                    pthread_mutex_unlock(&ctx->mtx);
                    break;
                }

                /* koniec replikacie: dosiahli sme (0,0) */
                if (st.x == 0 && st.y == 0) break;

                nanosleep((const struct timespec[]){{0, 100000000}}, NULL); // 100ms
            }
        }

        /* simulacia hotova -> MSG_DONE */
        pthread_mutex_lock(&ctx->mtx);
        if (ctx->client_fd >= 0) {
            int cfd = ctx->client_fd;
            pthread_mutex_unlock(&ctx->mtx);
            (void)proto_send(cfd, MSG_DONE, NULL, 0);
        } else {
            pthread_mutex_unlock(&ctx->mtx);
        }

        pthread_mutex_lock(&ctx->mtx);
        ctx->sim_running = 0;
        pthread_mutex_unlock(&ctx->mtx);

        printf("[server] simulation finished\n");
    }

    return NULL;
}

int server_run(uint16_t port) {
    int lfd = net_listen(port, 8);
    if (lfd < 0) {
        perror("net_listen");
        return 1;
    }

    server_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.listen_fd = lfd;
    ctx.client_fd = -1;
    ctx.running = 1;
    ctx.session_active = 0;
    ctx.sim_running = 0;
    pthread_mutex_init(&ctx.mtx, NULL);

    printf("[server] listening on %u...\n", (unsigned)port);

    pthread_t tnet, tsim;
    pthread_create(&tnet, NULL, net_thread, &ctx);
    pthread_create(&tsim, NULL, sim_thread, &ctx);

    /* accept loop */
    while (get_running(&ctx)) {
        int cfd = net_accept(lfd);
        if (cfd < 0) {
            /* accept() failed - pravdepodobne server shutting down */
            if (!get_running(&ctx)) break;
            continue;
        }

        printf("[server] client connected\n");

        /* handshake */
        msg_type_t t;
        char payload[64];
        uint32_t len = 0;

        if (proto_recv(cfd, &t, payload, (uint32_t)sizeof(payload), &len) != 0 || t != MSG_HELLO) {
            fprintf(stderr, "[server] expected HELLO\n");
            close(cfd);
            continue;
        }

        payload[(len < sizeof(payload)) ? len : (sizeof(payload) - 1)] = 0;
        printf("[server] HELLO payload: '%s'\n", payload);

        if (proto_send(cfd, MSG_HELLO_ACK, NULL, 0) != 0) {
            fprintf(stderr, "[server] failed to send HELLO_ACK\n");
            close(cfd);
            continue;
        }
        printf("[server] handshake OK\n");

        pthread_mutex_lock(&ctx.mtx);
        /* ak by bol stary klient, zavri ho */
        if (ctx.client_fd >= 0) {
            shutdown(ctx.client_fd, SHUT_RDWR);
            close(ctx.client_fd);
        }
        ctx.client_fd = cfd;
        ctx.session_active = 1;
        ctx.sim_running = 0;
        pthread_mutex_unlock(&ctx.mtx);
    }

    /* shutdown */
    pthread_join(tnet, NULL);
    pthread_join(tsim, NULL);

    pthread_mutex_destroy(&ctx.mtx);
    if (lfd >= 0) close(lfd);  // moze byt uz zavrety z net_thread

    printf("[server] shutdown\n");
    return 0;
}
