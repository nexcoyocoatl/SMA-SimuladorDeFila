#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define QUEUE_CAPACITY 5
#define MAX_NUM_RNG 100000
#define MAX_QUEUE_STATE QUEUE_CAPACITY+1

#define COMPARE_ASC(a, b) (((a) > (b)) - ((a) < (b)))

enum EntryType {NONE, ARRIVAL, SERVICE};

typedef struct {
    enum EntryType entry_type;
    uint64_t queue_size;
    double time;
    double queue_states[QUEUE_CAPACITY + 1];
} event_entry;

typedef struct {
    enum EntryType entry_type;
    double time;
    double draw;
} scheduler_entry;

bool b_finished = false;
const uint64_t num_servers = 1;
const uint64_t queue_capacity = QUEUE_CAPACITY;
const double first_arrival = 2.0;
const double min_arrival = 2.0;
const double max_arrival = 5.0;
const double min_service = 3.0;
const double max_service = 5.0;

double current_time = 0.0;
uint64_t rng_count = 0;
uint64_t current_queue_size = 0;
double current_queue_state[MAX_QUEUE_STATE];
uint64_t lost_queue_units = 0;
uint64_t previous = 4651815687; // SEED

scheduler_entry scheduled_entries[QUEUE_CAPACITY+5]; // TODO: MUDAR?
uint64_t scheduled_entries_count = 0;
event_entry event_entries[MAX_NUM_RNG];
uint64_t event_entries_count = 0;

double linear_congruential_generator(uint64_t a, uint64_t c, uint64_t M)
{
    previous = (((a * previous) + c) % M);    
    return ((double)previous)/M;
}

float next_random()
{
    uint64_t a = 54083;
    uint64_t c = 197;
    uint64_t M = UINT32_MAX;

    return linear_congruential_generator(a, c, M);
}

double calculate_draw(uint64_t min, uint64_t max)
{
    rng_count++;
    if (rng_count >= MAX_NUM_RNG)
    {
        b_finished = true;
    }
    return min + ((max - min) * next_random());
}

int compare_entries_asc(const void *a, const void *b)
{
    float entry_a = ((scheduler_entry *)a)->time;
    float entry_b = ((scheduler_entry *)b)->time;

    return COMPARE_ASC(entry_a, entry_b);
}

void add_to_scheduler(enum EntryType type, double a, double b)
{
    // double remaining_time_to_execute = uniform_interval(a, b);
    // current_queue_state[current_queue_size] = remaining_time_to_execute;

    // scheduled_entries[scheduled_entries_count] = (scheduler_entry){.entry_type = type, .queue_size = current_queue_size, .time = current_time + remaining_time_to_execute, .queue_states = {0.0}};
    // for (uint64_t i = 0; i < current_queue_size; i++)
    // {
    //     scheduled_entries[scheduled_entries_count].queue_states[i] = current_queue_state[i];
    // }
    // simulation_steps++;
    // scheduled_entries_count++;

    // switch (type)
    // {
    //     case NONE:
    //         printf("   -   ");
    //         break;
    //     case ARRIVAL:
    //         printf("ARRIVAL");
    //         break;
    //     case SERVICE:
    //         printf("SERVICE");
    //         break;
    // }
    // printf(" | %f | %f\n", current_time, current_queue_state[current_queue_size]);

    return;
}

void remove_from_queue(void)
{    
    for(uint64_t i = 1; i < current_queue_size; i++)
    {
        current_queue_state[i-1] = current_queue_state[i];
    }
}

void arrival(event_entry *event)
{
    current_time = event->time;

    if (current_queue_size < queue_capacity)
    {
        if (current_queue_size <= num_servers)
        {
            add_to_scheduler(SERVICE, min_service, max_service);
        }
        current_queue_size++;
    }
    else
    {
        lost_queue_units++;
    }

    add_to_scheduler(ARRIVAL, min_arrival, max_arrival);
}

void service(event_entry *event)
{
    current_time = event->time;
    remove_from_queue();
    current_queue_size--;

    if (current_queue_size >= num_servers)
    {
        add_to_scheduler(SERVICE, min_service, max_service);
    }
}

// void print_scheduler_entry(scheduler_entry entry)
// {
//     switch (entry.entry_type)
//     {
//         case NONE:
//             printf("   -   ");
//             break;
//         case ARRIVAL:
//             printf("ARRIVAL");
//             break;
//         case SERVICE:
//             printf("SERVICE");
//             break;
//     }
//     printf(" | %lu | %f |", entry.queue_size, entry.time);

//     for (uint64_t i = 0; i < MAX_QUEUE_VALUES_PER_STATE; i++)
//     {
//         printf(" %f |", entry.queue_states[i]);
//     }
//     printf("\n");
// }

int main(void)
{

    // while (count > 0)
    // {
    //     printf("%f ", next_random());
    //     count--;
    // }
    // printf("\n");

    // printf("Scheduled Events:\n");

    // Add first EVENT with the first arrival
    event_entries[event_entries_count++] = ((event_entry){.entry_type = NONE, .queue_size = 0, .time = first_arrival, .queue_states = {0.0}});
    arrival(&event_entries[0]);

    while (!b_finished)
    {
        
    }

    // for (int i = 0; i < 10; i++)
    // // while (!b_finished)
    // {
    //     // PROBLEM HERE
    //     // sort scheduler entries by time
    //     qsort(scheduled_entries, sizeof(scheduler_entry), MAX_STEPS, compare_entries_asc);        

    //     // execute first scheduled event that is <= current time by ascending order
    //     scheduler_entry entry = scheduled_entries[0];
    //     if (entry.entry_type == ARRIVAL)
    //     {
    //         schedule_arrival(entry.time);
    //     }
    //     else if (entry.entry_type == SERVICE)
    //     {
    //         schedule_service(entry.time);
    //     }

    //     for (uint64_t i = 1; i < scheduled_entries_count; i++)
    //     {
    //         scheduled_entries[i-1] = scheduled_entries[i];
    //     }
    //     scheduled_entries_count--;
    // }

    // printf("Chronological Events:\n");
    // print_scheduler_entry((scheduler_entry){.entry_type = NONE, 0, 0.0, {0.0}});
    // for (uint64_t i = 0; i < MAX_STEPS; i++)
    // {
    //     print_scheduler_entry(past_entries[i]);
    // }

    return 0;
}