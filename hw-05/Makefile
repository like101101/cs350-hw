CC = gcc
OS_NAME := $(shell uname -s | tr A-Z a-z)
CFLAGS = -O0 -g -w

LDLIBS_LINUX = -lcrypto -pthread
LDLIBS_DARWIN = -pthread

ifeq ($(OS_NAME), linux)
	LDLIBS := $(LDLIBS_LINUX)
else
	LDLIBS := $(LDLIBS_DARWIN)
endif

.SILENT: clean

all: UnHash Dispatcher Generator

UnHash: unhash.c hashutil.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

Dispatcher: dispatcher.c hashutil.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

Generator: generator.c hashutil.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

zip: Makefile unhash.c hashutil.c hashutil.h
	zip archive.zip $^

clean:
	rm -rf Dispatcher archive.zip *.dSYM UnHash Generator hashes.txt numbers.txt output.txt

