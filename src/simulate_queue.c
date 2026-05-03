#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "macro_dynarray.h"
#include "macro_dynbuffer.h"

// TODO: Documentação
// TODO: Leitura .yml

#define COMPARE_ASC(a, b) (((a) > (b)) - ((a) < (b)))       // Macro para funcão auxiliar de comparação entre valores do qsort

#define DEBUG 0                                             // Para ativar modo debug (imprime todos os eventos)

enum EntryType { ARRIVAL, SERVICE, EXCHANGE };              // Tipos de entrada na lista de eventos e escalonador (Entrada, Saída, Passagem)

typedef struct {
    uint64_t index;                                         // Número índice da fila
    uint64_t num_servers;                                   // Número de atendentes
    uint64_t capacity;                                      // Capacidade da fila, dinâmica se infinita
    uint64_t customers;                                     // Clientes na fila
    uint64_t loss;                                          // Unidades perdidas da fila
    double first_arrival;                                   // Primeira chegada
    double min_arrival;                                     // Número mínimo da chegada
    double max_arrival;                                     // Número máximo da chegada
    double min_service;                                     // Número mínimo da saída
    double max_service;                                     // Número máximo da saída
    dynbuffer(double) times;                                // Tempos acumulados para cada estado da fila
    dynbuffer(double) exit_odds;                            // Probabilidade para cada saída
    dynbuffer(int) exit_to;                                 // Destinos possíveis
    bool b_infinite_capacity;                               // Indica se a fila é infinita
} queue;

typedef struct {                                            // Struct de entradas da lista de eventos
    enum EntryType entry_type;                              // Tipo da entrada
    queue *queue_from;                                      // Ponteiro para fila origem
    queue *queue_to;                                        // Ponteiro para fila destino
    uint64_t index;                                         // Número índice da entrada
    double time;                                            // Tempo que será executado
    double draw;                                            // Sorteio do RNG
    dynbuffer(uint64_t) queue_sizes;                        // Tamanhos das filas
    dynbuffer(dynbuffer(double)) queue_states;              // Estados das filas (tempo total para cada estado de cada fila)
    bool b_removed;                                         // Para indicar se já foi utilizado e removido
    bool b_loss;                                            // Para indicar se houve uma perda de unidade neste evento
} event_entry;

bool b_finished = false;                                    // Boolean para finalizar o loop do main (quando o número máximo de números aleatórios é atingido)

// Tempo e RNG da simulação
double current_time = 0.0;                                  // Tempo atual da simulação (incrementa a cada evento)
uint64_t rng_count = 0;                                     // Contador de números RNG utilizados
uint64_t previous = 4651815687;                             // Último número RNG computado, inicializado aqui com o seed
uint64_t max_num_rng = 100000;                               // Número de números pseudoaleatórios a serem calculados

// Buffer de filas
uint64_t num_queues = 3;                                    // Número de filas
dynbuffer(queue) queues = NULL;                             // Buffer de filas da simulação

// Listas dinâmicas de eventos
dynarray(event_entry) events = NULL;                        // Lista de eventos em ordem de criação
dynarray(uint64_t) chronological_events_indexes = NULL;     // Lista de índices de eventos em ordem de execução
dynarray(uint64_t) current_scheduled_entries_indexes = NULL;// Lista de índices de eventos não executadas do escalonador

void setup();
int compare_entries_by_time_asc(const void *a, const void *b);
double linear_congruential_generator(uint64_t a, uint64_t c, uint64_t M);
float next_random();
double calculate_draw(uint64_t min, uint64_t max);
void add_to_scheduler(enum EntryType type, queue *q);
void update_event_queues(event_entry *event, double added_time);
void arrival(uint64_t event_index);
void service(uint64_t event_index);
void calculate_service_outcome(uint64_t event_index);
void print_chronological_entry(event_entry *entry);
void print_scheduled_entry(event_entry *entry);
void print_queue_state_probability_calc();

