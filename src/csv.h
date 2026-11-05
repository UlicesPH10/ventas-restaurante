#ifndef CSV_H
#define CSV_H

#define CSV_HEADER "fecha_hora,platillo,cantidad,precio"
#define PLATILLO_MAX 64

typedef struct {
    int year, month, day, hour, minute;
    char platillo[PLATILLO_MAX + 1];
    long cantidad;
    long precio_centavos;
} Venta;

/*
 * Valida y parsea una línea de datos (sin salto de línea). Modifica `line`.
 * Devuelve 0 si es válida; en caso contrario devuelve -1 y deja en *err
 * un mensaje estático que describe el problema.
 */
int csv_parse_line(char *line, Venta *v, const char **err);

#endif
