/*
	* Senior Design Project - Educational Puck
	* 
	* System Test
	* 
	* Justin Rubalcaba, Undergraduate Student
	* Electrical Engineering Department, Montana Technological University
	* jrubalcaba@mtech.edu
	* October 20, 2021
	* 
	* Seth Bettwieser, Undergraduate Student
	* Computer Science Department, Montana Technological University
	* sbettwieser@mtech.edu
	* March 12, 2022
*/

// Include Required Libraries
#include <Arduino_LSM9DS1.h>	// IMU sensor
#include <SPI.h>				// SPI communication
#include <SD.h>					// SD card control
#include <cstring>
#include <stdlib.h>

const int CS_PIN = 10;			// Chip select pin for SD card (D10)
const int S_PIN = 3;			// Serial pin (A3)
unsigned long START_TIME;		// Starting time for the session
bool MODE;						// 0 - SERIAL, 1 - SENSOR
char serInp[64];				// Serial input buffer
int SERMODE = 0;				// 0 - WAITING, 1 - SEND FILE, 2 - DELETE FILE

File myFile;					// Data file for storing sensor measures
float ca_x, ca_y, ca_z;			// Calibration offset for accelerometer
float cg_x, cg_y, cg_z;			// Calibration offset for gyroscope
/*----------------------------------------------------------------------------------------------------*/

// Definitions And Variables From Other Classes
#define LSM9DS1_ADDRESS			0x6b
#define LSM9DS1_CTRL_REG6_XL	0x20
#define LSM9DS1_ADDRESS_M		0x1e
#define LSM9DS1_CTRL_REG1_M		0x20

TwoWire* _wire = &Wire1;
/*----------------------------------------------------------------------------------------------------*/

// initializer function
void setup() {
	pinMode(LED_BUILTIN, OUTPUT);
	
	// start IMU
	if(!IMU.begin()) {
		while(1);
	}
	
	writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, 0x68);		// overwrite IMU accelerometer setup to 16 g's (IMU will still report as if +/- 4 g range)
	writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG1_M, 0xE2);	// overwrite IMU magnetometer setup to 155 Hz (IMU will still report 20 Hz for sample rate)
	
	// start SD
	initSD();
	
	// initialize calibration
	bool c = readCalibration();
	if(!c) {
		ca_x = 0.0;
		ca_y = 0.0;
		ca_z = 0.0;
		
		cg_x = 0.0;
		cg_y = 0.0;
		cg_z = 0.0;
	}
	
	// initial mode setup
	if(analogRead(S_PIN) < 500) {
		digitalWrite(LED_BUILTIN, LOW);
		Serial.begin(9600);
		MODE = false;
	} else {
		digitalWrite(LED_BUILTIN, HIGH);
		myFile = newSession();
		START_TIME = millis();
		MODE = true;
	}
}

// main loop
void loop() {
	while(1) {
		// check physical switch
		if(analogRead(S_PIN) < 500) {
			// detect mode switch
			if(MODE) {
				if(myFile) {
					myFile.close();
				}
				
				digitalWrite(LED_BUILTIN, LOW);
				
				Serial.begin(9600);
			}
			parseInput();
			
			MODE = false;
			delay(100);
			
		} else {
			// detect mode switch
			if(!MODE) {
				Serial.end();
				
				digitalWrite(LED_BUILTIN, HIGH);
				
				myFile = newSession();
				START_TIME = millis();
			}
			writeSD(myFile);
			
			MODE = true;
		}
	}
	
}

// Functions
/*--------------------------------------------------------------------------------------------*/

// initialize the SD card
void initSD(){
	if(!SD.begin(32000000, CS_PIN)){	// Sets clock speed at 32MHz (max speed for the nano's built-in SPI)
		// SerialUSB.println("INITIALZATION FAILED!");
		while(1);
	}
}

// delete all files from the SD
void clearSD(){
	File root = SD.open("/");
	
	clearDIR(root);
	root.close();
}

// delete all files in a directory
void clearDIR(File dir) {
	while(true) {
		File entry =  dir.openNextFile();
		if(!entry) break;
		
		char* fname = entry.name();
		if(entry.isDirectory()) {
			clearDIR(entry);
		}
		entry.close();
		SD.remove(fname);
	}
}

