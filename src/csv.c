#include "csv.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CANTIDAD_MAX 10000L
#define PRECIO_ENTEROS_MAX 9 /* hasta 999,999,999.99 */

static int es_bisiesto(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static int dias_del_mes(int y, int m)
{
    static const int dias[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && es_bisiesto(y)) ? 29 : dias[m - 1];
}

static int parse_fecha(const char *s, Venta *v)
{
    int consumed = 0;

    if (sscanf(s, "%4d-%2d-%2d %2d:%2d%n", &v->year, &v->month, &v->day,
               &v->hour, &v->minute, &consumed) != 5 || s[consumed] != '\0')
        return -1;
    if (strlen(s) != 16) /* exige YYYY-MM-DD HH:MM con ceros */
        return -1;
    if (v->year < 2000 || v->month < 1 || v->month > 12)
        return -1;
    if (v->day < 1 || v->day > dias_del_mes(v->year, v->month))
        return -1;
    if (v->hour < 0 || v->hour > 23 || v->minute < 0 || v->minute > 59)
        return -1;
    return 0;
}

/* "25", "25.5" o "25.50" -> centavos. Sin signo, sin notación científica. */
static int parse_precio(const char *s, long *centavos)
{
    long enteros = 0, decimales = 0;
    int n_enteros = 0, n_dec = 0;

    for (; *s >= '0' && *s <= '9'; s++, n_enteros++)
        enteros = enteros * 10 + (*s - '0');
    if (n_enteros == 0 || n_enteros > PRECIO_ENTEROS_MAX)
        return -1;
    if (*s == '.') {
        for (s++; *s >= '0' && *s <= '9'; s++, n_dec++)
            decimales = decimales * 10 + (*s - '0');
        if (n_dec == 0 || n_dec > 2)
            return -1;
        if (n_dec == 1)
            decimales *= 10;
    }
    if (*s != '\0')
        return -1;
    *centavos = enteros * 100 + decimales;
    return 0;
}

static int parse_cantidad(const char *s, long *out)
{
    char *end;
    long n;

    if (*s == '\0')
        return -1;
    errno = 0;
    n = strtol(s, &end, 10);
    if (errno != 0 || *end != '\0' || n < 1 || n > CANTIDAD_MAX)
        return -1;
    *out = n;
    return 0;
}

int csv_parse_line(char *line, Venta *v, const char **err)
{
    char *campos[4];
    char *p = line;
    int n = 0;

    if (*line == '\0') {
        *err = "línea vacía";
        return -1;
    }
    /* Separa en campos in situ; exactamente 4 */
    campos[n++] = p;
    while ((p = strchr(p, ',')) != NULL) {
        *p++ = '\0';
        if (n == 4) {
            *err = "número de campos incorrecto (se esperan 4)";
            return -1;
        }
        campos[n++] = p;
    }
    if (n != 4) {
        *err = "número de campos incorrecto (se esperan 4)";
        return -1;
    }

    if (parse_fecha(campos[0], v) != 0) {
        *err = "fecha_hora inválida (formato YYYY-MM-DD HH:MM)";
        return -1;
    }
    if (campos[1][0] == '\0') {
        *err = "platillo vacío";
        return -1;
    }
    if (strlen(campos[1]) > PLATILLO_MAX) {
        *err = "platillo demasiado largo (máximo 64 caracteres)";
        return -1;
    }
    if (strchr(campos[1], '"') != NULL) {
        *err = "platillo con comillas no soportado";
        return -1;
    }
    if (parse_cantidad(campos[2], &v->cantidad) != 0) {
        *err = "cantidad inválida (entero entre 1 y 10000)";
        return -1;
    }
    if (parse_precio(campos[3], &v->precio_centavos) != 0) {
        *err = "precio inválido (formato 25 o 25.50)";
        return -1;
    }
    strcpy(v->platillo, campos[1]);
    return 0;
}
