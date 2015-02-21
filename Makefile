OPTS = -g
LPATH =
IPATH =
CC = gcc
COMPILE = $(CC) $(OPTS) $(LPATH) $(IPATH)

all: targetdevice.o confparser.o maplib.o
		$(CC) $(OPTS) $(LPATH) $(IPATH) -o targetdevice targetdevice.o confparser.o -lyaml

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

clean:
		rm -f *.o targetdevice confparser_test maplib_test binder_test


# This is for test purposes only

confparser_test: confparser.o maplib.o confparser_test.c
		$(CC) $(OPTS) $(LPATH) $(IPATH) -o confparser_test confparser_test.c confparser.o maplib.o -lyaml


maplib_test: maplib.o maplib_test.c
		$(CC) $(OPTS) $(LPATH) $(IPATH) -o maplib_test maplib_test.c maplib.o


binder_test: maplib.o confparser.o binder.o binder_test.c
		$(COMPILE) -o binder_test maplib.o confparser.o binder.o binder_test.c -lyaml

processors_test: maplib.o processors.o processors_test.c
		$(COMPILE) -o processors_test maplib.o processors.o processors_test.c
