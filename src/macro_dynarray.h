
#ifndef _MACRO_DYNARRAY_H_
#define _MACRO_DYNARRAY_H_

// based on https://crocidb.com/post/simple-vector-implementation-in-c/
//    and https://www.youtube.com/watch?v=HvG03MY2H04

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

struct dynarray_header
{
    uint64_t m_capacity;
    uint64_t m_size;
};

// Creates a dynamic array (eg: dynarray(int) a -> int a*)
#define dynarray(T) T*

// Initializes dynamic array, allocating 8 spaces from the start
#define dynarray_init(DA) \
    do { \
        if (*(DA) != NULL) { \
            dynarray_free(DA); \
        } \
        uint64_t initial_capacity = 8; \
        uint64_t size = sizeof(struct dynarray_header) + (uint64_t)(initial_capacity*sizeof(**(DA))); \
        struct dynarray_header *header = (struct dynarray_header*) calloc(1, size); \
        if (header == NULL) { \
            fprintf(stderr, "dynarray error: malloc failed\n"); \
        } \
        else \
        { \
            header->m_capacity = initial_capacity; \
            header->m_size = 0; \
            (*(DA)) = (void*)(header + 1); \
        } \
    } while(0)

// Initialize with an exact size

// Initializes dynamic array, allocating n spaces from the start
#define dynarray_init_n(DA, n) \
    do { \
        if (*(DA) != NULL) { \
            dynarray_free(DA); \
        } \
        uint64_t initial_capacity = n; \
        uint64_t size = sizeof(struct dynarray_header) + (uint64_t)(initial_capacity*sizeof(**(DA))); \
        struct dynarray_header *header = (struct dynarray_header*) calloc(1, size); \
        if (header == NULL) { \
            fprintf(stderr, "dynarray error: malloc failed\n"); \
        } \
        else \
        { \
            header->m_capacity = initial_capacity; \
            header->m_size = 0; \
            (*(DA)) = (void*)(header + 1); \
        } \
    } while(0)
    
// To get to the header, casts array to header type and subtracts 1
#define dynarray_get_header(DA) \
    (((struct dynarray_header*)(*(DA))) - 1)

// Returns size by accessing the header
#define dynarray_size(DA) \
    ((*(DA))? dynarray_get_header(DA)->m_size : 0)

// Returns capacity from the header
#define dynarray_capacity(DA) \
    ((*(DA))? dynarray_get_header(DA)->m_capacity : 0)

// Resizes with realloc to the required size
#define dynarray_resize(DA, required_size) \
    do { \
        struct dynarray_header *header = dynarray_get_header(DA); \
        uint64_t old_capacity = header->m_capacity; \
        uint64_t new_total_bytes = sizeof(struct dynarray_header) + ((required_size) * sizeof(**(DA))); \
        void *temp = realloc(header, new_total_bytes); \
        if (temp) { \
            header = (struct dynarray_header*)temp; \
            header->m_capacity = (required_size); \
            (*(DA)) = (void*)(header + 1); \
            if ((required_size) > old_capacity) \
            { \
                uint64_t num_new_elements = (required_size) - old_capacity; \
                memset(&((*(DA))[old_capacity]), 0, num_new_elements * sizeof(**(DA))); \
            } \
        } \
        else \
        {   \
            fprintf(stderr, "dynarray error: realloc failed to allocate %zu bytes\n", new_total_bytes); \
        }   \
    } while(0)

// Multiplies capacity by 2 if needed
#define dynarray_check_enlarge(DA) \
do { \
    if  ( dynarray_size(DA) >= dynarray_capacity(DA) ) \
    { \
        uint64_t new_capacity = (dynarray_capacity(DA) == 0) ? 8 : dynarray_capacity(DA) * 2; \
        dynarray_resize(DA, new_capacity); \
    } \
} while(0)

// Divides capacity by 2 if needed
#define dynarray_check_reduce(DA) \
do { \
    if (dynarray_size(DA) > 0) \
    { \
        uint64_t capacity = dynarray_capacity(DA); \
        uint64_t size = dynarray_size(DA); \
        if ( capacity > 8 && size < (capacity / 2) ) \
        { \
            uint64_t new_capacity = capacity / 2; \
            dynarray_resize(DA, new_capacity); \
        } \
    } \
} while(0)

// Shifts left from end to index
#define dynarray_shift_left_to(DA, index) \
    do { \
        for (uint64_t i = index; i < dynarray_size(DA) - 1; i++) \
        { \
            (*(DA))[i] = (*(DA))[i+1]; \
        } \
    } while(0)