// create and open a new data file
File newSession() {
	File root = SD.open("/");
	
	char fname[13];
	int fint = -1;
	while(true) {
		File entry = root.openNextFile();
		if(!entry) break;
		
		if(!entry.isDirectory()) {
			char* ename = entry.name();
			if(std::strcmp(ename, "calibration.txt") == 0) continue;
			
			int eint = std::atoi(std::strtok(ename, "."));
			
			if(eint > fint) {
				fint = eint;
			}
		}
		entry.close();
	}
	root.close();
	
	fint ++;
	sprintf(fname, "%08d.puk", fint);
	
	return SD.open(fname, FILE_WRITE);
}

// write sensor data to file
void writeSD(File f){
	if(myFile){
		byte buf[22];
		while(!IMU.accelerationAvailable() && !IMU.gyroscopeAvailable());
		
		sensorRead(buf);
		myFile.write(buf, 22);
	} else {
		// SerialUSB.print("Error opening file");
	}
}

// print all files on SD to serial
void readAll() {
	File root = SD.open("/");
	
	while(true) {
		File entry = root.openNextFile();
		if(!entry) break;
		
		if(!entry.isDirectory()) {
			char* fname = entry.name();
			readSD(fname);
			Serial.println();
		}
		entry.close();
	}
	root.close();
}

// Prints the name and Hex values of the file to serial
void readSD(const char* fname){
	// open the file for reading:
	myFile = SD.open(fname);
	if(myFile) {
		Serial.print(fname);
		Serial.println(":");
		
		// read from the file until there's nothing else in it:
		char sbuf[4];
		while(myFile.available()) {
			sprintf(sbuf, "%02X", myFile.read());
			Serial.print(sbuf);
		}
		myFile.close();
	} else {
		Serial.print("Error opening ");
		Serial.println(fname);
	}
}

// Prints the size and binary of the file to serial
void sendSD(const char* fname) {
	myFile = SD.open(fname);
	if(myFile) {
		unsigned long sz_l = myFile.size();
		byte sz[4] = {
			(byte)((sz_l>>24)&0xFF),
			(byte)((sz_l>>16)&0xFF),
			(byte)((sz_l>>8)&0xFF),
			(byte)((sz_l)&0xFF)
		};
		Serial.write(sz, 4);
		// read from the file until there's nothing else in it:
		byte sbuf[4];
		while(myFile.available()) {
			int rd = myFile.read(sbuf, 4);
			if(rd > 0) Serial.write(sbuf, rd);
		}
		myFile.close();
	} else {
		byte sz[4] = {0,0,0,0};
		Serial.write(sz, 4);
	}
}

// Calibrates the sensor offsets
void calibrate() {
	static const int SAMPLE_SIZE = 1000;
	if(SD.exists("calibration.txt")) SD.remove("calibration.txt");
	
	// totals
	float a_tx = 0.0, a_ty = 0.0, a_tz = 0.0;	// Accelerometer
	float g_tx = 0.0, g_ty = 0.0, g_tz = 0.0;	// Gyroscope
	
	for(int i = 0; i < SAMPLE_SIZE; i ++) {
		while(!IMU.accelerationAvailable() && !IMU.gyroscopeAvailable());
		
		// instances
		float a_x, a_y, a_z;	// Accelerometer
		float g_x, g_y, g_z;	// Gyroscope
		
		// read sensor data to variables
		IMU.readAcceleration(a_x, a_y, a_z);
		IMU.readGyroscope(g_x, g_y, g_z);
		
		// adjust for different reported accelerometer range
		a_x *= 4.0;
		a_y *= 4.0;
		a_z *= 4.0;
		
		// account for gravity
		a_z -= 1.0;
		
		// add instanced data to totals
		a_tx += a_x;
		a_ty += a_y;
		a_tz += a_z;
		
		g_tx += g_x;
		g_ty += g_y;
		g_tz += g_z;
	}
	
	// convert totals to averages
	a_tx /= SAMPLE_SIZE;
	a_ty /= SAMPLE_SIZE;
	a_tz /= SAMPLE_SIZE;
	
	g_tx /= SAMPLE_SIZE;
	g_ty /= SAMPLE_SIZE;
	g_tz /= SAMPLE_SIZE;
	
	File cal = SD.open("calibration.txt", FILE_WRITE);
	if(cal){
		// save offsets in file
		String sa_x = String(-a_tx, 6);
		String sa_y = String(-a_ty, 6);
		String sa_z = String(-a_tz, 6);
		
		String sg_x = String(-g_tx, 6);
		String sg_y = String(-g_ty, 6);
		String sg_z = String(-g_tz, 6);
		
		cal.print(sa_x);
		cal.print(",");
		cal.print(sa_y);
		cal.print(",");
		cal.print(sa_z);
		cal.print(",");
		
		cal.print(sg_x);
		cal.print(",");
		cal.print(sg_y);
		cal.print(",");
		cal.print(sg_z);
		cal.print(";");
		
		cal.close();
	} else {
		// SerialUSB.print("Error opening calibration.txt");
	}
	
	// store in current calibration offsets
	ca_x = -a_tx;
	ca_y = -a_ty;
	ca_z = -a_tz;
	
	cg_x = -g_tx;
	cg_y = -g_ty;
	cg_z = -g_tz;
}

