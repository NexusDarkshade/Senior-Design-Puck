/*
 * Senior Design Project - Educational Puck
 * 
 * Operates the built in board sensors
 * 
 * Justin Rubalcaba, Undergraduate Student
 * Electrical Engineering Department, Montana Technological University
 * jrubalcaba@mtech.edu
 * October 20, 2021
 * 
 */

// Include Required Libraries
#include <Arduino_LSM9DS1.h>    // IMU sensor
#include <SPI.h>                // SPI communication
#include <SD.h>                 // SD card control
#include <SerialFlash.h>        // Flash memory control

File myFile;                    // Data file for storing sensor measures

const int CS_PIN = 10;          // Chip select pin for SD card (D10)

// Variables for storing sensor values
float a_x, a_y, a_z;  //Accelerometer
float g_x, g_y, g_z;  //Gyroscope
float m_x, m_y, m_z;  //Magnetometer

String sensData;      //Buffer to hold sensor data before writing to SD
/*----------------------------------------------------------------------------------------------------*/

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Started");

  // Initialize Sensors
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  //Serial Readout Heading - Probably unnecessary
  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");
  Serial.print("Magnetic field sample rate = ");
  Serial.print(IMU.magneticFieldSampleRate());
  Serial.println(" uT");
  
  Serial.print("Acceleration in G's\t");
  Serial.print("Gyroscope in deg/sec\t");
  Serial.println("Magnetic Field in uT");
  Serial.println();
  
  Serial.print("X\tY\tZ\t");
  Serial.print("X\tY\tZ\t");
  Serial.println("X\tY\tZ");
  
  // Initialize SD card
  initSD();
  clearSD();
  
}



void loop() {
  for(int i=0; i<10; i++){ //This is just to limit how much data is being written for testing purposes
    writeSD();  //this function calls the sensorRead to write data to file
  }
  while(1); // trap to prevent multiple looping
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
  SerialUSB.print("Clearing Stored Data...");
  if(SD.exists("test.txt")){  //remove SD test file - can delete later
    SD.remove("test.txt");
    SerialUSB.println("test.txt removed");
  }
  if(SD.exists("DATA.txt")){
    SD.remove("DATA.txt");
    SerialUSB.println("data.txt removed");
  }
}


void writeSD(){
  myFile = SD.open("DATA.txt",FILE_WRITE);
  if(myFile){
    SerialUSB.print("Writing to DATA.txt...");
    myFile.println(sensorRead());
    myFile.close();
    SerialUSB.println("Done");
  }
  else{
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
  Serial.write(myFile.read());
  }
  myFile.close();
  } 
  else {
Serial.println("Error opening DATA.txt");
  }
}



String sensorRead(){
  IMU.readAcceleration(a_x, a_y, a_z);
  IMU.readGyroscope(g_x, g_y, g_z);
  IMU.readMagneticField(m_x, m_y, m_z);
  
//convert float data to string, 3 decimal places // 3 decimal places is arbitrary, change to appropriate rounding
  String ax = String(a_x,3);
  String ay = String(a_y,3);
  String az = String(a_z,3);
  String gx = String(g_x,3);
  String gy = String(g_y,3);
  String gz = String(g_z,3);
  String mx = String(m_x,3);
  String my = String(m_y,3);
  String mz = String(m_z,3);

  sensData = ax + "," + ay + "," + az + "," + gx + "," + gy + "," + gz + "," + mx + "," + my + "," + mz;
  return sensData;
}
