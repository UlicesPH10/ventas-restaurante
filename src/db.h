#ifndef DB_H
#define DB_H

#include "csv.h"

typedef struct Db Db;

/* Conecta con las variables PG* estándar (PGHOST, PGPORT, PGUSER, PGPASSWORD, PGDATABASE). */
Db *db_connect(void);
void db_close(Db *db);

int db_begin(Db *db);
int db_insert(Db *db, const Venta *v);
int db_commit(Db *db);
int db_rollback(Db *db);

#endif
