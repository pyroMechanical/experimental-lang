#ifndef hash_table_header
#define hash_table_header

#include "core.h"

typedef struct {
    const char* key;
    void* value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entries;
} HashTable;

void initTable(HashTable* table);
void freeTable(HashTable* table);
Entry* searchTable(HashTable* table, const char* key);
bool deleteFromTable(HashTable* table, const char* key);
bool insertTable(HashTable* table, const char* key, void* value);
void insertAllTable(HashTable* from, HashTable* to);

size_t hashString(const char* str);

size_t hashInt(size_t num);

size_t hashFloat(float num);

#endif