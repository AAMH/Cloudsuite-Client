UNAME_S := $(shell uname -s)

CC ?= gcc
CFLAGS ?= -O3 -Wall -D_GNU_SOURCE
LDLIBS ?= -levent -pthread -lm

ifeq ($(UNAME_S),Darwin)
HOMEBREW_PREFIX := $(shell brew --prefix libevent 2>/dev/null)
ifneq ($(HOMEBREW_PREFIX),)
CFLAGS += -I$(HOMEBREW_PREFIX)/include
LDLIBS += -L$(HOMEBREW_PREFIX)/lib
endif
endif

all: *.c *.h
	$(CC) $(CFLAGS) *.c $(LDLIBS) -o loader

clean:
	rm -f loader
