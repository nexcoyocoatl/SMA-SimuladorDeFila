#include "random_gen.h"
#include "globals.h"

uint64_t rng_count = 0;             // Contador de números RNG utilizados
uint64_t previous = 4651815687;     // Último número RNG computado, inicializado aqui com o seed

// Gerador de número aleatório a partir de congruência linear
double linear_congruential_generator(uint64_t a, uint64_t c, uint64_t M)
{
    previous = (((a * previous) + c) % M);
    return ((double)previous)/M;
}

// Cálcula próximo número aleatório com números hardcoded escolhidos para simulação
float next_random(void)
{
    uint64_t a = 54083;
    uint64_t c = 197;
    uint64_t M = UINT32_MAX;

    return linear_congruential_generator(a, c, M);
}

// Faz sorteio entre dois números utilizando número aleatório
double calculate_draw(uint64_t min, uint64_t max)
{
    rng_count++;
    if (rng_count >= max_num_rng)
    {
        b_finished = true;
    }
    return min + ((max - min) * next_random());
}