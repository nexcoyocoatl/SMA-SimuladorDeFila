#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#define COMPARE_ASC(a, b) (((a) > (b)) - ((a) < (b)))   // Macro para funcão auxiliar de comparação entre valores do qsort

#define QUEUE_CAPACITY 5                                // Capacidade máxima da fila
#define MAX_NUM_RNG 1000000                             // Número de números pseudoaleatórios a serem calculados
#define MAX_QUEUE_STATE QUEUE_CAPACITY+1                // Número máximo de estados da fila (Capacidade da fila + 1)

enum EntryType {NONE, ARRIVAL, SERVICE};                // Tipos de entrada na lista de eventos e escalonador (Nenhum, Entrada e Saída)

typedef struct {                                        // Struct de entradas da lista de eventos
    enum EntryType entry_type;                          // Tipo da entrada
    uint64_t queue_size;                                // Tamanho da fila
    double time;                                        // Tempo que será executado
    double queue_states[QUEUE_CAPACITY + 1];            // Estados da fila (tempo total para cada estado da fila)
} event_entry;          

typedef struct {                                        // Struct de entradas da lista do escalonador
    enum EntryType entry_type;                          // Tipo da entrada
    double time;                                        // Tempo em que o evento ocorre
    double draw;                                        // Sorteio do RNG
} scheduler_entry;          

bool b_finished = false;                                // Boolean para finalizar o loop do main (quando o número máximo de números aleatórios é atingido)
const uint64_t num_servers = 1;                         // Número de atendentes
const uint64_t queue_capacity = QUEUE_CAPACITY;         // Capacidade da fila
const double first_arrival = 2.0;                       // Primeira chegada
const double min_arrival = 2.0;                         // Número mínimo da chegada
const double max_arrival = 5.0;                         // Número máximo da chegada
const double min_service = 3.0;                         // Número mínimo da saída
const double max_service = 5.0;                         // Número máximo da saída

double current_time = 0.0;                              // Tempo atual da simulação (incrementa a cada evento)
uint64_t rng_count = 0;                                 // Contador de números RNG utilizados
uint64_t current_queue_size = 0;                        // Número atual do tamanho da fila
double current_queue_state[MAX_QUEUE_STATE];            // Estado atual da fila (tempo total para cada estado da fila)
uint64_t lost_queue_units = 0;                          // Unidades perdidas no caso da fila estar no tamanho máximo
uint64_t previous = 4651815687;                         // Último número RNG computado, inicializado aqui com o seed

scheduler_entry scheduled_entries[MAX_NUM_RNG];         // Entradas do escalonador
uint64_t scheduled_entries_count = 0;                   // Contador do número de entradas no escalonador
event_entry event_entries[MAX_NUM_RNG];                 // Entradas da lista de eventos
uint64_t event_entries_count = 0;                       // Contador do número de entradas na lista de eventos
scheduler_entry total_scheduled_entries[MAX_NUM_RNG];   // Todas entradas do escalonador, inclusive passadas
uint64_t total_scheduled_entries_count = 0;             // Contador do número de entradas totais no escalonador, inclusive passadas

// Função auxiliar do quicksort para ordenar entradas de menor para maior tempo no escalonador
int compare_entries_asc(const void *a, const void *b)
{
    float entry_a = ((scheduler_entry *)a)->time;
    float entry_b = ((scheduler_entry *)b)->time;

    return COMPARE_ASC(entry_a, entry_b);
}

// Gerador de número aleatório a partir de congruência linear
double linear_congruential_generator(uint64_t a, uint64_t c, uint64_t M)
{
    previous = (((a * previous) + c) % M);    
    return ((double)previous)/M;
}

// Cálcula próximo número aleatório com números hardcoded escolhidos para simulação
float next_random()
{
    uint64_t a = 54083;
    uint64_t c = 197;
    uint64_t M = UINT32_MAX;

    return linear_congruential_generator(a, c, M);
}

// Faz sorteio utilizando número aleatório
double calculate_draw(uint64_t min, uint64_t max)
{
    rng_count++;
    if (rng_count >= MAX_NUM_RNG)
    {
        b_finished = true;
    }
    return min + ((max - min) * next_random());
}

void print_queue_state_probability_calc()
{
    for (uint64_t i = 0; i < MAX_QUEUE_STATE; i++)
    {
        printf("%lu: %f (%f%%)\n", i, current_queue_state[i], (current_queue_state[i]/current_time));
    }
}

// Adiciona nova entrada no escalonador
void add_to_scheduler(enum EntryType type, double a, double b)
{
    double new_draw = calculate_draw(a,b);
    scheduled_entries[scheduled_entries_count] = (scheduler_entry){.entry_type = type, .draw = new_draw, .time = (current_time + new_draw)};
    total_scheduled_entries[total_scheduled_entries_count++] = scheduled_entries[scheduled_entries_count++];
}

