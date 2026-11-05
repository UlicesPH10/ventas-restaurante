CREATE TABLE IF NOT EXISTS ventas (
    id              BIGSERIAL    PRIMARY KEY,
    fecha_hora      TIMESTAMP    NOT NULL,
    platillo        VARCHAR(64)  NOT NULL CHECK (platillo <> ''),
    cantidad        INTEGER      NOT NULL CHECK (cantidad > 0),
    precio_centavos BIGINT       NOT NULL CHECK (precio_centavos >= 0)
);

CREATE INDEX IF NOT EXISTS ventas_fecha_idx ON ventas (fecha_hora);
