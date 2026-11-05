#!/usr/bin/env bash
# Compila, levanta PostgreSQL, carga el CSV y muestra los reportes SQL.
# Uso: scripts/run.sh [archivo.csv]        (por defecto data/ventas_ejemplo.csv)
#      RESET=1 scripts/run.sh ...          (vacía la tabla antes de cargar)
# Código de salida: 0 ok, 2 si el CSV tuvo líneas rechazadas, 1 si hubo error.
set -euo pipefail
cd "$(dirname "$0")/.."

csv=${1:-data/ventas_ejemplo.csv}
export PGHOST=${PGHOST:-localhost} PGPORT=${PGPORT:-5432}
export PGUSER=${PGUSER:-ventas} PGPASSWORD=${PGPASSWORD:-ventas} PGDATABASE=${PGDATABASE:-ventas}

log() { echo "[$(date '+%F %T')] $*"; }

log "compilando"
make -s

log "levantando PostgreSQL"
docker compose up -d --wait

psql_db() { docker compose exec -T db psql -v ON_ERROR_STOP=1 -U "$PGUSER" -d "$PGDATABASE" "$@"; }

if [ "${RESET:-0}" = "1" ]; then
    log "vaciando tabla ventas"
    psql_db -c "TRUNCATE ventas RESTART IDENTITY"
fi

log "cargando $csv"
rc=0
./build/ventas "$csv" || rc=$?
if [ "$rc" -eq 1 ]; then
    log "la carga falló"
    exit 1
fi

log "reportes"
psql_db <sql/reportes.sql
exit "$rc"
