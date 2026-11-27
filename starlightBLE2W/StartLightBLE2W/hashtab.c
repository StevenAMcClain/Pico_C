// File: hashtab.c
// Open-addressed string hash table with tombstones and value-preserving resize.
// Slot 0 is reserved so Hash__Lookup() can continue to use 0 as "not found".

#include "common.h"
#include "hashtab.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MIN_NAMES 32u
#define LOAD_NUMERATOR 3u
#define LOAD_DENOMINATOR 4u

/* NAME is a signed short, so every valid slot index must fit in NAME. */
#define MAX_CAPACITY ((size_t)SHRT_MAX + 1u)


PRIVATE size_t hash_str(const char* s)
{
    size_t h = (size_t)146527u;

    while (*s != '\0')
    {
        h = (h * (size_t)33u) ^ (unsigned char)*s++;
    }

    return h;
}


/* Return the first usable slot for this key, or 0 if the table has no room. */
PRIVATE NAME ht_insert_owned(hash_table_t* ht, char* key, void* value)
{
    const size_t usable = ht->capacity - 1u;
    size_t idx = 1u + hash_str(key) % usable;
    size_t first_tomb = 0u;

    for (size_t probed = 0; probed < usable; ++probed)
    {
        hash_t *slot = &ht->slots[idx];

        if (!slot->used)
        {
            if (first_tomb != 0u)
            {
                slot = &ht->slots[first_tomb];
            }

            slot->key = key;
            slot->value = value;
            slot->used = true;
            slot->tomb = false;
            ++ht->count;
            return (NAME)(slot - ht->slots);
        }

        if (slot->tomb && first_tomb == 0u)
        {
            first_tomb = idx;
        }

        idx = (idx == ht->capacity - 1u) ? 1u : idx + 1u;
    }

    if (first_tomb != 0u)
    {
        hash_t *slot = &ht->slots[first_tomb];
        slot->key = key;
        slot->value = value;
        slot->used = true;
        slot->tomb = false;
        ++ht->count;
        return (NAME)first_tomb;
    }

    return 0;
}


/* Rehash into a new allocation. Keys and values are moved, not duplicated. */
PRIVATE bool ht_resize(hash_table_t* ht)
{
    if (!ht || !ht->slots || ht->capacity >= MAX_CAPACITY)
    {
        return false;
    }

    size_t new_capacity = ht->capacity * 2u;
    if (new_capacity > MAX_CAPACITY)
    {
        new_capacity = MAX_CAPACITY;
    }
    if (new_capacity <= ht->capacity || new_capacity < 2u)
    {
        return false;
    }

    hash_t *new_slots = calloc(new_capacity, sizeof(*new_slots));
    if (!new_slots)
    {
        return false;
    }

    hash_table_t new_ht = {
        .capacity = new_capacity,
        .count = 0u,
        .slots = new_slots
    };

    for (size_t i = 1u; i < ht->capacity; ++i)
    {
        hash_t *old = &ht->slots[i];
        if (old->used && !old->tomb && old->key)
        {
            if (ht_insert_owned(&new_ht, old->key, old->value) == 0)
            {
                /* Should not happen with a larger table; preserve old table. */
                free(new_slots);
                return false;
            }
        }
    }

    /* The key pointers now belong to new_slots. Do not free the strings. */
    free(ht->slots);
    ht->slots = new_slots;
    ht->capacity = new_capacity;
    ht->count = new_ht.count;
    return true;
}


PRIVATE NAME ht_insert(hash_table_t* ht, const char* key)
{
    if (!ht || !ht->slots || !key || ht->capacity < 2u)
    {
        return 0;
    }

    /* Grow before the insertion would exceed a 75% active-entry load. */
    if ((ht->count + 1u) * LOAD_DENOMINATOR >
        (ht->capacity - 1u) * LOAD_NUMERATOR)
    {
        (void)ht_resize(ht); /* If growth fails, insertion may still succeed. */
    }

    char *owned_key = malloc(strlen(key) + 1u);
    if (!owned_key)
    {
        return 0;
    }
    strcpy(owned_key, key);

    NAME result = ht_insert_owned(ht, owned_key, NULL);
    if (result == 0)
    {
        /* No slot available, even after a possible resize. */
        if (ht_resize(ht))
        {
            result = ht_insert_owned(ht, owned_key, NULL);
        }
    }

    if (result == 0)
    {
        free(owned_key);
    }
    return result;
}