// read the calibration file
bool readCalibration() {
	unsigned char idx = 0;	// Index in string
	char rc;				// Read character
	char calStr[16];		// Calibration string
	
	if(!SD.exists("calibration.txt")) return false;
	
	File cal = SD.open("calibration.txt");
	if(cal){
		// acceleration - x
		while(cal.available()) {
			rc = cal.read();
			
			if(rc != ',') {
				calStr[idx] = rc;
				idx ++;
				// detect overflow
				if (idx >= 16) {
					cal.close();
					return false;
				}
			} else {
				calStr[idx] = '\0'; // Terminate the string
				idx = 0;
				break;
			}
		}
		ca_x = std::atof(calStr);
		
		// acceleration - y
		while(cal.available()) {
			rc = cal.read();
			
			if(rc != ',') {
				calStr[idx] = rc;
				idx ++;
				// detect overflow
				if (idx >= 16) {
					cal.close();
					return false;
				}
			} else {
				calStr[idx] = '\0'; // Terminate the string
				idx = 0;
				break;
			}
		}
		ca_y = std::atof(calStr);
		
		// acceleration - z
		while(cal.available()) {
			rc = cal.read();
			
			if(rc != ',') {
				calStr[idx] = rc;
				idx ++;
				// detect overflow
				if (idx >= 16) {
					cal.close();
					return false;
				}
			} else {
				calStr[idx] = '\0'; // Terminate the string
				idx = 0;
				break;
			}
		}
		ca_z = std::atof(calStr);
		
		// rotation rate - x
		while(cal.available()) {
			rc = cal.read();
			
			if(rc != ',') {
				calStr[idx] = rc;
				idx ++;
				// detect overflow
				if (idx >= 16) {
					cal.close();
					return false;
				}
			} else {
				calStr[idx] = '\0'; // Terminate the string
				idx = 0;
				break;
			}
		}
		cg_x = std::atof(calStr);
		
		// rotation rate - y
		while(cal.available()) {
			rc = cal.read();
			
			if(rc != ',') {
				calStr[idx] = rc;
				idx ++;
				// detect overflow
				if (idx >= 16) {
					cal.close();
					return false;
				}
			} else {
				calStr[idx] = '\0'; // Terminate the string
				idx = 0;
				break;
			}
		}
		cg_y = std::atof(calStr);
		
		// rotation rate - z
		while(cal.available()) {
			rc = cal.read();
			
			if(rc != ';') {
				calStr[idx] = rc;
				idx ++;
				// detect overflow
				if (idx >= 16) {
					cal.close();
					return false;
				}
			} else {
				calStr[idx] = '\0'; // Terminate the string
				break;
			}
		}
		cg_z = std::atof(calStr);
		
		cal.close();
		return true;
	} else {
		return false;
	}
}

