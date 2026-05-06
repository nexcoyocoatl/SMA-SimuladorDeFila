#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "globals.h"
#include "simulate_queue.h"
#include "macro_dynarray.h"
#include "macro_dynbuffer.h"
#include "read_file.h"
#include "types.h"
#include "utils.h"

bool b_finished = false;                                                    // Boolean para finalizar o loop do main (quando o número máximo de números aleatórios é atingido)

// Tempo e RNG da simulação
uint64_t max_num_rng = 100;                                                 // Número de números pseudoaleatórios a serem calculados
double current_time = 0.0;                                                  // Tempo atual da simulação (incrementa a cada evento)

// Listas dinâmicas de filas e eventos
dynarray(queue) queues = NULL;                                              // Buffer de filas da simulação
dynarray(event_entry) events = NULL;                                        // Lista de eventos em ordem de criação
dynarray(uint64_t) chronological_events_indexes = NULL;                     // Lista de índices de eventos em ordem de execução
dynarray(uint64_t) current_scheduled_entries_indexes = NULL;                // Lista de índices de eventos não executadas do escalonador

void setup(void);                                                           // Função para inicializar valores padrão e outras configurações iniciais
void write_default_config(const char *filename);                            // Função para criar model.yml baseado no T1 caso não exista

char *filename = NULL;

// Função para inicializar valores padrão e outras configurações iniciais
void setup(void)
{   
    // Arrays dinâmicas não inicializam memória em 0, e o controle está no seu tamanho, não capacidade
    dynarray_init(&queues);

    parse_config(filename);

    dynarray_init_n(&events, max_num_rng+1);
    dynarray_init_n(&chronological_events_indexes, max_num_rng+1);
    dynarray_init_n(&current_scheduled_entries_indexes, max_num_rng+1);

    // Popula filas
    for (uint64_t i = 0; i < dynarray_size(&queues); i++)
    {
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

void write_default_config(const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (!f) { fprintf(stderr, "Error: could not create %s\n", filename); return; }

    fprintf(f,
        "arrivals:\n"
        "   Q1: 2.0\n"
        "\n"
        "queues:\n"
        "   Q1:\n"
        "      servers: 1\n"
        "      minArrival: 2.0\n"
        "      maxArrival: 4.0\n"
        "      minService: 1.0\n"
        "      maxService: 2.0\n"
        "   Q2:\n"
        "      servers: 2\n"
        "      capacity: 5\n"
        "      minService: 4.0\n"
        "      maxService: 6.0\n"
        "   Q3:\n"
        "      servers: 2\n"
        "      capacity: 10\n"
        "      minService: 5.0\n"
        "      maxService: 15.0\n"
        "\n"
        "network:\n"
        "-  source: Q1\n"
        "   target: Q2\n"
        "   probability: 0.8\n"
        "-  source: Q1\n"
        "   target: Q3\n"
        "   probability: 0.2\n"
        "-  source: Q2\n"
        "   target: Q1\n"
        "   probability: 0.3\n"
        "-  source: Q2\n"
        "   target: Q2\n"
        "   probability: 0.5\n"
        "-  source: Q3\n"
        "   target: Q3\n"
        "   probability: 0.7\n"
        "\n"
        "maxrndnumbers: 100000\n"
    );

    fclose(f);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        filename = "model.yml";
        FILE *test = fopen(filename, "r");
        if (!test)
        {
            printf("'model.yml' not found, creating default...\n\n");
            write_default_config("model.yml");
        }
        else fclose(test);
    }
    else
    {
        filename = argv[1];
        FILE *test = fopen(filename, "r");
        if (!test)
        {
            printf("Config file '%s' not found, using/creating default 'model.yml'...\n\n", filename);
            filename = "model.yml";
            FILE *default_test = fopen("model.yml", "r");
            if (!default_test)
            {
                printf("'model.yml' not found, creating default...\n\n");
                write_default_config("model.yml");
            }
            else fclose(default_test);
        }
        else fclose(test);
    }

    setup();

    // Inclui eventos com first arrival
    for (uint64_t i = 0; i < dynarray_size(&queues); i++)
    {
        if (queues[i].b_arrival) 
        {
            dynarray_push_last(&events, ((event_entry) {
                        .entry_type = ARRIVAL,
                        .queue_from = &queues[i],
                        .queue_to = NULL,
                        .index = (dynarray_size(&events)),
                        .time = queues[i].first_arrival,
                        .draw = queues[i].first_arrival,
                        .b_removed = false,
                        .b_loss = false
                    }));
            event_entry *new_event = &(events[0]);

            // Inicializa buffers
            if (DEBUG)
            {
                dynbuffer_init_n(&(new_event->queue_sizes), dynarray_size(&queues));
                dynbuffer_init_n(&(new_event->queue_states), dynarray_size(&queues));
                for (uint64_t i = 0; i < dynarray_size(&queues); i++)
                {
                    dynbuffer_init_n(&(new_event->queue_states[i]), queues[i].capacity + 1);
                }
            }

            dynarray_push_last(&current_scheduled_entries_indexes, new_event->index);
        }
    }    

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

        for (uint64_t i = 0; i < dynarray_size(&queues); i++)
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
        for (uint64_t i = 0; i < dynarray_size(&queues); i++)
        {
            printf(" %8d |", 0);
            for (uint64_t j = 0; j < dynarray_size(&(queues[i].times)); j++)
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
        printf("\n");
    }

    printf("Probabilities of each queue state:\n");
    print_queue_state_percentage_calc();
    printf("Simulation time: %lf\n\n", current_time);

    // Libera memória das queues (listas dinâmicas)
    if (queues != NULL)
    {
        for (uint64_t i = 0; i < dynarray_size(&queues); i++)
        {
            free(queues[i].name);  // add this
            dynbuffer_free(&(queues[i].times));
            dynarray_free(&(queues[i].exit_to));
            dynarray_free(&(queues[i].exit_odds));
        }
        dynarray_free(&queues);
    }

    // Libera memória dos eventos (listas dinâmicas)
    if (events != NULL)
    {
        // Libera memória apenas dos structs que foram inicializados, para isso contando pelo size
        // Se debug estiver desligado, não foram inicializados
        if (DEBUG)
        {
            for (uint64_t i = 0; i < dynarray_size(&events); i++)
            {
                dynbuffer_free(&(events[i].queue_sizes));
                
                for (uint64_t j = 0; j < dynbuffer_capacity(&(events[i].queue_states)); j++)
                {
                    dynbuffer_free(&(events[i].queue_states[j]));
                }
                dynbuffer_free(&(events[i].queue_states));
            
            }
        }        
        dynarray_free(&events);
    }
    dynarray_free(&chronological_events_indexes);
    dynarray_free(&current_scheduled_entries_indexes);

    return 0;
}
