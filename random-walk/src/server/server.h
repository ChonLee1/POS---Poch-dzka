/**
 * @file server.h
 * @brief Rozhranie pre TCP server simulácie náhodnej prechádzky.
 *
 * Tento hlavičkový súbor definuje verejné API servera, ktorý:
 * - Prijíma pripojenia od klientov cez TCP
 * - Vykonáva simulácie náhodnej prechádzky na toroidálnej mriežke
 * - Posiela stavy simulácie klientovi v reálnom čase
 */

#pragma once
#include "net.h"
#include "protocol.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>
#include <stdlib.h>

/**
 * @brief Spustí serverový proces.
 *
 * @param port Číslo portu na počúvanie (napr. 5555).
 * @return 0 pri úspešnom ukončení, 1 pri chybe.
 */
int server_run(uint16_t port);