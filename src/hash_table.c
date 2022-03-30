#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "hash_table.h"

#define TABLE_MAX_LOAD 0.75
#define TOMBSTONE_VALUE (0x8000000000000000ull ^ ((size_t)NULL))

void initTable(HashTable* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(HashTable* table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

static Entry* findEntry(Entry* entries, int capacity, const char* key)
{
    size_t index = hashString(key) % capacity;
    Entry* tombstone = NULL;
    for(;;)
    {
        Entry* entry = &entries[index];
        if(entry->key == NULL)
        {
            if(entry->value == NULL)
            {
                return tombstone != NULL ? tombstone : entry;
            }
            else
            {
                if(tombstone == NULL) tombstone = entry;
            }
        }
        else if (strcmp(entry->key, key) == 0)
        {
            return entry;
        }
 
        index = (index + 1) % capacity;
    }
}

Entry* searchTable(HashTable* table, const char* key)
{
    if (table->count == 0) return NULL;
    Entry* val = findEntry(table->entries, table->capacity, key);
    if(val->key == NULL) return NULL;
    return val;
}

static void adjustCapacity(HashTable* table, size_t capacity)
{
    //calloc will always set values to 0,
    Entry* entries = calloc(capacity, sizeof(Entry));
    if(NULL != 0)
    {
        for (size_t i = 0; i < capacity; i++) entries[i].key = NULL;
    }
    table->count = 0;
    for (size_t i = 0; i < table->capacity; i++)
    {
        Entry* entry = &table->entries[i];
        if(entry->key == NULL) continue;

        Entry* dest = findEntry(entries, capacity, entry->key);
        *dest = *entry;
        table->count++;
    }
    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool insertTable(HashTable* table, const char* key, void* value)
{
    if(table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        size_t capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }
    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey && entry->value == NULL) table->count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

bool deleteFromTable(HashTable* table, const char* key)
{
    if(table->count == 0) return false;


    Entry* entry = findEntry(table->entries, table->capacity, key);
    if(entry->key == NULL) return false;

    entry->key = NULL;
    entry->value = (void*) TOMBSTONE_VALUE;
    return true;
}

void insertAllTable(HashTable* from, HashTable* to)
{
    for (size_t i = 0; i < from->capacity; i++)
    {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL)
        {
            insertTable(to, entry->key, entry->value);
        }
    }
}

//assumes all strings are null-terminated.
size_t hashString(const char* str)
{
    size_t hash = 14695981039346656037ull;
    char* byte = str;
    while(*byte++)
    {
        hash ^= (uint8_t)*byte;
        hash *= 1099511628211;
    }
    return hash;
}

size_t hashInt(size_t num)
{
     size_t hash = 14695981039346656037ull;
    for(uint8_t byte = 0; num != 0; byte = (uint8_t)(num >>= 8))
    {
        hash ^= byte;
        hash *= 1099511628211;
    }
    return hash;
}

size_t hashFloat(float num)
{
    size_t hash = 14695981039346656037ull;
    uint32_t bits = *(int*)(&num);
    for(uint8_t byte = 0; num != 0; byte = (uint8_t)(bits >>= 8))
    {
        hash ^= byte;
        hash *= 1099511628211;
    }
    return hash;
}