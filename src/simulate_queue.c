#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define COMPARE_ASC(a, b) (((a) > (b)) - ((a) < (b)))       // Macro para funcão auxiliar de comparação entre valores do qsort

#define DEBUG 1                                             // Para ativar modo debug (imprime todos os eventos)
#define QUEUE_CAPACITY_1 3                                  // Capacidade máxima da fila 1
#define QUEUE_CAPACITY_2 5                                  // Capacidade máxima da fila 2
#define MAX_NUM_RNG 100000                                  // Número de números pseudoaleatórios a serem calculados
#define MAX_QUEUE_STATE QUEUE_CAPACITY_2+1                  // Número máximo de estados da fila (Capacidade da fila + 1)

// TODO: mudar filas e eventos para funcionar de forma genérica
// TODO: incluir modificações nas funções para suporte de mais de uma fila
// TODO: completar a função de troca de fila (exchange)

enum EntryType {NONE, ARRIVAL, SERVICE, LOSS};              // Tipos de entrada na lista de eventos e escalonador (Nenhum, Entrada, Saída e Unidade Perdida)

typedef struct {                                            // Struct de entradas da lista de eventos
    enum EntryType entry_type;                              // Tipo da entrada
    uint64_t index;                                         // Número índice da entrada;
    uint64_t queue_size;                                    // Tamanho da fila
    double time;                                            // Tempo que será executado
    double queue_states[MAX_QUEUE_STATE];                   // Estados da fila (tempo total para cada estado da fila)
} event_entry;          

typedef struct {                                            // Struct de entradas da lista do escalonador
    enum EntryType entry_type;                              // Tipo da entrada
    uint64_t index;                                         // Número índice da entrada;
    double time;                                            // Tempo em que o evento ocorre
    double draw;                                            // Sorteio do RNG
    bool b_removed;                                         // Para indicar se já foi utilizado e removido
} scheduler_entry;          

typedef struct {
    uint64_t num_servers;                           // Número de atendentes
    uint64_t capacity;                              // Capacidade da fila
    uint64_t customers;                             // Clientes na fila
    uint64_t loss;                                  // Unidades perdidas da fila
    double first_arrival;                           // Primeira chegada
    double min_arrival;                             // Número mínimo da chegada
    double max_arrival;                             // Número máximo da chegada
    double min_service;                             // Número mínimo da saída
    double max_service;                             // Número máximo da saída
    double times[MAX_QUEUE_STATE];                  // Tempos acumulados para cada estado da fila
    bool b_first_queue;                             // Boolean
} queue;

queue q1 = {
            .b_first_queue = true,
            .num_servers = 2,
            .capacity = QUEUE_CAPACITY_1,
            .customers = 0,
            .loss = 0,
            .first_arrival = 1.5,
            .min_arrival = 1,
            .max_arrival = 4,
            .min_service = 3,
            .max_service = 4
        };

queue q2 = {
            .b_first_queue = false,
            .num_servers = 1,
            .capacity = QUEUE_CAPACITY_2,
            .customers = 0,
            .loss = 0,
            .first_arrival = 0,
            .min_arrival = 0,
            .max_arrival = 0,
            .min_service = 2,
            .max_service = 3
        };

bool b_finished = false;                                    // Boolean para finalizar o loop do main (quando o número máximo de números aleatórios é atingido)

// TODO: Pensar se faz sentido um temporizador global ou por fila
double current_time = 0.0;                                  // Tempo atual da simulação (incrementa a cada evento)
uint64_t rng_count = 0;                                     // Contador de números RNG utilizados
uint64_t current_queue_size = 0;                            // Número atual do tamanho da fila
double current_queue_state[MAX_QUEUE_STATE];                // Estado atual da fila (tempo total para cada estado da fila)
uint64_t previous = 4651815687;                             // Último número RNG computado, inicializado aqui com o seed

