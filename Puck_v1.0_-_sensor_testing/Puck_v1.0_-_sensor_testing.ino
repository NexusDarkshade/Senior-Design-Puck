/*
 * Senior Design Project - Educational Puck
 * 
 * Operates the built in board sensors
 * Reads and Writes to SD card on board storage
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

const int CS_PIN = 13;    // Chip select pin for SD card
/*----------------------------------------------------------------------------------------------------*/

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Started");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
/*
  SerialFlash.begin(CS_PIN);
//SD card initialized - need to figure out!!!
  if(!SD.begin(CS_PIN)){
    SerialUSB.println("SD INIT FAILED");
    while(1);
  }
  else{
    SerialUSB.println("SD INIT SUCCESS");
  }
*/
 
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

// For blink setup (testing)
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // Variables for storing sensor values
  float a_x, a_y, a_z;   //Accelerometer
  float g_x, g_y, g_z;  //Gyroscope
  float m_x, m_y, m_z;   //Magnetometer

  // From example code - read sensor to variable
  IMU.readAcceleration(a_x, a_y, a_z);
  IMU.readGyroscope(g_x, g_y, g_z);
  IMU.readMagneticField(m_x, m_y, m_z);

  Serial.print(a_x);
  Serial.print('\t');
  Serial.print(a_y);
  Serial.print('\t');
  Serial.print(a_z);
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

  
/*  
  //BLINK (testing - acknowledge in loop)
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);  
*/
}
