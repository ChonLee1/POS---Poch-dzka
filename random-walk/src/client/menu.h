#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

int menu_read_int(const char* prompt, int minv, int maxv, int def);
unsigned menu_read_uint(const char* prompt, unsigned minv, unsigned maxv, unsigned def);
int menu_read_choice(void);