#include "protocol.h"
#include "net.h"

#include <arpa/inet.h>

int proto_send(int fd, msg_type_t type, const void* payload, uint32_t len) {
    msg_header_t h;
    h.type = htonl((uint32_t)type);
    h.length = htonl(len);

    if (net_send_all(fd, &h, sizeof(h)) != 0) return -1;
    if (len > 0 && payload != NULL) {
        if (net_send_all(fd, payload, len) != 0) return -1;
    }
    return 0;
}

int proto_recv(int fd, msg_type_t* out_type, void* payload_buf, uint32_t buf_cap, uint32_t* out_len) {
    msg_header_t h;
    if (net_recv_all(fd, &h, sizeof(h)) != 0) return -1;

    uint32_t type = ntohl(h.type);
    uint32_t len  = ntohl(h.length);

    if (out_type) *out_type = (msg_type_t)type;
    if (out_len) *out_len = len;

    if (len > buf_cap) return -1;
    if (len > 0) {
        if (payload_buf == NULL) return -1;
        if (net_recv_all(fd, payload_buf, len) != 0) return -1;
    }
    return 0;
}
