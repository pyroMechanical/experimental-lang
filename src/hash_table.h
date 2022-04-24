#ifndef hash_table_header
#define hash_table_header

#include "core.h"

typedef struct {
    const char* key;
    void* value;
} Entry;

typedef struct {
    size_t count;
    size_t capacity;
    Entry* entries;
} HashTable;

void initTable(HashTable* table);
void freeTable(HashTable* table);
void* searchTable(HashTable* table, const char* key);
bool deleteFromTable(HashTable* table, const char* key);
bool insertTable(HashTable* table, const char* key, void* value);
void insertAllTable(HashTable* from, HashTable* to);
void applyTable(HashTable* table, void(*func)(void*));
size_t nextEntryIndex(HashTable* table, size_t previous);

size_t hashString(const char* str);

size_t hashInt(size_t num);

size_t hashFloat(float num);

#endif