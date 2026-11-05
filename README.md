# ventas-restaurante

Cargador de ventas de restaurante escrito en **C** con **PostgreSQL** (libpq). Lee un CSV, valida cada línea,
agrupa los datos en una **tabla hash propia**, los guarda en la base de datos en una sola transacción y
genera reportes con **SQL**. Un script de **bash** lo ejecuta todo y se puede programar con **cron**.

```
 ventas.csv ──► csv.c (validación) ──► hashtable.c (resumen en memoria) ──► resumen por consola
                     │
                     └────────────► db.c (libpq, transacción) ──► PostgreSQL ──► sql/reportes.sql
```

## Requisitos
- `gcc` o `clang`, `make`
- libpq (cabeceras y librería): `brew install libpq` en macOS, `apt install libpq-dev` en Debian/Ubuntu
- Docker (para la base de datos de ejemplo)

Si `pg_config` no es el que quieres, indícalo: `make PG_CONFIG=/opt/homebrew/opt/libpq/bin/pg_config`.

## Uso rápido
```bash
./scripts/run.sh                 # compila, levanta PostgreSQL, carga data/ventas_ejemplo.csv y muestra los reportes
RESET=1 ./scripts/run.sh otro.csv   # vacía la tabla antes de cargar
PGPORT=5433 ./scripts/run.sh     # si el puerto 5432 ya está ocupado
```

Solo el cargador (usa las variables estándar `PGHOST`, `PGPORT`, `PGUSER`, `PGPASSWORD`, `PGDATABASE`):
```bash
make
./build/ventas data/ventas_ejemplo.csv            # carga en PostgreSQL
./build/ventas data/ventas_ejemplo.csv --dry-run  # solo valida y resume, sin base de datos
```

### Formato del CSV
```
fecha_hora,platillo,cantidad,precio
2026-10-01 13:45,Tacos al pastor,3,25.50
```
- `fecha_hora`: `YYYY-MM-DD HH:MM` (se validan rangos y años bisiestos)
- `platillo`: no vacío, máximo 64 caracteres, sin comas ni comillas
- `cantidad`: entero de 1 a 10000
- `precio`: `25`, `25.5` o `25.50` (se guarda en centavos como entero, nunca en `float`)

### Códigos de salida
| Código | Significado |
|---|---|
| 0 | Todo correcto |
| 1 | Error fatal (archivo, encabezado inválido, base de datos). Se hace `ROLLBACK`: no se guarda nada |
| 2 | Había líneas inválidas: se reportan en stderr con su número y se cargan las válidas |

## Ejemplo de reporte
```
Filas válidas: 339, rechazadas: 0

== Top 10 platillos más vendidos ==
            platillo             | unidades |  total
---------------------------------+----------+----------
 Agua de horchata                |       77 | 2,310.00
 Refresco                        |       62 | 1,736.00
 Tacos al pastor                 |       62 | 1,581.00
 Pozole rojo                     |       49 | 5,390.00
 ...

== Ventas por hora del día ==
 hora  | unidades |  total
-------+----------+----------
 13:00 |      100 | 5,666.00
 14:00 |      107 | 6,734.50
 ...
```
El resumen que calcula el programa en C con la tabla hash coincide con el que devuelven las consultas de `sql/reportes.sql`.

## Calidad
```bash
make          # -std=c11 -Wall -Wextra -Wpedantic -Werror: compila sin advertencias
make test     # pruebas con datos buenos y mal formados (tests/datos), sin necesitar la base de datos
make asan     # las mismas pruebas con AddressSanitizer + UBSan
```
Para revisar fugas de memoria:
- Linux: `valgrind --leak-check=full ./build/ventas data/ventas_ejemplo.csv --dry-run`
- macOS (no hay valgrind): `MallocStackLogging=1 leaks --atExit -- ./build/ventas data/ventas_ejemplo.csv --dry-run`

Resultado actual: 0 fugas.

## Automatización con cron
Cargar el archivo del día todas las noches a las 23:00:
```
0 23 * * * cd /ruta/a/ventas-restaurante && ./scripts/run.sh /ruta/a/ventas_del_dia.csv >> /var/log/ventas.log 2>&1
```

## Decisiones de diseño
- **Tabla hash propia** (encadenamiento, FNV-1a, crece al superar 0.75 de carga) para agrupar por platillo; el top se obtiene con `qsort`.
- **Dinero en centavos** (`long`) para evitar errores de redondeo de punto flotante.
- **Una transacción por archivo** con sentencia preparada (`PQprepare` / `PQexecPrepared`): o entra todo o no entra nada, y no hay inyección SQL porque los valores van como parámetros.
- **Validación en el borde**: cada campo se revisa (`strtol` con `errno`, longitudes, rangos de fecha) y las líneas inválidas no detienen la carga.
- La base de datos también tiene restricciones `CHECK` (`cantidad > 0`, `precio_centavos >= 0`) como segunda defensa.

## Limitaciones conocidas
- Cargar dos veces el mismo archivo duplica las filas (no hay clave única natural en las ventas); usa `RESET=1` o carga archivos distintos.
- El CSV no admite comillas ni comas dentro de un campo.

## Estructura
```
src/        main.c, csv.c, hashtable.c, db.c
sql/        schema.sql (tabla), reportes.sql (consultas)
scripts/    run.sh (compila, carga y reporta)
tests/      run_tests.sh y datos de prueba
data/       ventas_ejemplo.csv (339 ventas de octubre de 2026)
```
