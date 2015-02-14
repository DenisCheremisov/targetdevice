all: targetdevice.o
		gcc targetdevice.o -o targetdevice

targetdevice.o: targetdevice.c
		gcc -c targetdevice.c

clean:
		rm *.o targetdevice
