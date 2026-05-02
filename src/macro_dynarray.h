
#ifndef _MACRO_DYNARRAY_H_
#define _MACRO_DYNARRAY_H_

// based on https://crocidb.com/post/simple-vector-implementation-in-c/
//    and https://www.youtube.com/watch?v=HvG03MY2H04

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct dynarray_header
{
    uint64_t m_capacity;
    uint64_t m_size;
};

// Creates a dynamic array (eg: dynarray(int) a -> int a*)
#define dynarray(T) T*

// Initializes dynamic array, allocating 8 spaces from the start
#define dynarray_init(DA) \
    { \
        uint64_t initial_capacity = 8; \
        struct dynarray_header *header = malloc((sizeof(*header)) + (uint64_t)(initial_capacity*sizeof(**(DA)))); \
        header->m_capacity = initial_capacity; \
        header->m_size = 0; \
        (*(DA)) = (void*)(header + 1); \
    }

// Initializes dynamic array, allocating n spaces from the start
#define dynarray_init_n(DA, n) \
    { \
        uint64_t initial_capacity = n; \
        struct dynarray_header *header = malloc((sizeof(*header)) + (uint64_t)(initial_capacity*sizeof(**(DA)))); \
        header->m_capacity = initial_capacity; \
        header->m_size = 0; \
        (*(DA)) = (void*)(header + 1); \
    }
    
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
    { \
        struct dynarray_header *header = dynarray_get_header(DA); \
        header->m_capacity = required_size; \
        header = (struct dynarray_header*)realloc(header, (sizeof *header) + (uint64_t)(required_size * sizeof(**(DA)))); \
        (*(DA)) = (void*)(header + 1); \
    }

// Multiplies capacity by 2 if needed
#define dynarray_check_enlarge(DA) \
{ \
    if ( (dynarray_get_header(DA)->m_size * 2) > dynarray_get_header(DA)->m_capacity ) \
    { \
        uint64_t new_capacity = dynarray_get_header(DA)->m_capacity * 2; \
        dynarray_resize(DA, new_capacity); \
    } \
}

// Divides capacity by 2 if needed
#define dynarray_check_reduce(DA) \
{ \
    if ( dynarray_get_header(DA)->m_size < (dynarray_get_header(DA)->m_capacity / 2) ) \
    { \
        uint64_t new_capacity = dynarray_get_header(DA)->m_capacity / 2; \
        dynarray_resize(DA, new_capacity); \
    } \
}

// Shifts left from end to index
#define dynarray_shift_left_to(DA, index) \
    { \
        for (uint64_t i = index; i < dynarray_size(DA) - 1; i++) \
        { \
            (*(DA))[i] = (*(DA))[i+1]; \
        } \
    } \

// Shifts right from index
#define dynarray_shift_right_from(DA, index) \
    { \
        for (uint64_t i = dynarray_size(DA); i > index; i--) \
        { \
            (*(DA))[i] = (*(DA))[i-1]; \
        } \
    } \

// Shifts all elements to the left by one
#define dynarray_shift_left(DA) \
    { \
        dynarray_shift_left_to(DA, 0); \
    }

// Shifts all elements to the right by one
#define dynarray_shift_right(DA) \
    { \
        dynarray_shift_right_from(DA, 0); \
    }

// Inserts element at index
#define dynarray_push(DA, E, index) \
    { \
        dynarray_check_enlarge(DA); \
        dynarray_shift_right_from(DA, index); \
        (*(DA))[index] = E; \
        dynarray_get_header(DA)->m_size++; \
    } \

// Removes element at index
#define dynarray_remove(DA, index) \
    { \
        dynarray_shift_left_to(DA, index) \
        dynarray_check_reduce(DA); \
        dynarray_get_header(DA)->m_size--; \
    } \

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
    { \
        dynarray_push(DA, E, 0); \
    }

// Inserts an element to the end of the list,
#define dynarray_push_last(DA, E) \
    { \
        dynarray_check_enlarge(DA); \
        (*(DA))[dynarray_size(DA)] = E; \
        dynarray_get_header(DA)->m_size++; \
    }

// Removes element from the start of the list,
#define dynarray_remove_first(DA) \
    { \
        if (dynarray_get_header(DA)->m_size) \
        { \
            dynarray_shift_left(DA); \
            dynarray_check_reduce(DA); \
            dynarray_get_header(DA)->m_size--; \
        } \
    }

// Removes element from the end of the list,
#define dynarray_remove_last(DA) \
    { \
        if (dynarray_get_header(DA)->m_size) \
        { \
            dynarray_get_header(DA)->m_size--; \
            dynarray_check_reduce(DA); \
        } \
    }

// Takes out element from the end of the list
#define dynarray_pop_last(DA, out) \
    { out = (*(DA))[dynarray_size(DA)-1]; dynarray_remove_last(DA); }

// Takes out element from the start of the list
#define dynarray_pop_first(DA, out) \
    { out = (*(DA))[0]; dynarray_remove_first(DA); }

// Clears list memory
#define dynarray_free(DA) \
    if (*(DA)) \
    {\
        memset(*(DA), 0, (uint64_t)dynarray_size(DA) * (sizeof(**(DA)))); \
        free(dynarray_get_header(DA));\
        (*(DA)) = NULL;\
    } \

// Clears list memory and its objects (for a list of objects in the heap)
#define dynarray_free_all(DA, F) \
    { \
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
    }

#endif
