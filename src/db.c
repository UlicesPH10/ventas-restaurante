#include "db.h"

#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>

struct Db {
    PGconn *conn;
};

static int exec_command(Db *db, const char *sql)
{
    PGresult *res = PQexec(db->conn, sql);
    int ok = PQresultStatus(res) == PGRES_COMMAND_OK;

    if (!ok)
        fprintf(stderr, "error SQL (%s): %s", sql, PQerrorMessage(db->conn));
    PQclear(res);
    return ok ? 0 : -1;
}

Db *db_connect(void)
{
    Db *db = malloc(sizeof *db);
    PGresult *res;

    if (!db)
        return NULL;
    db->conn = PQconnectdb(""); /* toma la configuración de las variables PG* */
    if (PQstatus(db->conn) != CONNECTION_OK) {
        fprintf(stderr, "no se pudo conectar a PostgreSQL: %s", PQerrorMessage(db->conn));
        db_close(db);
        return NULL;
    }

    res = PQprepare(db->conn, "ins_venta",
                    "INSERT INTO ventas (fecha_hora, platillo, cantidad, precio_centavos) "
                    "VALUES ($1::timestamp, $2, $3::integer, $4::bigint)",
                    4, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "no se pudo preparar el INSERT: %s", PQerrorMessage(db->conn));
        PQclear(res);
        db_close(db);
        return NULL;
    }
    PQclear(res);
    return db;
}

void db_close(Db *db)
{
    if (!db)
        return;
    PQfinish(db->conn); /* acepta NULL */
    free(db);
}

int db_begin(Db *db)
{
    return exec_command(db, "BEGIN");
}

int db_commit(Db *db)
{
    return exec_command(db, "COMMIT");
}

int db_rollback(Db *db)
{
    return exec_command(db, "ROLLBACK");
}

int db_insert(Db *db, const Venta *v)
{
    char ts[32], cantidad[24], precio[24];
    const char *params[4];
    PGresult *res;
    int ok;

    snprintf(ts, sizeof ts, "%04d-%02d-%02d %02d:%02d:00",
             v->year, v->month, v->day, v->hour, v->minute);
    snprintf(cantidad, sizeof cantidad, "%ld", v->cantidad);
    snprintf(precio, sizeof precio, "%ld", v->precio_centavos);
    params[0] = ts;
    params[1] = v->platillo;
    params[2] = cantidad;
    params[3] = precio;

    res = PQexecPrepared(db->conn, "ins_venta", 4, params, NULL, NULL, 0);
    ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    if (!ok)
        fprintf(stderr, "error al insertar: %s", PQerrorMessage(db->conn));
    PQclear(res);
    return ok ? 0 : -1;
}
