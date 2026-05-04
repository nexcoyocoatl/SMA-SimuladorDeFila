#ifndef __RANDOM_GEN_H__
#define __RANDOM_GEN_H__

#include <stdint.h>

extern uint64_t rng_count;                                                  // Contador de números RNG utilizados
extern uint64_t previous;                                                   // Último número RNG computado, inicializado aqui com o seed

double linear_congruential_generator(uint64_t a, uint64_t c, uint64_t M);   // Gerador de número aleatório a partir de congruência linear
float next_random(void);                                                    // Cálcula próximo número aleatório com números hardcoded escolhidos para simulação
double calculate_draw(uint64_t min, uint64_t max);                          // Faz sorteio entre dois números utilizando número aleatório

#endif