// Shifts right from index
#define dynarray_shift_right_from(DA, index) \
    do { \
        for (uint64_t i = dynarray_size(DA); i > index; i--) \
        { \
            (*(DA))[i] = (*(DA))[i-1]; \
        } \
    } while(0)

// Shifts all elements to the left by one
#define dynarray_shift_left(DA) \
    do { \
        dynarray_shift_left_to(DA, 0); \
    } while(0)

// Shifts all elements to the right by one
#define dynarray_shift_right(DA) \
    do { \
        dynarray_shift_right_from(DA, 0); \
    } while(0)

// Inserts element at index
#define dynarray_push(DA, E, index) \
    do { \
        dynarray_check_enlarge(DA); \
        dynarray_shift_right_from(DA, index); \
        (*(DA))[index] = E; \
        dynarray_get_header(DA)->m_size++; \
    } while(0)

// Removes element at index
#define dynarray_remove(DA, index) \
    do { \
        dynarray_shift_left_to(DA, index); \
        dynarray_get_header(DA)->m_size--; \
        dynarray_check_reduce(DA); \
    } while(0)

// Returns the i element of the list
#define dynarray_get(DA, i) \
    (assert((i) < dynarray_size(DA) && "dynarray: out of bounds!"), (*(DA))[(i)])

// Returns the ADDRESS of the i-th element
#define dynarray_get_ptr(DA, i) \
    (assert((i) < dynarray_size(DA) && "dynarray: out of bounds!"), &((*(DA))[(i)]))

// Returns the first element of the list
#define dynarray_get_first(DA) \
    (assert(dynarray_size((DA)) > 0 && "dynarray: empty!"), (*(DA))[0])

// Returns the ADDRESS of the first element of the list
#define dynarray_get_first_ptr(DA) \
    (assert(dynarray_size((DA)) > 0 && "dynarray: empty!"), &((*(DA))[0]))

// Returns the last element of the list
#define dynarray_get_last(DA) \
    (assert(dynarray_size((DA)) > 0 && "dynarray: empty!"), (*(DA))[(dynarray_size(DA)-1)])

// Returns the ADDRESS of the last element of the list
#define dynarray_get_last_ptr(DA) \
    (assert(dynarray_size((DA)) > 0 && "dynarray: empty!"), &((*(DA))[(dynarray_size(DA)-1)]))

// Adds to the start of the list
#define dynarray_push_first(DA, E) \
    do { \
        dynarray_push(DA, E, 0); \
    } while(0)

// Inserts an element to the end of the list,
#define dynarray_push_last(DA, E) \
    do { \
        dynarray_check_enlarge(DA); \
        (*(DA))[dynarray_size(DA)] = E; \
        dynarray_get_header(DA)->m_size++; \
    } while(0)

// Removes element from the start of the list,
#define dynarray_remove_first(DA) \
    do { \
        if (dynarray_size(DA) > 0) \
        { \
            dynarray_shift_left(DA); \
            dynarray_get_header(DA)->m_size--; \
            dynarray_check_reduce(DA); \
        } \
        else { \
            fprintf(stderr, "dynarray error: remove_first on empty array\n"); \
        } \
    } while(0)

// Removes element from the end of the list,
#define dynarray_remove_last(DA) \
    do { \
        if (dynarray_size(DA) > 0) \
        { \
            dynarray_get_header(DA)->m_size--; \
            dynarray_check_reduce(DA); \
        } \
            else { \
             fprintf(stderr, "dynarray error: remove_last on empty array\n"); \
        } \
    } while(0)

// Takes out element from the end of the list
#define dynarray_pop_last(DA, out) \
    do { \
        assert(dynarray_size(DA) > 0); \
        out = (*(DA))[dynarray_size(DA)-1]; \
        dynarray_remove_last(DA); \
    } while(0)

// Takes out element from the start of the list
#define dynarray_pop_first(DA, out) \
    do { \
        assert(dynarray_size(DA) > 0); \
        out = (*(DA))[0]; \
        dynarray_remove_first(DA); \
    } while(0)

// Clears list memory
#define dynarray_free(DA) \
do { \
    if (*(DA)) \
    {\
        struct dynarray_header *header = dynarray_get_header(DA); \
        memset(header, 0, sizeof(struct dynarray_header) + (header->m_capacity * sizeof(**(DA)))); \
        free(header);\
        (*(DA)) = NULL;\
    } \
} while(0)

// Clears list memory and its objects (for a list of objects in the heap)
#define dynarray_free_all(DA, F) \
    do { \
    if (*(DA)) \
        { \
            for (uint64_t i = 0; i < dynarray_size(DA); i++) \
            { \
                F((*(DA))[i]); \
            } \
            dynarray_free(DA); \
        } \
    } while(0)

#endif
