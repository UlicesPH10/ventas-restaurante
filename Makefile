CC       ?= gcc
PG_CONFIG ?= pg_config
PGINC    := $(shell $(PG_CONFIG) --includedir)
PGLIB    := $(shell $(PG_CONFIG) --libdir)

CFLAGS   := -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -I$(PGINC)
LDFLAGS  := -L$(PGLIB) -Wl,-rpath,$(PGLIB)
LDLIBS   := -lpq

SRC := src/main.c src/csv.c src/hashtable.c src/db.c
OBJ := $(SRC:src/%.c=build/%.o)
BIN := build/ventas
ASAN_BIN := build/ventas_asan

.PHONY: all test asan clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

# Binario con AddressSanitizer + UBSan (en macOS reemplaza a valgrind)
asan: | build
	$(CC) -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 -fno-omit-frame-pointer \
	    -fsanitize=address,undefined -I$(PGINC) $(SRC) $(LDFLAGS) $(LDLIBS) -o $(ASAN_BIN)
	BIN=$(ASAN_BIN) tests/run_tests.sh

test: $(BIN)
	BIN=$(BIN) tests/run_tests.sh

clean:
	rm -rf build
