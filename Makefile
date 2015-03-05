CPP=g++
LDFLAGS=
IFLAGS=
OPTS=-g -O0
COMPILE=$(CPP) $(LDFLAGS) $(IFLAGS) $(OPTS)
TESTFLAGS=-lboost_unit_test_framework

targetdevice.o: targetdevice.cpp targetdevice.hpp
	$(COMPILE) -c targetdevice.cpp

clean:
	rm *.o test_*

test_targetdevice: targetdevice.o test/test_targetdevice.cpp
	$(COMPILE) -o test_targetdevice targetdevice.o test/test_targetdevice.cpp $(TESTFLAGS)
