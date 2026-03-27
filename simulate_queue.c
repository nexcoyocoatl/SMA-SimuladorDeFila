#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define QUEUE_CAPACITY 4
#define MAX_QUEUE_VALUES_PER_STATE QUEUE_CAPACITY+1
#define MAX_STEPS 6

#define COMPARE_ASC(a, b) (((a) > (b)) - ((a) < (b)))

enum EntryType {NONE, ARRIVAL, SERVICE};

typedef struct {
    enum EntryType entry_type;
    uint64_t queue_size;
    double time;
    double queue_states[QUEUE_CAPACITY + 1];
} scheduler_entry;

bool b_finished = false;
uint64_t num_servers = 2;
uint64_t current_queue_size = 0;
uint64_t queue_capacity = QUEUE_CAPACITY;
uint64_t lost_units = 0;
uint64_t simulation_steps = 0;
double current_queue_state[MAX_QUEUE_VALUES_PER_STATE];
double current_time = 0.0;
double first_arrival = 2.0;
double min_arrival = 2.0;
double max_arrival = 3.0;
double min_service = 2.0;
double max_service = 4.0;
double rng_list[MAX_STEPS] = {0.9, 0.8, 0.1, 0.8, 0.3, 0.5};
scheduler_entry scheduled_entries[MAX_STEPS];
uint64_t scheduled_entries_count = 0;
scheduler_entry past_entries[MAX_STEPS];
uint64_t past_entries_count = 0;

int compare_entries_asc(const void *a, const void *b)
{
    float entry_a = ((scheduler_entry *)a)->time;
    float entry_b = ((scheduler_entry *)b)->time;

    return COMPARE_ASC(entry_a, entry_b);
}

double uniform_interval(uint64_t a, uint64_t b)
{
    return ((b-a) * rng_list[simulation_steps++]) + a;
}

void add_to_scheduler(enum EntryType type, double a, double b)
{
    if (simulation_steps >= MAX_STEPS)
    {
        b_finished = true;
        return;
    }

    double remaining_time_to_execute = uniform_interval(a, b);
    current_queue_state[current_queue_size] = remaining_time_to_execute;

    scheduled_entries[scheduled_entries_count] = (scheduler_entry){.entry_type = type, .queue_size = current_queue_size, .time = current_time + remaining_time_to_execute, .queue_states = {0.0}};
    for (uint64_t i = 0; i < current_queue_size; i++)
    {
        scheduled_entries[scheduled_entries_count].queue_states[i] = current_queue_state[i];
    }
    simulation_steps++;
    scheduled_entries_count++;

    switch (type)
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
    printf(" | %f | %f\n", current_time, current_queue_state[current_queue_size]);

    return;
}

void remove_from_queue(void)
{
    for(uint64_t i = 1; i < current_queue_size; i++)
    {
        current_queue_state[i-1] = current_queue_state[i];
    }
    current_queue_size--;
}

void schedule_arrival(double time)
{
    current_time = time;

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
        lost_units++;
    }

    add_to_scheduler(ARRIVAL, min_arrival, max_arrival);
}

void schedule_service(uint64_t time)
{
    if (simulation_steps >= MAX_STEPS)
    {
        return;
    }

    current_time = time;
    remove_from_queue();

    if (current_queue_size >= num_servers) // ?
    {
        add_to_scheduler(ARRIVAL, min_arrival, max_arrival);
    }
}

void print_scheduler_entry(scheduler_entry entry)
{
    switch (entry.entry_type)
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
    printf(" | %lu | %f |", entry.queue_size, entry.time);

    for (uint64_t i = 0; i < MAX_QUEUE_VALUES_PER_STATE; i++)
    {
        printf(" %f |", entry.queue_states[i]);
    }
    printf("\n");
}

int main(void)
{
    printf("Scheduled Events:\n");

    // Add first EVENT with the first arrival
    schedule_arrival(first_arrival);

    for (int i = 0; i < 10; i++)
    // while (!b_finished)
    {
        // PROBLEM HERE
        // sort scheduler entries by time
        qsort(scheduled_entries, sizeof(scheduler_entry), MAX_STEPS, compare_entries_asc);
        

        // execute first scheduled event that is <= current time by ascending order
        scheduler_entry entry = scheduled_entries[0];
        if (entry.entry_type == ARRIVAL)
        {
            schedule_arrival(entry.time);
        }
        else if (entry.entry_type == SERVICE)
        {
            schedule_service(entry.time);
        }

        for (uint64_t i = 1; i < scheduled_entries_count; i++)
        {
            scheduled_entries[i-1] = scheduled_entries[i];
        }
        scheduled_entries_count--;
    }

    printf("Chronological Events:\n");
    print_scheduler_entry((scheduler_entry){.entry_type = NONE, 0, 0.0, {0.0}});
    for (uint64_t i = 0; i < MAX_STEPS; i++)
    {
        print_scheduler_entry(past_entries[i]);
    }

    return 0;
}