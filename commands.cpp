#include "commands.hpp"
#include "locker.hpp"


Result *SwitcherOn::execute() throw() {
    try {
        Locker<DeviceSwitcher, TargetDeviceDriver> cover(device);
        cover->turn_on();
        return new IntResult(1);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceValidationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}


Result *SwitcherOff::execute() throw() {
    try {
        Locker<DeviceSwitcher, TargetDeviceDriver> cover(device);
        cover->turn_off();
        return new IntResult(1);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceValidationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}


Result *TemperatureGet::execute() throw() {
    try {
        Locker<DeviceTemperature, TargetDeviceDriver> cover(device);
        double res = cover->get_temperature();
        return new FloatResult(res);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceValidationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}


Result *BoilerOn::execute() throw() {
    try {
        Locker<DeviceBoiler, TargetDeviceDriver> cover(device);
        cover->turn_on();
        return new IntResult(1);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceValidationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}


Result *BoilerOff::execute() throw() {
    try {
        Locker<DeviceBoiler, TargetDeviceDriver> cover(device);
        cover->turn_off();
        return new IntResult(1);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceValidationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}


Result *BoilerTemperatureGet::execute() throw() {
    try {
        Locker<DeviceBoiler, TargetDeviceDriver> cover(device);
        double res = cover->get_temperature();
        return new FloatResult(res);
    } catch(TargetDeviceInternalError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceOperationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    } catch(TargetDeviceValidationError error) {
        return new ErrorResult(RESULT_SERIAL_ERROR,
                               error.what());
    }
}
