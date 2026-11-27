// File: hashtab.c

#include "common.h"
#include "hashtab.h"

#include <stdlib.h>
#include <string.h>


#define LOAD_FACTOR 0.75


PRIVATE char* strdup_owned(const char* s) 
{
    size_t len = strlen(s) + 1;
    char* p = malloc(len);

	if (p) { memcpy(p, s, len); }

	return p;
}


PRIVATE size_t hash_str(const char* s) 
{
    size_t h = 146527;

	while (*s)
	{
        h = (h * 33) ^ (unsigned char)*s++;
	}

	return h;
}


PRIVATE void ht_resize(hash_table_t* ht);

PRIVATE NAME ht_insert(hash_table_t* ht, const char* key) 
{
    if ((double)ht->count / ht->capacity > LOAD_FACTOR)
	{
        ht_resize(ht);
	}

    size_t h = hash_str(key);
    size_t idx = h % ht->capacity;
    size_t dist = 0;

    while (1) 
	{
        hash_t* slot = &ht->slots[idx];

        if (!slot->used) 
		{
            slot->key = strdup_owned(key);
            slot->used = true;
            slot->tomb = false;

            ht->count++;

            return (NAME)idx;
        }

        size_t existing_h = hash_str(slot->key);
        size_t existing_dist = (idx + ht->capacity - (existing_h % ht->capacity)) % ht->capacity;

        if (existing_dist < dist) 
		{
            // Robin Hood swap
            char* tmp_key = slot->key;
            void* tmp_val = slot->value;
            bool tmp_used = slot->used;
            bool tmp_tomb = slot->tomb;

            slot->key = strdup_owned(key);
            slot->value = NULL;
            slot->used = true;
            slot->tomb = false;

            key = tmp_key;
            h = existing_h;
            dist = existing_dist;

            slot->key = tmp_key;
            slot->value = tmp_val;
            slot->used = tmp_used;
            slot->tomb = tmp_tomb;
        }

        idx = (idx + 1) % ht->capacity;
        dist++;
    }
}


PRIVATE void ht_resize(hash_table_t* ht) 
{
    size_t old_cap = ht->capacity;
    hash_t* old_slots = ht->slots;

    ht->capacity *= 2;
    ht->slots = calloc(ht->capacity, sizeof(hash_t));
    ht->count = 0;

    for (size_t i = 0; i < old_cap; i++) 
	{
        if (old_slots[i].used && !old_slots[i].tomb)
		{
            ht_insert(ht, old_slots[i].key);
		}
        free(old_slots[i].key);
    }

    free(old_slots);
}

#define MIN_NAMES 32

PUBLIC hash_table_t* Hash__Initialize(NAME num_names) 
{
    if (num_names < MIN_NAMES) { num_names = MIN_NAMES; }

    hash_table_t *ht = calloc(1, sizeof(hash_table_t));

	if (ht)
	{
		ht->capacity = num_names;
		ht->slots = calloc(ht->capacity, sizeof(hash_t));
	}
    return ht;
}


PUBLIC void Hash__Terminate(hash_table_t *ht) 
{
    if (!ht) { return; }

    for (size_t i = 0; i < ht->capacity; i++)
	{
        free(ht->slots[i].key);
	}

    free(ht->slots);
    free(ht);
}


PUBLIC NAME Hash__Lookup(hash_table_t* ht, const char* key, bool update) 
{
    size_t h = hash_str(key);
    size_t idx = h % ht->capacity;
    size_t dist = 0;

    while (1) 
	{
        hash_t *slot = &ht->slots[idx];

        if (!slot->used) 
		{
            return update ? ht_insert(ht, key) : 0;
        }

        if (!slot->tomb && strcmp(slot->key, key) == 0)
		{
			return (NAME)idx;
		}

        size_t existing_h = hash_str(slot->key);
        size_t existing_dist = (idx + ht->capacity - (existing_h % ht->capacity)) % ht->capacity;

        if (existing_dist < dist)
		{
            return update ? ht_insert(ht, key) : 0;
		}

        idx = (idx + 1) % ht->capacity;
        dist++;
    }
}


PUBLIC void Hash__Remove(hash_table_t *ht, NAME name) 
{
    if (name < 0 || (size_t)name >= ht->capacity)  { return; }

    hash_t* slot = &ht->slots[name];

	if (!slot->used || slot->tomb) { return; }

    free(slot->key);

	slot->key = NULL;
    slot->value = NULL;
    slot->tomb = true;
    slot->used = false;
}


PUBLIC void* Hash__Set_Value(hash_table_t *ht, NAME name, void *ptr) 
{
    if (name < 0 || (size_t)name >= ht->capacity) { return NULL; }

    void *old = ht->slots[name].value;
    ht->slots[name].value = ptr;

    return old;
}


PUBLIC void* Hash__Get_Value(hash_table_t* ht, NAME name) 
{
	return (name < 0 || (size_t)name >= ht->capacity) ? NULL : ht->slots[name].value;
}


// EndFile: hashtab.c