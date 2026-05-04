#include "string_t.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void string_init(string_t *s_t, const char *s)
{
    if (!s)
    {
        s_t->data = NULL;
        s_t->size = 0;
        s_t->capacity = 0;
        return;
    }

    s_t->size = strlen(s);
    s_t->capacity = s_t->size+1;
    s_t->data = malloc(s_t->capacity);

    if (!s_t->data)
    {
        s_t->size = 0;
        s_t->capacity = 0;
        return;
    }

    memcpy(s_t->data, s, s_t->size);
    s_t->data[s_t->size] = '\0';
}

void string_reserve(string_t *s_t, uint64_t new_capacity)
{
    if (new_capacity > s_t->capacity)
    {
        s_t->data = realloc(s_t->data, new_capacity);
        s_t->capacity = new_capacity;
    }
}

void string_append(string_t *s_t, const char *extra)
{
    uint64_t extra_len = strlen(extra);
    uint64_t needed = s_t->size + extra_len + 1;

    if (needed > s_t->capacity)
    {
        // Double the capacity to avoid frequent reallocs
        uint64_t new_cap = s_t->capacity * 2;
        if (new_cap < needed)
        {
            new_cap = needed;
        }
        
        s_t->data = realloc(s_t->data, new_cap);
        s_t->capacity = new_cap;
    }

    memcpy(s_t->data + s_t->size, extra, extra_len);
    s_t->size += extra_len;
    s_t->data[s_t->size] = '\0';
}

void string_copy(string_t *dest, const string_t *src)
{
    string_init(dest, src->data);
}

int string_compare(const string_t *s1, const string_t *s2)
{
    uint64_t min_size = (s1->size < s2->size) ? s1->size : s2->size;

    int cmp = memcmp(s1->data, s2->data, min_size);

    if (cmp != 0)
    {
        return cmp;
    }

    if (s1->size == s2->size)
    {
        return 0;
    }

    return (s1->size > s2->size) ? 1 : -1;
}

int64_t string_find(const string_t *s_t, const char *substr)
{
    char *ptr = strstr(s_t->data, substr);
    if (!ptr)
    {
        return -1;
    }

    return (int64_t)(ptr - s_t->data);
}

bool string_is_empty(string_t *s)
{
    return (!(s->size));
}

void string_to_lower(string_t *s)
{
    for (uint64_t i = 0; i < s->size; i++)
    {
        s->data[i] = (char)tolower((unsigned char)s->data[i]);
    }
}

void string_to_upper(string_t *s)
{
    for (uint64_t i = 0; i < s->size; i++)
    {
        s->data[i] = (char)toupper((unsigned char)s->data[i]);
    }
}

void string_trim(string_t *s)
{
    char *start = s->data;
    char *end = s->data + s->size-1;

    while (start <= end && isspace((unsigned char) *start))
    {
        start++;
    }

    while (end >= start && isspace((unsigned char) *end))
    {
        end--;
    }

    uint64_t new_size = (start > end) ? 0 : (uint64_t)(end - start + 1);

    if (start != s->data && new_size > 0)
    {
        memmove(s->data, start, new_size);
    }

    s->size = new_size;
    s->data[new_size] = '\0';
}

void string_clear(string_t *s_t)
{
    s_t->size = 0;
    if (s_t->data)
    {
        s_t->data[0] = '\0';
    }
}

void string_free(string_t *s_t)
{
    free(s_t->data);
    s_t->data = NULL;
    s_t->size = 0;
    s_t->capacity = 0;
}

void string_printf(string_t *s_t, const char *format, ...)
{
    va_list args;
    
    // Start args to determine length needed
    va_start(args, format);
    int needed = vsnprintf(NULL, 0, format, args); // Dry run returns length
    va_end(args);

    if (needed < 0) return;

    // Capacity
    uint64_t total_needed = (uint64_t)needed + 1;
    if (total_needed > s_t->capacity)
    {
        s_t->data = realloc(s_t->data, total_needed);
        s_t->capacity = total_needed;
    }

    // Format the string into the buffer
    va_start(args, format);
    vsnprintf(s_t->data, total_needed, format, args);
    va_end(args);

    s_t->size = (uint64_t)needed;
}

string_t *string_split(const string_t *s_t, const char *delim, int *count)
{
    // Create a copy because strtok is destructive
    char *temp_str = malloc(s_t->size + 1);

    if (!temp_str)
    {
        return NULL;
    }

    memcpy(temp_str, s_t->data, s_t->size + 1);
        
    // Count tokens first to malloc the right amount of string_t objects
    int n = 0;
    char *token = strtok(temp_str, delim);
    while (token)
    {
        n++;
        token = strtok(NULL, delim);
    }
    free(temp_str); // Done with the counting copy

    if (n == 0)
    {
        *count = 0;
        return NULL;
    }

    // Allocate array of string_t
    string_t *result = malloc(sizeof(string_t) * n);
    *count = n;

    // Second pass to actually populate the array
    char *temp_str2 = malloc(s_t->size + 1);
    if (!temp_str2)
    {
        free(result);
        return NULL;
    }
    memcpy(temp_str2, s_t->data, s_t->size + 1);

    token = strtok(temp_str2, delim);
    for (int i = 0; i < n; i++) {
        string_init(&result[i], token); // Deep copy each token
        token = strtok(NULL, delim);
    }
    
    free(temp_str2);
    return result;
}

int string_get_line(string_t *s_t, FILE *f)
{
    // Check if there's a big enough buffer
    if (s_t->capacity == 0) 
    {
        s_t->capacity = 256; 
        s_t->data = malloc(s_t->capacity);
        if (!s_t->data)
        {
            return -1;
        }
    }

    s_t->size = 0;

    while (1)
    {
        char *pos = s_t->data + s_t->size;
        int space = (int)(s_t->capacity - s_t->size);

        if (fgets(pos, space, f) == NULL)
        {
            if (s_t->size == 0) return -1; // EOF
            break; 
        }

        s_t->size += strlen(pos);

        // Check for newline
        if (s_t->data[s_t->size - 1] == '\n')
        {
            s_t->data[--s_t->size] = '\0'; // Strip newline
            break;
        }

        // If buffer is full, grow it by 2 times
        if (s_t->size + 1 >= s_t->capacity)
        {
            s_t->capacity *= 2;
            s_t->data = realloc(s_t->data, s_t->capacity);
            if (!s_t->data)
            {
                return -1;
            }
        }
    }
    return 0;
}