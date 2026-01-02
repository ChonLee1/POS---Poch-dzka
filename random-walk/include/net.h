#pragma once
#include <stddef.h>
#include <stdint.h>

int net_listen(uint16_t port, int backlog);
int net_accept(int listen_fd);
int net_connect(const char* host, uint16_t port);

int net_send_all(int fd, const void* buf, size_t len);
int net_recv_all(int fd, void* buf, size_t len);
