#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "macro_fixedynarray.h"

// TODO: Documentação
// TODO: Leitura .yml
// TODO: Evento mantém contexto de passagem de uma fila para outra (origem/destino/exterior)
// TODO: Probabilidade de saída para diversas filas ou exterior

#define COMPARE_ASC(a, b) (((a) > (b)) - ((a) < (b)))       // Macro para funcão auxiliar de comparação entre valores do qsort

#define DEBUG 1                                             // Para ativar modo debug (imprime todos os eventos)

enum EntryType { ARRIVAL, SERVICE, EXCHANGE };              // Tipos de entrada na lista de eventos e escalonador (Entrada, Saída, Passagem)

typedef struct {
    uint64_t index;                                         // Número índice da fila
    uint64_t num_servers;                                   // Número de atendentes
    uint64_t capacity;                                      // Capacidade da fila
    uint64_t customers;                                     // Clientes na fila
    uint64_t exchange_to;                                   // Fila de destino
    uint64_t loss;                                          // Unidades perdidas da fila
    double first_arrival;                                   // Primeira chegada
    double min_arrival;                                     // Número mínimo da chegada
    double max_arrival;                                     // Número máximo da chegada
    double min_service;                                     // Número mínimo da saída
    double max_service;                                     // Número máximo da saída
    fixedynarray(double) times;                                 // Tempos acumulados para cada estado da fila
    bool b_first_queue;                                     // Boolean
} queue;

typedef struct {                                            // Struct de entradas da lista de eventos
    enum EntryType entry_type;                              // Tipo da entrada
    queue *queue_from;                                      // Ponteiro para fila origem
    queue *queue_to;                                        // Ponteiro para fila destino
    uint64_t index;                                         // Número índice da entrada
    double time;                                            // Tempo que será executado
    double draw;                                            // Sorteio do RNG
    fixedynarray(uint64_t) queue_sizes;                         // Tamanhos das filas
    fixedynarray(fixedynarray(double)) queue_states;                // Estados das fila (tempo total para cada estado de cada fila)
    bool b_removed;                                         // Para indicar se já foi utilizado e removido
    bool b_loss;                                            // Para indicar se houve uma perda de unidade neste evento
} event_entry;

bool b_finished = false;                                    // Boolean para finalizar o loop do main (quando o número máximo de números aleatórios é atingido)

uint64_t num_queues = 2;                                        // Número de filas
// fixedynarray(uint64_t) queue_capacities; // TODO: Necessário?                                // Capacidade máxima da fila 1
uint64_t max_num_rng = 10;                                 // Número de números pseudoaleatórios a serem calculados
// uint64_t max_queue_state = 0; // LARGEST_QUEUE_CAPACITY+1;                  // Número máximo de estados da fila (Capacidade da fila maior + 1)

// Tempo e RNG da simulação
double current_time = 0.0;                                  // Tempo atual da simulação (incrementa a cada evento)
uint64_t rng_count = 0;                                     // Contador de números RNG utilizados
uint64_t previous = 4651815687;                             // Último número RNG computado, inicializado aqui com o seed

// Listas dinâmicas de filas e eventos
fixedynarray(queue) queues = NULL;                              // Array de filas da simulação
fixedynarray(event_entry) events = NULL;                        // Lista de eventos em ordem de criação
fixedynarray(uint64_t) chronological_events_indexes = NULL;     // Lista de índices de eventos em ordem de execução
fixedynarray(uint64_t) current_scheduled_entries_indexes = NULL;// Lista de índices de eventos não executadas do escalonador

void setup();
int compare_entries_by_time_asc(const void *a, const void *b);
double linear_congruential_generator(uint64_t a, uint64_t c, uint64_t M);
float next_random();
double calculate_draw(uint64_t min, uint64_t max);
// void add_to_scheduler(enum EntryType type, double a, double b);
void add_to_scheduler(enum EntryType type, queue *q);
void arrival(uint64_t event_index);
void service(uint64_t event_index);
void calculate_service_outcome(uint64_t event_index);
// void exchange_queue(event_entry *event);
void print_chronological_entry(event_entry *entry);
void print_scheduled_entry(event_entry *entry);
void print_queue_state_probability_calc();

