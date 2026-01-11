/**
 * @file results.h
 * @brief Jednoduché štatistiky výsledkov simulácie.
 */

#pragma once
#include <stdint.h>

/**
 * @struct results_t
 * @brief Štruktúra na ukladanie štatistických údajov simulácie.
 *
 * Uchovává informácie o úspešnosti simulácií, počte krokov a histograme.
 * Obsahuje aj kopie parametrov simulácie na účely výstupu.
 */
typedef struct {
	uint32_t reps_total;           /**< Celkový počet replikácií */

	uint32_t success_count;        /**< Počet úspešných replikácií (dosiahli (0,0)) */
	uint32_t fail_count;           /**< Počet neúspešných replikácií */

	uint64_t sum_steps_success;    /**< Súčet krokov všetkých úspešných replikácií */
	uint32_t min_steps;            /**< Minimálny počet krokov medzi úspešnými */
	uint32_t max_steps;            /**< Maximálny počet krokov medzi úspešnými */

	/** Jednoduchý histogram krokov (iba úspešné replikácie):
	 *  - bins[0]: 0-20 krokov
	 *  - bins[1]: 21-50 krokov
	 *  - bins[2]: 51-100 krokov
	 *  - bins[3]: 101+ krokov
	 */
	uint32_t bins[4];

	/* Echo parametre (nepovinné, len na tlač) */
	int32_t  width;                /**< Šírka simulačného sveta */
	int32_t  height;               /**< Výška simulačného sveta */
	uint32_t k_max;                /**< Maximálny počet krokov na replikáciu */
	uint8_t  p_up, p_down, p_left, p_right;  /**< Pravdepodobnosti pohybu v percentách */
} results_t;

/**
 * @brief Resetuje štatistiku simulácie na počiatočný stav.
 * @param r Ukazovateľ na štruktúru s výsledkami.
 */
void results_reset(results_t* r);

/**
 * @brief Nastavuje parametre simulácie v štatistikách.
 * @param r Ukazovateľ na štruktúru s výsledkami.
 * @param width Šírka sveta simulácie.
 * @param height Výška sveta simulácie.
 * @param kmax Maximálny počet krokov v jednom opakovaní.
 * @param p_up Pravdepodobnosť pohybu nahor (v percentách).
 * @param p_down Pravdepodobnosť pohybu nadol (v percentách).
 * @param p_left Pravdepodobnosť pohybu vľavo (v percentách).
 * @param p_right Pravdepodobnosť pohybu vpravo (v percentách).
 * @param reps_total Celkový počet opakovaní simulácie.
 */
void results_set_params(results_t* r,
						int32_t width, int32_t height, uint32_t kmax,
						uint8_t p_up, uint8_t p_down, uint8_t p_left, uint8_t p_right,
						uint32_t reps_total);

/**
 * @brief Zaznamenáva výsledok jedného opakovaní simulácie.
 * @param r Ukazovateľ na štruktúru s výsledkami.
 * @param steps Počet krokov, ktoré trvalo opakovaní.
 * @param success Indikátor úspechu (1 = dosiahlo (0,0), 0 = nezasiahlo).
 */
void results_record_rep(results_t* r, uint32_t steps, int success);

/**
 * @brief Vypisuje podrobný súhrn výsledkov simulácie.
 * @param r Ukazovateľ na štruktúru s výsledkami na výstup.
 */
void results_print(const results_t* r);