// TODO: Mudar para eventos genéricos
event_entry event_entries[MAX_NUM_RNG+1];                   // Entradas da lista de eventos
uint64_t event_entries_count = 0;                           // Contador do número de entradas na lista de eventos
scheduler_entry total_scheduled_entries[MAX_NUM_RNG+1];     // Todas entradas do escalonador, inclusive passadas
uint64_t total_scheduled_entries_count = 0;                 // Contador do número de entradas totais no escalonador, inclusive passadas
scheduler_entry *current_scheduled_entries[MAX_NUM_RNG+1];  // Lista de pointers de entradas ainda não executadas do escalonador
uint64_t current_scheduled_entries_count = 0;               // Contador do número da lista de pointers de entradas ainda não executadas do escalonador

// Função auxiliar do quicksort para ordenar entradas de menor para maior tempo no escalonador
int compare_entries_by_time_asc(const void *a, const void *b)
{
    float entry_a = ((*(scheduler_entry **)a))->time;
    float entry_b = ((*(scheduler_entry **)b))->time;

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

// Adiciona nova entrada no escalonador
void add_to_scheduler(enum EntryType type, double a, double b)
{
    double new_draw = calculate_draw(a,b);
    total_scheduled_entries[total_scheduled_entries_count] = (scheduler_entry){.entry_type = type, .index = (total_scheduled_entries_count + 1), .draw = new_draw, .time = (current_time + new_draw), .b_removed = false};
    current_scheduled_entries[current_scheduled_entries_count++] = &total_scheduled_entries[total_scheduled_entries_count++];
}

// Função de entrada de uma unidade na fila
void arrival(scheduler_entry *scheduled_event)
{
    scheduled_event->b_removed = true;

    // Atualiza o tempo da simulação e calcula tempo adicional comparado ao anterior
    double previous_time = current_time;
    current_time = scheduled_event->time;
    double added_time = current_time - previous_time;

    // Cria novo evento, adicionando o tipo de entrada, índice e tempo que está sendo executado
    event_entry *new_event = &event_entries[event_entries_count++];
    new_event->entry_type = scheduled_event->entry_type;
    new_event->time = current_time;
    new_event->index = scheduled_event->index;

    // Verifica se existe espaço na fila
    if (current_queue_size < queue_capacity)
    {
        // Utiliza estado da fila atual para ser copiado a este evento
        current_queue_state[current_queue_size] += added_time;
        for (uint64_t i = 0; i < MAX_QUEUE_STATE; i++)
        {
            new_event->queue_states[i] = current_queue_state[i];
        }
        
        // Aumenta tamanho da fila na simulação e no evento
        current_queue_size++;
        new_event->queue_size = current_queue_size;

        // Se existe atendente livre, entra e adiciona ao escalonador uma nova saída
        if (current_queue_size <= num_servers)
        {
            add_to_scheduler(SERVICE, min_service, max_service);
        }
    }
    else
    {
        // Caso não exista espaço a unidade é perdida
        // Volta para o tempo anterior, incrementa contador e muda tipo para impressão posterior dos eventos
        current_time = previous_time;
        lost_queue_units++;
        scheduled_event->entry_type = LOSS;
        new_event->entry_type = LOSS;
        new_event->queue_size = current_queue_size;
    }

    // Agenda nova chegada de outra unidade
    add_to_scheduler(ARRIVAL, min_arrival, max_arrival);
}

// Função de saída de uma unidade na fila
void service(scheduler_entry *scheduled_event)
{
    scheduled_event->b_removed = true;
    
    // Atualiza o tempo da simulação e calcula tempo adicional comparado ao anterior
    double previous_time = current_time;
    current_time = scheduled_event->time;
    double added_time = current_time - previous_time;

    // Cria novo evento, adicionando o tipo de entrada, índice e tempo que está sendo executado
    event_entry *new_event = &event_entries[event_entries_count++];
    new_event->entry_type = scheduled_event->entry_type;
    new_event->time = current_time;
    new_event->index = scheduled_event->index;

    // Utiliza estado da fila atual para ser copiado a este evento
    current_queue_state[current_queue_size] += added_time;
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

void exchange_queue(scheduler_entry *scheduled_event, queue *q)
{
    // Atualiza o tempo da simulação e calcula tempo adicional comparado ao anterior
    double previous_time = current_time;
    current_time = scheduled_event->time;
    double added_time = current_time - previous_time;

    // TODO: Continuar
}

// Imprime entrada da lista de eventos
void print_event_entry(event_entry *entry)
{
    bool b_loss = false;

    printf("(%-7lu) ", entry->index);
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
        case LOSS:
            b_loss = true;
            printf(" LOSS  ");
            break;
    }

    if (b_loss)
    {
        printf(" || %10lu || %15f || LOST UNIT!\n", entry->queue_size, entry->time);
        return;
    }

    printf(" || %10lu || %15f ||", entry->queue_size, entry->time);

    for (uint64_t i = 0; i < MAX_QUEUE_STATE; i++)
    {
        printf(" %15f |", entry->queue_states[i]);
    }
    printf("|\n");
}

// Imprime entrada da lista de eventos
void print_scheduled_entry(scheduler_entry *entry)
{
    bool b_loss = false;

    // Strikethrough no texto caso tenha sido removido
    if (entry->b_removed) { printf ("\e[9m"); }

    printf("(%-7lu) ", entry->index);
    
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
        case LOSS:
            b_loss = true;
            printf("ARRIVAL");
            break;
    }
    printf(" || %15f || %10f ||", entry->time, entry->draw);

    // Strikethrough no texto caso tenha sido removido
    printf("\e[m");    

    if (b_loss)
    {
        printf(" LOST UNIT!");
    }
    printf("\n");
}

