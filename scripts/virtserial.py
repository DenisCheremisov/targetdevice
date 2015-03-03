import os
import pty
import serial
import inspect

# Hardware limits
LINE_LIMIT = 18
RELAY_LIMIT = 4
ADC_LIMIT = 4
FREQ_LIMIT = 400
DATA_LIMIT_32 = 32

# Constant values to not get bored of excessive quotes
KE = 'KE'
WR = 'WR'
RD = 'RD'
REL = 'REL'
ALL = 'ALL'
AFR = 'AFR'
ADC = 'ADC'
IO = 'IO'
GET = 'GET'
SET = 'SET'
UD = 'UD'
USB = 'USB'
SER = 'SER'
RST = 'RST'

# Hardware related conventions
DIR_OUT, DIR_IN = 0, 1
SERIAL_DATA = 'TEST_SERIAL_DATA'

# These strings are reserved part
reserved = {KE, WR, RD, REL, ALL, AFR, ADC, IO, GET, SET, UD, USB, SER, RST}

# Type check functions


def bit_type(value):
    value = int(value)
    if value not in (0, 1):
        raise ValueError()
    return value


def line_number(value):
    value = int(value)
    if not 1 <= value <= LINE_LIMIT:
        raise ValueError()
    return value


def relay_number(value):
    value = int(value)
    if not 1 <= value <= RELAY_LIMIT:
        raise ValueError()
    return value


def adc_number(value):
    value = int(value)
    if not 1 <= value <= ADC_LIMIT:
        raise ValueError()
    return value


def freq_value(value):
    value = int(value)
    if not 0 <= value <= FREQ_LIMIT:
        raise ValueError()
    return value


def storage_value(value):
    if value not in {'CUR', 'MEM'}:
        raise ValueError()
    return value


def str_data(value):
    if len(value) > DATA_LIMIT_32:
        raise ValueError()
    return value


class MetaSerial(type):
    def __new__(cls, name, bases, attrs):
        handling_methods = {}
        newattrs = {
            'req_handling': handling_methods
            }
        for attr in attrs:
            handling = getattr(attrs[attr], 'handling', None)
            if handling is not None:
                handling_methods[handling] = attrs[attr]
            newattrs[attr] = attrs[attr]
        return super(MetaSerial, cls).__new__(cls, name, bases, newattrs)


def handling(*keys):
    if not all([isinstance(i, str) for i in keys]):
        raise ValueError('Key sequence length can only be either 1 or 2')

    def proper_deco(f):
        f.handling = tuple(keys)
        return f

    return proper_deco


def handling_error(f):
    def new_f(*args, **kwargs):
        try:
            return f(*args, **kwargs)
        except ValueError:
            return '#ERR'
    return new_f


class Serial_FW1(metaclass=MetaSerial):
    def renew(self):
        self.iodirs = [DIR_OUT]*LINE_LIMIT
        self.lines = [0]*LINE_LIMIT
        self.lines[1] = 1
        self.relays = [0]*RELAY_LIMIT
        self.freq = 0
        self.adcs = [0]*ADC_LIMIT
        self.user_data = 'TEST_DATA'
        self.usb_data = 'TEST_USB_DATA'

    def __init__(self):
        self.renew()

    @handling(KE)
    def is_connected(self):
        return '#OK'

    @handling(KE, WR)
    def write_line(self, lineno: line_number, value: bit_type):
        if self.iodirs[lineno - 1] == DIR_OUT:
            return '#WR,WRONGLINE'
        elif self.iodirs[lineno - 1] == DIR_IN:
            self.lines[lineno - 1] = value
            return '#WR,OK'
        raise ValueError()

    @handling(KE, RD)
    def read_line(self, lineno: line_number):
        if self.iodirs[lineno - 1] == DIR_OUT:
            return '#RD,{lineno},{val}'.format(
                lineno=lineno, val=self.lines[lineno-1])
        elif self.iodirs[lineno - 1] == DIR_IN:
            return '#RD,WRONGLINE'
        raise ValueError()

    @handling(KE, RD, ALL)
    def read_all_lines(self):
        lines = list(map(str, self.lines))
        for i, item in enumerate(self.iodirs):
            if item == 1:
                lines[i] = 'x'
        return '#RD,' + ''.join(lines)

    @handling(KE, REL)
    def write_relay(self, relno: relay_number, value: bit_type):
        self.relays[relno - 1] = value
        return '#REL,OK'

    @handling(KE, AFR)
    def set_freq(self, freq: freq_value):
        self.freq = freq
        return '#AFR,OK'

    @handling(KE, ADC)
    def get_adc(self, channel: adc_number, *args):
        if len(args) > 1:
            # We actually ingore adc auto readout
            raise ValueError()
        value = self.adcs[channel - 1]
        self.adcs[channel - 1] = (value + 1) % 1024
        return '#ADC,{channel},{val:04}'.format(channel=channel, val=value)

    @handling(KE, IO, SET)
    def ioset(self, lineno: line_number, direction: bit_type, *args):
        if len(args) > 1:
            raise ValueError()
        self.iodirs[lineno - 1] = direction
        return '#IO,SET,OK'

    @handling(KE, IO, GET)
    def ioget(self, storage: storage_value, *args):
        if len(args) > 1:
            raise ValueError()
        if len(args) == 1:
            value = line_number(args[0])
            result = str(self.iodirs[value - 1])
        else:
            result = ''.join(map(str, self.iodirs))
        return '#IO,{result}'.format(
            storage=storage, result=result)

    @handling(KE, UD, SET)
    def set_ud(self, data: str_data):
        self.user_data = data
        return '#UD,SET,OK'

    @handling(KE, UD, GET)
    def get_ud(self):
        return '#UD,' + self.user_data

    @handling(KE, USB, SET)
    def set_usb(self, data: str_data):
        self.usb_data = data
        return '#USB,SET,OK'

    @handling(KE, USB, GET)
    def get_usb(self):
        return '#USB,' + self.usb_data

    @handling(KE, SER)
    def get_ser(self):
        return '#SER,' + TEST_SERIAL_DATA

    @handling(KE, RST)
    def reset(self):
        self.renew()
        return '#RST,OK'

    @handling_error
    def process(self, request):
        if not (request.startswith('$') and request.endswith('\r\n')):
            raise ValueError()
        request = request[1:-2].split(',')

        command = []
        args = []
        on_command = True
        for part in request:
            if part not in reserved:
                on_command = False
            if on_command:
                command.append(part)
            else:
                args.append(part)

        command = tuple(command)
        try:
            handler = self.req_handling[command]
        except KeyError:
            raise ValueError()

        argspec = inspect.getfullargspec(handler)
        arg_count = len(argspec.args) - 1
        if argspec.varargs:
            arg_count += 1
        if len(args) > arg_count:
            raise ValueError()
        for i, (arg, arg_name) in enumerate(zip(args, argspec.args[1:])):
            args[i] = argspec.annotations[arg_name](arg)

        return handler(self, *args)


def main():
    master, slave = pty.openpty()
    port_name = os.ttyname(slave)
    print('-'*80)
    print(port_name)

    ser = serial.Serial(port_name, baudrate=115200)
    virtser = Serial_FW1()

    while True:
        data = os.read(master, 1000)
        result = virtser.process(data.decode('ASCII'))
        os.write(master, (result + '\r\n').encode('ASCII'))


if __name__ == '__main__':
    main()
