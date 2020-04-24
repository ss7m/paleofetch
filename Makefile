CFLAGS=-O3 -Wall -Wextra -lX11 -lpci
PREFIX=$(HOME)/.local

all: paleofetch

clean:
	rm -f paleofetch

paleofetch: paleofetch.c paleofetch.h config.h
	$(CC) paleofetch.c -o paleofetch $(CFLAGS)
	strip paleofetch

install: paleofetch
	mkdir -p $(PREFIX)/bin
	install ./paleofetch $(PREFIX)/bin/paleofetch