// Função para inicializar valores padrão e outras configurações iniciais
void setup()
{
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

    dynarray_push_last(&events, ((event_entry) {
                        .entry_type = type,
                        .queue_from = q,
                        .queue_to = NULL,
                        .index = (dynarray_size(&events)),
                        .time = (current_time + new_draw),
                        .draw = new_draw,
                        .b_removed = false,
                        .b_loss = false
                    }));
    
    event_entry *new_event = (dynarray_get_last_ptr(&events));

    // Inicializa arrays do novo evento
    dynbuffer_init_n(&(new_event->queue_sizes), num_queues);
    dynbuffer_init_n(&(new_event->queue_states), num_queues);
    for (uint64_t i = 0; i < num_queues; i++)
    {
        // new_event->queue_sizes[i] = 0;

        if (queues[i].b_infinite_capacity)
        {
            dynbuffer_init(&(new_event->queue_states[i]));
        }
        else
        {
            dynbuffer_init_n(&(new_event->queue_states[i]), queues[i].capacity+1);
        }
    }

    dynarray_push_last(&current_scheduled_entries_indexes, new_event->index);
}

// Acrescenta tempo às filas globais e atualiza filas do evento em execução
void update_event_queues(event_entry *event, double added_time)
{
    for (uint64_t i = 0; i < num_queues; i++)
    {
        queues[i].times[queues[i].customers] += added_time;

        // Verifica se o buffer de queue_states é grande o suficiente
        uint64_t current_queue_capacity = queues[i].capacity + 1;
        {
            if (event->queue_states[i] == NULL)
            {
                dynbuffer_init_n(&(event->queue_states[i]), current_queue_capacity);
            }
            else if (dynbuffer_capacity(&(event->queue_states[i])) < current_queue_capacity)
            {
                dynbuffer_resize(&(event->queue_states[i]), current_queue_capacity);
            }
        }

        // Atualiza tamanho das filas no tempo de execução do evento com as filas globais atuais
        event->queue_sizes[i] = queues[i].customers;
        
        // Atualiza estado de todas as filas no tempo de execução do evento com as filas globais atuais
        for (uint64_t j = 0; j < current_queue_capacity; j++)
        {
            event->queue_states[i][j] = queues[i].times[j];
        }
    }
}

