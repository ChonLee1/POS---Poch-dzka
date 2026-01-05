#pragma once
#include <stdint.h>

typedef enum {
    MSG_HELLO = 1,
    MSG_HELLO_ACK = 2,
    MSG_STATE = 3,
    MSG_QUIT = 4,
    MSG_START = 5
} msg_type_t;

typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t length;   // dĺžka payloadu v bajtoch
} msg_header_t;

typedef struct __attribute__((packed)) {
    int32_t x;
    int32_t y;
    uint32_t step;
} msg_state_t;

typedef struct __attribute__((packed)) {
    int32_t width;     // šírka sveta (>=1)
    int32_t height;    // výška sveta (>=1)
    uint32_t k_max;    // max počet krokov
    uint32_t seed;     // seed pre rand()
} msg_start_t;

int proto_send(int fd, msg_type_t type, const void* payload, uint32_t len);
int proto_recv(int fd, msg_type_t* out_type, void* payload_buf, uint32_t buf_cap, uint32_t* out_len);
