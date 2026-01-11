/**
 * @file results.c
 * @brief Implementácia jednoduchých štatistík simulácie.
 */

#include "results.h"

#include <stdio.h>
#include <string.h>

/**
 * Resetuje štatistiku simulácie na počiatočný stav.
 * Vynuluje všetky počítadlá a inicializuje min_steps na maximálnu hodnotu.
 * 
 * @param r Ukazovateľ na štruktúru s výsledkami
 */
void results_reset(results_t* r) {
	if (!r) return;
	memset(r, 0, sizeof(*r));
	r->min_steps = (uint32_t)-1; /* inicializácia na maximum */
}

/**
 * Nastavuje parametre simulácie v štatistikách.
 * Ukladá informácie o veľkosti sveta, pravdepodobnostiach a počte opakovaní.
 * 
 * @param r Ukazovateľ na štruktúru s výsledkami
 * @param width Šírka sveta simulácie
 * @param height Výška sveta simulácie
 * @param kmax Maximálny počet krokov v jednom opakovaní
 * @param p_up Pravdepodobnosť pohybu nahor (v percentách)
 * @param p_down Pravdepodobnosť pohybu nadol (v percentách)
 * @param p_left Pravdepodobnosť pohybu vľavo (v percentách)
 * @param p_right Pravdepodobnosť pohybu vpravo (v percentách)
 * @param reps_total Celkový počet opakovaní simulácie
 */
void results_set_params(results_t* r,
						int32_t width, int32_t height, uint32_t kmax,
						uint8_t p_up, uint8_t p_down, uint8_t p_left, uint8_t p_right,
						uint32_t reps_total) {
	if (!r) return;
	r->width = width;
	r->height = height;
	r->k_max = kmax;
	r->p_up = p_up;
	r->p_down = p_down;
	r->p_left = p_left;
	r->p_right = p_right;
	r->reps_total = reps_total;
}

/**
 * Prevádza počet krokov na index histogramu.
 * Rozdeľuje kroky do 4 kategórií:
 * - bin 0: 0-20 krokov
 * - bin 1: 21-50 krokov  
 * - bin 2: 51-100 krokov
 * - bin 3: 101+ krokov
 * 
 * @param steps Počet krokov v simulácii
 * @return Index histogramu (0-3)
 */
static int steps_to_bin(uint32_t steps) {
	if (steps <= 20u) return 0;
	if (steps <= 50u) return 1;
	if (steps <= 100u) return 2;
	return 3;
}

/**
 * Zaznamenáva výsledok jedného opakovaní simulácie.
 * Aktualizuje štatistiky úspešnosti, počet krokov a histogramy.
 * 
 * @param r Ukazovateľ na štruktúru s výsledkami
 * @param steps Počet krokov, ktoré trvalo opakovaní
 * @param success Indikátor úspechu (1 = dosiahlo (0,0), 0 = nezasiahlo)
 */
void results_record_rep(results_t* r, uint32_t steps, int success) {
	if (!r) return;
	if (success) {
		r->success_count++;
		r->sum_steps_success += steps;
		if (steps < r->min_steps) r->min_steps = steps;
		if (steps > r->max_steps) r->max_steps = steps;
		int b = steps_to_bin(steps);
		if (b >= 0 && b < 4) r->bins[b]++;
	} else {
		r->fail_count++;
	}
}

/**
 * Vypisuje podrobný súhrn výsledkov simulácie.
 * Zobrazuje parametre sveta, percento úspešnosti, štatistiku krokov a histogram.
 * 
 * @param r Ukazovateľ na štruktúru s výsledkami na výstup
 */
void results_print(const results_t* r) {
	if (!r) return;

	printf("\n=== Simulation summary ===\n");
	printf("World: %dx%d, Kmax=%u, reps=%u\n",
		   (int)r->width, (int)r->height, (unsigned)r->k_max, (unsigned)r->reps_total);
	printf("Percents: U=%u D=%u L=%u R=%u\n",
		   (unsigned)r->p_up, (unsigned)r->p_down, (unsigned)r->p_left, (unsigned)r->p_right);

	printf("Total reps: %u\n", (unsigned)r->reps_total);
	printf("Reached (0,0): %u (%.1f%%)\n", (unsigned)r->success_count,
		   r->reps_total ? (100.0 * (double)r->success_count / (double)r->reps_total) : 0.0);
	printf("Not reached:  %u (%.1f%%)\n", (unsigned)r->fail_count,
		   r->reps_total ? (100.0 * (double)r->fail_count / (double)r->reps_total) : 0.0);

	if (r->success_count > 0) {
		double avg = (double)r->sum_steps_success / (double)r->success_count;
		printf("Steps (successful): avg=%.2f min=%u max=%u\n", avg,
			   (unsigned)r->min_steps, (unsigned)r->max_steps);
		printf("Histogram (successful steps):\n");
		printf("  0-20 : %u\n", (unsigned)r->bins[0]);
		printf("  21-50: %u\n", (unsigned)r->bins[1]);
		printf("  51-100: %u\n", (unsigned)r->bins[2]);
		printf("  101+ : %u\n", (unsigned)r->bins[3]);
	} else {
		printf("No successful replications -> no step stats available.\n");
	}

	printf("==========================\n\n");
}

