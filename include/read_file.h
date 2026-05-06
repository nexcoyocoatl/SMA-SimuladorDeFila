#ifndef __READ_FILE_H__
#define __READ_FILE_H__

#include "types.h"
#include "globals.h"
#include "macro_dynarray.h"
#include <stdint.h>

enum CurrentSection { SECTION_ARRIVALS = 1, SECTION_QUEUES, SECTION_NETWORK, SECTION_MAXRNDNUMBERS };

extern dynarray(queue_parameters) queues_param;

void parse_config(const char *filename);

#endif