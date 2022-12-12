
/*
 * ARPA-E Project wireless communication device code for testing.
 *  
 * In this code, gas interrupt is alway 1, signal is constantly sent
 * from MCU toLoRa module and sent to Gateway. Finally publishing on the Internet.
 * 
 * Microcontroller: STM32L051K8T6
 * Uses Arduino environment
 * 
 * Author: Bryan Hatasaka, Merek Goodrich
 * Some skeleton and setup code from PJRC Paul Stoffregen 
 * Libraries: RadioHead supplied with Teensyduino - modified to work with the 
 * Microcontroller STM32L051K8T6 and RF95 LoRa module
 * 
 */

#include "Arpa_RF95.h"
#include "Configuration.h"

//#include <SPI.h>
#include <EEPROM.h>

#define THIS_NODE_ID 1

#define RFM95_CS      PB0
#define RFM95_RST     PB1
#define RFM95_EN      PA3
#define RFM95_INT     digitalPinToInterrupt(PA4)    //connected with LoRa G0 pin

#define TX_PIN        PA9     //PA9 and PA10 are for configuration (set up with configurator)
#define RX_PIN        PA10

/*
int rled = PA8;
int wled = PA2;
#define R_LED_ON HIGH
#define R_LED_OFF LOW
#define W_LED_ON HIGH
#define W_LED_OFF LOW
*/

#define RFM95_POWER 20
#define RFM95_FREQ 915.0
#define LTE_UART_BAUD 57600

bool SendLoraMessage(char* data);
void SetupNode();
void NodeLoop();
void SetupForwarder();
void SetupBase();
void BaseLoop();
//void SetupLEDs();

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);
//----- END TEENSY/STM32 CONFIG

// Class to manage message delivery and receipt, using the driver declared above
Arpa_RF95 lora(&driver, RFM95_RST, RFM95_FREQ, RFM95_POWER, RFM95_EN, EEPROM.read(1));
Configuration configuration;

void setup()
{
  // Setup STM32 Serial stuff
  Serial.setRx(RX_PIN);
  Serial.setTx(TX_PIN);
  
  pinMode(PB7, OUTPUT);

  pinMode(RX_PIN, INPUT);

  // If the RX pin is HIGH, then the MCU automatically goes into configuration mode.
  // This means any time a serial chip is connected, it will go into configuration mode.
  // Comment this out to disable this feature. However, the chip will not be configurable if this is
  // commented out
  //
  // To manually program the NodeId, BaseId, and Node Type, a program can be flashed that calls
  // the functions in the configuration.h file and sets the proper ids.
  // Then, this program can be flashed back
  /*if(digitalRead(RX_PIN) == HIGH)
  {
    // serial configuration
    configuration.StartConfiguration(RX_PIN);
  }*/
 
  Serial.begin(9600);

  auto nt = configuration.GetEEPromNodeType();
  switch(nt)
  {
    case Configuration::sensor: 
      SetupNode();
      NodeLoop();
      break;
    case Configuration::forwarder:
      SetupForwarder();
      lora.HandleMessageForwarding();
      break;
    case Configuration::base:
      SetupBase();
      BaseLoop();
      break;
    case Configuration::invalid:
    default:
      while(true) 
      {
        Serial.begin(9600);
        Serial.print("Unrecognized Node type: ");
        Serial.println(configuration.GetEEPromNodeType());
        delay(1000);
      }
  }

}

// Unused
void loop()
{
}

// Node stuff
char buf[ARPA_MAX_MSG_LENGTH];
uint8_t len = ARPA_MAX_MSG_LENGTH;
Arpa_msg_type msgType = ARPA_TYPE_ID_SYN;

// Base
int16_t currentConnectionId;

void SetupNode()
{
  randomSeed(THIS_NODE_ID);
  //SetupLEDs();
  
  Serial.begin(9600);
  //delay(1000); // Wait for Serial but don't require it

  Serial.println();
  Serial.print("SensorNode ");
  Serial.print(configuration.GetEEPromNodeId());
  Serial.println(" starting");

  if (!lora.InitModule())
  {
    Serial.println("LoRa couldn't be initialized");
    //digitalWrite(rled, R_LED_ON);
  }
  else
  {
    lora.SetNodeId(configuration.GetEEPromNodeId());
    lora.SetBaseId(configuration.GetEEPromBaseId());
    Serial.println("LoRa initialized successfully");
  }
  //delay(500);
}

