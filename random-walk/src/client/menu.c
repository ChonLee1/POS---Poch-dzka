#include "menu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int read_line(char* buf, size_t cap) {
    if (!fgets(buf, cap, stdin)) return -1;
    size_t n = strlen(buf);
    if (n && buf[n - 1] == '\n') buf[n - 1] = 0;
    return 0;
}

int menu_read_choice(void) {
    char line[64];
    printf("\n=== MENU ===\n");
    printf("1) Nova simulacia (spawn server + START)\n");
    printf("2) Pripojit sa k simulacii (iba connect)\n");
    printf("3) Koniec\n");
    printf("Volba: ");
    fflush(stdout);

    if (read_line(line, sizeof(line)) != 0) return 3;
    return atoi(line);
}

int menu_read_int(const char* prompt, int minv, int maxv, int def) {
    char line[128];
    for (;;) {
        printf("%s [%d..%d] (enter=%d): ", prompt, minv, maxv, def);
        fflush(stdout);

        if (read_line(line, sizeof(line)) != 0) return def;
        if (line[0] == 0) return def;

        errno = 0;
        char* end = NULL;
        long v = strtol(line, &end, 10);
        if (errno != 0 || end == line || *end != 0) {
            printf("Zla hodnota.\n");
            continue;
        }
        if (v < minv || v > maxv) {
            printf("Mimo rozsah.\n");
            continue;
        }
        return (int)v;
    }
}

unsigned menu_read_uint(const char* prompt, unsigned minv, unsigned maxv, unsigned def) {
    char line[128];
    for (;;) {
        printf("%s [%u..%u] (enter=%u): ", prompt, minv, maxv, def);
        fflush(stdout);

        if (read_line(line, sizeof(line)) != 0) return def;
        if (line[0] == 0) return def;

        errno = 0;
        char* end = NULL;
        unsigned long v = strtoul(line, &end, 10);
        if (errno != 0 || end == line || *end != 0) {
            printf("Zla hodnota.\n");
            continue;
        }
        if (v < minv || v > maxv) {
            printf("Mimo rozsah.\n");
            continue;
        }
        return (unsigned)v;
    }
}
