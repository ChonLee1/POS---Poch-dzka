#include "menu.h"

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

void menu_read_dir_percents(uint8_t* up, uint8_t* down, uint8_t* left, uint8_t* right) {
    for (;;) {
        unsigned u = menu_read_uint("Percent hore (UP)", 0, 100, 25);
        unsigned d = menu_read_uint("Percent dole (DOWN)", 0, 100, 25);
        unsigned l = menu_read_uint("Percent vlavo (LEFT)", 0, 100, 25);
        unsigned r = menu_read_uint("Percent vpravo (RIGHT)", 0, 100, 25);

        unsigned sum = u + d + l + r;
        if (sum != 100) {
            printf("Chyba: sucet percent musi byt 100 (teraz %u). Skus znova.\n", sum);
            continue;
        }

        *up = (uint8_t)u;
        *down = (uint8_t)d;
        *left = (uint8_t)l;
        *right = (uint8_t)r;
        return;
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

