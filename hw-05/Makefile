OS_NAME := $(shell uname -s | tr A-Z a-z)

os:
	@echo $(OS_NAME)

all: unhash dispatcher generator

unhash: unhash.c 
	gcc -o UnHash unhash.c hashutil.c -w #-lcrypto

dispatcher: dispatcher.c
	gcc -o Dispatcher dispatcher.c hashutil.c -w #-lcrypto -pthread

generator: generator.c
	gcc -o Generator generator.c hashutil.c -w 

zip:
	zip archive.zip Makefile unhash.c hashutil.c hashutil.h

clean:
	rm -rf Dispatcher archive.zip *.dSYM UnHash Generator hashes.txt numbers.txt output.txt

