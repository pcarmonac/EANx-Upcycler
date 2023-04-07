/*****************************************************************************

  EANx Analysis with output to an OLED color display

  Reads an analog input from an ADC, converts it to mV, and creates a running average 
  of ADC values and and prints the result to the display and debug to Serial Monitor.

  Based on prior EANx scripts: 
  https://github.com/ppppaoppp/DIY-Nitrox-Analyzer-04_12_2019.gitf
  https://github.com/ejlabs/arduino-nitrox-analyzer.git

*****************************************************************************/

// Libraries
#include <Arduino.h>
#include <Wire.h>
#include <RunningAverage.h>
#include <SPI.h>
#include <Adafruit_GFX.h>  // Core graphics library
#include <TFT_eSPI.h>
#include <Adafruit_ADS1X15.h>
#include "pin_config.h"
#include "version.h"
#if defined(ESP32)  
  #include "ElegantOTA.h"
#endif

//Debugging
#define DEBUG 1

#if DEBUG == 1
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
#else
  #define debug(x)
  #define debugln(x)
#endif

// display definitions
#define TFT_WIDTH 240   // OLED display width, in pixels
#define TFT_HEIGHT 240  // OLED display height, in pixels
#define ResFact 2       // 1 = 128x128   2 = 240x240
#define LCDROT 0 // 0 = default, 1 = CW 90 

TFT_eSPI tft = TFT_eSPI();

Adafruit_ADS1115 ads;  // Define ADC - 16-bit version

// Running Average definitions
#define RA_SIZE 20           //Define running average pool size
RunningAverage RA(RA_SIZE);  //Initialize Running Average

// Global Variables
float prevaveSensorValue = 0;
float aveSensorValue = 0;
float mVolts = 0;
float batVolts = 0;
float prevO2 = 0;
float currentO2 = 0;
float calFactor = 1;
float calErrChk = 1;
int modfsw = 0;
int modmsw = 0;
int modmaxfsw = 0;
int modmaxmsw = 0;
float modppo = 1.4;
float modmaxppo = 1.6;
float multiplier = 0;
int msgid = 0;

const int buttonPin = BUTTON_PIN;  // push button

//Functions
float batStat();

void BatGauge(int locX, int locY, float batV) {
  // Draw the outline and clear the box
  tft.drawRect (locX, locY, 25, 12, TFT_WHITE); 
  tft.drawRect ((locX + 25), (locY + 4), 3, 4, TFT_WHITE);
  tft.fillRect ((locX +1), (locY +1), 23, 10, TFT_BLACK); 
  
  // Fill with the color that matches the charge state 
  if (batV > 3.6 and batV < 3.8) { tft.fillRect ((locX +1), (locY +1), 15, 10, TFT_YELLOW); }
  if (batV < 3.6) { tft.fillRect ((locX +1), (locY +1), 10, 10, TFT_RED); }
  if (batV > 3.8) { tft.fillRect ((locX +1), (locY +1), 23, 10, TFT_GREEN); }
}

void SenseCheck() {
    debugln("Low mV reading from Sensor");
    tft.fillScreen(TFT_YELLOW);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1 * ResFact);
    tft.drawCentreString("Error", TFT_WIDTH * 0.5, TFT_HEIGHT * 0, 4);
    tft.drawCentreString("Sensor mV", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.3, 4);
    tft.drawCentreString("LOW", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.6, 4);
    tft.setTextSize(1);
    tft.drawCentreString(String(mVolts), TFT_WIDTH * .5, TFT_HEIGHT * 0.8, 4);
    delay(30000);
}

