// Chip Specific settings to interface battery status and LCD

#if defined(SEEED_XIAO_M0)  // Seeed XIAO
  #define SDA           4     
  #define SCL           5
  #define TFT_CS        2
  #define TFT_RST       -1     // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC        3
  #define TFT_MOSI      10    // Data out
  #define TFT_SCLK      8     // Clock out
  #define BUTTON_PIN    1
  #define VAL_MCU       "Seeed Xiao M0"

#elif defined(ARDUINO_XIAO_ESP32C3)  // Seeed XIAO ESP32 C3
  #define SDA           6     
  #define SCL           7
  #define TFT_CS        9  
  #define TFT_DC        3 
  #define TFT_MOSI      10    // Data out
  #define TFT_SCLK      8     // Clock out  
  #define TFT_RST       4     // Or set to -1 and connect to Arduino RESET pin 
  #define TFT_BL        5
  #define BUTTON_PIN    2
  #define VAL_MCU       "Seeed Xiao ESP32 C3"
  #include "bat_stat.h"
  #define BAT_ADJ       3.5

#elif defined(ARDUINO_ARCH_ESP32)
  #define SDA         21     
  #define SCL         22
  #define TFT_MOSI    13 // In some display driver board, it might be written as "SDA" and so on.
  #define TFT_SCLK    14
  #define TFT_CS      15  // Chip select control pin
  #define TFT_DC      2  // Data Command control pin
  #define TFT_RST     -1  // Reset pin (could connect to Arduino RESET pin)
  #define TFT_BL      27  // LED back-light
  #define TOUCH_CS    33     // Chip select pin (T_CS) of touch screen
  #define BUTTON_PIN  -1
  #define VAL_MCU       "ESP32 Elecrow"
  #include "bat_stat.h"
  #define BAT_ADJ       15.0

#elif defined(ARDUINO_TINYS3)
  #define TFT_SDA       8    
  #define TFT_SCL       9
  #define TFT_MISO      37
  #define TFT_MOSI      35    // Data out
  #define TFT_SCLK      36    // Clock out  #define 
  #define TFT_RST       -1    // Or set to -1 and connect to Arduino RESET pin                                            
  #define TFT_DC        3
  #define TFT_CS        34
  #define BUTTON_PIN    4
  #define VAL_MCU       "UM Tiny S3 ESP32"
  #include "bat_stat.h"
  #define BAT_ADJ       1.9
    
#elif defined(ARDUINO_ESP32C3_DEV)
  #define SDA       4   
  #define SCL       5
  #define TFT_MOSI      7
  #define TFT_SCLK      6
  #define TFT_CS        10  // Chip select control pin
  #define TFT_DC        2  // Data Command control pin
  #define TFT_RST      -1  // Reset pin (could connect to RST pin)
  #define TFT_BL        3
  #define BUTTON_PIN    9
  #define VAL_MCU       "ESP32-C3-2424S0122"
  #include "bat_stat.h"
  #define BAT_ADJ       1.9

#elif defined(ARDUINO_TTGO_T_OI_PLUS_DEV)  //Lilygo OI
  #define SDA           19    
  #define SCL           18

  // Traditional Pin Setup
  #define TFT_MOSI      6    // Data out
  #define TFT_SCLK      4    // Clock out  #define 
  #define TFT_DC        10
  #define TFT_CS        5    //Unused MISO pin

  /*// Hybrid Straight Line PCB Setup
  #define TFT_CS        4
  #define TFT_DC        5
  #define TFT_MOSI      6    // Data out
  #define TFT_SCLK      10   // Clock out  */
  
  #define TFT_RST       -1   // Or set to -1 and connect to Arduino RESET pin
  #define BUTTON_PIN    9
  #define VAL_MCU       "TTGO T-OI PLUS RISC-V ESP32-C3"
  #include "bat_stat.h"
  #define BAT_ADJ       1.0
  
#else
  // For the breakout board, you can use any 2 or 3 pins.
  // These pins will also work for the 1.8" TFT shield.
  //#define TFT_RST       -1    // Or set to -1 and connect to Arduino RESET pin                                            
  //#define TFT_DC        4
  //#define TFT_CS        10
  #define VAL_MCU       "Unknown"
  #define SDA         21     
  #define SCL         22
  #define TFT_MOSI    13 // In some display driver board, it might be written as "SDA" and so on.
  #define TFT_SCLK    14
  #define TFT_CS      15  // Chip select control pin
  #define TFT_DC      2  // Data Command control pin
  #define TFT_RST     -1  // Reset pin (could connect to Arduino RESET pin)
  #define TFT_BL      27  // LED back-light
  #define TOUCH_CS    33     // Chip select pin (T_CS) of touch screen
  #define BUTTON_PIN  4
  #include "bat_stat.h"
  #define BAT_ADJ       4.0

#endif