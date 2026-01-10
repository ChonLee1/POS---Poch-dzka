#include "client.h"
#include "menu.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    const char* host = "127.0.0.1";
    uint16_t port = 5555;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = (uint16_t)atoi(argv[2]);

    client_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.fd = -1;
    ctx.running = 1;
    ctx.host = host;
    ctx.port = port;

    pthread_mutex_init(&ctx.mtx, NULL);

    pthread_t trecv;
    pthread_create(&trecv, NULL, recv_thread, &ctx);

    while (ctx.running) {
        if (ctx.simulation_done) {
        // len aby si to videl a nezmätkoval
        printf("\n[client] Simulacia skoncila. Vraciame sa do menu...\n");
        // reset, aby sa to nepísalo stále
        pthread_mutex_lock(&ctx.mtx);
        ctx.simulation_done = 0;
        pthread_mutex_unlock(&ctx.mtx);
        }
        
        int choice = menu_read_choice();

        if (choice == 1) {
            int w = menu_read_int("Sirka W", 2, 2000, 10);
            int h = menu_read_int("Vyska H", 2, 2000, 10);
            unsigned k = menu_read_uint("Max kroky K", 1, 1000000, 200);
            unsigned r = menu_read_uint("Replikacie R", 1, 1000000, 5);
            unsigned seed = menu_read_uint("Seed (0=auto)", 0, 0xFFFFFFFFu, 0);

            /* spawn=1 -> vytvor server proces */
            (void)client_start_simulation(&ctx, 1, (int32_t)w, (int32_t)h, (uint32_t)k, (uint32_t)r, (uint32_t)seed);

        } else if (choice == 2) {
            (void)client_connect_only(&ctx);

        } else if (choice == 3) {
            (void)client_quit_server_and_close(&ctx);
            ctx.running = 0;

        } else {
            printf("Neznama volba.\n");
        }
    }

    pthread_join(trecv, NULL);

    pthread_mutex_destroy(&ctx.mtx);
    if (ctx.fd >= 0) close(ctx.fd);

    return 0;
}
