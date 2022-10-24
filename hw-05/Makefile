all: unhash dispatcher

unhash: unhash.c
	gcc -o UnHash unhash.c hashutil.c -w

dispatcher: dispatcher.c
	gcc -o Dispatcher -lpthread dispatcher.c hashutil.c -w

zip:
	zip archive.zip Makefile unhash.c

clean:
	rm -rf Dispatcher archive.zip *.dSYM UnHash