// Função de entrada de uma unidade na fila
void arrival(scheduler_entry *scheduled_event, double rng)
{
    // Atualiza o tempo da simulação
    current_time = scheduled_event->time;

    // Cria novo evento, adicionando o tipo de entrada e tempo que está sendo executado
    event_entry *new_event = &event_entries[event_entries_count++];
    new_event->entry_type = scheduled_event->entry_type;
    new_event->time = current_time;

    // Verifica se existe espaço na fila
    if (current_queue_size < queue_capacity)
    {
        // Se existe atendente livre, entra e adiciona ao escalonador uma nova saída
        if (current_queue_size <= num_servers)
        {
            add_to_scheduler(SERVICE, min_service, max_service);
        }

        // Utiliza estado da fila atual para ser copiado a este evento
        current_queue_state[current_queue_size] += rng;
        for (uint64_t i = 0; i < MAX_QUEUE_STATE; i++)
        {
            new_event->queue_states[i] = current_queue_state[i];
        }
        
        // Aumenta tamanho da fila na simulação e no evento
        current_queue_size++;   // TODO: antes ou depois?
        new_event->queue_size = current_queue_size;
    }
    else
    {
        // Caso não exista espaço, unidade é perdida
        lost_queue_units++;
    }

    // Agenda nova chegada de outra unidade
    add_to_scheduler(ARRIVAL, min_arrival, max_arrival);
}

// Função de saída de uma unidade na fila
void service(scheduler_entry *scheduled_event, double rng)
{
    // Atualiza o tempo da simulação
    current_time = scheduled_event->time;

    // Cria novo evento, adicionando o tipo de entrada e tempo que está sendo executado
    event_entry *new_event = &event_entries[event_entries_count++];
    new_event->entry_type = scheduled_event->entry_type;
    new_event->time = current_time;

    // Utiliza estado da fila atual para ser copiado a este evento
    current_queue_state[current_queue_size] += rng;
    for (uint64_t i = 0; i < MAX_QUEUE_STATE; i++)
    {
        new_event->queue_states[i] = current_queue_state[i];
    }

    // Diminui tamanho da fila na simulação e no evento
    current_queue_size--;
    new_event->queue_size = current_queue_size;

    // Se saiu da fila, verifica se há alguém na lista de espera para ser atendido e agendar nova saída
    if (current_queue_size >= num_servers)
    {
        add_to_scheduler(SERVICE, min_service, max_service);
    }
}

// Imprime entrada da lista de eventos
void print_event_entry(event_entry *entry, uint64_t index)
{
    printf("(%lu) ", index);
    switch (entry->entry_type)
    {
        case NONE:
            printf("   -   ");
            break;
        case ARRIVAL:
            printf("ARRIVAL");
            break;
        case SERVICE:
            printf("SERVICE");
            break;
    }
    printf(" | %lu | %f |", entry->queue_size, entry->time);

    for (uint64_t i = 0; i < MAX_QUEUE_STATE; i++)
    {
        printf(" %f |", entry->queue_states[i]);
    }
    printf("\n");
}

// Imprime entrada da lista de eventos
void print_scheduled_entry(scheduler_entry *entry, uint64_t index)
{
    printf("(%lu) ", index);
    switch (entry->entry_type)
    {
        case NONE:
            printf("   -   ");
            break;
        case ARRIVAL:
            printf("ARRIVAL");
            break;
        case SERVICE:
            printf("SERVICE");
            break;
    }
    printf(" | %f | %f\n", entry->time, entry->draw);
}

int main(void)
{
    // Cria uma primeira entrada no escalonador e envia na função de entrada na fila para início da simulação
    add_to_scheduler(ARRIVAL, min_arrival, max_arrival);
    arrival(&scheduled_entries[0], first_arrival);

    while (!b_finished)
    {
        // Ordena entradas do escalonador por tempo, 
        qsort(scheduled_entries, sizeof(scheduler_entry), scheduled_entries_count, compare_entries_asc);

        scheduler_entry *entry = &scheduled_entries[0];
        if (entry->entry_type == ARRIVAL)
        {
            arrival(entry, entry->draw);
        }
        else if (entry->entry_type == SERVICE)
        {
            service(entry, entry->draw);
        }

        for (uint64_t i = 1; i < scheduled_entries_count; i++)
        {
            scheduled_entries[i-1] = scheduled_entries[i];
        }
        scheduled_entries_count--;
    }

    printf("Chronological Events:\n");
    print_event_entry(&(event_entry){.entry_type = NONE, .queue_size = 0, .time = 0.0, .queue_states = {0.0}}, 0);
    for (uint64_t i = 0; i < event_entries_count; i++)
    {
        print_event_entry(&event_entries[i], i+1);
    }

    printf("\nScheduled Events:\n");
    for (uint64_t i = 0; i < total_scheduled_entries_count; i++)
    {
        print_scheduled_entry(&total_scheduled_entries[i], i+1);
    }

    printf("\nProbabilities of each queue state:\n");
    print_queue_state_probability_calc();

    return 0;
}