// Função de entrada de uma unidade na fila
void arrival(uint64_t event_index)
{
    event_entry *event = &events[event_index];
    queue *q = event->queue_from;

    // Indica que foi removido dos eventos escalonados
    event->b_removed = true;

    // Adiciona à lista de eventos por ordem de execução
    dynarray_push_last(&chronological_events_indexes, event->index);

    // Atualiza o tempo da simulação e calcula tempo adicional comparado ao anterior
    double previous_time = current_time;
    current_time = event->time;
    double added_time = current_time - previous_time;

    // Acrescenta o tempo no estado atual de cada fila e atualiza no tempo global
    update_event_queues(event, added_time);

    // Verifica se existe espaço na fila
    if (q->b_infinite_capacity || q->customers < q->capacity)
    {
        // Aumenta tamanho da fila na simulação e no evento
        q->customers++;

        // Se a fila for infinita e a capacidade indicada for menor que as unidades que contém, aumenta
        if (q->b_infinite_capacity && q->customers > q->capacity)
        {
            q->capacity = q->customers;

            // Se não cabe nos tempos da fila, aumenta
            if (dynbuffer_capacity(&(q->times)) < q->customers+1)
            {
                dynbuffer_resize(&(q->times), q->customers+1);
            }
        }

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

    // Agenda nova chegada de outra unidade
    add_to_scheduler(ARRIVAL, q);
}

// Função de saída de uma unidade na fila
void service(uint64_t event_index)
{
    event_entry *event = &events[event_index];
    queue *q = event->queue_from;

    // Indica que foi removido dos eventos escalonados
    event->b_removed = true;

    // Adiciona à lista de eventos por ordem de execução
    dynarray_push_last(&chronological_events_indexes, event->index);

    // Atualiza o tempo da simulação e calcula tempo adicional comparado ao anterior
    double previous_time = current_time;
    current_time = event->time;
    double added_time = current_time - previous_time;

    // Diminui tamanho da fila origem (caso seja também de destino será incrementada depois)
    // Verifica se há alguém na lista de espera de origem para ser atendido e agendar nova saída
    q->customers--;
    if (q->customers >= q->num_servers)
    {
        add_to_scheduler(SERVICE, q);
    }

    // Acrescenta o tempo no estado atual de cada fila e atualiza no tempo global
    update_event_queues(event, added_time);

    // É ATENDIDO AQUI! Pode ir para outra fila, sair, ou voltar para a mesma fila.
    calculate_service_outcome(event_index);
}

void calculate_service_outcome(uint64_t event_index)
{
    event_entry *event = &events[event_index];
    bool b_exit = true;
    queue *queue_from = event->queue_from;

    // Gera um número aleatório
    double rng = calculate_draw(0,1);
    double sum = 0.0;
    int exit_index = -2;

    // Verifica em qual das possibilidades de saída o número gerado cai
    for (uint64_t i = 0; i < dynbuffer_capacity(&(queue_from->exit_odds)); i++)
    {
        sum += queue_from->exit_odds[i];
        if (rng <= sum)
        {
            exit_index = queue_from->exit_to[i];
            break;
        }
    }

    // Verifica se é uma saída
    if (exit_index != -1)
    {
        // Se não for uma saída, muda tipo para passagem
        event->entry_type = EXCHANGE;
        b_exit = false;
    }
    
    queue *queue_to = &(queues[exit_index]);

    if (!b_exit)
    {
        // Verifica se existe espaço na fila destino
        if (queue_to->b_infinite_capacity || queue_to->customers < queue_to->capacity)
        {
            // Aumenta tamanho da fila destino na simulação e no evento
            queue_to->customers++;

            // Se a fila for infinita e a capacidade indicada for menor que as unidades que contém, aumenta
            if (queue_to->b_infinite_capacity && queue_to->customers > queue_to->capacity)
            {
                queue_to->capacity = queue_to->customers;

                // Se não cabe nos tempos da fila, aumenta
                if (dynbuffer_capacity(&(queue_to->times)) < queue_to->customers+1)
                {
                    dynbuffer_resize(&(queue_to->times), queue_to->customers+1);
                }
            }

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
void print_chronological_entry(event_entry *entry) {
    printf("Event %lu\n", entry->index + 1);
    printf("Type: ");
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
        printf("| Time: %f\n", entry->time);
    
    for (uint64_t i = 0; i < num_queues; i++) {
        printf("  Queue %lu: Size = %lu\n", i, entry->queue_sizes[i]);
        // Only print states that actually have time accumulated to save space
        printf("  States: ");

        uint64_t states_in_this_event = dynbuffer_capacity(&(entry->queue_states[i]));
        for (uint64_t j = 0; j < states_in_this_event; j++) {
            {
                printf("[%lu]: %.2f  ", j, entry->queue_states[i][j]);
            }
        }
        printf("\n");
    }
    printf("--------------------\n\n");
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

        printf(" Capacity:");
        if (queues[i].b_infinite_capacity)
        {
            printf(" Infinite\n");
        }
        else
        {
            printf(" %-5lu\n", queues[i].capacity);
        }

        // TODO: Ver se não quebra
        for (uint64_t j = 0; j < queues[i].capacity+1; j++)
        {
            printf("%5lu: %13f (%8f%%)\n", j, queues[i].times[j], (queues[i].times[j] / current_time * 100));
        }
        printf("TOTAL: %13f (%8f%%)\n", current_time, 100.0);
        uint64_t chronological_events_num = (dynarray_size(&chronological_events_indexes));
        printf("Units: %13lu\n", chronological_events_num);
        if (!queues[i].b_infinite_capacity)
        {
            printf(" Loss: %5lu / %5lu (%8f%%)\n", queues[i].loss, chronological_events_num, (double)queues[i].loss/chronological_events_num*100);
        }
        printf("\n");
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
    print_queue_state_probability_calc();

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
