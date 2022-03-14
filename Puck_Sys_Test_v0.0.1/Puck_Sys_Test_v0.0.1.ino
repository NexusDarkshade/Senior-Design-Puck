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
/*----------------------------------------------------------------------------------------------------*/
// Definitions And Variables From Other Classes
#define LSM9DS1_ADDRESS			0x6b
#define LSM9DS1_CTRL_REG6_XL	0x20
#define LSM9DS1_ADDRESS_M		0x1e
#define LSM9DS1_CTRL_REG1_M		0x20

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
	writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG1_M, 0xE2);	// overwrite IMU magnetometer setup to 155 Hz (IMU will still report 20 Hz for sample rate)
	
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
		for(int i = 0; i < 1000; i ++) {
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
		char sbuf[4];
		while (myFile.available()) {
			sprintf(sbuf, "%02X", myFile.read());
			Serial.print(sbuf);
			//Serial.print(" ");
		}
		myFile.close();
	} else {
		Serial.println("Error opening DATA.txt");
	}
}

void sensorRead(byte buff[22]){
	static float a_x, a_y, a_z;	// Accelerometer
	static float g_x, g_y, g_z;	// Gyroscope
	static float m_x, m_y, m_z;	// Magnetometer

	// read sensor data to variables
	IMU.readAcceleration(a_x, a_y, a_z);
	IMU.readGyroscope(g_x, g_y, g_z);
	IMU.readMagneticField(m_x, m_y, m_z);
	
	//Serial.print("Converted: ");
	//char sbuf[16];
	
	// convert millis time to bytes
	unsigned long t = millis() - START_TIME;
	for(int i = 0; i < 4; i ++) {
		buff[3-i] = (byte) ((t>>(i*8))&0xFF);
	}
	//sprintf(sbuf, "%04X%04X; ", (unsigned int) (t>>16)&0xFFFF, (unsigned int)t&0xFFFF);
	//Serial.print(sbuf);
	
	//convert float data to binary according to the SRS
	
	// accelerometer x
	int ax = (int) (a_x*4 * 2048);
	for(int i = 0; i < 2; i ++) {
		buff[5-i] = (byte) ((ax>>(i*8))&0xFF);
	}
	//sprintf(sbuf, "%04X, ", ax&0xFFFF);
	//Serial.print(sbuf);
	
	// accelerometer y
	int ay = (int) (a_y*4 * 2048);
	for(int i = 0; i < 2; i ++) {
		buff[7-i] = (byte) ((ay>>(i*8))&0xFF);
	}
	//sprintf(sbuf, "%04X, ", ay&0xFFFF);
	//Serial.print(sbuf);
	
	// accelerometer z
	int az = (int) (a_z*4 * 2048);
	for(int i = 0; i < 2; i ++) {
		buff[9-i] = (byte) ((az>>(i*8))&0xFF);
	}
	//sprintf(sbuf, "%04X, ", az&0xFFFF);
	//Serial.print(sbuf);
	
	// gyroscope x
	int gx = (int) (g_x * 16);
	for(int i = 0; i < 2; i ++) {
		buff[11-i] = (byte) ((gx>>(i*8))&0xFF);
	}
	//sprintf(sbuf, "%04X, ", gx&0xFFFF);
	//Serial.print(sbuf);
	
	// gyroscope y
	int gy = (int) (g_y * 16);
	for(int i = 0; i < 2; i ++) {
		buff[13-i] = (byte) ((gy>>(i*8))&0xFF);
	}
	//sprintf(sbuf, "%04X, ", gy&0xFFFF);
	//Serial.print(sbuf);
	
	// gyroscope z
	int gz = (int) (g_z * 16);
	for(int i = 0; i < 2; i ++) {
		buff[15-i] = (byte) ((gz>>(i*8))&0xFF);
	}
	//sprintf(sbuf, "%04X, ", gz&0xFFFF);
	//Serial.print(sbuf);
	
	// magnetometer x
	int mx = (int) (m_x/100 * 8192);
	for(int i = 0; i < 2; i ++) {
		buff[17-i] = (byte) ((mx>>(i*8))&0xFF);
	}
	//sprintf(sbuf, "%04X, ", mx&0xFFFF);
	//Serial.print(sbuf);
	
	// magnetometer y
	int my = (int) (m_y/100 * 8192);
	for(int i = 0; i < 2; i ++) {
		buff[19-i] = (byte) ((my>>(i*8))&0xFF);
	}
	//sprintf(sbuf, "%04X, ", my&0xFFFF);
	//Serial.print(sbuf);
	
	// magnetometer z
	int mz = (int) (m_z/100 * 8192);
	for(int i = 0; i < 2; i ++) {
		buff[21-i] = (byte) ((mz>>(i*8))&0xFF);
	}
	//sprintf(sbuf, "%04X", mz&0xFFFF);
	//Serial.println(sbuf);
	
	// Serial.print("Stored: ");
	// for(int i = 0; i < 22; i ++) {
		// sprintf(sbuf, "%02X", buff[i]);
		// Serial.print(sbuf);
	// }
	// Serial.println();
	
	// Serial.print("Actual: ");
	// Serial.print(t);
	// Serial.print("; ");
	// Serial.print(a_x*4, 4);
	// Serial.print('\t');
	// Serial.print(a_y*4, 4);
	// Serial.print('\t');
	// Serial.print(a_z*4, 4);
	// Serial.print('\t');
	// Serial.print(g_x, 4);
	// Serial.print('\t');
	// Serial.print(g_y, 4);
	// Serial.print('\t');
	// Serial.print(g_z, 4);
	// Serial.print('\t');
	// Serial.print(m_x/100, 6);
	// Serial.print('\t');
	// Serial.print(m_y/100, 6);
	// Serial.print('\t');
	// Serial.println(m_z/100, 6);
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
