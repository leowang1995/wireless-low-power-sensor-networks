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

#define THIS_NODE_ID ARPA_BASE_ID

#if defined(TEENSYDUINO) // __MK66FX1M0__
  #define RFM95_RST 4
  #define RFM95_EN 5
  #define RFM95_CS 10
  #define RFM95_INT digitalPinToInterrupt(3)
#elif defined(SEEED_XIAO_M0)
  #define RFM95_RST 4
  #define RFM95_EN 2
  #define RFM95_CS 5
  #define RFM95_INT digitalPinToInterrupt(3)
#endif
  
#define RFM95_POWER 23
#define RFM95_FREQ 915.0

#define LTE_UART_BAUD 57600


// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);
//----- END TEENSY CONFIG

// Class to manage message delivery and receipt, using the driver declared above
Arpa_RF95 lora(&driver, RFM95_RST, RFM95_FREQ, RFM95_POWER, RFM95_EN, THIS_NODE_ID);

void setup()
{
  Serial.begin(9600);
  Serial1.begin(LTE_UART_BAUD);
  delay(200); // Wait for Serial but don't require it

  if (!lora.InitModule())
    Serial.println("LoRa couldn't be initialized");
}

// Dont put this on the stack:
uint8_t buf[ARPA_MAX_MSG_LENGTH];
uint8_t len = ARPA_MAX_MSG_LENGTH;
Arpa_msg_type msgType;
int16_t currentConnectionId;

void loop()
{
  Serial.println("===== Waiting for syn =====");
  Serial.println("");
  currentConnectionId = lora.WaitForSyn();

  if (currentConnectionId >= 0)
  {
    Serial.print("===== Got syn from:  ");
    Serial.println(currentConnectionId);

    // Time to wait for messages
    // Serial.println("===== Waiting for data =====");
    while (lora.GetCurrentConnectionOriginId() >= 0)
    {
      Serial.print("===== Current connection id: ");
      Serial.println(lora.GetCurrentConnectionOriginId());
      // The len variable is used as a input in the WaitForMessage func and must be set every call
      len = ARPA_MAX_MSG_LENGTH;
      msgType = lora.WaitForConnectedMessage(buf, &len);
      if (msgType == ARPA_TYPE_ID_INVALID)
      {
        Serial.println("===== WaitForConnectedMessage returned invalid =====");
        continue;
      }

      Serial.println("===== Got something =====");
      Serial.print("Type:");
      Serial.println(msgType);
      Serial.print("Message:");
      buf[len] = '\0';
      Serial.println((char *)buf);

      if(msgType == ARPA_TYPE_ID_DATA)
      {
        // Send to LTE module
//      sendToLTE(buf, currentConnectionId);
        Serial1.print(currentConnectionId);
        Serial1.print((char *)buf);
        Serial1.print(';');
      }
    }
  }
}