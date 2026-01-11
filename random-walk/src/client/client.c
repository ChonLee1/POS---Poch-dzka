#include "client.h"

/**
 * @brief Thread-safe získanie príznaku running.
 *
 * @param ctx Ukazovateľ na kontext klienta.
 * @return 1 ak klient beží, 0 ak sa má ukončiť.
 */
static int get_running(client_ctx_t* ctx) {
    int r;
    pthread_mutex_lock(&ctx->mtx);
    r = ctx->running;
    pthread_mutex_unlock(&ctx->mtx);
    return r;
}

/**
 * @brief Thread-safe nastavenie príznaku running.
 *
 * @param ctx Ukazovateľ na kontext klienta.
 * @param value Nová hodnota príznaku (1 = bežiaci, 0 = ukončenie).
 */
static void set_running(client_ctx_t* ctx, int value) {
    pthread_mutex_lock(&ctx->mtx);
    ctx->running = value;
    pthread_mutex_unlock(&ctx->mtx);
}

/**
 * @brief Thread-safe získanie file descriptora socketu.
 *
 * @param ctx Ukazovateľ na kontext klienta.
 * @return File descriptor socketu, alebo -1 ak nie je pripojený.
 */
static int ctx_get_fd(client_ctx_t* ctx) {
    int fd;
    pthread_mutex_lock(&ctx->mtx);
    fd = ctx->fd;
    pthread_mutex_unlock(&ctx->mtx);
    return fd;
}

/**
 * @brief Thread-safe nastavenie file descriptora socketu.
 *
 * @param ctx Ukazovateľ na kontext klienta.
 * @param fd Nový file descriptor socketu.
 */
static void ctx_set_fd(client_ctx_t* ctx, int fd) {
    pthread_mutex_lock(&ctx->mtx);
    ctx->fd = fd;
    pthread_mutex_unlock(&ctx->mtx);
}

/**
 * @brief Thread-safe zatvorenie socketu.
 *
 * Bezpečne zatvorí socket a nastaví fd na -1.
 *
 * @param ctx Ukazovateľ na kontext klienta.
 */
static void ctx_close_fd(client_ctx_t* ctx) {
    int fd;
    pthread_mutex_lock(&ctx->mtx);
    fd = ctx->fd;
    ctx->fd = -1;
    pthread_mutex_unlock(&ctx->mtx);

    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
}

/**
 * @brief Thread-safe nastavenie príznaku dokončenia simulácie.
 *
 * @param ctx Ukazovateľ na kontext klienta.
 * @param v Nová hodnota príznaku (1 = dokončené, 0 = nie).
 */
static void ctx_set_done(client_ctx_t* ctx, int v) {
    pthread_mutex_lock(&ctx->mtx);
    ctx->simulation_done = v;
    pthread_mutex_unlock(&ctx->mtx);
}

/**
 * @brief Uspi vlákno na zadaný počet milisekúnd.
 *
 * @param ms Počet milisekúnd na spánok.
 */
static void sleep_ms(long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/**
 * @brief Spustí serverový proces ako child proces.
 *
 * Použije fork() a execl() na spustenie ./bin/server s daným portom.
 *
 * @param port Číslo portu pre server.
 * @return 0 pri úspechu, -1 pri chybe.
 */
static int spawn_server(uint16_t port) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        char port_str[16];
        snprintf(port_str, sizeof(port_str), "%u", (unsigned)port);
        execl("./bin/server", "server", port_str, (char*)NULL);
        perror("execl");
        _exit(127);
    }
    return 0;
}

/**
 * @brief Pripojí sa k serveru a vykoná handshake protokol.
 *
 * Pošle MSG_HELLO a očakáva MSG_HELLO_ACK od servera.
 *
 * @param host IP adresa alebo hostname servera.
 * @param port Číslo portu servera.
 * @return File descriptor socketu pri úspechu, -1 pri chybe.
 */
static int connect_and_handshake(const char* host, uint16_t port) {
    int fd = net_connect(host, port);
    if (fd < 0) return -1;

    const char* hello = "hello-from-client";
    if (proto_send(fd, MSG_HELLO, hello, (uint32_t)strlen(hello)) != 0) {
        close(fd);
        return -1;
    }

    msg_type_t t;
    uint32_t len = 0;
    if (proto_recv(fd, &t, NULL, 0, &len) != 0 || t != MSG_HELLO_ACK) {
        close(fd);
        return -1;
    }

    return fd;
}

/**
 * @brief Pripojí sa k serveru bez spúšťania simulácie.
 *
 * Pokiaľ je klient už pripojený, nevykoná nič.
 * Inak sa pripojí k serveru a vykoná handshake.
 *
 * @param ctx Ukazovateľ na kontext klienta.
 * @return 0 pri úspechu, -1 pri chybe.
 */
int client_connect_only(client_ctx_t* ctx) {
    if (ctx_get_fd(ctx) >= 0) {
        printf("[client] uz pripojeny.\n");
        return 0;
    }

    int fd = connect_and_handshake(ctx->host, ctx->port);
    if (fd < 0) {
        fprintf(stderr, "[client] connect/handshake failed\n");
        return -1;
    }
    ctx_set_fd(ctx, fd);
    printf("[client] connected + handshake OK\n");
    return 0;
}

