OPTS = -g -O0
LPATH =
IPATH =
CC = gcc
COMPILE = $(CC) $(OPTS) $(LPATH) $(IPATH)


all: targetdevice.o confparser.o maplib.o binder.o network.o processors.o daemon.c getters.o
		$(COMPILE) -o targetdevice daemon.c targetdevice.o confparser.o maplib.o binder.o network.o processors.o getters.o -lyaml -lcrypto -lssl -lbsd

targetdevice.o: targetdevice.c
		$(CC) $(OPTS) $(LPATH) $(IPATH) -c targetdevice.c

confparser.o: confparser.c
		$(CC) $(OPTS) $(LPATH) $(IPATH) -c confparser.c

maplib.o: maplib.c
		$(CC) $(OPTS) $(LPATH) $(IPATH) -c maplib.c

binder.o: binder.c
		$(COMPILE) -c binder.c

processors.o: processors.c
		$(COMPILE) -c processors.c

network.o: network.c
		$(COMPILE) -c network.c

getters.o: getters.c
		$(COMPILE) -c getters.c

clean:
		rm -f *.o targetdevice test_confparser test_maplib test_binder test_processors test_network


# This is for test purposes only

test_confparser: confparser.o maplib.o test_confparser.c
		$(CC) $(OPTS) $(LPATH) $(IPATH) -o test_confparser test_confparser.c confparser.o maplib.o -lyaml

test_maplib: maplib.o test_maplib.c
		$(CC) $(OPTS) $(LPATH) $(IPATH) -o test_maplib test_maplib.c maplib.o

test_binder: maplib.o confparser.o binder.o test_binder.c
		$(COMPILE) -o test_binder maplib.o confparser.o binder.o test_binder.c -lyaml

test_processors: maplib.o processors.o test_processors.c
		$(COMPILE) -o test_processors maplib.o processors.o test_processors.c

test_network: network.o maplib.o confparser.o test_network.c
		$(COMPILE) -o test_network network.o maplib.o confparser.o test_network.c -lcrypto -lssl -lyaml
