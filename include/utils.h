#ifndef __UTILS_H__
#define __UTILS_H__

// Macro para funcão auxiliar de comparação entre valores do qsort
#define COMPARE_ASC(a, b) (((a) > (b)) - ((a) < (b)))

// Função auxiliar do quicksort para ordenar entradas de menor para maior tempo no escalonador
int compare_entries_by_time_asc(const void *a, const void *b);

#endif