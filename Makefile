CFLAGS=-O2 -Wall -Wextra -lX11 -lpci
PREFIX=$(HOME)/.local
CACHE=$(shell if [ "$$XDG_CACHE_HOME" ]; then echo "$$XDG_CACHE_HOME"; else echo "$$HOME"/.cache; fi)

all: paleofetch

clean:
	rm -f paleofetch $(CACHE)/paleofetch

paleofetch: paleofetch.c paleofetch.h config.h
	$(CC) paleofetch.c -o paleofetch $(CFLAGS)
	strip paleofetch

install: paleofetch
	mkdir -p $(PREFIX)/bin
	install ./paleofetch $(PREFIX)/bin/paleofetch
