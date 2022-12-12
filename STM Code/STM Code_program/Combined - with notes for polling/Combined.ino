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
#include "Configuration.h"
#include "STM32LowPower.h"
#include "stm32yyxx_ll_exti.h"

#include <EEPROM.h>

#define THIS_NODE_ID 4

#define RFM95_INT     digitalPinToInterrupt(PA4)
#define RFM95_CS      PB0
#define RFM95_RST     PB1
#define RFM95_EN PA3
int rled = PA0;
int wled = PA1;
#define R_LED_ON HIGH
#define R_LED_OFF LOW
#define W_LED_ON HIGH
#define W_LED_OFF LOW
#define TX_PIN PA9
#define RX_PIN PA10

#define GAS_INT PA11
#define GAS_INT_EXTI LL_EXTI_LINE_11

#define RFM95_POWER 23
#define RFM95_FREQ 915.0
#define LTE_UART_BAUD 57600

bool SendLoraMessage(char* data);
void SetupLEDs();
void SetupNode();
void NodeLoop();
void SetupForwarder();
void SetupBase();
void BaseLoop();
void SetupAnalogInputs();
void Sleep();
void SetupLowPower();
void GasPinInt();

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);
//----- END TEENSY CONFIG

// Class to manage message delivery and receipt, using the driver declared above
Arpa_RF95 lora(&driver, RFM95_RST, RFM95_FREQ, RFM95_POWER, RFM95_EN, EEPROM.read(1));
Configuration configuration;

void setup()
{
  SetupAnalogInputs();
  // Setup STM32 Serial stuff
  Serial.setRx(RX_PIN);
  Serial.setTx(TX_PIN);
  
  pinMode(PB7, OUTPUT);

  pinMode(RX_PIN, INPUT);
  if(digitalRead(RX_PIN) == HIGH)
  {
    // serial configuration
    configuration.StartConfiguration(RX_PIN);
  }

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
bool gasDetected = false;

// Base
int16_t currentConnectionId;

void SetupNode()
{
  randomSeed(THIS_NODE_ID);
  SetupLEDs();
  SetupLowPower();
  
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
    lora.SetNodeId(configuration.GetEEPromNodeId());
    lora.SetBaseId(configuration.GetEEPromBaseId());
    Serial.println("LoRa initialized successfully");
  }

  // delay(1000L * random(10, 15));
  delay(1000);
}

{
  // turn on power to hexanol circuit
  digitalWrite(HEXANOL_POWER_PIN, HIGH);

  // wait to stabilize
  delay(1 second);

  // check the status of the hexanol pin
  if(digitalRead(GAS_INT) == HIGH)
    hexanolDetected = true;
  else 
    hexanolDetected = false;
    hexanolSent = false;
  
  // turn off power to hexanol circuit
  digitalWrite(HEXANOL_POWER_PIN, LOW);
}

bool checkEthlyene()
{
  // turn on power to hexanol circuit
  digitalWrite(HEXANOL_POWER_PIN, HIGH);
  // enable i2c bus peripheral

  // wait to stabilize
  delay(100ms);
  
  Library.checkethylene();
  
  // disable i2c bus peripheral
  // turn off power to hexanol circuit
  digitalWrite(HEXANOL_POWER_PIN, LOW);
}

void NodeLoop()
{
  hexanalSent = false;
  while(1)
  {
    Sleep(10 seconds);
    
    checkHexanal();
    checkIndole();
    checkEthylene();

    if((hexanolDetected && !hexanalSent)
      || indoleDetected || ethylenenedjffjdklsa detected)
      .... send message
      hexanalSent = true;

    // if(gasDetected)
    // {
      sprintf(buf, "hexanal=1");
      // Send the message and make sure it sent
      bool success = SendLoraMessage(buf);
      
    //   if(success)
    //   {
    //     digitalWrite(rled, R_LED_OFF);
    //   }

    //   gasDetected = false;
    // }
  }
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

void SetupForwarder()
{
  Serial.begin(9600);
  delay(200); // Wait for Serial but don't require it

  if (!lora.InitModule())
    Serial.println("LoRa couldn't be initialized");

  lora.SetNodeId(configuration.GetEEPromNodeId());
  lora.SetBaseId(configuration.GetEEPromBaseId());
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
      // Serial.println("===== Waiting for data =====");
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
  //      sendToLTE(buf, currentConnectionId);
          // Serial1.print(currentConnectionId);
          // Serial1.print((char *)buf);
          // Serial1.print(';');
        }
      }
    }
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

void Sleep()
{
  LowPower.attachInterruptWakeup(GAS_INT, GasPinInt, RISING, DEEP_SLEEP_MODE);
  LowPower.deepSleep();
}

void SetupLowPower()
{
  pinMode(GAS_INT, INPUT);
  LowPower.begin();
}

void GasPinInt()
{
  detachInterrupt(GAS_INT);
  LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_11);

  gasDetected = true;
}

void SetupAnalogInputs()
{
   analogRead(PA0);
   analogRead(PA1);
   analogRead(PA2);
   analogRead(PA8);
   analogRead(PA9);
   analogRead(PA10);
   analogRead(PA11);
   analogRead(PA12);
   analogRead(PA13);
   analogRead(PA14);
   analogRead(PA15);
   analogRead(PB3);
   analogRead(PB4);
   analogRead(PB5);
   analogRead(PB6);
   analogRead(PB7);
   analogRead(PC14);
   analogRead(PC15);
}