PUBLIC hash_table_t* Hash__Initialize(NAME num_names)
{
    size_t capacity = (num_names < (NAME)MIN_NAMES)
                    ? MIN_NAMES : (size_t)num_names;

    if (capacity > MAX_CAPACITY)
    {
        capacity = MAX_CAPACITY;
    }
    if (capacity < 2u)
    {
        return NULL;
    }

    hash_table_t *ht = calloc(1u, sizeof(*ht));
    if (!ht)
    {
        return NULL;
    }

    ht->slots = calloc(capacity, sizeof(*ht->slots));
    if (!ht->slots)
    {
        free(ht);
        return NULL;
    }

    ht->capacity = capacity;
    ht->count = 0u;
    return ht;
}


PUBLIC void Hash__Terminate(hash_table_t* ht)
{
    if (!ht)
    {
        return;
    }

    if (ht->slots)
    {
        for (size_t i = 1u; i < ht->capacity; ++i)
        {
            if (ht->slots[i].used && !ht->slots[i].tomb)
            {
                free(ht->slots[i].key);
            }
        }
        free(ht->slots);
    }
    free(ht);
}


PUBLIC NAME Hash__Lookup(hash_table_t* ht, const char* key, bool update)
{
    if (!ht || !ht->slots || !key || ht->capacity < 2u)
    {
        return 0;
    }

    const size_t usable = ht->capacity - 1u;
    size_t idx = 1u + hash_str(key) % usable;
    size_t first_tomb = 0u;

    for (size_t probed = 0; probed < usable; ++probed)
    {
        hash_t *slot = &ht->slots[idx];

        if (!slot->used)
        {
            return update ? ht_insert(ht, key) : 0;
        }

        if (slot->tomb)
        {
            if (first_tomb == 0u)
            {
                first_tomb = idx;
            }
        }
        else if (slot->key && strcmp(slot->key, key) == 0)
        {
            return (NAME)idx;
        }

        idx = (idx == ht->capacity - 1u) ? 1u : idx + 1u;
    }

    /* A full probe means the table contains no empty slot; tombstones may remain. */
    if (update)
    {
        if (first_tomb != 0u)
        {
            char *owned_key = malloc(strlen(key) + 1u);
            if (!owned_key)
            {
                return 0;
            }
            strcpy(owned_key, key);

            hash_t *slot = &ht->slots[first_tomb];
            slot->key = owned_key;
            slot->value = NULL;
            slot->used = true;
            slot->tomb = false;
            ++ht->count;
            return (NAME)first_tomb;
        }
        return ht_insert(ht, key);
    }

    return 0;
}


PUBLIC void Hash__Remove(hash_table_t* ht, NAME name)
{
    if (!ht || !ht->slots || name <= 0 || (size_t)name >= ht->capacity)
    {
        return;
    }

    hash_t *slot = &ht->slots[name];
    if (!slot->used || slot->tomb)
    {
        return;
    }

    free(slot->key);
    slot->key = NULL;
    slot->value = NULL;
    slot->tomb = true;
    /* Keep used=true so lookups continue through the deleted slot. */
    slot->used = true;
    if (ht->count > 0u)
    {
        --ht->count;
    }
}


PUBLIC void* Hash__Set_Value(hash_table_t* ht, NAME name, void* ptr)
{
    if (!ht || !ht->slots || name <= 0 || (size_t)name >= ht->capacity)
    {
        return NULL;
    }

    hash_t *slot = &ht->slots[name];
    if (!slot->used || slot->tomb)
    {
        return NULL;
    }

    void *old = slot->value;
    slot->value = ptr;
    return old;
}


PUBLIC void* Hash__Get_Value(hash_table_t* ht, NAME name)
{
    if (!ht || !ht->slots || name <= 0 || (size_t)name >= ht->capacity)
    {
        return NULL;
    }

    hash_t *slot = &ht->slots[name];
    return (!slot->used || slot->tomb) ? NULL : slot->value;
}


// End of file: hashtab.c
