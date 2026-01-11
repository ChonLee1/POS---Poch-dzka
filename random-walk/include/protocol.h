/**
 * @file protocol.h
 * @brief Definícia komunikačného protokolu medzi klientom a serverom.
 *
 * Protokol používa binárne správy s hlavičkou obsahujúcou typ a dĺžku payloadu.
 * Všetky číselné hodnoty sú v network byte order (big-endian).
 */

#pragma once
#include <stdint.h>
#include "net.h"
#include <arpa/inet.h>   // htonl(), ntohl()

/**
 * @brief Typy správ v protokole.
 */
typedef enum {
    MSG_HELLO = 1,       /**< Klient -> Server: Žiadosť o pripojenie */
    MSG_HELLO_ACK = 2,   /**< Server -> Klient: Potvrdenie pripojenia */

    MSG_START = 3,       /**< Klient -> Server: Parametre simulácie */
    MSG_STATE = 4,       /**< Server -> Klient: Aktuálny stav simulácie */
    MSG_DONE  = 5,       /**< Server -> Klient: Koniec simulácie */
    MSG_QUIT  = 6        /**< Klient -> Server: Ukončiť server */
} msg_type_t;

/**
 * @brief Hlavička správy v binárnom protokole.
 *
 * Všetky polia sú v network byte order (big-endian).
 */
typedef struct __attribute__((packed)) {
    uint32_t type;       /**< Typ správy (msg_type_t) */
    uint32_t length;     /**< Dĺžka payloadu v bajtoch */
} msg_header_t;

/**
 * @brief Stav simulácie posielaný serverom klientovi (MSG_STATE).
 */
typedef struct __attribute__((packed)) {
    int32_t x;           /**< Aktuálna x-ová súradnica */
    int32_t y;           /**< Aktuálna y-ová súradnica */
    uint32_t step;       /**< Aktuálny krok v replikácii */
    uint32_t rep;        /**< Číslo aktuálnej replikácie (1..reps) */
    uint32_t reps_total; /**< Celkový počet replikácií */
} msg_state_t;

/**
 * @brief Parametre simulácie posielané klientom serveru (MSG_START).
 */
typedef struct __attribute__((packed)) {
    int32_t width;       /**< Šírka sveta (>=1) */
    int32_t height;      /**< Výška sveta (>=1) */
    uint32_t k_max;      /**< Maximálny počet krokov na replikáciu */
    uint32_t reps;       /**< Počet replikácií */
    uint32_t seed;       /**< Seed pre generátor náhodných čísel (0 = použiť čas) */

    uint8_t  p_up;       /**< Pravdepodobnosť pohybu hore (%) */
    uint8_t  p_down;     /**< Pravdepodobnosť pohybu dole (%) */
    uint8_t  p_left;     /**< Pravdepodobnosť pohybu doľava (%) */
    uint8_t  p_right;    /**< Pravdepodobnosť pohybu doprava (%) */
} msg_start_t;

typedef struct __attribute__((packed)) {
    uint32_t reps_total;

    uint32_t success_count;   // koľko replikácií došlo na (0,0)
    uint32_t fail_count;      // reps_total - success_count

    uint64_t sum_steps_success; // súčet krokov len pre úspešné replikácie
    uint32_t min_steps;         // min krokov medzi úspešnými
    uint32_t max_steps;         // max krokov medzi úspešnými

    // jednoduchý histogram krokov (úspešné replikácie)
    // 0: 0-20, 1: 21-50, 2: 51-100, 3: 101+
    uint32_t bins[4];

    // voliteľné – echo vstupných parametrov (na obhajobu super)
    int32_t  width;
    int32_t  height;
    uint32_t k_max;
    uint8_t  p_up, p_down, p_left, p_right;
} msg_result_t;

/**
 * @brief Odošle správu cez socket.
 *
 * Funkcia vytvorí hlavičku správy, prevedie hodnoty do network byte order
 * a odošle hlavičku aj payload.
 *
 * @param fd File descriptor socketu.
 * @param type Typ správy.
 * @param payload Ukazovateľ na payload (môže byť NULL ak len=0).
 * @param len Dĺžka payloadu v bajtoch.
 * @return 0 pri úspechu, -1 pri chybe.
 */
int proto_send(int fd, msg_type_t type, const void* payload, uint32_t len);

/**
 * @brief Prijme správu zo socketu.
 *
 * Funkcia prijme hlavičku správy, prevedie hodnoty z network byte order
 * a prijme payload do poskytnutého bufferu (ak je dostatočne veľký).
 *
 * @param fd File descriptor socketu.
 * @param out_type Výstupný parameter pre typ prijatej správy.
 * @param payload_buf Buffer pre payload (môže byť NULL).
 * @param buf_cap Kapacita bufferu v bajtoch.
 * @param out_len Výstupný parameter pre skutočnú dĺžku payloadu.
 * @return 0 pri úspechu, -1 pri chybe alebo odpojení.
 */
int proto_recv(int fd, msg_type_t* out_type, void* payload_buf, uint32_t buf_cap, uint32_t* out_len);
