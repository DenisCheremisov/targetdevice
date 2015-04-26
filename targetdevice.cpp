#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "targetdevice.hpp"


int set_interface_attribs (int fd, int speed, int parity) {
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if(tcgetattr (fd, &tty) != 0) {
        perror("get attribute");
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if(tcsetattr (fd, TCSANOW, &tty) != 0) {
        return -1;
    }
    return 0;
}


int set_blocking (int fd, int should_block) {
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0) {
        return -1;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if(tcsetattr (fd, TCSANOW, &tty) != 0) {
        return -1;
    }
    return 0;
}


int port_open(const char *portname) {
    int fd;
    if(portname == NULL) {
        portname = "/dev/usb/tts/0";
    }
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if(fd <= 0) {
        return -1;
    }
    if(set_interface_attribs(fd, B115200, 0) < 0) { // set speed to 115,200 bps, 8n1 (no parity)
        return 0;
    }
    if(set_blocking(fd, 1) < 0) {// set no blocking
        return 0;
    }
    return fd;
}


using namespace std;


TargetDeviceDriver::TargetDeviceDriver(string file_name) {
    this->fd = port_open(file_name.c_str());
    if(this->fd <= 0) {
        throw NoDeviceError(file_name);
    }
}


TargetDeviceDriver::~TargetDeviceDriver() throw() {
    close(this->fd);
}


void TargetDeviceDriver::port_talk(string request, string &response) {
    const size_t len = request.length();
    if(len < 5) {
        throw TargetDeviceValidationError(request + ": wrong command");
    }
    if(request.substr(0,3) != "$KE") {
        throw TargetDeviceValidationError(request + ":wrong command");
    }
    if(request.substr(len-2, 2) != "\r\n") {
        throw TargetDeviceValidationError(request + ": command must ends with \\r\\n");
    }

    ssize_t count;
    count = write(fd, request.c_str(), len);
    if(count < (int)len) {
        throw TargetDeviceInternalError(request);
    }
    char response_buf[256];
    count = read(fd, response_buf, 255);
    if(count < 3 || count > 255) {
        throw TargetDeviceInternalError(request);
    }
    response_buf[count] = '\0';
    response = string(response_buf);
    if(response.substr(0, 4) == "#ERR") {
        throw TargetDeviceOperationError(response);
    }
}

#include <cassert>

bool TargetDeviceDriver::connected() {
    stringstream buf;

    buf << "$KE\r\n";

    string command = buf.str();
    string response;
    this->port_talk(command, response);

    if(response.substr(0, 3) == "#OK") {
        return true;
    } else {
        throw TargetDeviceOperationError(response);
    }
}


int TargetDeviceDriver::line_get(int lineno) {
    stringstream buf;
    if(lineno < LINE_LOWER_BOUND || lineno > LINE_UPPER_BOUND) {
        throw TargetDeviceValidationError("Line number out of bounds [1,18]");
    }

    buf << "$KE,RD," << lineno << "\r\n";

    string command = buf.str();
    string response;
    this->port_talk(command, response);
    if(response.substr(0, 13) == "#RD,WRONGLINE") {
        throw TargetDeviceWronglineError(lineno, TARGETDEVICE_READ);
    }

    istringstream parse_buf(response);
    string token;
    if(!getline(parse_buf, token, ',') || token != "#RD") {
        throw TargetDeviceOperationError(response);
    }
    if(!getline(parse_buf, token, ',') || atol(token.c_str()) != lineno) {
        throw TargetDeviceOperationError(response);
    }
    if(getline(parse_buf, token, ',')) {
        return atol(token.c_str());
    }
    return 0;
}


string TargetDeviceDriver::line_get_all() {
    stringstream buf;

    buf << "$KE,RD,ALL\r\n";

    string command = buf.str();
    string response;
    this->port_talk(command, response);

    istringstream parse_buf(response);
    string token;
    if(!getline(parse_buf, token, ',') || token != "#RD") {
        throw TargetDeviceOperationError(response);
    }
    if(getline(parse_buf, token, ',') || token.length() == LINE_UPPER_BOUND) {
        return token;
    } else {
        throw TargetDeviceOperationError(response);
    }
}


void TargetDeviceDriver::line_set(int lineno, int value) {
    stringstream buf;
    if(lineno < LINE_LOWER_BOUND || lineno > LINE_UPPER_BOUND) {
        buf << "Line number out of bounds 1..18: " << lineno;
        throw TargetDeviceValidationError(buf.str());
    }
    if(value != 0 && value != 1) {
        buf << "Value out of bounds 0..1: " << value;
        throw TargetDeviceValidationError(buf.str());
    }

    buf << "$KE,WR," << lineno << "," << value << "\r\n";

    string command = buf.str();
    string response;
    this->port_talk(command, response);
    if(response.substr(0, 13) == "#WR,WRONGLINE") {
        throw TargetDeviceWronglineError(lineno, TARGETDEVICE_WRITE);
    }
    if(response.substr(0, 7) != "#WR,OK") {
        throw TargetDeviceOperationError(response);
    }
}


void TargetDeviceDriver::relay_set(int relayno, int value) {
    stringstream buf;
    if(relayno < RELAY_LOWER_BOUND || relayno > RELAY_UPPER_BOUND) {
        buf << "Relay number out of bounds 1..4: " << relayno;
        throw TargetDeviceValidationError(buf.str());
    }
    if(value != 0 && value != 1) {
        buf << "Value out of bounds 0..1: " << value;
        throw TargetDeviceValidationError(buf.str());
    }

    buf << "$KE,REL," << relayno << "," << value << "\r\n";

    string command = buf.str();
    string response;
    this->port_talk(command, response);
    if(response.substr(0, 7) != "#REL,OK") {
        throw TargetDeviceOperationError(response);
    }
}


int TargetDeviceDriver::io_get(int lineno) {
    stringstream buf;
    if(lineno < LINE_LOWER_BOUND || lineno > LINE_UPPER_BOUND) {
        throw TargetDeviceValidationError("Line number out of bounds 1..18");
    }

    buf << "$KE,IO,GET,MEM," << lineno << "\r\n";

    string command = buf.str();
    string response;
    this->port_talk(command, response);

    istringstream parse_buf(response);
    string token;
    if(!getline(parse_buf, token, ',') || token != "#IO") {
        throw TargetDeviceOperationError(response);
    }
    if(getline(parse_buf, token, ',')) {
        return atol(token.c_str());
    } else {
        throw TargetDeviceOperationError(response);
    }
}


string TargetDeviceDriver::io_get_all() {
    stringstream buf;

    buf << "$KE,IO,GET,MEM\r\n";

    string command = buf.str();
    string response;
    this->port_talk(command, response);

    istringstream parse_buf(response);
    string token;
    if(!getline(parse_buf, token, ',') || token != "#IO") {
        throw TargetDeviceOperationError(response);
    }
    if(getline(parse_buf, token, ',')) {
        return token;
    } else {
        throw TargetDeviceOperationError(response);
    }
}


void TargetDeviceDriver::io_set(int lineno, int value) {
    stringstream buf;
    if(lineno < LINE_LOWER_BOUND || lineno > LINE_UPPER_BOUND) {
        buf << "Line number out of bounds 1..18: " << lineno;
        throw TargetDeviceValidationError(buf.str());
    }
    if(value != 0 && value != 1) {
        throw TargetDeviceValidationError("Value out of bounds 0..1");
    }

    buf << "$KE,IO,SET," << lineno << "," << value << ",S\r\n";

    string command = buf.str();
    string response;
    this->port_talk(command, response);
    if(response.substr(0, 10) != "#IO,SET,OK") {
        throw TargetDeviceOperationError(response);
    }
}


int TargetDeviceDriver::adc_get(int channel) {
    stringstream buf;
    if(channel < ADC_LOWER_BOUND || channel > ADC_UPPER_BOUND) {
        buf << "Channel number out of bounds 1..4: " << channel;
        throw TargetDeviceValidationError(buf.str());
    }

    buf << "$KE,ADC," << channel << "\r\n";

    string command = buf.str();
    string response;
    this->port_talk(command, response);

    istringstream parse_buf(response);
    string token;
    if(!getline(parse_buf, token, ',') || token != "#ADC") {
        throw TargetDeviceOperationError(response);
    }
    if(!getline(parse_buf, token, ',') || atol(token.c_str()) != channel) {
        throw TargetDeviceOperationError(response);
    }
    if(!getline(parse_buf, token, ',')) {
        throw TargetDeviceOperationError(response);
    } else {
        return atol(token.c_str());
    }
}


void TargetDeviceDriver::afr_set(int frequency) {
    stringstream buf;
    if(frequency < FREQUENCY_LOWER_BOUND || frequency > FREQUENCY_UPPER_BOUND) {
        throw TargetDeviceValidationError("Frequency value out of bounds 0..400");
    }

    buf << "$KE,AFR," << frequency << "\r\n";
    string command = buf.str();
    string response;
    this->port_talk(command, response);
    if(response.substr(0, 7) != "#AFR,OK") {
        throw TargetDeviceOperationError(response);
    }
}
