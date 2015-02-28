import os
import sys
import serial

ser = serial.Serial(sys.argv[1], baudrate=115200, timeout=0.5)

ser.write(sys.argv[2] + '\r\n')
print ser.read(1000).strip()
