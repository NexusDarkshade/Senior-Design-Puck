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

const int CS_PIN = 10;			// Chip select pin for SD card (D10)
unsigned long START_TIME;

File myFile;					// Data file for storing sensor measures
String sensData;				//Buffer to hold sensor data before writing to SD
/*----------------------------------------------------------------------------------------------------*/
// Definitions And Variables From Other Classes
#define LSM9DS1_ADDRESS			0x6b
#define LSM9DS1_CTRL_REG6_XL	0x20
#define LSM9DS1_ADDRESS_M		0x1e
#define LSM9DS1_CTRL_REG2_M		0x21

TwoWire* _wire = &Wire1;
/*----------------------------------------------------------------------------------------------------*/

void setup() {
	Serial.begin(9600);
	while (!Serial);
	Serial.println("Started");
	
	if (!IMU.begin()) {
		Serial.println("Failed to initialize IMU!");
		while (1);
	}
	writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, 0x68);		// overwrite IMU accelerometer setup to 16 g's (IMU will still report as if +/- 4 g range)
	writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG2_M, 0xE2);	// overwrite IMU magnetometer setup to 155 Hz (IMU will still report 20 Hz for sample rate)
	
	Serial.print("Accelerometer sample rate = ");
	Serial.print(IMU.accelerationSampleRate());
	Serial.println(" Hz");
	Serial.print("Gyroscope sample rate = ");
	Serial.print(IMU.gyroscopeSampleRate());
	Serial.println(" Hz");
	Serial.print("Magnetic field sample rate = ");
	Serial.print(IMU.magneticFieldSampleRate());
	Serial.println(" Hz");
	
	//Serial.print("Acceleration in G's\t");
	//Serial.print("Gyroscope in deg/sec\t");
	//Serial.println("Magnetic Field in uT");
	//Serial.println();
	
	initSD();
	clearSD();
	START_TIME = millis();
	
	writeSD();
	readSD();
}

void loop() {
	while(1);
}

// Functions
/*--------------------------------------------------------------------------------------------*/

void initSD(){
	SerialUSB.print("INITIALIZING SD CARD...");
	if(!SD.begin(CS_PIN)){
		SerialUSB.println("INITIALZATION FAILED!");
		while(1);
	}
	SerialUSB.println("SUCCESS");
}

void clearSD(){
	SerialUSB.println("Clearing Stored Data...");
	File root = SD.open("/");
	
	clearDIR(root);
}

void clearDIR(File dir) {
	while (true) {
		File entry =  dir.openNextFile();
		if (! entry) break;
		
		String fname = entry.name();
		if (entry.isDirectory()) {
			clearDIR(entry);
		}
		SD.remove(fname);
		SerialUSB.println(fname+" removed");
	}
}

void writeSD(){
	myFile = SD.open("DATA.txt",FILE_WRITE);
	if(myFile){
		byte buf[22];
		SerialUSB.println("Writing to DATA.txt...");
		for(int i = 0; i < 10; i ++) {
			sensorRead(buf);
			myFile.write(buf, 22);
		}
		myFile.close();
		SerialUSB.println("Done");
	} else {
		SerialUSB.print("Error opening DATA.txt");
	}
}


void readSD(){
	// open the file for reading:
	myFile = SD.open("DATA.txt");
	if (myFile) {
		Serial.println("DATA.txt:");
		// read from the file until there's nothing else in it:
		while (myFile.available()) {
			Serial.print(myFile.read());
			//Serial.print(" ");
		}
		myFile.close();
	} else {
		Serial.println("Error opening DATA.txt");
	}
}

void sensorRead(byte buff[22]){
	float a_x, a_y, a_z;	// Accelerometer
	float g_x, g_y, g_z;	// Gyroscope
	float m_x, m_y, m_z;	// Magnetometer

	// read sensor data to variables
	IMU.readAcceleration(a_x, a_y, a_z);
	IMU.readGyroscope(g_x, g_y, g_z);
	IMU.readMagneticField(m_x, m_y, m_z);
	
	// convert millis time to bytes
	unsigned long t = millis() - START_TIME;
	for(int i = 0; i < 4; i ++) {
		buff[3-i] = (byte) ((t>>(i*8))&&0xFF);
	}
	
	//convert float data to binary according to the SRS
	
	// accelerometer x
	int ax = (int) (a_x*4 * 2048);
	for(int i = 0; i < 2; i ++) {
		buff[5-i] = (byte) ((ax>>(i*8))&&0xFF);
	}
	// accelerometer y
	int ay = (int) (a_y*4 * 2048);
	for(int i = 0; i < 2; i ++) {
		buff[7-i] = (byte) ((ay>>(i*8))&&0xFF);
	}
	// accelerometer z
	int az = (int) (a_z*4 * 2048);
	for(int i = 0; i < 2; i ++) {
		buff[9-i] = (byte) ((az>>(i*8))&&0xFF);
	}
	
	// gyroscope x
	int gx = (int) (g_x * 16);
	for(int i = 0; i < 2; i ++) {
		buff[11-i] = (byte) ((gx>>(i*8))&&0xFF);
	}
	// gyroscope y
	int gy = (int) (g_y * 16);
	for(int i = 0; i < 2; i ++) {
		buff[13-i] = (byte) ((gy>>(i*8))&&0xFF);
	}
	// gyroscope z
	int gz = (int) (g_z * 16);
	for(int i = 0; i < 2; i ++) {
		buff[15-i] = (byte) ((gz>>(i*8))&&0xFF);
	}
	
	// magnetometer x
	int mx = (int) (m_x/100 * 8192);
	for(int i = 0; i < 2; i ++) {
		buff[17-i] = (byte) ((mx>>(i*8))&&0xFF);
	}
	// magnetometer y
	int my = (int) (m_y/100 * 8192);
	for(int i = 0; i < 2; i ++) {
		buff[19-i] = (byte) ((my>>(i*8))&&0xFF);
	}
	// magnetometer z
	int mz = (int) (m_z/100 * 8192);
	for(int i = 0; i < 2; i ++) {
		buff[21-i] = (byte) ((mz>>(i*8))&&0xFF);
	}
	
	sensData = String(a_x,3)+", "+String(a_y,3)+", "+String(a_z,3)+", "+String(g_x,3)+", "+String(g_y,3)+", "+String(g_z,3)+", "+String(m_x,3)+", "+String(m_y,3)+", "+String(m_z,3);
	//return sensData;
	
	Serial.print(a_x*4);
	Serial.print('\t');
	Serial.print(a_y*4);
	Serial.print('\t');
	Serial.print(a_z*4);
	Serial.print('\t');
	Serial.print(g_x);
	Serial.print('\t');
	Serial.print(g_y);
	Serial.print('\t');
	Serial.print(g_z);
	Serial.print('\t');
	Serial.print(m_x);
	Serial.print('\t');
	Serial.print(m_y);
	Serial.print('\t');
	Serial.println(m_z);
}

// Modified Functions From Other Classes
/*--------------------------------------------------------------------------------------------*/

// Hopefully doesn't cause any collisions w/ original IMU function
int writeRegister(uint8_t slaveAddress, uint8_t address, uint8_t value)
{
  _wire->beginTransmission(slaveAddress);
  _wire->write(address);
  _wire->write(value);
  if (_wire->endTransmission() != 0) {
    return 0;
  }

  return 1;
}
