#define _POSIX_C_SOURCE 200809L
#include "hashtable.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUCKETS 16

/* FNV-1a de 32 bits */
static uint32_t fnv1a(const char *s)
{
    uint32_t h = 2166136261u;

    for (; *s; s++) {
        h ^= (unsigned char)*s;
        h *= 16777619u;
    }
    return h;
}

HashTable *ht_new(void)
{
    HashTable *ht = malloc(sizeof *ht);

    if (!ht)
        return NULL;
    ht->buckets = calloc(INITIAL_BUCKETS, sizeof *ht->buckets);
    if (!ht->buckets) {
        free(ht);
        return NULL;
    }
    ht->nbuckets = INITIAL_BUCKETS;
    ht->count = 0;
    return ht;
}

void ht_free(HashTable *ht)
{
    if (!ht)
        return;
    for (size_t i = 0; i < ht->nbuckets; i++) {
        HtEntry *e = ht->buckets[i];
        while (e) {
            HtEntry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(ht->buckets);
    free(ht);
}

/* Duplica el número de cubetas y reubica las entradas. Si falla, la tabla sigue válida. */
static int ht_grow(HashTable *ht)
{
    size_t nnew = ht->nbuckets * 2;
    HtEntry **buckets = calloc(nnew, sizeof *buckets);

    if (!buckets)
        return -1;
    for (size_t i = 0; i < ht->nbuckets; i++) {
        HtEntry *e = ht->buckets[i];
        while (e) {
            HtEntry *next = e->next;
            size_t idx = fnv1a(e->key) % nnew;
            e->next = buckets[idx];
            buckets[idx] = e;
            e = next;
        }
    }
    free(ht->buckets);
    ht->buckets = buckets;
    ht->nbuckets = nnew;
    return 0;
}

int ht_add(HashTable *ht, const char *key, long cantidad, long importe_centavos)
{
    size_t idx = fnv1a(key) % ht->nbuckets;
    HtEntry *e;

    for (e = ht->buckets[idx]; e; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            e->cantidad += cantidad;
            e->importe_centavos += importe_centavos;
            return 0;
        }
    }

    e = malloc(sizeof *e);
    if (!e)
        return -1;
    e->key = strdup(key);
    if (!e->key) {
        free(e);
        return -1;
    }
    e->cantidad = cantidad;
    e->importe_centavos = importe_centavos;

    /* Carga > 0.75: crecer. Si falla, se inserta igual (solo pierde rendimiento). */
    if ((ht->count + 1) * 4 > ht->nbuckets * 3 && ht_grow(ht) == 0)
        idx = fnv1a(key) % ht->nbuckets;
    e->next = ht->buckets[idx];
    ht->buckets[idx] = e;
    ht->count++;
    return 0;
}

HtEntry **ht_entries(const HashTable *ht, size_t *n)
{
    HtEntry **arr;
    size_t k = 0;

    *n = 0;
    if (ht->count == 0)
        return NULL;
    arr = malloc(ht->count * sizeof *arr);
    if (!arr)
        return NULL;
    for (size_t i = 0; i < ht->nbuckets; i++)
        for (HtEntry *e = ht->buckets[i]; e; e = e->next)
            arr[k++] = e;
    *n = k;
    return arr;
}
