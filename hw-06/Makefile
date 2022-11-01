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

all: Dispatcher

Dispatcher: dispatcher.c hashutil.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

Generator: generator.c hashutil.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

zip: Makefile hashutil.c hashutil.h dispatcher.c
	zip archive.zip $^

clean:
	rm -rf Dispatcher archive.zip *.dSYM UnHash Generator hashes.txt numbers.txt output.txt

