/*
 *  battery voltage sketch for TTGO OI / ESP32 and others
 */

#include <Arduino.h>
#include "esp_adc_cal.h"

#if defined(ARDUINO_TINYS3)
    #define BAT_ADC       10  // UM Tiny S3
#elif defined(ARDUINO_XIAO_ESP32C3) 
    #define BAT_ADC       1  // General Batt ADC
#elif defined(ARDUINO_ARCH_ESP32) 
    #define BAT_ADC       2  // General Batt ADC
#else  
    #define BAT_ADC       2  // General Batt ADC
#endif

float BatVoltage = 0.0;
uint32_t readADC_Cal(int ADC_Raw);

float batStat()
{
    BatVoltage = (readADC_Cal(analogRead(BAT_ADC))) * 2;
    //Serial.printf("%.2fV", BatVoltage / 1000.0); // Print Voltage (in V)
    //Serial.println();
    return (BatVoltage);
}

uint32_t readADC_Cal(int ADC_Raw)
{
    esp_adc_cal_characteristics_t adc_chars;

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    return (esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}