// Parse serial input
void parseInput() {
	static unsigned char idx = 0;	// Index in string
	static bool eol = false;		// End of line flag
	static bool ovf = false;		// Overflow flag
	char rc;						// Read character
	
	while(!ovf && Serial.available()) {
		rc = Serial.read();
		
		// String terminator = ;
		if(rc == '\r' || rc == '\n') {
			continue;
		} else if(rc != ';') {
			serInp[idx] = rc;
			idx ++;
			if (idx >= 64) {
				idx = 0;
				ovf = true;
			}
		} else {
			serInp[idx] = '\0'; // Terminate the string
			idx = 0;
			eol = true;
			break;
		}
	}
	
	// End of line reached
	if(eol) {
		// Base serial command
		if(SERMODE == 0) {
			// Ping the arduino
			if(std::strcmp(serInp, "PING") == 0) {
				Serial.write(1);
				
			// calibrate the sensors
			} else if(std::strcmp(serInp, "CALIBRATE") == 0) {
				calibrate();
				
			// send a file
			} else if(std::strcmp(serInp, "SEND") == 0) {
				SERMODE = 1;
				
			// delete a file
			} else if(std::strcmp(serInp, "DELETE") == 0) {
				SERMODE = 2;
				
			// delete all files on SD
			} else if(std::strcmp(serInp, "CLEAR") == 0) {
				clearSD();
				
			// list files on SD
			} else if(std::strcmp(serInp, "LIST") == 0) {
				File root = SD.open("/");
				String flist = "";
				while(true) {
					File entry = root.openNextFile();
					if(!entry) break;
					
					if(!entry.isDirectory()) {
						String ename = entry.name();
						if(ename.equals("calibration.txt")) continue;
						
						if(flist == "") {
							flist = ename;
						} else {
							flist = "," + ename;
						}
						Serial.print(flist);	
					}
					entry.close();
				}
				Serial.print(";");
				root.close();
			} else {
				// Do nothing on bad input
			}
		// SEND; serial command
		} else if(SERMODE == 1) {
			sendSD(serInp);
			SERMODE = 0;
		// DELETE; serial command
		} else if(SERMODE == 2) {
			if(SD.exists(serInp)) SD.remove(serInp);
			SERMODE = 0;
		}
		
		eol = false;
	}
}

// Read from the sensors to a buffer
void sensorRead(byte buff[22]){
	static float a_x, a_y, a_z;	// Accelerometer
	static float g_x, g_y, g_z;	// Gyroscope
	static float m_x, m_y, m_z;	// Magnetometer

	// read sensor data to variables
	IMU.readAcceleration(a_x, a_y, a_z);
	IMU.readGyroscope(g_x, g_y, g_z);
	IMU.readMagneticField(m_x, m_y, m_z);
	
	// convert millis time to bytes
	unsigned long t = millis() - START_TIME;
	for(int i = 0; i < 4; i ++) {
		buff[3-i] = (byte) ((t>>(i*8))&0xFF);
	}
	//convert float data to binary according to the SRS
	
	// accelerometer x
	int ax = (int) ((a_x*4 + ca_x) * 2048);
	for(int i = 0; i < 2; i ++) {
		buff[5-i] = (byte) ((ax>>(i*8))&0xFF);
	}
	// accelerometer y
	int ay = (int) ((a_y*4 + ca_y) * 2048);
	for(int i = 0; i < 2; i ++) {
		buff[7-i] = (byte) ((ay>>(i*8))&0xFF);
	}
	// accelerometer z
	int az = (int) ((a_z*4 + ca_z) * 2048);
	for(int i = 0; i < 2; i ++) {
		buff[9-i] = (byte) ((az>>(i*8))&0xFF);
	}
	
	// gyroscope x
	int gx = (int) ((g_x + cg_x) * 16);
	for(int i = 0; i < 2; i ++) {
		buff[11-i] = (byte) ((gx>>(i*8))&0xFF);
	}
	// gyroscope y
	int gy = (int) ((g_y + cg_y) * 16);
	for(int i = 0; i < 2; i ++) {
		buff[13-i] = (byte) ((gy>>(i*8))&0xFF);
	}
	// gyroscope z
	int gz = (int) ((g_z + cg_z) * 16);
	for(int i = 0; i < 2; i ++) {
		buff[15-i] = (byte) ((gz>>(i*8))&0xFF);
	}
	
	// magnetometer x
	int mx = (int) (m_x/100 * 8192);
	for(int i = 0; i < 2; i ++) {
		buff[17-i] = (byte) ((mx>>(i*8))&0xFF);
	}
	// magnetometer y
	int my = (int) (m_y/100 * 8192);
	for(int i = 0; i < 2; i ++) {
		buff[19-i] = (byte) ((my>>(i*8))&0xFF);
	}
	// magnetometer z
	int mz = (int) (m_z/100 * 8192);
	for(int i = 0; i < 2; i ++) {
		buff[21-i] = (byte) ((mz>>(i*8))&0xFF);
	}
}

// Modified Functions From Other Classes
/*--------------------------------------------------------------------------------------------*/

// Hopefully doesn't cause any collisions w/ original IMU function
int writeRegister(uint8_t slaveAddress, uint8_t address, uint8_t value) {
  _wire->beginTransmission(slaveAddress);
  _wire->write(address);
  _wire->write(value);
  if(_wire->endTransmission() != 0) {
    return 0;
  }

  return 1;
}
