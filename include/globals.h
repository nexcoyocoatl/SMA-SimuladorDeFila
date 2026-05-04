#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "macro_dynarray.h"
#include "types.h"

extern bool b_finished;                                         // Boolean para finalizar o loop do main (quando o número máximo de números aleatórios é atingido)

// Tempo e RNG da simulação
extern double current_time;                                     // Tempo atual da simulação (incrementa a cada evento)

// Buffer dinâmico de filas
extern uint64_t num_queues;                                     // Número de filas
extern dynbuffer(queue) queues;                                 // Buffer de filas da simulação

// Listas dinâmicas de eventos
extern dynarray(event_entry) events;                            // Lista de eventos em ordem de criação
extern dynarray(uint64_t) chronological_events_indexes;         // Lista de índices de eventos em ordem de execução
extern dynarray(uint64_t) current_scheduled_entries_indexes;    // Lista de índices de eventos não executadas do escalonador

#endif