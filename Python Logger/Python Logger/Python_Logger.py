#!/usr/bin/env python
#coding: latin1
"""Log data from the BMP085 barometric pressure sensor.
"""

"""
class Notification():
	def __init__(self, token="nuc760hMtnSoOK04wijEc1GgmAWYf6", user="X26uaYJTHtrtsondtxZDGQ0pan29Ti", title="Untitled", message="No message"):
		self.token 	 = token
		self.user 	 = user
		self.title 	 = title
		self.message = message
		
	def __str__(self):
		return self.title + ": " + self.message
	
	def send(self):
		import httplib, urllib
		try:
			conn = httplib.HTTPSConnection("api.pushover.net:443")
			conn.request("POST", "/1/messages",
				urllib.urlencode({
					"token":	self.token,
					"user":		self.user,
					"title": 	self.title,
					"message":	self.message,
				}), { "Content-type": "application/x-www-form-urlencoded" })
			self.log()
		except:
			err = "error de conexion!\n" + self.title + ": " + self.message
			self.log(err)
			sys.exit()
	
	def log(self, err=None):
		from datetime import datetime as dt
		from os import path, makedirs				# libreria que se empleará para comprobar rutas
		now  = dt.now()
		date = now.date()
		time = now.time().replace(microsecond=0)   	# elimina los microsegundos para quedarse con un tiempo en formato HH:MM:SS
		
		if not path.isdir("C:\\python27\\log"): 	# comprueba la existencia del directorio
			makedirs("C:\\python27\\log")       	# si no, lo crea
		try: 
			f = open("C:\\python27\\log\\log_pushover_{}.txt".format(date), "a")	# abre el archivo en modo "append" para guardar el registro
		except: 
			f = open("C:\\python27\\log\\log_pushover_{}.txt".format(date), "w") 	# si no existe el archivo, lo crea
		
		f.write("From {} on {} at {}\n".format(path.basename(__file__), str(date), str(time)))
		if err:
			f.write("{}\n\n".format(str(err)))
			f.close()
		else:
			f.write("{}\n\n".format(str(self)))
			f.close()
"""
"""
def DataHandler(e):
    global start
    global outfile
 
    now = time() - start
 
    # Write the data to the text file
    outfile.write(str(now) + "," + str(e.temperature) + "," + str(ambientTemp) + "\n")
 
    # Print a dot to indicate one more point recorded
    sys.stdout.write(".")
    sys.stdout.flush()
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
        ser = serial.Serial('/dev/ttyUSB0',9600)
    elif _platform == 'win32':
        ser = serial.Serial('COM10',9600)

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
    import sys
    sys.exit(main())
