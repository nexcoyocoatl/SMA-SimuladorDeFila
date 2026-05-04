#ifndef __SIMULATE_QUEUE_H__
#define __SIMULATE_QUEUE_H__

#include <stdint.h>

#include "globals.h"

// Protótipos de funções
void add_to_scheduler(enum EntryType type, queue *q);                       // Adiciona nova entrada no escalonador
void update_event_queues(event_entry *event, double added_time);            // Acrescenta tempo às filas globais e atualiza filas do evento em execução
void arrival(uint64_t event_index);                                         // Função de entrada de uma unidade na fila
void service(uint64_t event_index);                                         // Função de saída de uma unidade na fila
void calculate_service_outcome(uint64_t event_index);                       // Calcula probabilidade entre possíveis saídas e realiza a lógica de saída
void print_chronological_entry(event_entry *entry);                         // Imprime entradas cronológicas da lista de eventos
void print_scheduled_entry(event_entry *entry);                             // Imprime entradas escalonadas da lista de eventos
void print_queue_state_percentage_calc(void);                               // Imprime estados das filas e porcentagem

#endif