import serial
import struct
import time
import pylab


tempx = [];
pressx = [];
ser = serial.Serial('/dev/ttyUSB0', 9600)

while(1):
    try:
        ser.write('1'); 
        temp = ser.read(8)
	press = temp[-4:]
	temp = temp[:4]
        temp = float(struct.unpack('>i', temp)[0])/10
        press = float(struct.unpack('>i', press)[0])/1000
	print str(temp)+" C"
	print str(press)+" kPa"
        print "----------"
	tempx.append(temp)
    except:
        print "Error"
        print "----------"
    time.sleep(1)
