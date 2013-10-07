#!/usr/bin/env python
#coding: latin1
"""Log data from the BMP085 barometric pressure sensor.
"""

def main():
    import serial
    from datetime import datetime as dt
    from time import time, strftime
    from sys import platform as _platform

    global start
    global outfile

    # Open serial port
    if _platform == 'linux' or _platform == 'linux2':
        ser = serial.Serial('/dev/ttyUSB0',9600,timeout=1)
    elif _platform == 'win32':
        ser = serial.Serial('COM10',9600,timeout=1)

    # Get date and time
    now = dt.now()
    date = now.date()
    _time = strftime("%H%M%S")

    # Open and write a header to the output file
    outfile = open('altimeter_{}_{}.csv'.format(date,_time),'w')
    outfile.write("Time,Barometric Pressure,Altitude\n")

    # Obtain an initial time from which to generate timestamps
    start = time()

    # Loop
    while True:
        try:
            now = time() - start
            tmp = ser.readline().rstrip('\r\n').replace('\0','')
            print tmp
            outfile.write(str(now) + ',' + tmp + '\n')
        except (KeyboardInterrupt, SystemExit):
            ser.close()
            outfile.close()
            print "Exiting\n"
            break

if __name__ == "__main__":
    from sys import exit
    exit(main())