#include "protocol.h"


/**
 * @brief Odošle jednu správu protokolu cez socket.
 *
 * Formát správy:
 *   1) msg_header_t (type + length)
 *   2) payload (len bajtov), ak len > 0
 *
 * Hodnoty type a length sa posielajú v network byte order, aby to fungovalo
 * konzistentne na rôznych architektúrach.
 *
 * @param fd File descriptor pripojeného socketu.
 * @param type Typ správy (msg_type_t).
 * @param payload Ukazovateľ na payload dáta (môže byť NULL, ak len == 0).
 * @param len Veľkosť payloadu v bajtoch.
 * @return 0 pri úspechu, -1 pri chybe.
 */
int proto_send(int fd, msg_type_t type, const void* payload, uint32_t len) {
    // Priprav hlavičku
    msg_header_t h;
    h.type = htonl((uint32_t)type); // host -> network byte order
    h.length = htonl(len);

    // Najprv odošli hlavičku
    if (net_send_all(fd, &h, sizeof(h)) != 0) return -1;

    // Ak existuje payload, odošli ho
    if (len > 0 && payload != NULL) {
        if (net_send_all(fd, payload, len) != 0) return -1;
    }

    return 0;
}

/**
 * @brief Prijme jednu správu protokolu zo socketu.
 *
 * Funkcia najprv prečíta hlavičku (msg_header_t), zistí dĺžku payloadu a
 * potom payload načíta do poskytnutého bufferu.
 *
 * Bezpečnostné pravidlo:
 * - Ak je payload väčší než buf_cap, funkcia skončí chybou (-1),
 *   aby sme nezapisovali mimo buffer (preťaženie pamäte).
 *
 * @param fd File descriptor pripojeného socketu.
 * @param out_type Výstupný parameter pre typ správy (môže byť NULL).
 * @param payload_buf Buffer, kam sa uloží payload (musí byť nenulový, ak len > 0).
 * @param buf_cap Kapacita payload_buf v bajtoch.
 * @param out_len Výstupná dĺžka payloadu (môže byť NULL).
 * @return 0 pri úspechu, -1 pri chybe.
 */
int proto_recv(int fd, msg_type_t* out_type, void* payload_buf, uint32_t buf_cap, uint32_t* out_len) {
    msg_header_t h;

    // Najprv prijmi hlavičku správy
    if (net_recv_all(fd, &h, sizeof(h)) != 0) return -1;

    // Preveď z network -> host byte order
    uint32_t type = ntohl(h.type);
    uint32_t len  = ntohl(h.length);

    // Vyplň výstupné parametre
    if (out_type) *out_type = (msg_type_t)type;
    if (out_len) *out_len = len;

    // Ak je payload väčší ako buffer, odmietni (ochrana)
    if (len > buf_cap) return -1;

    // Ak správa obsahuje payload, načítaj ho
    if (len > 0) {
        if (payload_buf == NULL) return -1;
        if (net_recv_all(fd, payload_buf, len) != 0) return -1;
    }

    return 0;
}
