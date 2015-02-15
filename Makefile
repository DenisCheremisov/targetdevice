all: targetdevice.o confparser.o maplib.o
		gcc -o targetdevice targetdevice.o confparser.o -lyaml

targetdevice.o: targetdevice.c
		gcc -c targetdevice.c

confparser.o: confparser.c
		gcc -c confparser.c

maplib.o: maplib.c
		gcc -c maplib.c

clean:
		rm -f *.o targetdevice confparser_test


# This is for test purposes only

confparser_test: confparser.o maplib.o confparser_test.c
		gcc -o confparser_test confparser_test.c confparser.o maplib.o -lyaml
