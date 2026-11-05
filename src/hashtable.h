#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>

/* Acumulado por platillo. */
typedef struct HtEntry {
    char *key;
    long cantidad;
    long importe_centavos;
    struct HtEntry *next;
} HtEntry;

typedef struct {
    HtEntry **buckets;
    size_t nbuckets;
    size_t count;
} HashTable;

HashTable *ht_new(void);
void ht_free(HashTable *ht);

/* Suma cantidad e importe a la entrada de `key` (la crea si no existe). 0 ok, -1 sin memoria. */
int ht_add(HashTable *ht, const char *key, long cantidad, long importe_centavos);

/*
 * Devuelve un arreglo recién reservado con punteros a todas las entradas
 * (liberar con free(); las entradas pertenecen a la tabla). NULL si no hay memoria
 * o la tabla está vacía; *n recibe el tamaño.
 */
HtEntry **ht_entries(const HashTable *ht, size_t *n);

#endif
