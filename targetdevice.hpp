#ifndef _TARGETDEVICE_HPP_INCLUDED_
#define _TARGETDEVICE_HPP_INCLUDED_

#include <string>
#include <sstream>
#include <iostream>

#include "constants.hpp"


class TargetDeviceError: public std::exception {
public:
    virtual const std::string message() const {
        return "";
    }
    virtual ~TargetDeviceError() throw() {};
    const char* what() const throw() {
        return this->message().c_str();
    }
};


class NoDeviceError: public TargetDeviceError {
private:
    std::string data;

public:
    NoDeviceError(std::string file_name) {
        this->data = file_name;
    }
    virtual ~NoDeviceError() throw() {};
    const std::string message() const {
        return std::string("Cannot open ") + this->data;
    }
};


class TargetDeviceInternalError: public TargetDeviceError {
private:
    std::string details;

public:
    TargetDeviceInternalError(std::string _details) {
        this->details = _details;
    };
    virtual ~TargetDeviceInternalError() throw() {};
    const std::string message() const {
        return std::string("Internal error on ") + details;
    }
};


class TargetDeviceOperationError: public TargetDeviceError {
private:
    std::string response;

public:
    TargetDeviceOperationError(std::string _response) {
        response = _response;
    };
    virtual ~TargetDeviceOperationError() throw() {};
    const std::string message() const {
        return std::string("Device returned error response: ") + response;
    }
};


typedef enum {
    TARGETDEVICE_WRITE,
    TARGETDEVICE_READ
} operation_code_t;


class TargetDeviceWronglineError: public TargetDeviceError {
private:
    std::string message_data;

public:
    TargetDeviceWronglineError(int lineno, operation_code_t op) {
        std::stringstream buf;

        buf << "Cannot to ";
        switch(op) {
        case TARGETDEVICE_WRITE:
            buf << "write into " << lineno;
        case TARGETDEVICE_READ:
            buf << "read from " << lineno;
        }
        message_data = buf.str();
    };
    virtual ~TargetDeviceWronglineError() throw() {};
    const std::string message() const {
        return message_data;
    }
};


class TargetDeviceValidationError: public TargetDeviceError {
private:
    std::string message_data;

public:
    TargetDeviceValidationError(std::string message) {
        message_data = message;
    }
    virtual ~TargetDeviceValidationError() throw() {};
    const std::string message() const {
        return message_data;
    }
};


class BaseSerialCommunicator {
public:
    virtual ~BaseSerialCommunicator() throw() {};
    virtual std::string talk(std::string req) = 0;
};


class SerialCommunicator: public BaseSerialCommunicator {
private:
    int fd;

public:
    SerialCommunicator(std::string path);
    ~SerialCommunicator() throw();
    std::string talk(std::string req);
};


class TargetDeviceDriver {
protected:
    SerialCommunicator *comm;
    void port_talk(std::string request, std::string &response);

public:
    TargetDeviceDriver(SerialCommunicator *comm);
    virtual ~TargetDeviceDriver() throw();

    bool connected();

    int line_get(int line);
    std::string line_get_all();
    void line_set(int line, int value);

    void relay_set(int relay, int value);

    int io_get(int line);
    std::string io_get_all();
    void io_set(int line, int direction);

    int adc_get(int channel);

    void afr_set(int frequency);
};

#endif
