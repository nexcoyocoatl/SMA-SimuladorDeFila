#include "read_file.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "string_t.h"
#include "types.h"
#include "globals.h"
#include "macro_dynarray.h"

dynarray(queue_parameters) queues_param = NULL;
queue *from_queue = NULL;
queue *to_queue   = NULL;

void parse_config(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) { fprintf(stderr, "Error: could not open %s\n", filename); return; }

    dynarray_init(&queues_param);

    STRING_CREATE(line, NULL);

    enum CurrentSection current_section = 0;
    int64_t current_queue_idx = -1;

    while (string_get_line(&line, f) == 0)
    {
        string_trim(&line);

        // Ignora linhas vazias e comentários
        if (line.size == 0 || line.data[0] == '#' || line.data[0] == '!')
        {
            continue;
        }

        // Remove os caracteres desnecessários da seção de network
        if (line.data[0] == '-')
        {
            uint64_t i = 1;
            while (i < line.size && (line.data[i] == ' ' || line.data[i] == '\t')) i++;
            memmove(line.data, line.data + i, line.size - i + 1);
            line.size -= i;
        }

        // Procura por comentários para remover
        int64_t comment_pos = string_find(&line, "#");
        if (comment_pos >= 0)
        {
            line.data[comment_pos] = '\0';
            line.size = (uint64_t)comment_pos;
            string_trim(&line);
        }

        // Se a linha é vazia, continua
        if (line.size == 0) { continue; }

        // Detecta seções
        if      (string_find(&line, "arrivals:")      == 0) { current_section = SECTION_ARRIVALS;      continue; }
        else if (string_find(&line, "queues:")        == 0) { current_section = SECTION_QUEUES;        continue; }
        else if (string_find(&line, "network:")       == 0) { current_section = SECTION_NETWORK;       continue; }
        else if (string_find(&line, "maxrndnumbers:") == 0) { current_section = SECTION_MAXRNDNUMBERS; }

        // Detecta valores só de nome
        int64_t colon_pos = string_find(&line, ":");
        if (colon_pos >= 0 && (uint64_t)colon_pos == line.size - 1)
        {
            if (current_section == SECTION_QUEUES)
            {
                queue new_q = {
                    .index               = (int64_t)dynarray_size(&queues),
                    .name                = NULL,
                    .b_infinite_capacity = true,
                    .b_arrival           = false,
                    .num_servers         = 1,
                    .capacity            = 0,
                    .customers           = 0,
                    .loss                = 0,
                    .first_arrival       = 0.0,
                    .min_arrival         = 0.0,
                    .max_arrival         = 0.0,
                    .min_service         = 0.0,
                    .max_service         = 0.0,
                    .times               = NULL,
                    .exit_odds           = NULL,
                    .exit_to             = NULL
                };

                uint64_t len = (uint64_t)colon_pos + 1;
                new_q.name = malloc(len);
                strncpy(new_q.name, line.data, (uint64_t)colon_pos);
                new_q.name[colon_pos] = '\0';

                dynarray_push_last(&queues, new_q);
                current_queue_idx = (int64_t)dynarray_size(&queues) - 1;
                dynarray_init(&(queues[current_queue_idx].exit_odds));
                dynarray_init(&(queues[current_queue_idx].exit_to));
            }
            continue;
        }

        // Corta valores em duas partes
        uint64_t parts_count;
        string_t *parts = string_split(&line, ":", &parts_count);
        if (!parts || parts_count < 2) { STRING_SPLIT_FREE(parts, parts_count); continue; }

        string_trim(&parts[0]);
        string_trim(&parts[1]);

        switch (current_section)
        {
            case SECTION_ARRIVALS:
            {
                dynarray_push_last(&queues_param, ((queue_parameters)
                {
                    .first_arrival = atof(parts[1].data),
                    .name = NULL
                }));
                queue_parameters *q_p = dynarray_get_last_ptr(&queues_param);
                uint64_t len = parts[0].size + 1;
                q_p->name = malloc(len);
                strncpy(q_p->name, parts[0].data, len);
                break;
            }

            case SECTION_QUEUES:
            {
                if (current_queue_idx < 0) break;
                queue *q = &queues[current_queue_idx];

                if      (strcmp(parts[0].data, "servers")    == 0) q->num_servers = (uint64_t)atoi(parts[1].data);
                else if (strcmp(parts[0].data, "capacity")   == 0) { q->capacity = (uint64_t)atoi(parts[1].data); q->b_infinite_capacity = false; }
                else if (strcmp(parts[0].data, "minArrival") == 0) q->min_arrival = atof(parts[1].data);
                else if (strcmp(parts[0].data, "maxArrival") == 0) q->max_arrival = atof(parts[1].data);
                else if (strcmp(parts[0].data, "minService") == 0) q->min_service = atof(parts[1].data);
                else if (strcmp(parts[0].data, "maxService") == 0) q->max_service = atof(parts[1].data);
                break;
            }

            case SECTION_NETWORK:
            {
                if (strcmp(parts[0].data, "source") == 0)
                {
                    from_queue = NULL;
                    for (uint64_t i = 0; i < dynarray_size(&queues); i++)
                        if (strcmp(parts[1].data, queues[i].name) == 0) { from_queue = &queues[i]; break; }
                }
                else if (strcmp(parts[0].data, "target") == 0)
                {
                    to_queue = NULL;
                    for (uint64_t i = 0; i < dynarray_size(&queues); i++)
                        if (strcmp(parts[1].data, queues[i].name) == 0) { to_queue = &queues[i]; break; }
                }
                else if (strcmp(parts[0].data, "probability") == 0)
                {
                    if (from_queue != NULL && to_queue != NULL)
                    {
                        dynarray_push_last(&(from_queue->exit_to),   (int)to_queue->index);
                        dynarray_push_last(&(from_queue->exit_odds), atof(parts[1].data));
                    }
                    else fprintf(stderr, "Error: probability without valid source/target.\n");
                }
                break;
            }
            case SECTION_MAXRNDNUMBERS:
            {
                max_num_rng = (uint64_t)atoll(parts[1].data);
                if (max_num_rng == 0) fprintf(stderr, "Error: invalid maxrndnumbers.\n");
                break;
            }
        }

        STRING_SPLIT_FREE(parts, parts_count);
    }

    // Detecta e junta valores de first arrival com suas respectivas queues
    for (uint64_t i = 0; i < dynarray_size(&queues_param); i++)
    {
        for (uint64_t j = 0; j < dynarray_size(&queues); j++)
        {
            if (strcmp(queues_param[i].name, queues[j].name) == 0)
            {
                queues[j].b_arrival     = true;
                queues[j].first_arrival = queues_param[i].first_arrival;
            }
        }
    }

    string_free(&line);
    fclose(f);

    for (uint64_t i = 0; i < dynarray_size(&queues_param); i++)
    {
        free(queues_param[i].name);
    }
    dynarray_free(&queues_param);
}