/**
 * @brief Spustí simuláciu náhodnej prechádzky.
 *
 * Funkcia vykoná nasledujúce kroky:
 * 1. Ak je spawn=1 a nie je pripojený, spustí serverový proces
 * 2. Ak nie je pripojený, pripojí sa k serveru
 * 3. Pošle MSG_START so všetkými parametrami simulácie
 *
 * @param ctx Ukazovateľ na kontext klienta.
 * @param spawn 1 ak má spustiť server ako child proces, 0 inak.
 * @param w Šírka sveta.
 * @param h Výška sveta.
 * @param k Maximálny počet krokov na replikáciu.
 * @param reps Počet replikácií.
 * @param seed Seed pre generátor náhodných čísiel (0 = použiť aktuálny čas).
 * @param p_up Pravdepodobnosť pohybu hore (%).
 * @param p_down Pravdepodobnosť pohybu dole (%).
 * @param p_left Pravdepodobnosť pohybu doľava (%).
 * @param p_right Pravdepodobnosť pohybu doprava (%).
 * @return 0 pri úspechu, -1 pri chybe.
 */
int client_start_simulation(client_ctx_t* ctx, int spawn,
                            int32_t w, int32_t h,
                            uint32_t k, uint32_t reps, uint32_t seed,
                            uint8_t p_up, uint8_t p_down, uint8_t p_left, uint8_t p_right) {
    /* 1) ak treba, spusti server */
    if (spawn && ctx_get_fd(ctx) < 0) {
        if (spawn_server(ctx->port) != 0) {
            fprintf(stderr, "[client] failed to spawn server\n");
            return -1;
        }
        /* retry connect kym server nezacne listen */
        int fd = -1;
        for (int i = 0; i < 40; i++) { /* ~4s */
            fd = connect_and_handshake(ctx->host, ctx->port);
            if (fd >= 0) break;
            sleep_ms(100);
        }
        if (fd < 0) {
            fprintf(stderr, "[client] connect/handshake failed (server not ready?)\n");
            return -1;
        }
        ctx_set_fd(ctx, fd);
        printf("[client] connected + handshake OK\n");
    }

    /* 2) ak nie sme pripojeni, tak iba connect */
    if (ctx_get_fd(ctx) < 0) {
        if (client_connect_only(ctx) != 0) return -1;
    }

    /* 3) posli START */
    msg_start_t s;
    s.width  = w;
    s.height = h;
    s.k_max  = k;
    s.reps   = reps;
    s.seed   = seed;
    s.p_up   = p_up;
    s.p_down = p_down;
    s.p_left = p_left;
    s.p_right = p_right;

    int fd2 = ctx_get_fd(ctx);
    if (proto_send(fd2, MSG_START, &s, (uint32_t)sizeof(s)) != 0) {
        ctx_set_done(ctx, 0);
        fprintf(stderr, "[client] failed to send MSG_START\n");
        ctx_close_fd(ctx);
        return -1;
    }

    printf("[client] START sent (W=%d H=%d K=%u reps=%u seed=%u)\n",
           s.width, s.height, (unsigned)s.k_max, (unsigned)s.reps, (unsigned)s.seed);

    return 0;
}

/**
 * @brief Pošle serveru príkaz na ukončenie a zatvorí spojenie.
 *
 * @param ctx Ukazovateľ na kontext klienta.
 * @return Vždy vráti 0.
 */
int client_quit_server_and_close(client_ctx_t* ctx) {
    int fd = ctx_get_fd(ctx);
    if (fd >= 0) {
        (void)proto_send(fd, MSG_QUIT, NULL, 0);
    }
    ctx_close_fd(ctx);
    return 0;
}

/**
 * @brief Vlákno pre príjem správ od servera.
 *
 * Toto vlákno beží po celú dobu života klienta a:
 * - Prijíma správy MSG_STATE (stav simulácie) a vypisuje ich
 * - Prijíma správu MSG_DONE (koniec simulácie)
 * - Deteguje odpojenie servera
 *
 * @param arg Ukazovateľ na client_ctx_t štruktúru.
 * @return NULL pri ukončení.
 */
void* recv_thread(void* arg) {
    client_ctx_t* ctx = (client_ctx_t*)arg;

    while (get_running(ctx)) {
        int fd = ctx_get_fd(ctx);
        if (fd < 0) {
            nanosleep((const struct timespec[]){{0, 100000000}}, NULL); // 100ms
            continue;
        }

        msg_type_t t;
        uint32_t len = 0;

        /* najvacsi payload co cakame = msg_state_t */
        msg_state_t st;

        if (proto_recv(fd, &t, &st, (uint32_t)sizeof(st), &len) != 0) {
            printf("[client] disconnected from server\n");
            ctx_close_fd(ctx);
            continue; // klient zije dalej, vrat sa do menu
        }

        if (t == MSG_STATE && len == sizeof(st)) {
            printf("[client] rep=%u/%u step=%u pos=(%d,%d)\n",
                   st.rep, st.reps_total, st.step, st.x, st.y);
        } else if (t == MSG_DONE) {
            printf("[client] simulation finished (MSG_DONE)\n");
            /* server moze zostat bezat alebo zatvorit session; my len informujeme */
            ctx_set_done(ctx, 1);
        } else {
            /* ignoruj */
        }
    }

    return NULL;
}

/**
 * @brief Vlákno pre spracovanie používateľského vstupu z terminu.
 *
 * Umožňuje používateľovi ukončiť klienta zadaním 'q'.
 * Pri iných vstupoch informuje používateľa, aby použil hlavné menu.
 *
 * @param arg Ukazovateľ na client_ctx_t štruktúru.
 * @return NULL pri ukončení.
 */
void* input_thread(void* arg) {
    client_ctx_t* ctx = (client_ctx_t*)arg;
    char line[64];

    while (get_running(ctx)) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            set_running(ctx, 0);
            ctx_close_fd(ctx);
            break;
        }

        if (line[0] == 'q') {
            /* q v input threade = ukonci klienta aj server (ak je pripojeny) */
            (void)client_quit_server_and_close(ctx);
            set_running(ctx, 0);
            break;
        }

        printf("[client] input thread: pouzi menu v main okne.\n");
    }

    return NULL;
}