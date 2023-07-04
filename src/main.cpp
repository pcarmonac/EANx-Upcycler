/*****************************************************************************

  EANx Analysis with output to an OLED color display

  Reads an analog input from an ADC, converts it to mV, and creates a running average 
  of ADC values and and prints the result to the display and debug to Serial Monitor.

  Based on prior EANx scripts: 
  https://github.com/ppppaoppp/DIY-Nitrox-Analyzer-04_12_2019.gitf
  https://github.com/ejlabs/arduino-nitrox-analyzer.git

  Brian Ehrler 2022-2023
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
#if defined(OTA_UP)  
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

// Display Definitions
#define TFT_WIDTH 240   // OLED display width, in pixels
#define TFT_HEIGHT 240  // OLED display height, in pixels
#define ResFact 2       // 1 = 128x128   2 = 240x240

// User Interface Settings
#define GUI 1     // 1= on 0= off 
#define metric 0  // 1= on 0= off 

//Init tft and sprites
TFT_eSPI tft = TFT_eSPI();


TFT_eSprite gauge = TFT_eSprite(&tft);  
TFT_eSprite background = TFT_eSprite(&tft);
TFT_eSprite needle = TFT_eSprite(&tft);


// Init ADS
Adafruit_ADS1115 ads;  // Define ADC - 16-bit version

// Running Average definitions
#define RA_SIZE 20           //Define running average pool size
RunningAverage RA(RA_SIZE);  //Initialize Running Average

// Global Variables
int LCDROT = 0; // 0 = default, 1 = CW 90 
float prevaveSensorValue = 0;
float aveSensorValue = 0;
float mVolts = 0;
float batVolts = 0;
float prevO2 = 0;
float currentO2 = 0;
float calFactor = 1;
float calErrChk = 1;
int mod14fsw = 0;
int mod14msw = 0;
int mod16fsw = 0;
int mod16msw = 0;
float modppo = 1.4;
float mod16ppo = 1.6;
float multiplier = 0;
int msgid = 0;

#if GUI == 1
  // Define display colors
  #define backColor TFT_BLACK
  #define gaugeColor 0x055D
  #define dataColor 0x0311
  #define purple 0xEA16
  #define needleColor 0xF811

  #define DEG2RAD 0.0174532925

  // Gauge variables 
  bool o2Pointer = 0;
  bool modPointer = 0;
  float o2Angle = 0; // range 0-100
  float modAngle = 0; // range 10-250 

  int cx=120;
  int cy=140;
  int r=135;
  int ir= (r*.95);
  int n=0;
  int angle=210;
  unsigned short color1;
  unsigned short color2;

  float x[360]; //outer points of Speed gaouges
  float y[360];
  float px[360]; //ineer point of Speed gaouges
  float py[360];
  float lx[360]; //text of Speed gaouges
  float ly[360];
  float nx[360]; //needle low of Speed gaouges
  float ny[360];
#endif


const int buttonPin = BUTTON_PIN;  // push button

//Functions
float batStat();

void BatGauge(int locX, int locY, float batV) {
  // Draw the outline and clear the box
  tft.drawRect (locX, locY, 25, 12, TFT_WHITE); 
  tft.drawRect ((locX + 25), (locY + 4), 3, 4, TFT_WHITE);
  tft.fillRect ((locX +1), (locY +1), 23, 10, TFT_BLACK); 
  
  // Fill with the color that matches the charge state 
  if (batV > 3.4 and batV < 3.6) { tft.fillRect ((locX +1), (locY +1), 15, 10, TFT_YELLOW); }
  if (batV < 3.4) { tft.fillRect ((locX +1), (locY +1), 10, 10, TFT_RED); }
  if (batV > 3.6) { tft.fillRect ((locX +1), (locY +1), 23, 10, TFT_GREEN); }
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
    delay(5000);
}
  
void BattCheck() {
    debugln("Low V reading from Battery");
    tft.fillScreen(TFT_YELLOW);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1 * ResFact);
    tft.drawCentreString("Error", TFT_WIDTH * 0.5, TFT_HEIGHT * 0, 4);
    tft.drawCentreString("Battery", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.3, 4);
    tft.drawCentreString("LOW", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.6, 4);
    tft.setTextSize(1);
    tft.drawCentreString(String(mVolts), TFT_WIDTH * .5, TFT_HEIGHT * 0.8, 4);
    delay(5000);
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
      delay(12);  // was 8
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
      delay(36); // was 24
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
      tft.fillScreen(TFT_CYAN);
      tft.setTextColor(TFT_BLACK);
      tft.setTextSize(1 * ResFact);
      tft.drawCentreString("+++++++++++++", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.1, 2);
      tft.drawCentreString("Refining", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.25, 2);
      tft.drawCentreString("Calibration", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.45, 2);
      tft.drawCentreString("Standby", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.65, 2);
      tft.drawCentreString("+++++++++++++", TFT_WIDTH * 0.5, TFT_HEIGHT * 0.80, 2);
      tft.drawCentreString(String(abs((calFactor / calErrChk) - 1)*100), TFT_WIDTH * 0.85 , TFT_HEIGHT * 0.90, 1);
      delay(2000);
      cal = 1;
    }
    else  {
      cal = 0;
    }
  }
  while (cal);

  calFactor = (1 / RA.getAverage() * 20.900);  // Auto Calibrate to 20.9%
}


void textBaseLayout() {
  // Draw Layout -- Adjust this layouts to suit you LCD  
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

#if GUI == 1
  void gaugeBaseLayout() {
    // Draw Layout -- Adjust this layouts to suit you LCD  
    gauge.createSprite(240,100);
    gauge.setSwapBytes(true);
    gauge.setTextDatum(4);
    gauge.setTextColor(TFT_WHITE,backColor);    
    needle.createSprite(240,100);
    needle.setSwapBytes(true);
    background.createSprite(240,100);

    background.fillSprite(TFT_BLACK);
    
    for(int i=0;i<360;i++) {
        x[i]=((r-10)*cos(DEG2RAD*angle))+cx;
        y[i]=((r-10)*sin(DEG2RAD*angle))+cy;
        px[i]=((r-14)*cos(DEG2RAD*angle))+cx;
        py[i]=((r-14)*sin(DEG2RAD*angle))+cy;
        lx[i]=((r-24)*cos(DEG2RAD*angle))+cx;
        ly[i]=((r-24)*sin(DEG2RAD*angle))+cy;
        nx[i]=((r-36)*cos(DEG2RAD*angle))+cx;
        ny[i]=((r-36)*sin(DEG2RAD*angle))+cy;

        angle++;
        if(angle==360)
        angle=0;
      }

    gauge.drawSmoothArc(cx, cy, r, ir, 120, 240, gaugeColor, backColor);
    gauge.drawSmoothArc(cx, cy, r-5, r-6, 120, 240, TFT_WHITE, backColor);
    gauge.drawSmoothArc(cx, cy, r-9, r-8, 120, 140, TFT_RED, backColor);
    gauge.drawSmoothArc(cx, cy, r-38, ir-37, 110, 250, gaugeColor, backColor);

    //.....................................................draw GAUGES
    for(int i=0;i<21;i++){
      //if(i<2) {color1=TFT_RED; color2=TFT_RED;} else {color1=TFT_GREEN; color2=TFT_GREEN;}

      if (i*5 <= 21) { color1=TFT_CYAN, color2=TFT_CYAN; }
      if (i*5  <= 18) { color1=TFT_RED; color2=TFT_YELLOW; }
      if (i*5  >= 22) { color1=TFT_GREEN; color2=TFT_GREEN; }
      int wedginc = 6;
      if(i%2==0) {
      gauge.drawWedgeLine(x[i*wedginc],y[i*wedginc],px[i*wedginc],py[i*wedginc],2,1,color1);
      gauge.setTextColor(color1,backColor);
      gauge.drawString(String(i*5),lx[i*wedginc],ly[i*wedginc]);
      } else
      gauge.drawWedgeLine(x[i*wedginc],y[i*wedginc],px[i*wedginc],py[i*wedginc],1,1,color2);
      }

    gauge.pushSprite(0,40);


  }
#endif

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
  delay(2000);
  tft.fillScreen(TFT_BLACK);
}

void displayUtilData() {
    BatGauge((TFT_WIDTH * 0.8), (TFT_HEIGHT * 0.05), (batVolts));
    tft.setTextSize(1);
    if (mVolts > 7.5 and mVolts <= 9.0) { tft.setTextColor(TFT_YELLOW, TFT_BLACK); }
    if (mVolts <= 7.5) { tft.setTextColor(TFT_RED, TFT_BLACK); }
    if (mVolts > 9.0) { tft.setTextColor(TFT_GREEN, TFT_BLACK); }
    if (mVolts < 4.0) { SenseCheck(); }
    String mv = String(mVolts, 1);
    tft.drawString(String(mv + " mV "), TFT_WIDTH * 0.05, TFT_HEIGHT * 0.1, 2);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    if (batVolts > 3.4 and mVolts < 3.6) { tft.setTextColor(TFT_YELLOW, TFT_BLACK); }
    if (batVolts < 3.4) { tft.setTextColor(TFT_RED, TFT_BLACK); }
    if (batVolts > 3.6) { tft.setTextColor(TFT_GREEN, TFT_BLACK); }
    String bv = String(batVolts, 1);
    if (batVolts < 3.2) { BattCheck(); }
    tft.drawString(String(bv + " V  "), TFT_WIDTH * 0.05, TFT_HEIGHT * 0, 2);
    //tft.drawString(String(millis() / 1000), TFT_WIDTH * 0.05, TFT_HEIGHT * 0, 2);
}

void displayTextData() {
  // Generate Text layout
  if (prevO2 != currentO2) {
    if (currentO2 > 20 and currentO2 < 22) { tft.setTextColor(TFT_CYAN, TFT_BLACK); }
    if (currentO2 <= 20) { tft.setTextColor(TFT_YELLOW, TFT_BLACK); }
    if (currentO2 <= 18) { tft.setTextColor(TFT_RED, TFT_BLACK); }
    if (currentO2 >= 22) { tft.setTextColor(TFT_GREEN, TFT_BLACK); }

    // Draw Text Layout -- Adjust these layouts to suit you LCD
    tft.setTextSize(1 * ResFact);
    String o2 = String(currentO2, 1);
    tft.drawCentreString(o2, TFT_WIDTH * 0.5, TFT_HEIGHT * 0.2, 7);

    tft.setTextSize(1 * ResFact);

    if (metric == 1)  {
      String mod14m = String(mod14msw);
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.drawString(String(mod14m + "-m  "), TFT_WIDTH * 0.05, TFT_HEIGHT * 0.83, 2);
      tft.setTextColor(TFT_GOLD, TFT_BLACK);
      String mod16m = String(mod16msw);
      tft.drawString(String(mod16m + "-m  "), TFT_WIDTH * 0.65, TFT_HEIGHT * 0.83, 2);
    }
    else {
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      String mod14f = String(mod14fsw);
      tft.drawString(String(mod14f + "-FT  "), TFT_WIDTH * 0, TFT_HEIGHT * 0.72, 2);
      tft.setTextColor(TFT_GOLD, TFT_BLACK);
      String mod16f = String(mod16fsw);
      tft.drawString(String(mod16f + "-FT  "), TFT_WIDTH * 0.6, TFT_HEIGHT * 0.72, 2);
    }

  }
}

#if GUI == 1
  void  displayGaugeData() {

  // Draw gauge needle
  int sA=currentO2*1.2;
  //int bA=prevO2;
  //int sA=o2Angle;
  //int bA=o2Angle -1;
  //needle.drawWedgeLine(px[(int)bA],py[(int)bA],nx[(int)bA],ny[(int)bA],2,2,backColor);
  //gauge.pushSprite(0,40,needleColor);
  needle.drawWedgeLine(px[(int)sA],py[(int)sA],nx[(int)sA],ny[(int)sA],2,2,TFT_RED);

  // o2Angle++;
  // if(o2Angle==120) {
  //   o2Angle=0; 
  // }

  gauge.pushToSprite(&background,0,0,TFT_BLACK);
  needle.pushToSprite(&background,0,0,TFT_BLACK);

  //gauge.pushSprite(0,40,TFT_BLACK);
  // needle.pushSprite(0,40);
  //delay(100);
  background.pushSprite(0,40);
  // gauge.pushSprite(0,40);  
  //needle.deleteSprite();
  //needle.fillSprite(TFT_BLACK);
  needle.fillSprite(TFT_BLACK);
  needle.pushToSprite(&background,0,0);

  // Text info
  if (prevO2 != currentO2) {
    if (currentO2 > 20 and currentO2 < 22) { tft.setTextColor(TFT_CYAN, TFT_BLACK); }
    if (currentO2 <= 20) { tft.setTextColor(TFT_YELLOW, TFT_BLACK); }
    if (currentO2 <= 18) { tft.setTextColor(TFT_RED, TFT_BLACK); }
    if (currentO2 >= 22) { tft.setTextColor(TFT_GREEN, TFT_BLACK); }

    // Text info
    tft.setTextSize(1 * ResFact);
    String o2 = String(currentO2, 1);
    tft.drawCentreString(o2, TFT_WIDTH * 0.5, TFT_HEIGHT * 0.6, 4);

    tft.setTextSize(1 * ResFact);

    if (metric == 1)  {
      String mod14m = String(mod14msw);
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.drawString(String(mod14m + "-m  "), TFT_WIDTH * 0.05, TFT_HEIGHT * 0.83, 2);
      tft.setTextColor(TFT_GOLD, TFT_BLACK);
      String mod16m = String(mod16msw);
      tft.drawString(String(mod16m + "-m  "), TFT_WIDTH * 0.65, TFT_HEIGHT * 0.83, 2);
    }
    else {
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      String mod14f = String(mod14fsw);
      tft.drawString(String(mod14f + "-FT  "), TFT_WIDTH * 0, TFT_HEIGHT * 0.83, 2);
      tft.setTextColor(TFT_GOLD, TFT_BLACK);
      String mod16f = String(mod16fsw);
      tft.drawString(String(mod16f + "-FT  "), TFT_WIDTH * 0.6, TFT_HEIGHT * 0.83, 2);
    }


    }
  }
#endif

void setup() {

  Serial.begin(115200);
  // Call our validation to output the message (could be to screen / web page etc)
  printVersionToSerial();

  pinMode(buttonPin, INPUT_PULLUP);
  debugln("Pinmode Init");

  //setup TFT
   debugln("Pre TFT Init");
  tft.init();
  debugln("TFT Init");
  tft.setRotation(LCDROT);
  tft.fillScreen(TFT_BLACK);

  debugln("Display Initialized");

  tft.fillScreen(TFT_BLACK);
  testfillcircles(5, TFT_BLUE);
  testdrawcircles(5, TFT_WHITE);
  delay(500);


  #if defined(OTA_UP)  
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
  #if defined(OTA_UP)  
    tft.drawCentreString((WiFi.localIP().toString()), TFT_WIDTH * 0.5, TFT_HEIGHT * 0.9, 4);
    Serial.println(WiFi.localIP());
  #endif
  delay(1500);
  tft.fillScreen(TFT_BLACK);

  // setup display and calibrate unit
  initADC();
  debugln("Post ADS check statement");
  o2calibration();
  safetyrule();
  #if GUI == 1
    gaugeBaseLayout(); //Graphic Layout
  #else
    textBaseLayout();  //Text Layout
  #endif

  debugln("Setup Complete");
}

// the loop routine runs over and over again forever:
void loop() {

  multiplier = initADC();
  
  int bstate = digitalRead(buttonPin);
  // debugln(bstate);
  if (bstate == LOW) {
    if (LCDROT == 0) {
      LCDROT = 2; 
      }
    else {
      LCDROT = 0;
    }
    tft.setRotation(LCDROT);
    tft.fillScreen(TFT_BLACK);
    textBaseLayout();
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
  
  mod14fsw = 33 * ((modppo / (currentO2 / 100)) - 1);
  mod14msw = 10 * ((modppo / (currentO2 / 100)) - 1);
  mod16fsw = 33 * ((mod16ppo / (currentO2 / 100)) - 1);
  mod16msw = 10 * ((mod16ppo / (currentO2 / 100)) - 1);

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
  debug("MOD 1.4:");
  debug(mod14fsw);
  debug("\t");
  debug("MOD 1.6:");
  debugln(mod16fsw);

  displayUtilData();

  #if GUI == 1  
    displayGaugeData(); //Graphic Layout
  #else 
    displayTextData();  //Text Layout
  #endif
}