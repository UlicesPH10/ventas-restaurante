#!/usr/bin/env bash
# Pruebas del cargador en modo --dry-run (no requiere PostgreSQL).
# Uso: BIN=build/ventas tests/run_tests.sh
set -u
cd "$(dirname "$0")/.."
BIN=${BIN:-build/ventas}
fallos=0
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# Ejecuta el binario; guarda stdout/stderr y devuelve el código de salida en $rc
correr() {
    "$BIN" "$@" >"$tmp/out" 2>"$tmp/err"
    rc=$?
}

verificar() { # descripción, condición (0 = ok)
    if [ "$2" -eq 0 ]; then
        echo "ok   - $1"
    else
        echo "FALLA - $1"
        fallos=$((fallos + 1))
    fi
}

contiene() { grep -qF -- "$2" "$1"; echo $?; }

# 1. Datos buenos: salida 0, 5 filas válidas, bisiesto aceptado
correr tests/datos/buenos.csv --dry-run
verificar "buenos.csv sale con 0" $((rc != 0))
verificar "buenos.csv: 5 válidas, 0 rechazadas" "$(contiene "$tmp/out" 'Filas válidas: 5, rechazadas: 0')"
verificar "buenos.csv: Tacos al pastor suma 7 unidades y \$178.50" \
    "$(grep -E 'Tacos al pastor +7 +\$178\.50' "$tmp/out" >/dev/null; echo $?)"
verificar "buenos.csv: sin errores en stderr" $([ ! -s "$tmp/err" ]; echo $?)

# 2. Datos malos: salida 2, 2 válidas, 10 rechazadas, cada línea reportada
correr tests/datos/malos.csv --dry-run
verificar "malos.csv sale con 2" $((rc != 2))
verificar "malos.csv: 2 válidas, 10 rechazadas" "$(contiene "$tmp/out" 'Filas válidas: 2, rechazadas: 10')"
for n in 3 4 5 6 7 8 9 10 11 13; do
    verificar "malos.csv: reporta la línea $n" "$(contiene "$tmp/err" "malos.csv:$n:")"
done

# 3. Errores fatales: salida 1
correr no_existe.csv --dry-run
verificar "archivo inexistente sale con 1" $((rc != 1))

printf 'columna1,columna2\n' >"$tmp/encabezado_malo.csv"
correr "$tmp/encabezado_malo.csv" --dry-run
verificar "encabezado inválido sale con 1" $((rc != 1))

: >"$tmp/vacio.csv"
correr "$tmp/vacio.csv" --dry-run
verificar "archivo vacío sale con 1" $((rc != 1))

correr
verificar "sin argumentos sale con 1" $((rc != 1))

# 4. Muchos platillos distintos: ejercita el crecimiento de la tabla hash
{
    echo "fecha_hora,platillo,cantidad,precio"
    for i in $(seq 1 500); do echo "2026-10-01 12:00,Platillo $i,1,10.00"; done
} >"$tmp/muchos.csv"
correr "$tmp/muchos.csv" --dry-run
verificar "500 platillos distintos: 500 válidas" "$(contiene "$tmp/out" 'Filas válidas: 500, rechazadas: 0')"

echo
if [ "$fallos" -eq 0 ]; then
    echo "Todas las pruebas pasaron"
else
    echo "$fallos prueba(s) fallaron"
    exit 1
fi
