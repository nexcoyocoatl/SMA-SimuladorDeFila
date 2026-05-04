#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "globals.h"
#include "simulate_queue.h"
#include "random_gen.h"
#include "macro_dynarray.h"
#include "macro_dynbuffer.h"
#include "types.h"
#include "utils.h"
#include "string_t.h"

// TODO: Documentação
// TODO: Leitura .yml

#define DEBUG 0                                                             // Para ativar modo debug (imprime todos os eventos)

bool b_finished = false;                                                    // Boolean para finalizar o loop do main (quando o número máximo de números aleatórios é atingido)

// Tempo e RNG da simulação
uint64_t max_num_rng = 100000;                                              // Número de números pseudoaleatórios a serem calculados
double current_time = 0.0;                                                  // Tempo atual da simulação (incrementa a cada evento)

// Buffer dinâmico de filas
uint64_t num_queues = 3;                                                    // Número de filas
dynbuffer(queue) queues = NULL;                                             // Buffer de filas da simulação

// Listas dinâmicas de eventos
dynarray(event_entry) events = NULL;                                        // Lista de eventos em ordem de criação
dynarray(uint64_t) chronological_events_indexes = NULL;                     // Lista de índices de eventos em ordem de execução
dynarray(uint64_t) current_scheduled_entries_indexes = NULL;                // Lista de índices de eventos não executadas do escalonador

void setup(void);                                                           // Função para inicializar valores padrão e outras configurações iniciais

// Função para inicializar valores padrão e outras configurações iniciais
void setup(void)
{
    STRING_CREATE(dsad, "DSADSA");
    string_free(&dsad);

    
    // TODO: leitura do .yml para popular as variáveis de número de eventos, filas e etc
    // num_queues = ?;
    // max_num_rng = ?;

    dynbuffer_init_n(&queues, num_queues);
    dynarray_init_n(&events, max_num_rng+1);
    dynarray_init_n(&chronological_events_indexes, max_num_rng+1);
    dynarray_init_n(&current_scheduled_entries_indexes, max_num_rng+1);

    // TODO: código do escopo abaixo será removido e receberá do .yml
    {
    queues[0] = (queue){
            .index = 0,
            .b_infinite_capacity = true,
            .num_servers = 1,
            .capacity = 0,
            .customers = 0,
            .loss = 0,
            .first_arrival = 2.0,
            .min_arrival = 2.0,
            .max_arrival = 4.0,
            .min_service = 1.0,
            .max_service = 2.0
        };
        
        dynbuffer_init_n(&(queues[0].exit_to), 2);
        dynbuffer_init_n(&(queues[0].exit_odds), 2);

        queues[0].exit_to[0] = 1;
        queues[0].exit_odds[0] = 0.8;
        queues[0].exit_to[1] = 2;
        queues[0].exit_odds[1] = 0.2;

     queues[1] = (queue){
            .index = 1,
            .b_infinite_capacity = false,
            .num_servers = 2,
            .capacity = 5,
            .customers = 0,
            .loss = 0,
            .first_arrival = 0.0,
            .min_arrival = 0.0,
            .max_arrival = 0.0,
            .min_service = 4.0,
            .max_service = 6.0
        };

        dynbuffer_init_n(&(queues[1].exit_to), 3);
        dynbuffer_init_n(&(queues[1].exit_odds), 3);

        queues[1].exit_to[0] = 0;
        queues[1].exit_odds[0] = 0.3;
        queues[1].exit_to[1] = -1;
        queues[1].exit_odds[1] = 0.2;
        queues[1].exit_to[2] = 1;
        queues[1].exit_odds[2] = 0.5;

        queues[2] = (queue){
            .index = 2,
            .b_infinite_capacity = false,
            .num_servers = 2,
            .capacity = 10,
            .customers = 0,
            .loss = 0,
            .first_arrival = 0.0,
            .min_arrival = 0.0,
            .max_arrival = 0.0,
            .min_service = 5.0,
            .max_service = 15.0
        };

        dynbuffer_init_n(&(queues[2].exit_to), 2);
        dynbuffer_init_n(&(queues[2].exit_odds), 2);

        queues[2].exit_to[0] = 2;
        queues[2].exit_odds[0] = 0.7;
        queues[2].exit_to[1] = -1;
        queues[2].exit_odds[1] = 0.3;
    }

    // Popula filas
    for (uint64_t i = 0; i < dynbuffer_capacity(&queues); i++)
    {
        // uint64_t queue_capacity = ?;  // TODO: receberá do .yml
        // queues[i].capacity = queue_capacity;

        // uint64_t queue_capacity = queues[i].capacity; // TODO: remover quando o código de leitura do .yml estiver funcionando

        if (queues[i].b_infinite_capacity)
        {
            // Inicia fila com capacidade para uma unidade
            queues[i].capacity = 1;
            dynbuffer_init(&(queues[i].times));
        }
        else
        {
            dynbuffer_init_n(&(queues[i].times), queues[i].capacity+1);
        }
    }
}

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
