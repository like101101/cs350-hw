all: unhash dispatcher

unhash: unhash.c
	gcc -o UnHash unhash.c hashutil.c -w -lcrypto

dispatcher: dispatcher.c
	gcc -o Dispatcher dispatcher.c hashutil.c -w -lcrypto -pthread

zip:
	zip archive.zip Makefile unhash.c

clean:
	rm -rf Dispatcher archive.zip *.dSYM UnHash

