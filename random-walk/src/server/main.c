/**
 * @file main.c
 * @brief Vstupný bod serverovej aplikácie.
 *
 * Spúšťa TCP server pre simuláciu náhodnej prechádzky.
 */

#include "server.h"
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Vstupný bod serverovej aplikácie.
 *
 * Spracúva argumenty príkazového riadka:
 * - argv[1]: Číslo portu (predvolené: 5555)
 *
 * @param argc Počet argumentov.
 * @param argv Pole argumentov.
 * @return 0 pri úspšnom ukončení, 1 pri chybe.
 */
int main(int argc, char** argv) {
    uint16_t port = 5555;
    if (argc >= 2) port = (uint16_t)atoi(argv[1]);
    return server_run(port);
}