void NodeLoop()
{
  while(1)
  {
   sprintf(buf, "gas=1");
   delay(5000); 
    /*Test that fills the buffer full
    for(int i = 0; i < ARPA_MAX_MSG_LENGTH; i++)
       buf[i] = 'a'; // Fill the buffer with 249 'a' characters
    buf[ARPA_MAX_MSG_LENGTH-1] = '\0'; // NULL terminate the buffer */
      
    //digitalWrite(wled, W_LED_ON);
    // Send the message and make sure it sent
    bool success = SendLoraMessage(buf);
    //digitalWrite(wled, W_LED_OFF);
    
    if(success)
    {
      delay(1000);
      lora.ResetFailureToSendDelay();
    }
    else
    {
      lora.FailureToSendDelay();
    }
  }
}

bool SendLoraMessage(char* data)
{
  Serial.println();
  Serial.println("Calling Synchronize()");
  if (lora.Synchronize())
  {
    Serial.println("===== Sync Success =====");

    Serial.println("===== Calling SendMessage() and sending a data type message =====");
    switch (lora.SendConnectedMessage(lora.GetBaseId(), ARPA_TYPE_ID_DATA, data))
    {
    case ARPA_TYPE_ID_ACK:
      // Got a good response from the base
      Serial.println("===== Data sent Success! =====");

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
      Serial.println();
      Serial.println();
      return false;
      break;
    }
  }
  else
  {
    Serial.println("===== Could not synchronize =====");
    Serial.println();
    Serial.println();
    
    return false;
  }
  return true;
}

void SetupForwarder()
{
  Serial.begin(9600);
  //delay(200); // Wait for Serial but don't require it

  Serial.println();
  Serial.print("Forwarder ");
  Serial.print(configuration.GetEEPromNodeId());
  Serial.println(" starting");

  if (!lora.InitModule())
    Serial.println("LoRa couldn't be initialized");

  lora.SetNodeId(configuration.GetEEPromNodeId());
  lora.SetBaseId(configuration.GetEEPromBaseId());
  Serial.println("LoRa initialized successfully");
}

void SetupBase()
{
  if(!Serial)
    Serial.begin(9600);
  // Serial1.begin(LTE_UART_BAUD);

  if (!lora.InitModule())
  {
    while(true)
    {
      Serial.println("LoRa couldn't be initialized");
      delay(1000);
    }
  }

  Serial.println("LoRa Initialized sucessfully");

  lora.SetNodeId(configuration.GetEEPromNodeId());
}

void BaseLoop()
{
  while(true)
  {
    Serial.println("===== Waiting for syn =====");
    Serial.println("");
    currentConnectionId = lora.WaitForSyn();

    if (currentConnectionId >= 0)
    {
      Serial.print("===== Got syn from:  ");
      Serial.println(currentConnectionId);

      // Time to wait for messages
      Serial.println("===== Waiting for data =====");
      while (lora.GetCurrentConnectionOriginId() >= 0)
      {
        Serial.print("===== Current connection id: ");
        Serial.println(lora.GetCurrentConnectionOriginId());
        // The len variable is used as a input in the WaitForMessage func and must be set every call
        len = ARPA_MAX_MSG_LENGTH;
        msgType = lora.WaitForConnectedMessage((uint8_t *)buf, &len);
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
          //sendToLTE(buf, currentConnectionId);
          // Serial1.print(currentConnectionId);
          // Serial1.print((char *)buf);
          // Serial1.print(';');
        }
      }
    }
  }
}

/*void SetupLEDs()
{
  pinMode(rled, OUTPUT);
  pinMode(wled, OUTPUT);
  digitalWrite(rled, R_LED_OFF);
  digitalWrite(wled, W_LED_ON);
  delay(100);
  digitalWrite(wled, W_LED_OFF);
}*/
