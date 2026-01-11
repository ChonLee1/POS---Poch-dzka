#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

int menu_read_int(const char* prompt, int minv, int maxv, int def);
unsigned menu_read_uint(const char* prompt, unsigned minv, unsigned maxv, unsigned def);
int menu_read_choice(void);
void menu_read_dir_percents(uint8_t* up, uint8_t* down, uint8_t* left, uint8_t* right);