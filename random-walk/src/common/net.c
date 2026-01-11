#include "net.h"

/**
 * @brief Nastaví voľbu SO_REUSEADDR na sockete.
 *
 * Umožňuje rýchle znovu-spustenie servera na rovnakom porte bez čakania,
 * kým sa staré spojenia úplne "uvoľnia" (TIME_WAIT).
 *
 * @param fd File descriptor socketu.
 * @return 0 pri úspechu, -1 pri chybe (podľa setsockopt()).
 */
static int set_reuseaddr(int fd) {
    int opt = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

/**
 * @brief Vytvorí TCP serverový socket a začne počúvať na zadanom porte.
 *
 * Kroky:
 * 1) socket(AF_INET, SOCK_STREAM) -> TCP IPv4 socket
 * 2) SO_REUSEADDR
 * 3) bind na INADDR_ANY:port
 * 4) listen(backlog)
 *
 * @param port Port na počúvanie (host byte order).
 * @param backlog Maximálny počet čakajúcich pripojení.
 * @return File descriptor počúvajúceho socketu, alebo -1 pri chybe.
 */
int net_listen(uint16_t port, int backlog) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    // (void) ignoruje návratovú hodnotu, lebo SO_REUSEADDR nie je kritické
    (void)set_reuseaddr(fd);

    // Nastavenie adresy servera
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 (počúvaj na všetkých rozhraniach)
    addr.sin_port = htons(port);              // port v network byte order

    // Naviazanie socketu na adresu a port
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    // Začni počúvať na prichádzajúce pripojenia
    if (listen(fd, backlog) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

/**
 * @brief Prijme (accept) prichádzajúce pripojenie.
 *
 * Funkcia blokuje, kým sa nepripojí klient (ak socket nie je neblokujúci).
 * Vráti nový file descriptor, cez ktorý sa komunikuje s konkrétnym klientom.
 *
 * @param listen_fd File descriptor počúvajúceho socketu.
 * @return File descriptor klientského socketu, alebo -1 pri chybe.
 */
int net_accept(int listen_fd) {
    return accept(listen_fd, NULL, NULL);
}

/**
 * @brief Pripojí sa na TCP server na host:port.
 *
 * Používa getaddrinfo(), aby podporovalo:
 * - hostname (napr. "frios2.fri.uniza.sk")
 * - IPv4 adresy (napr. "127.0.0.1")
 *
 * Postup:
 * 1) getaddrinfo(host, port_str, hints, &res)
 * 2) skúša postupne každú adresu z výsledku (res)
 * 3) pri prvom úspešnom connect() skončí a vráti fd
 *
 * @param host Názov hostiteľa alebo IP adresa.
 * @param port Port servera (host byte order).
 * @return File descriptor pripojeného socketu, alebo -1 pri chybe.
 *
 * @note Ak getaddrinfo zlyhá, vracia -1. (Neodlišujeme typ chyby, je to OK pre skeleton.)
 */
int net_connect(const char* host, uint16_t port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    // port ako string (getaddrinfo to vyžaduje)
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%u", (unsigned)port);

    // Výsledný zoznam adries od DNS/resolve
    struct addrinfo* res = NULL;
    if (getaddrinfo(host, port_str, &hints, &res) != 0) return -1;

    int fd = -1;

    // Skús sa pripojiť na prvú funkčnú adresu
    for (struct addrinfo* p = res; p != NULL; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;

        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) break;

        // connect zlyhal -> zatvor fd a skús ďalšiu adresu
        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    return fd;
}

/**
 * @brief Pošle presne len bajtov (v prípade potreby opakovane volá send()).
 *
 * send() môže poslať menej bajtov než požadujeme, preto posielame v slučke.
 * Ošetrujeme EINTR (prerušenie signálom) tak, že volanie zopakujeme.
 *
 * @param fd Socket file descriptor.
 * @param buf Dáta na odoslanie.
 * @param len Počet bajtov na odoslanie.
 * @return 0 pri úspechu, -1 pri chybe.
 */
int net_send_all(int fd, const void* buf, size_t len) {
    const unsigned char* p = (const unsigned char*)buf;
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue; // prerušené signálom -> skús znovu
            return -1;
        }
        if (n == 0) return -1; // peer zatvoril / nič sa neposlalo
        sent += (size_t)n;
    }

    return 0;
}

/**
 * @brief Prijme presne len bajtov (v prípade potreby opakovane volá recv()).
 *
 * recv() môže prijať menej bajtov, preto čítame v slučke.
 * Ak recv() vráti 0, znamená to, že peer ukončil spojenie.
 *
 * @param fd Socket file descriptor.
 * @param buf Buffer pre prijaté dáta.
 * @param len Počet bajtov na prijatie.
 * @return 0 pri úspechu, -1 pri chybe alebo ukončení spojenia.
 */
int net_recv_all(int fd, void* buf, size_t len) {
    unsigned char* p = (unsigned char*)buf;
    size_t recvd = 0;

    while (recvd < len) {
        ssize_t n = recv(fd, p + recvd, len - recvd, 0);
        if (n < 0) {
            if (errno == EINTR) continue; // prerušené signálom -> skús znovu
            return -1;
        }
        if (n == 0) return -1; // peer ukončil spojenie
        recvd += (size_t)n;
    }

    return 0;
}