// Função para inicializar valores padrão e outras configurações iniciais
void setup()
{
    // TODO: leitura do .yml para popular as variáveis de número de eventos, filas e etc
    // num_queues = ?;
    // max_num_rng = ?;

    fixedynarray_init(&queues, num_queues);
    fixedynarray_init(&events, max_num_rng+1);
    fixedynarray_init(&chronological_events_indexes, max_num_rng+1);
    fixedynarray_init(&current_scheduled_entries_indexes, max_num_rng+1);

    // TODO: código do escopo abaixo será removido e receberá do .yml
    {
    fixedynarray_push_last(&queues, ((queue){
            .index = 0,
            .b_first_queue = true,
            .num_servers = 2,
            .capacity = 3,
            .customers = 0,
            .exchange_to = 1,
            .loss = 0,
            .first_arrival = 1.5,
            .min_arrival = 1.0,
            .max_arrival = 4.0,
            .min_service = 3.0,
            .max_service = 4.0
        }));

     fixedynarray_push_last(&queues, ((queue){
            .index = 1,
            .b_first_queue = false,
            .num_servers = 1,
            .capacity = 5,
            .customers = 0,
            .exchange_to = -1,
            .loss = 0,
            .first_arrival = 0.0,
            .min_arrival = 0.0,
            .max_arrival = 0.0,
            .min_service = 2.0,
            .max_service = 3.0
        }));
    }

    // Popula filas
    for (uint64_t i = 0; i < fixedynarray_size(&queues); i++)
    {
        // uint64_t queue_capacity = ?;  // TODO: receberá do .yml
        // queues[i].capacity = queue_capacity; 

        uint64_t queue_capacity = queues[i].capacity; // TODO: remover quando o código de leitura do .yml estiver funcionando

        fixedynarray_init(&(queues[i].times), queue_capacity+1);

        for (uint64_t j = 0; j < queue_capacity+1; j++)
        {
            queues[i].times[j] = 0;
        }
    }
}

// Função auxiliar do quicksort para ordenar entradas de menor para maior tempo no escalonador
int compare_entries_by_time_asc(const void *a, const void *b)
{
    double time_entry_a = events[*(const uint64_t *)a].time;
    double time_entry_b = events[*(const uint64_t *)b].time;

    return COMPARE_ASC(time_entry_a, time_entry_b);
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
    if (rng_count >= max_num_rng)
    {
        b_finished = true;
    }
    return min + ((max - min) * next_random());
}

// Adiciona nova entrada no escalonador
void add_to_scheduler(enum EntryType type, queue *q)
{
    if (b_finished) { return; }

    double new_draw = 0.0;

    if (type == ARRIVAL)
    {
        new_draw = calculate_draw( q->min_arrival, q->max_arrival );
    }
    else
    {
        new_draw = calculate_draw( q->min_service, q->max_service );
    }

    fixedynarray_push_last(&events, ((event_entry) {
                        .entry_type = type,
                        .queue_from = q,
                        .queue_to = NULL,
                        .index = (fixedynarray_size(&events)),
                        .time = (current_time + new_draw),
                        .draw = new_draw,
                        .b_removed = false,
                        .b_loss = false
                    }));
    
    event_entry *new_event = &fixedynarray_last(&events);

    // Inicializa arrays do novo evento
    // TODO: Inicializa em 0 mesmo?!
    fixedynarray_init(&(new_event->queue_sizes), num_queues);
    fixedynarray_init(&(new_event->queue_states), num_queues);
    for (uint64_t i = 0; i < num_queues; i++)
    {
        new_event->queue_sizes[i] = 0;

        fixedynarray_init(&(new_event->queue_states[i]), queues[i].capacity+1);

        for (uint64_t j = 0; j < queues[i].capacity+1; j++)
        {
            new_event->queue_states[i][j] = 0;
        }
    }

    fixedynarray_push_last(&current_scheduled_entries_indexes, new_event->index);
}

// Função de entrada de uma unidade na fila
void arrival(uint64_t event_index)
{
    event_entry *event = &events[event_index];
    queue *q = event->queue_from;
    uint64_t queue_index = q->index;

    // Indica que foi removido dos eventos escalonados
    event->b_removed = true;

    // Adiciona à lista de eventos por ordem de execução
    fixedynarray_push_last(&chronological_events_indexes, event->index);

    // Atualiza o tempo da simulação e calcula tempo adicional comparado ao anterior
    double previous_time = current_time;
    current_time = event->time;
    double added_time = current_time - previous_time;

    // Acrescenta o tempo no estado atual de cada fila e atualiza no tempo global
    for (uint64_t i = 0; i < num_queues; i++)
    {
        queues[i].times[queues[i].customers] += added_time;
        for (uint64_t j = 0; j < queues[i].capacity+1; j++)
        {
            event->queue_states[i][j] = queues[i].times[queues[i].customers];
        }
    }

    // Verifica se existe espaço na fila
    if (q->customers < q->capacity)
    {

        // Aumenta tamanho da fila na simulação e no evento
        q->customers++;

        // Se existe atendente livre, entra e adiciona ao escalonador uma troca de fila
        if (q->customers <= q->num_servers)
        {
            add_to_scheduler(SERVICE, q);
        }
    }
    else
    {
        // Caso não exista espaço a unidade é perdida
        // Incrementa contador de loss e ativa boolean indicando loss
        q->loss++;
        event->b_loss = true;
    }

    event->queue_sizes[queue_index] = q->customers;

    // Agenda nova chegada de outra unidade
    add_to_scheduler(ARRIVAL, q);
}