void print_queue_state_probability_calc()
{
    for (uint64_t i = 0; i < MAX_QUEUE_STATE; i++)
    {
        printf("%5lu: %15f (%10f%%)\n", i, current_queue_state[i], (current_queue_state[i] / current_time * 100));
    }
    printf("    T: %15f (%10f%%)\n", current_time, 100.0);
}

int main(void)
{
    // Cria uma primeira entrada no escalonador e envia na função de entrada na fila para início da simulação
    // Esta primeira entrada no escalonador será logo descartada e sobreescrita, e por isso não incrementa o schedule_entries_count
    total_scheduled_entries[total_scheduled_entries_count] = (scheduler_entry){.entry_type = ARRIVAL, .index = (total_scheduled_entries_count + 1), .draw = first_arrival, .time = first_arrival, .b_removed=false};
    current_scheduled_entries[current_scheduled_entries_count] = &total_scheduled_entries[total_scheduled_entries_count++];
    arrival(current_scheduled_entries[0]);

    while (!b_finished)
    {
        // Ordena entradas do escalonador por tempo
        qsort(current_scheduled_entries, current_scheduled_entries_count, sizeof(scheduler_entry *), compare_entries_by_time_asc);

        scheduler_entry *entry = current_scheduled_entries[0];
        if (entry->entry_type == ARRIVAL)
        {
            arrival(entry);
        }
        else if (entry->entry_type == SERVICE)
        {
            service(entry);
        }
        for (uint64_t i = 1; i < current_scheduled_entries_count; i++)
        {
            current_scheduled_entries[i-1] = current_scheduled_entries[i];
        }
        current_scheduled_entries_count--;
        
    }

    if (DEBUG)
    {
        printf("Chronological Events:\n       TYPE       || QUEUE SIZE ||      TIME       ||");
        for (uint64_t i = 0; i < MAX_QUEUE_STATE; i++)
        {
            printf("   %10lu    |", i);
        }
        printf("|\n");
        print_event_entry(&(event_entry){.entry_type = NONE, .queue_size = 0, .time = 0.0, .queue_states = {0.0}});
        for (uint64_t i = 0; i < event_entries_count; i++)
        {
            print_event_entry(&event_entries[i]);
        }

        printf("\nScheduled Events:\n       TYPE       ||    TIME         ||    DRAW    ||\n");
        for (uint64_t i = 0; i < total_scheduled_entries_count; i++)
        {
            print_scheduled_entry(&total_scheduled_entries[i]);
        }
    }

    printf("\nProbabilities of each queue state:\n");
    print_queue_state_probability_calc();

    printf("\nUnits lost: %lu\n\n", lost_queue_units);

    return 0;
}