#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "macro_dynarray.h"
#include "types.h"

#define DEBUG 0                                                 // Para ativar modo debug (imprime todos os eventos)

extern bool b_finished;                                         // Boolean para finalizar o loop do main (quando o número máximo de números aleatórios é atingido)

// Tempo e RNG da simulação
extern uint64_t max_num_rng;                                    // Número de números pseudoaleatórios a serem calculados
extern double current_time;                                     // Tempo atual da simulação (incrementa a cada evento)

// Listas dinâmicas de eventos e filas
extern dynarray(queue) queues;                                  // Lista de filas da simulação
extern dynarray(event_entry) events;                            // Lista de eventos em ordem de criação
extern dynarray(uint64_t) chronological_events_indexes;         // Lista de índices de eventos em ordem de execução
extern dynarray(uint64_t) current_scheduled_entries_indexes;    // Lista de índices de eventos não executadas do escalonador

extern dynarray(queue_parameters) queues_param;

#endif