float initADC() {
  // init ADC and Set gain

  // The ADC input range (or gain) can be changed via the following
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads.setGain(GAIN_TWO);  // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

  /* Be sure to update this value based on the IC and the gain settings! */
  //float   multiplier = 3.0F;    /* ADS1015 @ +/- 6.144V gain (12-bit results) */
  float multiplier = 0.0625; /* ADS1115  @ +/- 6.144V gain (16-bit results) */

  // Check that the ADC is operational
  if (!ads.begin()) {
    debugln("Failed to initialize ADC.");
    tft.fillScreen(TFT_YELLOW);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1 * ResFact);
    tft.drawCentreString("Error", TFT_WIDTH * 0.5, TFT_HEIGHT * 0, 4);
    tft.drawCentreString("ADC", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.3, 4);
    tft.drawCentreString("Fail", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.6, 4);
    delay(5000);
    while (1);
  }
  return (multiplier);
}

void o2calibration() {
  int cal = 1;
  do {
    //display "Calibrating"
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1 * ResFact);
    tft.drawCentreString("+++++++++++++", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.1, 2);
    tft.drawCentreString("Calibrating", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.3, 2);
    tft.drawCentreString("O2 Sensor", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.6, 2);
    tft.drawCentreString("+++++++++++++", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.8, 2);
    debugln("Calibration Screen Text");

    // get running average value from ADC input Pin
    RA.clear();
    for (int x = 0; x <= (RA_SIZE * 3); x++) {
      int sensorValue = 0;
      sensorValue = abs(ads.readADC_Differential_0_1());
      RA.addValue(sensorValue);
      delay(8);
    }
    debug("average calibration read ");
    debugln(RA.getAverage());  // average cal factor serial print for debugging

    calFactor = RA.getAverage();
    delay(1000); // Slow the loop for checksum

    //Checksum on calibrate ... is the sensor still reseting from earlier read 
      RA.clear();
    for (int x = 0; x <= (RA_SIZE * 3); x++) {
      int sensorValue = 0;
      sensorValue = abs(ads.readADC_Differential_0_1());
      RA.addValue(sensorValue);
      delay(24);
    }

    tft.fillScreen(TFT_BLACK);

    calErrChk = RA.getAverage();
    debug("calibration raw values Calfactor=");
    debug(calFactor);  // average cal factor
    debug(" CalErrChk="); 
    debugln(calErrChk);  // average cal err factor serial print for debugging

    debugln(abs((calFactor / calErrChk) - 1)*100); 

    // checking against a 0.15% error
    if(abs((calFactor / calErrChk) - 1)*100 > 0.15) {  
      debugln("Calibration Checksum out of spec");
      tft.fillScreen(TFT_DARKCYAN);
      tft.setTextColor(TFT_BLACK);
      tft.setTextSize(1 * ResFact);
      tft.drawCentreString("+++++++++++++", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.1, 2);
      tft.drawCentreString("Refining", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.25, 2);
      tft.drawCentreString("Calibration", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.45, 2);
      tft.drawCentreString("Standby", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.65, 2);
      tft.drawCentreString("+++++++++++++", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.80, 2);
      tft.drawCentreString(String(abs((calFactor / calErrChk) - 1)*100), TFT_WIDTH * 0.85 , TFT_HEIGHT * 0.90, 1);
      delay(5000);
      cal = 1;
    }
    else  {
      cal = 0;
    }
  }
  while (cal);

  calFactor = (1 / RA.getAverage() * 20.900);  // Auto Calibrate to 20.9%
}

// Draw Layout -- Adjust this layouts to suit you LCD
void printLayout() {
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);  
  tft.setTextSize(1 * ResFact);
  tft.drawCentreString("O %", TFT_WIDTH * 0.5, TFT_HEIGHT * 0, 4);
  tft.setTextSize(1);
  tft.drawCentreString("2", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.1, 4);
    tft.setTextSize(1 * ResFact);
  //tft.setTextColor(TFT_GREY, TFT_BLACK);
  //tft.drawString("Info", TFT_WIDTH * 0.10, TFT_HEIGHT * 0.60, 2);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawCentreString("@1.4  MOD  @1.6", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.6, 2);
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x = radius; x < tft.width(); x += radius * 2) {
    for (int16_t y = radius; y < tft.height(); y += radius * 2) {
      tft.fillCircle(x, y, radius, color);
    }
  }
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (int16_t x = 0; x < tft.width() + radius; x += radius * 2) {
    for (int16_t y = 0; y < tft.height() + radius; y += radius * 2) {
      tft.drawCircle(x, y, radius, color);
    }
  }
}

