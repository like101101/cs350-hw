CC = gcc
OS_NAME := $(shell uname -s | tr A-Z a-z)
CFLAGS = -O0 -w -g

LDLIBS_LINUX = -lcrypto -pthread
LDLIBS_DARWIN = -pthread

ifeq ($(OS_NAME), linux)
	LDLIBS := $(LDLIBS_LINUX)
else
	LDLIBS := $(LDLIBS_DARWIN)
endif

.SILENT: clean

all: Pirate

Pirate: pirate.c hashutil.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

Generator: generator.c hashutil.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

zip: Makefile hashutil.c hashutil.h pirate.c
	zip archive.zip $^

test: test.c hashutil.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -rf Pirate archive.zip *.dSYM UnHash Generator hashes.txt numbers.txt output.txt

