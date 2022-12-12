/*
 * ARPA-E Project wireless communication device code.
 * 
 * Microcontroller: Teensy3.6
 * Uses Arduino environment
 * 
 * Author: Bryan Hatasaka, Merek Goodrich
 * Some skeleton and setup code from PJRC Paul Stoffregen 
 * Libraries: RadioHead supplied with Teensyduino - modified to work with the 
 * Teensy 3.6 and RF95 LoRa module
 * 
 */

#include "Arpa_RF95.h"

#define THIS_NODE_ID 1

//For Teensy 3.x and T4.x the following format is required to operate correctly
//This is a limitation of the RadioHead radio drivers


#if defined(__AVR_ATmega328P__)
  #define RFM95_INT     3  // 
  #define RFM95_CS      10  //
  #define RFM95_RST     2  // "A"
  #define RFM95_EN 5
#elif defined(TEENSYDUINO)
  #define RFM95_RST 4
  #define RFM95_EN 5
  #define RFM95_CS 10
  #define RFM95_INT digitalPinToInterrupt(3)
#elif defined(__AVR_ATmega32U4__)
  #define RFM95_RST 4
  #define RFM95_EN 5
  #define RFM95_CS 3
  #define RFM95_INT digitalPinToInterrupt(2)
#endif

#define RFM95_POWER 23
#define RFM95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);
//----- END TEENSY CONFIG

// Class to manage message delivery and receipt, using the driver declared above
Arpa_RF95 lora(&driver, RFM95_RST, RFM95_FREQ, RFM95_POWER, RFM95_EN, THIS_NODE_ID);

void setup()
{
  Serial.begin(9600);
  delay(200); // Wait for Serial but don't require it

  if (!lora.InitModule())
    Serial.println("LoRa couldn't be initialized");
}

void loop()
{
  lora.HandleMessageForwarding();
}