void safetyrule() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(1 * ResFact);
  randomSeed(millis());
  int randNumber = random(5);
  if (randNumber == 0) {
    tft.drawCentreString("Seek proper", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.1, 2);
    tft.drawCentreString("training", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.25, 2);
  } else if (randNumber == 1) {
    tft.drawCentreString("Maintain a", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.1, 2);
    tft.drawCentreString("continious", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.25, 2);
    tft.drawCentreString("guideline to", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.4, 2);
    tft.drawCentreString("the surface", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.55, 2);
  } else if (randNumber == 2) {
    tft.drawCentreString("Stay within", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.1, 2);
    tft.drawCentreString("your depth", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.25, 2);
    tft.drawCentreString("limitations", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.4, 2);
  } else if (randNumber == 3) {
    tft.drawCentreString("Proper gas", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.1, 2);
    tft.drawCentreString("management", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.25, 2);
  } else {
    tft.drawCentreString("Use appropriate", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.1, 2);
    tft.drawCentreString("properly maintaned", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.25, 2);
    tft.drawCentreString("equipment", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.4, 2);
  }
  delay(3000);
  tft.fillScreen(TFT_BLACK);
}

void setup() {

  Serial.begin(115200);
  // Call our validation to output the message (could be to screen / web page etc)
  printVersionToSerial();

  pinMode(buttonPin, INPUT_PULLUP);
  debugln("Pinmode Init");

  //setup TFT
  tft.init();  
  debugln("TFT Init");
  tft.setRotation(LCDROT);
  tft.fillScreen(TFT_BLACK);

  debugln("Display Initialized");

  tft.fillScreen(TFT_BLACK);
  testfillcircles(5, TFT_BLUE);
  testdrawcircles(5, TFT_WHITE);
  delay(500);


  #if defined(ESP32)  
    //Startup Elegant OTA
    ElOTA();
    debugln("OTA Startup");
  #endif

  tft.fillScreen(TFT_GOLD);
  tft.setTextSize(1 * ResFact);
  tft.setTextColor(TFT_BLACK);
  debugln("init display test done");
  tft.drawCentreString("init", TFT_WIDTH * 0.5, TFT_HEIGHT * 0, 4);
  tft.drawCentreString("complete", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.2, 4);
  tft.setTextSize(1);  
  tft.drawCentreString(MODEL, TFT_WIDTH * 0.5, TFT_HEIGHT * 0.45, 4);
  tft.drawCentreString(VERSION, TFT_WIDTH * 0.5, TFT_HEIGHT * 0.6, 4);
  tft.drawCentreString(PROTO, TFT_WIDTH * 0.5, TFT_HEIGHT * 0.75, 4);
  #if defined(ESP32)  
    tft.drawCentreString((WiFi.localIP().toString()), TFT_WIDTH * 0.5, TFT_HEIGHT * 0.9, 4);
    Serial.println(WiFi.localIP());
  #endif
  delay(3000);
  tft.fillScreen(TFT_BLACK);

  // setup display and calibrate unit
  initADC();
  debugln("Post ADS check statement");
  o2calibration();
  safetyrule();
  printLayout();

  debugln("Setup Complete");
}

