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

#define THIS_NODE_ID 4

//For Teensy 3.x and T4.x the following format is required to operate correctly
//This is a limitation of the RadioHead radio drivers


#if defined(__AVR_ATmega328P__)
  #define RFM95_INT     3  // 
  #define RFM95_CS      10  //
  #define RFM95_RST     2  // "A"
  #define RFM95_EN 5
  int rled = A7;
  int wled = A6;
  #define R_LED_ON HIGH
  #define R_LED_OFF LOW
  #define W_LED_ON HIGH
  #define W_LED_OFF LOW
#elif defined(TEENSYDUINO) // __MK66FX1M0__
  #define RFM95_RST 4
  #define RFM95_EN 5
  #define RFM95_CS 10
  #define RFM95_INT digitalPinToInterrupt(3)
  int rled = 33;
  int wled = 16;
  #define R_LED_ON HIGH
  #define R_LED_OFF LOW
  #define W_LED_ON HIGH
  #define W_LED_OFF LOW
#elif defined(ARDUINO_SAMD_MKRZERO)
  #define RFM95_RST 6
  #define RFM95_EN 4
  #define RFM95_CS 7
  #define RFM95_INT digitalPinToInterrupt(5)
  int rled = A1;
  int wled = 3;
  #define R_LED_ON HIGH
  #define R_LED_OFF LOW
  #define W_LED_ON HIGH
  #define W_LED_OFF LOW
#elif defined(SEEED_XIAO_M0)
  #define RFM95_RST 4
  #define RFM95_EN 2
  #define RFM95_CS 5
  #define RFM95_INT digitalPinToInterrupt(3)
  int rled = LED_BUILTIN; // builtin led, active LOW
  int wled = 1;
  #define R_LED_ON LOW
  #define R_LED_OFF HIGH
  #define W_LED_ON HIGH
  #define W_LED_OFF LOW
#elif defined(STM32F411xE)//BLACKPILL_F411CE
  #define RFM95_RST PB0
  #define RFM95_EN PA2
  #define RFM95_CS PA4
  #define RFM95_INT digitalPinToInterrupt(PA3)
  int rled = PB9;
  int wled = PC13; // builtin led, active low
  #define R_LED_ON HIGH
  #define R_LED_OFF LOW
  #define W_LED_ON LOW
  #define W_LED_OFF HIGH
#elif defined(ARDUINO_AVR_NANO_EVERY)
  #define RFM95_INT     digitalPinToInterrupt(3)  // 
  #define RFM95_CS      4  //
  #define RFM95_RST     5  // "A"
  #define RFM95_EN 2
  int rled = A1;
  int wled = A0;
  #define R_LED_ON HIGH
  #define R_LED_OFF LOW
  #define W_LED_ON HIGH
  #define W_LED_OFF LOW
#else
  #define RFM95_INT     digitalPinToInterrupt(3)  // 
  #define RFM95_CS      10  //
  #define RFM95_RST     9  // "A"
  #define RFM95_EN 5
  int rled = A5;
  int wled = A6;
  #define R_LED_ON HIGH
  #define R_LED_OFF LOW
  #define W_LED_ON HIGH
  #define W_LED_OFF LOW
#endif

#define RFM95_POWER 23
#define RFM95_FREQ 915.0

bool SendLoraMessage(char* data);
void SetupLEDs();

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);
//----- END TEENSY CONFIG

// Class to manage message delivery and receipt, using the driver declared above
Arpa_RF95 lora(&driver, RFM95_RST, RFM95_FREQ, RFM95_POWER, RFM95_EN, THIS_NODE_ID);

void setup()
{
  randomSeed(THIS_NODE_ID);
  SetupLEDs();
  
  Serial.begin(9600);
  delay(1000); // Wait for Serial but don't require it
  Serial.print("Node ");
  Serial.print(THIS_NODE_ID);
  Serial.println(" starting");

  if (!lora.InitModule())
  {
    Serial.println("LoRa couldn't be initialized");
    digitalWrite(rled, R_LED_ON);
  }
  else
  {
    Serial.println("LoRa initialized successfully");
  }

  delay(1000L * random(10, 15));
}

// Dont put this on the stack:
char buf[ARPA_MAX_MSG_LENGTH];
uint8_t len = ARPA_MAX_MSG_LENGTH;
Arpa_msg_type msgType = ARPA_TYPE_ID_SYN;

void loop()
{
//  sprintf(buf, "from=%d", THIS_NODE_ID);
  for(int i = 0; i < ARPA_MAX_MSG_LENGTH; i++)
    buf[i] = 'a'; // Fill the buffer with 249 'a' characters
  buf[ARPA_MAX_MSG_LENGTH-1] = '\0'; // NULL terminate the buffer
    
  digitalWrite(wled, W_LED_ON);
  // Send the message and make sure it sent
  bool success = SendLoraMessage(buf);
  digitalWrite(wled, W_LED_OFF);
  
  if(success)
  {
    delay(1000L * random(10, 15));
    digitalWrite(rled, R_LED_OFF);
    lora.ResetFailureToSendDelay();
  }
  else
  {
    // Unsuccessful, turn on the red led
    digitalWrite(rled, R_LED_ON);
    lora.FailureToSendDelay();
  }

}

void SetupLEDs()
{
  pinMode(rled, OUTPUT);
  pinMode(wled, OUTPUT);
  digitalWrite(rled, R_LED_OFF);
  digitalWrite(wled, W_LED_ON);
  delay(100);
  digitalWrite(wled, W_LED_OFF);
}

bool SendLoraMessage(char* data)
{
  Serial.println("Calling Synchronize()");
  if (lora.Synchronize())
  {
    Serial.println();
    Serial.println("===== Sync Success =====");
    Serial.println();

    Serial.println("===== Calling SendMessage() and sending a data type message =====");
    switch (lora.SendConnectedMessage(lora.GetBaseId(), ARPA_TYPE_ID_DATA, data))
    {
    case ARPA_TYPE_ID_ACK:
      // Got a good response from the base
      Serial.println("===== Data Success! =====");

      Serial.println("===== Calling lora.Close() =====");

      if (lora.Close())
      {
        Serial.println("===== Close Success =====");
        return true;
      }
      else
        Serial.println("===== Could not close connection =====");
      break;

    case ARPA_TYPE_ID_NACK:
      // Got a nack - we don't have a current connection with the base
      Serial.println("===== Data fail - not connected to base =====");
      return false;
      break;

    case ARPA_TYPE_ID_INVALID:
    default:
      Serial.println("===== Data message failed :( =====");
      return false;
      break;
    }
  }
  else
  {
    Serial.println("===== Could not synchronize =====");
    return false;
  }
  return true;
}
