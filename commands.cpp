#include "commands.hpp"


Result *SwitcherOn::execute() throw() {
    try {
        device->turn_on();
        return new IntResult(1);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}


Result *SwitcherOff::execute() throw() {
    try {
        device->turn_off();
        return new IntResult(1);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}


Result *TemperatureGet::execute() throw() {
    try {
        double res = device->get_temperature();
        return new FloatResult(res);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}


Result *BoilerOn::execute() throw() {
    try {
        device->turn_on();
        return new IntResult(1);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}


Result *BoilerOff::execute() throw() {
    try {
        device->turn_off();
        return new IntResult(1);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}


Result *BoilerTemperatureGet::execute() throw() {
    try {
        double res = device->get_temperature();
        return new FloatResult(res);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}
