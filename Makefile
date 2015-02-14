all: targetdevice.o
		gcc -o targetdevice targetdevice.o confparser.o

targetdevice.o: targetdevice.c
		gcc -c targetdevice.c

confparser.o: confparser.c
		gcc -lyaml -c confparser.c

clean:
		rm *.o targetdevice

confparser: confparser.o
		gcc -o confparser confparser.o -lyaml
