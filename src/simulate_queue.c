#include "simulate_queue.h"

#include "macro_dynarray.h"
#include "macro_dynbuffer.h"
#include "globals.h"
#include "random_gen.h"

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

// Calcula probabilidade entre possíveis saídas e realiza a lógica de saída
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

// Imprime entradas cronológicas da lista de eventos
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

// Imprime entradas escalonadas da lista de eventos
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

// Imprime estados das filas e porcentagem
void print_queue_state_percentage_calc(void)
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