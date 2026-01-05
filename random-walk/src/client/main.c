#include <net.h>
#include <protocol.h>
#include "client.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>

int main(int argc, char** argv) {
    const char* host = "127.0.0.1";
    uint16_t port = 5555;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = (uint16_t)atoi(argv[2]);

    int fd = net_connect(host, port);
    if (fd < 0) { perror("net_connect"); return 1; }

    const char* hello = "hello-from-client";
    if (proto_send(fd, MSG_HELLO, hello, (uint32_t)strlen(hello)) != 0) {
        fprintf(stderr, "[client] failed to send HELLO\n");
        close(fd);
        return 1;
    }

    msg_type_t t;
    uint32_t len = 0;
    if (proto_recv(fd, &t, NULL, 0, &len) != 0 || t != MSG_HELLO_ACK) {
        fprintf(stderr, "[client] expected HELLO_ACK\n");
        close(fd);
        return 1;
    }
    printf("[client] handshake OK\n");

    client_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.fd = fd;
    ctx.running = 1;
    pthread_mutex_init(&ctx.mtx, NULL);

    pthread_t trecv, tin;
    pthread_create(&trecv, NULL, recv_thread, &ctx);
    pthread_create(&tin, NULL, input_thread, &ctx);

    printf("[client] Press 'q' + Enter to quit\n");

    pthread_join(tin, NULL);
    pthread_join(trecv, NULL);

    pthread_mutex_destroy(&ctx.mtx);
    close(fd);

    return 0;
}
