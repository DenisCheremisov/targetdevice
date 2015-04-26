CFLAGS?=-O2
CFLAGS+=
WFLAGS:=-Wall -Werror -pedantic
LDFLAGS?=-lyaml -lssl -lcrypto -lpthread -lbsd
BINARY:=tdevice

COMPILE=$(CC) $(CFLAGS) $(WFLAGS)

all: $(BINARY)

targetdevice.o: targetdevice.cpp targetdevice.hpp
	$(COMPILE) -c targetdevice.cpp

confparser.o: confparser.cpp confparser.hpp drivers.hpp devices.hpp
	$(COMPILE) -c confparser.cpp

runtime.o: runtime.cpp runtime.hpp
	$(COMPILE) -c runtime.cpp

confbind.o: confbind.cpp confbind.hpp
	$(COMPILE) -c confbind.cpp

commands.o: commands.cpp commands.hpp
	$(COMPILE) -c commands.cpp

background.o: background.cpp
	$(COMPILE) -c background.cpp

model.o: model.cpp model.hpp
	$(COMPILE) -c model.cpp

network.o: network.cpp network.hpp
	$(COMPILE) -c network.cpp

controller.o: controller.cpp controller.hpp
	$(COMPILE) -c controller.cpp

$(BINARY): main.cpp targetdevice.o confparser.o runtime.o confbind.o commands.o background.o model.o network.o controller.o
	$(CPP) $(CFLAGS) $(LDFLAGS) $(WFLAGS) -o $(BINARY) main.cpp targetdevice.o confparser.o runtime.o confbind.o commands.o background.o model.o network.o controller.o

clean:
	rm -f $(BINARY) *.o
