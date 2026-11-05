\echo '== Ventas por día =='
SELECT fecha_hora::date                              AS dia,
       SUM(cantidad)                                 AS unidades,
       TO_CHAR(SUM(cantidad * precio_centavos) / 100.0, 'FM999,999,990.00') AS total
FROM ventas
GROUP BY dia
ORDER BY dia;

\echo
\echo '== Top 10 platillos más vendidos =='
SELECT platillo,
       SUM(cantidad)                                 AS unidades,
       TO_CHAR(SUM(cantidad * precio_centavos) / 100.0, 'FM999,999,990.00') AS total
FROM ventas
GROUP BY platillo
ORDER BY unidades DESC, platillo COLLATE "C"
LIMIT 10;

\echo
\echo '== Ventas por hora del día =='
SELECT TO_CHAR(EXTRACT(HOUR FROM fecha_hora), 'FM00') || ':00' AS hora,
       SUM(cantidad)                                 AS unidades,
       TO_CHAR(SUM(cantidad * precio_centavos) / 100.0, 'FM999,999,990.00') AS total
FROM ventas
GROUP BY EXTRACT(HOUR FROM fecha_hora)
ORDER BY EXTRACT(HOUR FROM fecha_hora);
