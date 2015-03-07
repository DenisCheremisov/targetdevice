CPP=g++
LDFLAGS=
IFLAGS=
OPTS=-g -O0
COMPILE=$(CPP) $(LDFLAGS) $(IFLAGS) $(OPTS)
TESTFLAGS=-lboost_unit_test_framework

targetdevice.o: targetdevice.cpp targetdevice.hpp
	$(COMPILE) -c targetdevice.cpp

confparser.o: confparser.cpp confparser.hpp
	$(COMPILE) -c confparser.cpp

clean:
	rm *.o test_*

test_targetdevice: targetdevice.o test/test_targetdevice.cpp
	$(COMPILE) -o test_targetdevice targetdevice.o test/test_targetdevice.cpp $(TESTFLAGS)

test_confparser: confparser.o test/test_confparser.cpp targetdevice.o
	$(COMPILE) -o test_confparser confparser.o targetdevice.o test/test_confparser.cpp $(TESTFLAGS) -lyaml
