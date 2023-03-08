/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-spi-communication-arduino/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

//Find the default SPI pins for your board
//Make sure you have the right board selected in Tools > Boards

// Libraries
#include <Arduino.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
  Serial.print("SCL: ");
  Serial.println(SCL);
  Serial.print("SDA: ");
  Serial.println(SDA);
  Serial.print("SS: ");
  Serial.println(SS);  
}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.begin(115200);
  Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
  Serial.print("SCL: ");
  Serial.println(SCL);
  Serial.print("SDA: ");
  Serial.println(SDA);
  Serial.print("SS: ");
  Serial.println(SS);  
}