CPP=g++
LDFLAGS=
IFLAGS=
OPTS=-g -O0 -Wall -Werror -pedantic
COMPILE=$(CPP) $(LDFLAGS) $(IFLAGS) $(OPTS)
TESTFLAGS=-lboost_unit_test_framework

all: main.cpp targetdevice.o confparser.o runtime.o confbind.o commands.o background.o model.o network.o controller.o
	$(COMPILE) -o tdevice main.cpp targetdevice.o confparser.o runtime.o confbind.o commands.o background.o model.o network.o controller.o $(TESTFLAGS) -lyaml -lssl -lcrypto -lpthread

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

yamlparser.o: yamlparser.cpp yamlparser.hpp
	$(COMPILE) -c yamlparser.cpp

clean:
	rm -f *.o test_*

prepare:
	cp *.hpp *.cpp openwrt/src/

test_targetdevice: targetdevice.o test/test_targetdevice.cpp
	$(COMPILE) -o test_targetdevice targetdevice.o test/test_targetdevice.cpp $(TESTFLAGS)

test_confparser: confparser.o test/test_confparser.cpp targetdevice.o
	$(COMPILE) -o test_confparser confparser.o targetdevice.o test/test_confparser.cpp $(TESTFLAGS) -lyaml

test_runtime: runtime.o test/test_runtime.cpp
	$(COMPILE) -o test_runtime runtime.o test/test_runtime.cpp $(TESTFLAGS)

test_confbind: confbind.o targetdevice.o confparser.o test/test_confbind.cpp
	$(COMPILE) -o test_confbind confbind.o targetdevice.o confparser.o test/test_confbind.cpp $(TESTFLAGS) -lyaml

test_commands: commands.o confbind.o targetdevice.o confparser.o test/test_commands.cpp
	$(COMPILE) -o test_commands confbind.o targetdevice.o confparser.o commands.o test/test_commands.cpp $(TESTFLAGS) -lyaml -lpthread

test_model: runtime.o confbind.o targetdevice.o confparser.o model.o commands.o test/test_model.cpp
	$(COMPILE) -o test_model runtime.o commands.o model.o confbind.o targetdevice.o confparser.o test/test_model.cpp $(TESTFLAGS) -lyaml

test_network: network.o test/test_network.cpp
	$(COMPILE) -o test_network network.o test/test_network.cpp $(TESTFLAGS) -lssl -lcrypto

test_controller: runtime.o confbind.o targetdevice.o confparser.o model.o commands.o controller.o network.o test/test_controller.cpp
	$(COMPILE) -o test_controller runtime.o confbind.o targetdevice.o confparser.o model.o commands.o controller.o network.o test/test_controller.cpp $(TESTFLAGS) -lyaml -lssl -lcrypto

test_yamlparser: test/test_yamlparser.cpp yamlparser.hpp
	$(COMPILE) -o test_yamlparser test/test_yamlparser.cpp $(TESTFLAGS) -lyaml