// Função de saída de uma unidade na fila
void service(uint64_t event_index)
{
    event_entry *event = &events[event_index];
    queue *q = event->queue_from;
    uint64_t queue_index = q->index;

    // Indica que foi removido dos eventos escalonados
    event->b_removed = true;

    // Adiciona à lista de eventos por ordem de execução
    fixedynarray_push_last(&chronological_events_indexes, event->index);

    // Atualiza o tempo da simulação e calcula tempo adicional comparado ao anterior
    double previous_time = current_time;
    current_time = event->time;
    double added_time = current_time - previous_time;

    // Acrescenta o tempo no estado atual de cada fila e atualiza no tempo global
    for (uint64_t i = 0; i < num_queues; i++)
    {
        queues[i].times[queues[i].customers] += added_time;
        for (uint64_t j = 0; j < queues[i].capacity+1; j++)
        {
            event->queue_states[i][j] = queues[i].times[queues[i].customers];
        }
    }

    // É ATENDIDO AQUI! Pode ir para outra fila, sair, ou voltar para a mesma fila.
    // Cálculo nesta função
    calculate_service_outcome(event_index);

    // Atualiza número de unidades no evento
    event->queue_sizes[queue_index] = q->customers;
}

void calculate_service_outcome(uint64_t event_index)
{
    event_entry *event = &events[event_index];
    bool b_exit = true;
    queue *queue_from = event->queue_from;

    // Verifica se é uma saída
    if (queue_from->exchange_to != -1)
    {
        // Se não for uma saída, muda tipo para passagem
        event->entry_type = EXCHANGE;
        b_exit = false;
    }
    
    queue *queue_to = &(queues[queue_from->exchange_to]);
    
    // Diminui tamanho da fila origem (caso seja também de destino será incrementada depois)
    // Verifica se há alguém na lista de espera de origem para ser atendido e agendar nova saída
    queue_from->customers--;
    if (queue_from->customers >= queue_from->num_servers)
    {
        add_to_scheduler(SERVICE, queue_from);
    }

    if (!b_exit)
    {
        // Verifica se existe espaço na fila destino
        if (queue_to->customers < queue_to->capacity)
        {
            // Aumenta tamanho da fila destino na simulação e no evento
            queue_to->customers++;

            // Se existe atendente livre, entra e adiciona ao escalonador uma nova saída
            if (queue_to->customers <= queue_to->num_servers)
            {
                add_to_scheduler(SERVICE, queue_to);
            }
        }
        else
        {
            // Caso não exista espaço a unidade é perdida
            // Incrementa contador de loss e ativa boolean indicando loss
            queue_to->loss++;
        }
    }
}

// Imprime entrada da lista de eventos
void print_chronological_entry(event_entry *entry)
{
    printf("(%5lu) ", entry->index + 1);

    if (entry->b_loss)
    {
        if (entry->entry_type == ARRIVAL)
        {
            printf("AR.");
        }
        else if (entry->entry_type == EXCHANGE)
        {
            printf("EX.");
        }
        printf(" LOSS");
    }
    else
    {
        switch (entry->entry_type)
        {
            case ARRIVAL:
                printf("ARRIVAL ");
                break;
            case SERVICE:
                printf("SERVICE ");
                break;
            case EXCHANGE:
                printf("EXCHANGE");
                break;
        }
    }

    printf(" || %13f ||", entry->time);

    for (uint64_t i = 0; i < num_queues; i++)
    {
        printf(" %8lu |", entry->queue_sizes[i]);
        for (uint64_t j = 0; j < queues[i].capacity+1; j++)
        {
            printf(" %13f |", entry->queue_states[i][j]);
        }
        printf("|");
    }

    if (entry->b_loss)
    {
        printf(" LOST UNIT!");
    }

    printf("\n");
}

