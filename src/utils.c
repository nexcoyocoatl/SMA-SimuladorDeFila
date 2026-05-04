#include "utils.h"

#include "globals.h"
#include "types.h"

// Função auxiliar do quicksort para ordenar entradas de menor para maior tempo no escalonador
int compare_entries_by_time_asc(const void *a, const void *b)
{
    double time_entry_a = events[*(const uint64_t *)a].time;
    double time_entry_b = events[*(const uint64_t *)b].time;

    return COMPARE_ASC(time_entry_a, time_entry_b);
}