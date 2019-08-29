import serial
from pip._internal import exceptions

ser = serial.Serial('COM3', 115200)

a= ser.read()
print(a)
ser.write("gfdsdf:vasasd".encode())


# while 1:
#    try :
# 
#        print(a)
#    except KeyboardInterrupt:
#        print("exi")
#        ser.close()

