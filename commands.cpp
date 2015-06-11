#include "commands.hpp"
#include "locker.hpp"


SwitcherOn::SwitcherOn(device_reference_t *ref) {
    device = dynamic_cast<DeviceSwitcher*>(ref->basepointer);
    if(device == NULL) {
        throw CommandSetupError(
            "Attempt to set on an instance that is not a switcher");
    }
}


SwitcherOn::~SwitcherOn() throw() {
    try {
        Locker<DeviceSwitcher, TargetDeviceDriver> cover(device);
        cover->turn_off();
    } catch(...) {
    }
}


SwitcherOff::SwitcherOff(device_reference_t *ref) {
    device = dynamic_cast<DeviceSwitcher*>(ref->basepointer);
    if(device == NULL) {
        throw CommandSetupError(
            "Attempt to set off an instance that is not a switcher");
    }
}


TemperatureGet::TemperatureGet(device_reference_t *ref) {
    device = dynamic_cast<DeviceTemperature*>(ref->basepointer);
    if(device == NULL) {
        throw CommandSetupError(
            "Attempt to get a temperature with an "
            "instance that is not a switcher");
    }
}


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
