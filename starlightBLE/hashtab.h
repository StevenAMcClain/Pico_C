#ifndef HASHTAB_H
#define HASHTAB_H

#include <stdbool.h>
#include <stddef.h>

typedef signed short NAME;

typedef struct hash 
{
    char*   key;       // owned string
    void*   value;     // user payload
    bool    used;      // slot is occupied
    bool    tomb;      // slot was used but deleted
} hash_t;


typedef struct hash_table 
{
    size_t capacity;   // total slots
    size_t count;      // active entries
    hash_t* slots;     // array of slots
} hash_table_t;


hash_table_t* Hash__Initialize(NAME num_names);
void          Hash__Terminate(hash_table_t* ht);

NAME          Hash__Lookup(hash_table_t* ht, const char* string, bool update);
void          Hash__Remove(hash_table_t* ht, NAME name);

void*         Hash__Set_Value(hash_table_t* ht, NAME name, void* ptr);
void*         Hash__Get_Value(hash_table_t* ht, NAME name);

#endif
