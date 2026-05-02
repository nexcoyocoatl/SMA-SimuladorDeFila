
#ifndef _MACRO_DYNARRAY_H_
#define _MACRO_DYNARRAY_H_

// based on https://crocidb.com/post/simple-vector-implementation-in-c/
//    and https://www.youtube.com/watch?v=HvG03MY2H04

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

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
        uint64_t new_size = sizeof(struct dynarray_header) + ((uint64_t)(required_size) * sizeof(**(DA))); \
        void *temp = realloc(header, new_size); \
        if (temp) { \
            header = (struct dynarray_header*)temp; \
            header->m_capacity = (required_size); \
            (*(DA)) = (void*)(header + 1); \
        } \
        else \
        {   \
            fprintf(stderr, "dynarray error: realloc failed to allocate %zu bytes\n", new_size); \
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
    if ( dynarray_get_header(DA)->m_size < (dynarray_get_header(DA)->m_capacity / 2) ) \
    { \
        uint64_t new_capacity = dynarray_get_header(DA)->m_capacity / 2; \
        dynarray_resize(DA, new_capacity); \
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
        dynarray_shift_left_to(DA, index) \
        dynarray_check_reduce(DA); \
        dynarray_get_header(DA)->m_size--; \
    } while(0)

// out is assigned the i element of the list
#define dynarray_get(DA, i, out) \
    out = (*(DA))[i]

// out is assigned the first element of the list
#define dynarray_get_first(DA, out) \
    out = (*(DA))[0]

// Macro for the first element
#define dynarray_first(DA) (*(DA))[0]

// out is assigned the last element of the list
#define dynarray_get_last(DA, out) \
    out = (*(DA))[dynarray_size(DA)-1]

// Macro for the last element
#define dynarray_last(DA) (*(DA))[dynarray_size(DA)-1]

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
        if (dynarray_get_header(DA)->m_size) \
        { \
            dynarray_shift_left(DA); \
            dynarray_check_reduce(DA); \
            dynarray_get_header(DA)->m_size--; \
        } \
    } while(0)

// Removes element from the end of the list,
#define dynarray_remove_last(DA) \
    do { \
        if (dynarray_get_header(DA)->m_size) \
        { \
            dynarray_get_header(DA)->m_size--; \
            dynarray_check_reduce(DA); \
        } \
    } while(0)

// Takes out element from the end of the list
#define dynarray_pop_last(DA, out) \
    do { out = (*(DA))[dynarray_size(DA)-1]; dynarray_remove_last(DA); } while(0)

// Takes out element from the start of the list
#define dynarray_pop_first(DA, out) \
    do { out = (*(DA))[0]; dynarray_remove_first(DA); } while(0)

// Clears list memory
#define dynarray_free(DA) \
do { \
    if (*(DA)) \
    {\
        memset(*(DA), 0, (uint64_t)dynarray_size(DA) * (sizeof(**(DA)))); \
        free(dynarray_get_header(DA));\
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
            memset(*(DA), 0, (uint64_t)dynarray_size(DA) * (sizeof(**(DA)))); \
            free(dynarray_get_header(DA)); \
            (*(DA)) = NULL; \
        } \
    } while(0)

#endif
