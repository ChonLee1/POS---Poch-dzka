/**
 * @file menu.h
 * @brief Pomocné funkcie pre interaktívne menu a čítanie vstupu od používateľa.
 *
 * Tieto funkcie sú používané v klientskej aplikácii na získanie parametrov simulácie.
 */

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

/**
 * @brief Prečíta celočíselnú hodnotu v zadanom rozsahu.
 *
 * Funkcia opakuje výzvu, kým používateľ nezadá platnú hodnotu.
 * Stlačenie Enter vráti predvolenú hodnotu.
 *
 * @param prompt Text výzvy zobrazený používateľovi.
 * @param minv Minimálna povolená hodnota.
 * @param maxv Maximálna povolená hodnota.
 * @param def Predvolená hodnota pri prázdnom vstupe.
 * @return Prečítaná hodnota v rozsahu [minv, maxv].
 */
int menu_read_int(const char* prompt, int minv, int maxv, int def);

/**
 * @brief Prečíta nezápornú celočíselnú hodnotu v zadanom rozsahu.
 *
 * @param prompt Text výzvy zobrazený používateľovi.
 * @param minv Minimálna povolená hodnota.
 * @param maxv Maximálna povolená hodnota.
 * @param def Predvolená hodnota pri prázdnom vstupe.
 * @return Prečítaná hodnota v rozsahu [minv, maxv].
 */
unsigned menu_read_uint(const char* prompt, unsigned minv, unsigned maxv, unsigned def);

/**
 * @brief Zobrazí hlavné menu a vráti používateľovu voľbu.
 *
 * @return Číslo zvolenej možnosti (1-3), alebo 3 pri chybe.
 */
int menu_read_choice(void);

/**
 * @brief Prečíta pravdepodobnosti pohybu v percentách pre všetky smery.
 *
 * Funkcia opakuje výzvy, kým používateľ nezadá hodnoty, ktorých súčet je 100.
 *
 * @param up Výstupný parameter pre pravdepodobnosť pohybu hore (%).
 * @param down Výstupný parameter pre pravdepodobnosť pohybu dole (%).
 * @param left Výstupný parameter pre pravdepodobnosť pohybu doľava (%).
 * @param right Výstupný parameter pre pravdepodobnosť pohybu doprava (%).
 */
void menu_read_dir_percents(uint8_t* up, uint8_t* down, uint8_t* left, uint8_t* right);