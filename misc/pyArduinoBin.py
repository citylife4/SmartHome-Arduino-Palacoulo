import serial
from pip._internal import exceptions

ser = serial.Serial('COM3', 9600)
a=b'\x02'
b=b'_'
c=b';'
print(a+b+a+b+a+c)
r=a+b+a+b+a+c
while 1:
    try :
        a = ser.readline()
        if "Max" in a.decode():
            ser.write(r)
        print(a)
    except KeyboardInterrupt:
        print("exi")
        ser.close()

