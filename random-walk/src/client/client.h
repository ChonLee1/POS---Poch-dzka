/**
 * @file client.h
 * @brief Rozhranie pre TCP klienta simulácie náhodnej prechádzky.
 *
 * Tento hlavičkový súbor definuje kontext klienta a API funkcie pre:
 * - Pripojenie k serveru
 * - Spustenie simulácie s parametrami
 * - Príjem stavov simulácie v reálnom čase
 * - Ukončenie servera
 */

#pragma once
#include "net.h"
#include "protocol.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

/**
 * @brief Kontext klienta uchovávajúci stav spojenia a vlákien.
 */
typedef struct {
    int fd;                  /**< File descriptor TCP socketu (-1 ak nepripojený) */
    int running;             /**< Príznak, či klient beží (1) alebo sa má ukončiť (0) */

    pthread_mutex_t mtx;     /**< Mutex pre ochranu prístupu k zdieľaným údajom */

    const char* host;        /**< IP adresa alebo hostname servera */
    uint16_t port;           /**< Číslo portu servera */

    int simulation_done;     /**< Príznak ukončenia simulácie (1 = prišlo MSG_DONE) */
} client_ctx_t;

/**
 * @brief Vlákno pre príjem správ od servera.
 *
 * @param arg Ukazovateľ na client_ctx_t štruktúru.
 * @return NULL pri ukončení.
 */
void* recv_thread(void* arg);

/**
 * @brief Vlákno pre spracovanie používateľského vstupu.
 *
 * @param arg Ukazovateľ na client_ctx_t štruktúru.
 * @return NULL pri ukončení.
 */
void* input_thread(void* arg);

/**
 * @brief Pripojí sa k serveru bez spúšťania simulácie.
 *
 * @param ctx Ukazovateľ na kontext klienta.
 * @return 0 pri úspechu, -1 pri chybe.
 */
int client_connect_only(client_ctx_t* ctx);

/**
 * @brief Spustí simuláciu náhodnej prechádzky so zadanými parametrami.
 *
 * @param ctx Ukazovateľ na kontext klienta.
 * @param spawn 1 ak má spustiť server ako child proces, 0 inak.
 * @param w Šírka sveta.
 * @param h Výška sveta.
 * @param k Maximálny počet krokov na replikáciu.
 * @param reps Počet replikácií.
 * @param seed Seed pre generátor náhodných čísel (0 = použiť aktuálny čas).
 * @param p_up Pravdepodobnosť pohybu hore (%).
 * @param p_down Pravdepodobnosť pohybu dole (%).
 * @param p_left Pravdepodobnosť pohybu doľava (%).
 * @param p_right Pravdepodobnosť pohybu doprava (%).
 * @return 0 pri úspechu, -1 pri chybe.
 */
int client_start_simulation(client_ctx_t* ctx, int spawn,
                            int32_t w, int32_t h,
                            uint32_t k, uint32_t reps, uint32_t seed,
                            uint8_t p_up, uint8_t p_down, uint8_t p_left, uint8_t p_right);

/**
 * @brief Pošle serveru príkaz na ukončenie a zatvorí spojenie.
 *
 * @param ctx Ukazovateľ na kontext klienta.
 * @return Vždy vráti 0.
 */
int client_quit_server_and_close(client_ctx_t* ctx);       