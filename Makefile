all: targetdevice.o
		gcc -o targetdevice targetdevice.o confparser.o -lyaml

targetdevice.o: targetdevice.c
		gcc -c targetdevice.c

confparser.o: confparser.c
		gcc -c confparser.c

map_lib.o: map_lib.c
		gcc -c map_lib.c

clean:
		rm -f *.o targetdevice confparser_test


# This is for test purposes only

confparser_test: confparser.o map_lib.o confparser_test.c
		gcc -o confparser_test confparser_test.c confparser.o map_lib.o -lyaml
