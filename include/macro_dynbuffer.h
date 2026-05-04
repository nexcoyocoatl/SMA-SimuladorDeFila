
#ifndef _MACRO_DYNBUFFER_H_
#define _MACRO_DYNBUFFER_H_

// based on https://crocidb.com/post/simple-vector-implementation-in-c/
//    and https://www.youtube.com/watch?v=HvG03MY2H04

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

struct dynbuffer_header
{
    uint64_t m_capacity;
    uint64_t padding; // Padding to make it 16-byte aligned
};

// Creates a dynamic buffer (eg: dynbuffer(int) a -> int a*)
#define dynbuffer(T) T*

// Initializes dynamic buffer, allocating 8 spaces from the start
#define dynbuffer_init(DA) \
    do { \
        if (*(DA) != NULL) { \
            dynbuffer_free(DA); \
        } \
        uint64_t initial_capacity = 8; \
        uint64_t size = sizeof(struct dynbuffer_header) + (uint64_t)(initial_capacity*sizeof(**(DA))); \
        struct dynbuffer_header *header = (struct dynbuffer_header*) calloc(1, size); \
        if (header == NULL) { \
            fprintf(stderr, "dynbuffer error: malloc failed\n"); \
        } \
        else \
        { \
            header->m_capacity = initial_capacity; \
            (*(DA)) = (void*)(header + 1); \
        } \
    } while(0)

// Initializes dynamic buffer, allocating n spaces from the start
#define dynbuffer_init_n(DA, n) \
    do { \
        if (*(DA) != NULL) { \
            dynbuffer_free(DA); \
        } \
        uint64_t initial_capacity = n; \
        uint64_t size = sizeof(struct dynbuffer_header) + (uint64_t)(initial_capacity*sizeof(**(DA))); \
        struct dynbuffer_header *header = (struct dynbuffer_header*) calloc(1, size); \
        if (header == NULL) { \
            fprintf(stderr, "dynbuffer error: malloc failed\n"); \
        } \
        else \
        { \
            header->m_capacity = initial_capacity; \
            (*(DA)) = (void*)(header + 1); \
        } \
    } while(0)
    
// To get to the header, casts buffer to header type and subtracts 1
#define dynbuffer_get_header(DA) \
    (((struct dynbuffer_header*)((*(DA)))) - 1)

// Returns capacity from the header
#define dynbuffer_capacity(DA) \
    ((*(DA))? dynbuffer_get_header(DA)->m_capacity : 0)

// Resizes with realloc to the required size
#define dynbuffer_resize(DA, required_size) \
    do { \
        if (*(DA) == NULL) { \
            dynbuffer_init_n(DA, required_size); \
        } \
        else \
        { \
            struct dynbuffer_header *header = dynbuffer_get_header(DA); \
            uint64_t old_capacity = header->m_capacity; \
            uint64_t new_total_bytes = sizeof(struct dynbuffer_header) + ((required_size) * sizeof(**(DA))); \
            void *temp = realloc(header, new_total_bytes); \
            if (temp) { \
                header = (struct dynbuffer_header*)temp; \
                header->m_capacity = (required_size); \
                (*(DA)) = (void*)(header + 1); \
                if ((required_size) > old_capacity) \
                { \
                    uint64_t num_new_elements = (required_size) - old_capacity; \
                    memset(&((*(DA))[old_capacity]), 0, num_new_elements * sizeof(**(DA))); \
                } \
            } \
            else \
            { \
                fprintf(stderr, "dynbuffer error: realloc failed to allocate %lu bytes\n", new_total_bytes); \
                assert(0 && "critical: Memory allocation failed during resize!"); \
            } \
        } \
    } while(0)

#define dynbuffer_ensure_index(DA, i) \
do { \
    uint64_t current_capacity = dynbuffer_capacity(DA); \
    if ((i) >= current_capacity) { \
        uint64_t new_capacity = (current_capacity == 0) ? 8 : (current_capacity * 2); \
        if (new_capacity <= (i)) { \
            new_capacity = (i) + 1; \
        } \
        dynbuffer_resize(DA, new_capacity); \
    } \
} while(0)

// Returns the i element of the list
#define dynbuffer_get(DA, i) \
    (assert((i) < dynbuffer_capacity((DA)) && "dynbuffer: out of bounds!"), (*(DA))[(i)])

// Returns the ADDRESS of the i-th element
#define dynbuffer_get_ptr(DA, i) \
    (assert((i) < dynbuffer_capacity((DA)) && "dynbuffer: out of bounds!"), &((*(DA))[(i)]))

// Returns the first element of the list
#define dynbuffer_get_first(DA) \
    (assert(dynbuffer_capacity((DA)) > 0 && "dynbuffer: empty!"), (*(DA))[0])

// Returns the ADDRESS of the first element
#define dynbuffer_get_first_ptr(DA) \
    (assert(dynbuffer_capacity((DA)) > 0 && "dynbuffer: empty!"), &((*(DA))[(0)]))

// Returns the last element of the list
#define dynbuffer_get_last(DA) \
    (assert(dynbuffer_capacity((DA)) > 0 && "dynbuffer: empty!"), (*(DA))[(dynbuffer_capacity(DA)-1)])

    // Returns the last element of the list
#define dynbuffer_get_last_ptr(DA) \
    (assert(dynbuffer_capacity((DA)) > 0 && "dynbuffer: empty!"), &((*(DA))[(dynbuffer_capacity(DA)-1)]))


// Sets the i element of the list to E
#define dynbuffer_set(DA, i, E) \
    (assert((i) < dynbuffer_capacity((DA)) && "dynbuffer: out of bounds!"), (*(DA))[(i)] = (E))

// Sets the first element of the list to E
#define dynbuffer_set_first(DA, E) \
    (assert(dynbuffer_capacity((DA)) > 0 && "dynbuffer: empty!"), (*(DA))[0] = (E))

// Sets the last element of the list to E
#define dynbuffer_set_last(DA, E) \
    (assert(dynbuffer_capacity((DA)) > 0 && "dynbuffer: empty!"), (*(DA))[(dynbuffer_capacity(DA)-1)] = (E))

// Clears list memory
#define dynbuffer_free(DA) \
do { \
    if (*(DA)) \
    {\
        struct dynbuffer_header *header = dynbuffer_get_header(DA); \
        memset(header, 0, sizeof(struct dynbuffer_header) + (header->m_capacity * sizeof(**(DA)))); \
        free(header);\
        (*(DA)) = NULL;\
    } \
} while(0)

// Clears list memory and its objects (for a list of objects in the heap)
#define dynbuffer_free_all(DA, F) \
    do { \
    if (*(DA)) \
        { \
            uint64_t capacity = dynbuffer_capacity(DA); \
            for (uint64_t i = 0; i < capacity; i++) \
            { \
                F((*(DA))[i]); \
            } \
            dynbuffer_free(DA); \
        } \
    } while(0)

#endif
