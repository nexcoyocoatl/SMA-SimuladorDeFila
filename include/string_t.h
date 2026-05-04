#ifndef __STRING_T_H__
#define __STRING_T_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    char *data;
    uint64_t size;
    uint64_t capacity;
} string_t;

typedef struct {
    const char *data;
    uint64_t size;
} string_view_t;

void string_init(string_t *s_t, const char *s);
void string_reserve(string_t *s_t, uint64_t new_capacity);
void string_append(string_t *s_t, const char *extra);
void string_copy(string_t *dest, const string_t *src);
int string_compare(const string_t *s1, const string_t *s2);
int64_t string_find(const string_t *s_t, const char *substr);
bool string_is_empty(string_t *s);
void string_to_lower(string_t *s);
void string_to_upper(string_t *s);
void string_trim(string_t *s);
void string_clear(string_t *s_t);
void string_free(string_t *s_t);
void string_printf(string_t *s_t, const char *format, ...);
string_t *string_split(const string_t *s_t, const char *delim, int *count);
int string_get_line(string_t *s_t, FILE *f);

// Creates a string_t
#define STRING_CREATE(NAME, INIT_STR) \
    string_t NAME; \
    string_init(&NAME, INIT_STR);


#define STRING_SPLIT_FREE(ARRAY, COUNT) \
do { \
    if (ARRAY) \
    { \
        for (int _i = 0; _i < COUNT; _i++) \
        { \
            string_free(&(ARRAY)[_i]); \
        } \
        free(ARRAY); \
    } \
} while(0)

#endif