CFLAGS += -O2 -std=c99 -ggdb -Weverything -Wno-disabled-macro-expansion
CC = clang

SOURCES = src/yaib.c src/hooks.c ../yasl/src/yasl.c
HEADERS = src/yaib.h src/hooks.h ../yasl/src/yasl.h

all: yaib

yaib: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -Isrc -I../yasl/src -o $@ $(SOURCES)

clean:
	rm -f yaib

.PHONY: all
