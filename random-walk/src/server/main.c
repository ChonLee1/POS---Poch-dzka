#include "server.h"
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    uint16_t port = 5555;
    if (argc >= 2) port = (uint16_t)atoi(argv[1]);
    return server_run(port);
}
