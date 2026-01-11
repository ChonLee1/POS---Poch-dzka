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

int server_run(uint16_t port);