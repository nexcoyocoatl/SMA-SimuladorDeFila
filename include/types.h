#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdbool.h>
#include "macro_dynbuffer.h"

enum EntryType { ARRIVAL, SERVICE, EXCHANGE };      // Tipos de entrada na lista de eventos e escalonador (Entrada, Saída, Passagem)

// Struct da fila
typedef struct {
    uint64_t index;                                 // Número índice da fila
    uint64_t num_servers;                           // Número de atendentes
    uint64_t capacity;                              // Capacidade da fila, dinâmica se infinita
    uint64_t customers;                             // Clientes na fila
    uint64_t loss;                                  // Unidades perdidas da fila
    double first_arrival;                           // Primeira chegada
    double min_arrival;                             // Número mínimo da chegada
    double max_arrival;                             // Número máximo da chegada
    double min_service;                             // Número mínimo da saída
    double max_service;                             // Número máximo da saída
    dynbuffer(double) times;                        // Buffer dinâmico de tempos acumulados para cada estado da fila
    dynbuffer(double) exit_odds;                    // Buffer dinâmico de probabilidades para cada saída
    dynbuffer(int) exit_to;                         // Buffer dinâmico de destinos possíveis
    bool b_infinite_capacity;                       // Indica se a fila é infinita
} queue;

// Struct do evento
typedef struct {                                    // Struct de entradas da lista de eventos
    enum EntryType entry_type;                      // Tipo da entrada
    queue *queue_from;                              // Ponteiro para fila origem
    queue *queue_to;                                // Ponteiro para fila destino
    uint64_t index;                                 // Número índice da entrada
    double time;                                    // Tempo que será executado
    double draw;                                    // Sorteio do RNG
    dynbuffer(uint64_t) queue_sizes;                // Buffer dinâmico de tamanhos das filas
    dynbuffer(dynbuffer(double)) queue_states;      // Buffer dinâmico de estados de cada fila (tempo total de cada estado de cada fila)
    bool b_removed;                                 // Boolean para indicar se já foi utilizado e removido
    bool b_loss;                                    // Boolean para indicar se houve uma perda de unidade neste evento
} event_entry;

#endif