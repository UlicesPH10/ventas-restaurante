#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "csv.h"
#include "db.h"
#include "hashtable.h"

#define TOP_N 10

enum { EXIT_OK = 0, EXIT_FATAL = 1, EXIT_RECHAZOS = 2 };

typedef struct {
    long cantidad;
    long importe_centavos;
} Hora;

static void usage(const char *prog)
{
    fprintf(stderr, "uso: %s <ventas.csv> [--dry-run]\n"
                    "  --dry-run  valida y resume sin conectar a PostgreSQL\n", prog);
}

static int cmp_top(const void *a, const void *b)
{
    const HtEntry *x = *(const HtEntry *const *)a;
    const HtEntry *y = *(const HtEntry *const *)b;

    if (x->cantidad != y->cantidad)
        return x->cantidad < y->cantidad ? 1 : -1; /* más vendidos primero */
    return strcmp(x->key, y->key);
}

static void print_resumen(const HashTable *ht, const Hora horas[24])
{
    size_t n;
    HtEntry **top = ht_entries(ht, &n);

    printf("\nTop %d platillos (unidades / importe):\n", TOP_N);
    if (top) {
        qsort(top, n, sizeof *top, cmp_top);
        for (size_t i = 0; i < n && i < TOP_N; i++)
            printf("  %-32s %5ld  $%ld.%02ld\n", top[i]->key, top[i]->cantidad,
                   top[i]->importe_centavos / 100, top[i]->importe_centavos % 100);
        free(top);
    }

    printf("\nVentas por hora (unidades / importe):\n");
    for (int h = 0; h < 24; h++)
        if (horas[h].cantidad > 0)
            printf("  %02d:00 %6ld  $%ld.%02ld\n", h, horas[h].cantidad,
                   horas[h].importe_centavos / 100, horas[h].importe_centavos % 100);
}

int main(int argc, char **argv)
{
    const char *path = NULL;
    int dry_run = 0;
    FILE *f = NULL;
    Db *db = NULL;
    HashTable *ht = NULL;
    Hora horas[24] = {{0, 0}};
    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    long lineno = 0, validas = 0, rechazadas = 0;
    int rc = EXIT_OK;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dry-run") == 0)
            dry_run = 1;
        else if (!path && argv[i][0] != '-')
            path = argv[i];
        else {
            usage(argv[0]);
            return EXIT_FATAL;
        }
    }
    if (!path) {
        usage(argv[0]);
        return EXIT_FATAL;
    }

    f = fopen(path, "r");
    if (!f) {
        perror(path);
        return EXIT_FATAL;
    }
    ht = ht_new();
    if (!ht) {
        fprintf(stderr, "sin memoria\n");
        rc = EXIT_FATAL;
        goto fin;
    }
    if (!dry_run) {
        db = db_connect();
        if (!db || db_begin(db) != 0) {
            rc = EXIT_FATAL;
            goto fin;
        }
    }

    while ((len = getline(&line, &cap, f)) != -1) {
        Venta v;
        const char *err;

        lineno++;
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        if (lineno == 1) {
            if (strcmp(line, CSV_HEADER) != 0) {
                fprintf(stderr, "%s:1: encabezado inválido, se esperaba \"%s\"\n", path, CSV_HEADER);
                rc = EXIT_FATAL;
                goto fin;
            }
            continue;
        }
        if (csv_parse_line(line, &v, &err) != 0) {
            fprintf(stderr, "%s:%ld: %s\n", path, lineno, err);
            rechazadas++;
            continue;
        }

        long importe = v.cantidad * v.precio_centavos;
        if (ht_add(ht, v.platillo, v.cantidad, importe) != 0) {
            fprintf(stderr, "sin memoria\n");
            rc = EXIT_FATAL;
            goto fin;
        }
        horas[v.hour].cantidad += v.cantidad;
        horas[v.hour].importe_centavos += importe;
        if (db && db_insert(db, &v) != 0) {
            rc = EXIT_FATAL;
            goto fin;
        }
        validas++;
    }
    if (ferror(f)) {
        perror(path);
        rc = EXIT_FATAL;
        goto fin;
    }
    if (lineno == 0) {
        fprintf(stderr, "%s: archivo vacío\n", path);
        rc = EXIT_FATAL;
        goto fin;
    }

    if (db && db_commit(db) != 0) {
        rc = EXIT_FATAL;
        goto fin;
    }
    printf("Filas válidas: %ld, rechazadas: %ld%s\n", validas, rechazadas,
           dry_run ? " (dry-run, no se escribió en la BD)" : "");
    print_resumen(ht, horas);
    if (rechazadas > 0)
        rc = EXIT_RECHAZOS;

fin:
    if (rc == EXIT_FATAL && db) {
        db_rollback(db);
        fprintf(stderr, "carga cancelada: no se guardó ninguna fila\n");
    }
    free(line);
    ht_free(ht);
    db_close(db);
    if (f)
        fclose(f);
    return rc;
}
