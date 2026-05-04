#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "globals.h"
#include "simulate_queue.h"
#include "macro_dynarray.h"
#include "macro_dynbuffer.h"
#include "types.h"
#include "utils.h"

// TODO: Documentação
// TODO: Leitura .yml

#define DEBUG 0                                                             // Para ativar modo debug (imprime todos os eventos)

bool b_finished = false;                                                    // Boolean para finalizar o loop do main (quando o número máximo de números aleatórios é atingido)

// Tempo da simulação
double current_time = 0.0;                                                  // Tempo atual da simulação (incrementa a cada evento)

// Buffer dinâmico de filas
uint64_t num_queues = 3;                                                    // Número de filas
dynbuffer(queue) queues = NULL;                                             // Buffer de filas da simulação

// Listas dinâmicas de eventos
dynarray(event_entry) events = NULL;                                        // Lista de eventos em ordem de criação
dynarray(uint64_t) chronological_events_indexes = NULL;                     // Lista de índices de eventos em ordem de execução
dynarray(uint64_t) current_scheduled_entries_indexes = NULL;                // Lista de índices de eventos não executadas do escalonador

int main(void)
{
    setup();

    // TODO: .yml precisa dizer onde começa o first arrival

    // Cria uma primeira entrada no escalonador e envia na função de entrada na fila 1 para início da simulação    
    dynarray_push_last(&events, ((event_entry) {
                        .entry_type = ARRIVAL,
                        .queue_from = &queues[0],
                        .queue_to = NULL,
                        .index = (dynarray_size(&events)),
                        .time = queues[0].first_arrival,
                        .draw = queues[0].first_arrival,
                        .b_removed = false,
                        .b_loss = false
                    }));

    event_entry *new_event = &(events[0]);

    // Inicializa buffers
    dynbuffer_init_n(&(new_event->queue_sizes), num_queues);
    dynbuffer_init_n(&(new_event->queue_states), num_queues);
    for (uint64_t i = 0; i < num_queues; i++)
    {
        dynbuffer_init_n(&(new_event->queue_states[i]), queues[i].capacity + 1);
    }

    arrival(current_scheduled_entries_indexes[0]);

    while (!b_finished && dynarray_size(&current_scheduled_entries_indexes) > 0)
    {
        // Ordena entradas do escalonador por tempo
        qsort(current_scheduled_entries_indexes, dynarray_size(&current_scheduled_entries_indexes), sizeof(uint64_t), compare_entries_by_time_asc);

        uint64_t event_index;
        dynarray_pop_first(&current_scheduled_entries_indexes, event_index);
        event_entry *entry = &events[event_index];

        if (entry->entry_type == ARRIVAL)
        {
            arrival(event_index);
        }
        else if (entry->entry_type == SERVICE)
        {
            service(event_index);
        }
    }

    if (DEBUG)
    {
        printf("Chronological Events:\n       TYPE      || ");
        printf("    TIME      ||");

        for (uint64_t i = 0; i < num_queues; i++)
        {
            printf("QUEUE %4lu|", (uint64_t)1);
            for (uint64_t j = 0; j < dynbuffer_capacity(&(queues[i].times)); j++)
            {
                printf("   %8lu    |", j);
            }
            printf("|");
        }

        printf("\n");

        // Imprime o primeiro
        printf("(%5d)    -     || %13f ||", 0, 0.0);
        for (uint64_t i = 0; i < num_queues; i++)
        {
            printf(" %8d |", 0);
            for (uint64_t j = 0; j < dynbuffer_capacity(&(queues[i].times)); j++)
            {
                printf(" %13f |", 0.0);
            }
            printf("|");
        }
        printf("\n");

        // Imprime os próximos
        for (uint64_t i = 0; i < dynarray_size(&chronological_events_indexes); i++)
        {
            print_chronological_entry(&events[chronological_events_indexes[i]]);
        }

        printf("\nScheduled Events:\n       TYPE      ||     TIME      ||   DRAW   ||\n");
        for (uint64_t i = 0; i < dynarray_size(&events); i++)
        {
            print_scheduled_entry(&events[i]);
        }
    }

    printf("\nProbabilities of each queue state:\n");
    print_queue_state_percentage_calc();

    // Libera memória das listas dinâmicas de capacidade fixa
    if (events != NULL)
    {
        for (uint64_t i = 0; i < dynarray_capacity(&events); i++)
        {
            dynbuffer_free(&(events[i].queue_sizes));

            if (events[i].queue_states != NULL)
            {
                for (uint64_t j = 0; j < dynbuffer_capacity(&(events[i].queue_states)); j++)
                {
                    dynbuffer_free(&(events[i].queue_states[j]));
                }
                dynbuffer_free(&(events[i].queue_states));
            }
        }
        dynarray_free(&events);
    }

    if (queues != NULL)
    {
        for (uint64_t i = 0; i < dynbuffer_capacity(&queues); i++)
        {
            dynbuffer_free(&(queues[i].times));
            dynbuffer_free(&(queues[i].exit_to));
            dynbuffer_free(&(queues[i].exit_odds));
        }
        dynbuffer_free(&queues);
    }

    dynarray_free(&chronological_events_indexes);
    dynarray_free(&current_scheduled_entries_indexes);

    return 0;
}
