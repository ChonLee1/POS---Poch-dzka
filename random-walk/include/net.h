#pragma once
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Vytvorí TCP serverový socket a začne počúvať na zadanom porte.
 *
 * Vytvorí IPv4 (AF_INET) streamový (TCP) socket, nastaví voľbu SO_REUSEADDR,
 * naviaže socket na adresu INADDR_ANY a zadaný port a zavolá listen().
 *
 * @param port Číslo portu, na ktorom bude server počúvať (v host byte order).
 * @param backlog Maximálny počet čakajúcich pripojení.
 * @return File descriptor počúvajúceho socketu pri úspechu, -1 pri chybe.
 *
 * @note Volajúci je zodpovedný za zatvorenie socketu pomocou close(fd).
 */
int net_listen(uint16_t port, int backlog);

/**
 * @brief Prijme prichádzajúce pripojenie od klienta.
 *
 * Funkcia blokuje, kým sa nepripojí klient (ak socket nie je nastavený
 * ako neblokujúci).
 *
 * @param listen_fd File descriptor počúvajúceho socketu.
 * @return File descriptor nového socketu pre komunikáciu s klientom
 *         pri úspechu, -1 pri chybe.
 *
 * @note Volajúci je zodpovedný za zatvorenie vráteného socketu.
 */
int net_accept(int listen_fd);

/**
 * @brief Pripojí sa na TCP server na zadanej adrese a porte.
 *
 * Vytvorí TCP socket a pokúsi sa nadviazať spojenie so serverom.
 *
 * @param host IP adresa alebo názov hostiteľa (napr. "127.0.0.1").
 * @param port Číslo portu servera (v host byte order).
 * @return File descriptor pripojeného socketu pri úspechu, -1 pri chybe.
 *
 * @note Volajúci je zodpovedný za zatvorenie socketu pomocou close(fd).
 */
int net_connect(const char* host, uint16_t port);

/**
 * @brief Pošle presne zadaný počet bajtov cez socket.
 *
 * Funkcia opakovane volá send(), kým neodošle všetky dáta alebo nenastane chyba.
 * Ošetruje situácie, keď send() odošle menej bajtov alebo je prerušený signálom.
 *
 * @param fd File descriptor pripojeného socketu.
 * @param buf Ukazovateľ na buffer s dátami na odoslanie.
 * @param len Počet bajtov, ktoré sa majú odoslať.
 * @return 0 pri úspechu (všetky dáta odoslané), -1 pri chybe.
 */
int net_send_all(int fd, const void* buf, size_t len);

/**
 * @brief Prijme presne zadaný počet bajtov zo socketu.
 *
 * Funkcia opakovane volá recv(), kým neprijme všetky požadované dáta.
 * Ak vzdialená strana ukončí spojenie, považuje sa to za chybu.
 *
 * @param fd File descriptor pripojeného socketu.
 * @param buf Buffer, do ktorého sa uložia prijaté dáta.
 * @param len Počet bajtov, ktoré sa majú prijať.
 * @return 0 pri úspechu (všetky dáta prijaté), -1 pri chybe alebo ukončení spojenia.
 */
int net_recv_all(int fd, void* buf, size_t len);