// Imprime entrada da lista de eventos
void print_scheduled_entry(event_entry *entry)
{
    // Strikethrough no texto caso tenha sido removido
    if (entry->b_removed) { printf ("\x1b[9m"); }

    printf("(%5lu) ", entry->index + 1);

    if (entry->b_loss)
    {
        if (entry->entry_type == ARRIVAL)
        {
            printf("AR.");
        }
        else if (entry->entry_type == EXCHANGE)
        {
            printf("EX.");
        }
        printf(" LOSS");
    }
    else
    {
        switch (entry->entry_type)
        {
            case ARRIVAL:
                printf("ARRIVAL ");
                break;
            case SERVICE:
                printf("SERVICE ");
                break;
            case EXCHANGE:
                printf("EXCHANGE");
                break;
        }
    }

    printf(" || %13f || %8f ||", entry->time, entry->draw);

    // Strikethrough no texto caso tenha sido removido
    printf("\x1b[m");

    if (entry->b_loss)
    {
        printf(" LOST UNIT!");
    }
    printf("\n");
}

void print_queue_state_probability_calc()
{
    for (uint64_t i = 0; i < num_queues; i++)
    {
        printf("Queue %lu:\n", i+1);
        for (uint64_t j = 0; j < queues[i].capacity+1; j++)
        {
            printf("%5lu: %13f (%8f%%)\n", j, queues[i].times[j], (queues[i].times[j] / current_time * 100));
        }
        printf("TOTAL: %13f (%8f%%)\n", current_time, 100.0);
        printf(" Loss: %6lu / %6lu (%8f%%)\n\n", queues[i].loss, (fixedynarray_size(&chronological_events_indexes)+1), (double)queues[i].loss/(fixedynarray_size(chronological_events_indexes)+1)*100);
    }
}

int main(void)
{
    setup();

    // Cria uma primeira entrada no escalonador e envia na função de entrada na fila 1 para início da simulação    
    fixedynarray_push_last(&events, ((event_entry) {
                        .entry_type = ARRIVAL,
                        .queue_from = &queues[0],
                        .queue_to = NULL,
                        .index = (fixedynarray_size(&events)),
                        .time = queues[0].first_arrival,
                        .draw = queues[0].first_arrival,
                        .b_removed = false,
                        .b_loss = false
                    }));

    event_entry *new_event = &(events[0]);

    // Inicializa arrays do novo evento em 0
    fixedynarray_init(&(new_event->queue_sizes), num_queues);
    fixedynarray_init(&(new_event->queue_states), num_queues);
    for (uint64_t i = 0; i < num_queues; i++)
    {
        new_event->queue_sizes[i] = 0;

        fixedynarray_init(&(new_event->queue_states[i]), queues[i].capacity+1);

        for (uint64_t j = 0; j < queues[i].capacity+1; j++)
        {
            new_event->queue_states[i][j] = 0;
        }
    }

    // Adiciona para a lista de eventos escalonados
    fixedynarray_push_last(&current_scheduled_entries_indexes, new_event->index);

    arrival(current_scheduled_entries_indexes[0]);

    while (!b_finished)
    {
        // Ordena entradas do escalonador por tempo
        qsort(current_scheduled_entries_indexes, fixedynarray_size(&current_scheduled_entries_indexes), sizeof(uint64_t), compare_entries_by_time_asc);

        uint64_t event_index;
        fixedynarray_pop_first(&current_scheduled_entries_indexes, event_index);
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
            for (uint64_t j = 0; j < queues[i].capacity+1; j++)
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
            for (uint64_t j = 0; j < queues[i].capacity+1; j++)
            {
                printf(" %13f |", 0.0);
            }
            printf("|");
        }
        printf("\n");

        // Imprime os próximos
        for (uint64_t i = 0; i < fixedynarray_size(&chronological_events_indexes); i++)
        {
            print_chronological_entry(&events[chronological_events_indexes[i]]);
        }

        printf("\nScheduled Events:\n       TYPE      ||     TIME      ||   DRAW   ||\n");
        for (uint64_t i = 0; i < fixedynarray_size(&events); i++)
        {
            print_scheduled_entry(&events[i]);
        }
    }

    printf("\nProbabilities of each queue state:\n");
    print_queue_state_probability_calc();

    // Libera memória das listas dinâmicas
    if (queues != NULL)
    {
        for(uint64_t i = 0; i < fixedynarray_capacity(&queues); i++)
        {
            fixedynarray_free(&(queues[i].times));
        }
    }

    if (events != NULL)
    {
        for (uint64_t i = 0; i < fixedynarray_capacity(&events); i++)
        {    
            fixedynarray_free(&(events[i].queue_sizes));

            if (events[i].queue_states != NULL)
            {
                for (uint64_t j = 0; j < fixedynarray_capacity(events[i].queue_states); j++)
                {
                    fixedynarray_free(&(events[i].queue_states[j]));
                }
                fixedynarray_free(&(events[i].queue_states));
            }
        }
    }

    fixedynarray_free(&queues);
    fixedynarray_free(&events);
    fixedynarray_free(&chronological_events_indexes);
    fixedynarray_free(&current_scheduled_entries_indexes);

    return 0;
}
