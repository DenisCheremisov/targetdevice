#ifndef _TDEVICE_DRIVER_INCLUDED_HPP_
#define _TDEVICE_DRIVER_INCLUDED_HPP_


#include "../targetdevice.hpp"


const unsigned int
    LINE_LIMIT = 18,
    RELAY_LIMIT = 4,
    ADC_LIMIT = 4,
    DIR_OUT = 0,
    DIR_IN = 1;


const char SERIAL_DATA[] = "TEST_SERIAL_DATA";


class TestSerialCommunicator: public BaseSerialCommunicator {
private:
    int iodirs[LINE_LIMIT];
    int lines[LINE_LIMIT];
    int relays[RELAY_LIMIT];
    int adcs[RELAY_LIMIT];
    int freq;
    std::string user_data;
    std::string usb_data;

public:
    TestSerialCommunicator();
    ~TestSerialCommunicator() throw() {};

    std::string talk(std::string req);

    std::string is_connected();
    std::string write_line(int lineno, int value);
    std::string read_line(int lineno);
    std::string read_all_lines();
    std::string write_relay(int relno, int value);
    std::string read_relay(int relno);
    std::string set_freq(int freq);
    std::string get_adc(int channel);
    std::string ioset(int lineno, int direction);
    std::string ioget(int lineno);
    std::string ioget_all();
    std::string set_ud(std::string data);
    std::string get_ud();
    std::string set_usb(std::string data);
    std::string get_usb();
    std::string get_ser();
    std::string reset();

};


#endif