// the loop routine runs over and over again forever:
void loop() {

  multiplier = initADC();
  
  int bstate = digitalRead(buttonPin);
  // debugln(bstate);
  if (bstate == LOW) {
    o2calibration();
    safetyrule();
    printLayout();
  }

  // get running average value from ADC input Pin
  RA.clear();
  for (int x = 0; x <= RA_SIZE; x++) {
    int sensorValue = 0;
    sensorValue = abs(ads.readADC_Differential_0_1());
    RA.addValue(sensorValue);
    delay(16);
    //debugln(sensorValue);    //mV serial print for debugging
  }
  delay(100);  // slowing down loop a bit

  // Record old and new ADC values
  prevaveSensorValue = aveSensorValue;
  prevO2 = currentO2;
  aveSensorValue = RA.getAverage();

  currentO2 = (aveSensorValue * calFactor);  // Units: pct
  if (currentO2 > 99.9) currentO2 = 99.9;

  mVolts = (aveSensorValue * multiplier);  // Units: mV

  #ifdef ESP32
      batVolts = (batStat() / 1000) * BAT_ADJ;  //Battery Check ESP based boards
  #endif
  
  modfsw = 33 * ((modppo / (currentO2 / 100)) - 1);
  modmsw = 10 * ((modppo / (currentO2 / 100)) - 1);
  modmaxfsw = 33 * ((modmaxppo / (currentO2 / 100)) - 1);
  modmaxmsw = 10 * ((modmaxppo / (currentO2 / 100)) - 1);
  // DEBUG print out the value you read:
  msgid++;  
  debug("Msg_ID:");
  debug(msgid);
  debug("\t");
  debug("ADC:");
  debug(aveSensorValue);
  debug("\t");
  debug("Sensor_mV:");
  debug(mVolts);
  debug("\t");
  debug("Batt_V:");
  debug(batVolts);
  debug("\t");
  debug("O2:");
  debug(currentO2);
  debug("\t");
  debug("MOD:");
  debug(modfsw);
  debug("\t");
  debug("MAX_MOD:");
  debugln(modmaxfsw);

  if (prevO2 != currentO2) {
    if (currentO2 > 20 and currentO2 < 22) { tft.setTextColor(TFT_CYAN, TFT_BLACK); }
    if (currentO2 < 20) { tft.setTextColor(TFT_RED, TFT_BLACK); }
    if (currentO2 > 22) { tft.setTextColor(TFT_GREEN, TFT_BLACK); }

    // Draw Text Layout -- Adjust these layouts to suit you LCD
    tft.setTextSize(1 * ResFact);
    String o2 = String(currentO2, 1);
    tft.drawCentreString(o2, TFT_WIDTH * 0.5, TFT_HEIGHT * 0.2, 7);
    // String bv = String(batVolts, 1);
    //tft.drawString(String(bv + " V  "), TFT_WIDTH * 0.05, TFT_HEIGHT * 0.83, 2);
    BatGauge((TFT_WIDTH * 0.8), (TFT_HEIGHT * 0.05), (batVolts));
    tft.setTextSize(1);
    if (mVolts > 5.0 and mVolts < 9.0) { tft.setTextColor(TFT_YELLOW, TFT_BLACK); }
    if (mVolts < 5.0) { tft.setTextColor(TFT_RED, TFT_BLACK); }
    if (mVolts > 9.0) { tft.setTextColor(TFT_GREEN, TFT_BLACK); }
    if (mVolts < 4.0) { SenseCheck(); }
    String mv = String(mVolts, 1);
    tft.drawString(String(mv + " mV "), TFT_WIDTH * 0.05, TFT_HEIGHT * 0.1, 2);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString(String(millis() / 1000), TFT_WIDTH * 0.05, TFT_HEIGHT * 0, 2);
    tft.setTextSize(1 * ResFact);
    tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
    String modf = String(modfsw);
    tft.drawString(String(modf + "-FT  "), TFT_WIDTH * 0, TFT_HEIGHT * 0.72, 2);
    String modm = String(modmsw);
    tft.drawString(String(modm + "-m  "), TFT_WIDTH * 0.05, TFT_HEIGHT * 0.83, 2);
    tft.setTextColor(TFT_GOLD, TFT_BLACK);
    String modmaxf = String(modmaxfsw);
    tft.drawString(String(modmaxf + "-FT  "), TFT_WIDTH * 0.6, TFT_HEIGHT * 0.72, 2);
    String modmaxm = String(modmaxmsw);
    tft.drawString(String(modmaxm + "-m  "), TFT_WIDTH * 0.65, TFT_HEIGHT * 0.83, 2);
  }
}