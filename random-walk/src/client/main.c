#include "net.h"
#include "protocol.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
    // Predvolené nastavenie: lokálny server na porte 5555
    const char* host = "127.0.0.1";
    uint16_t port = 5555;

    // Voliteľné argumenty príkazového riadku:
    // 1) host (IP alebo hostname)
    // 2) port
    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = (uint16_t)atoi(argv[2]);

    // Pokus o pripojenie na server cez TCP
    int fd = net_connect(host, port);
    if (fd < 0) {
        // perror vypíše textovú hlášku podľa errno
        perror("net_connect");
        return 1;
    }
    printf("[client] connected to %s:%u\n", host, (unsigned)port);

    // Handshake správa (MSG_HELLO) – klient oznámi serveru, že je pripravený
    const char* hello = "hello-from-client";
    if (proto_send(fd, MSG_HELLO, hello, (uint32_t)strlen(hello)) != 0) {
        fprintf(stderr, "[client] failed to send HELLO\n");
        close(fd);
        return 1;
    }

    // Čakáme na potvrdenie od servera (MSG_HELLO_ACK)
    msg_type_t t;
    uint32_t len = 0;

    // V tomto prípade neočakávame žiadny payload, preto posielame NULL a buf_cap = 0
    if (proto_recv(fd, &t, NULL, 0, &len) != 0 || t != MSG_HELLO_ACK) {
        fprintf(stderr, "[client] expected HELLO_ACK\n");
        close(fd);
        return 1;
    }

    // Handshake prebehol úspešne
    printf("[client] handshake OK.\n");

    // Čakáme na vstup od používateľa (keby chcel ukončiť)
    printf("[client] Press Enter to disconnect...\n");
    getchar();

    // Odoslať MSG_QUIT serveru
    if (proto_send(fd, MSG_QUIT, NULL, 0) != 0) {
        fprintf(stderr, "[client] failed to send QUIT\n");
    }

    // Ukončenie spojenia a korektné zatvorenie socketu
    close(fd);
    printf("[client] disconnected\n");
    return 0;
}
