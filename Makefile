CFLAGS=-O3 -Wall -Wextra
PREFIX=$(HOME)/.local

all: fetch

clean:
	rm -f fetch

fetch: fetch.c
	$(CC) fetch.c -o fetch $(CFLAGS)
	strip fetch

install: fetch
	mkdir -p $(PREFIX)/bin
	install ./fetch $(PREFIX)/bin/fetch
