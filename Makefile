#CC = clang

LUA_PKG_NAME ?= lua
#LUA_PKG_NAME = luajit

# Directory of libspnav
LIBSPNAV = ./libspnav-0.2.2/

CFLAGS = -std=gnu99 -pedantic -Wall -g -I$(LIBSPNAV) $(shell pkg-config --cflags $(LUA_PKG_NAME))
LDFLAGS = -lrt -L$(LIBSPNAV) -lspnav -lX11 $(shell pkg-config --libs lua)

.PHONY: all
all: spnavkbd

simple_x11: spnavkbd.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

.PHONY: clean
clean:
	rm -f spnavkbd
