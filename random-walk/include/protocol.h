#pragma once
#include <stdint.h>

typedef enum {
    MSG_HELLO = 1,
    MSG_HELLO_ACK = 2
} msg_type_t;

typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t length;   // payload length
} msg_header_t;

int proto_send(int fd, msg_type_t type, const void* payload, uint32_t len);
int proto_recv(int fd, msg_type_t* out_type, void* payload_buf, uint32_t buf_cap, uint32_t* out_len);
