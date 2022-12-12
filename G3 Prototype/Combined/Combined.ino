
/*
 * ARPA-E Project wireless communication device code.
 *
 * Microcontroller: STM32L051K8T6
 * Uses Arduino environment, but can use the STM32L051C8T6
 * the only difference is the k chip has 32 pins where the C chip has 48
 *
 *
 * Author: Bryan Hatasaka && Leo Wang
 * Some skeleton and setup code from PJRC Paul Stoffregen
 * Libraries:
 *  RadioHead -https://www.airspayce.com/mikem/arduino/RadioHead/
 *  stm32duino - https://github.com/stm32duino/Arduino_Core_STM32
 *  stm32lowpower - https://github.com/stm32duino/STM32LowPower
 *
 */
 
#include <low_power.h>
#include <STM32LowPower.h>

#include "Arpa_RF95.h"
#include "Configuration.h"
#include "stm32yyxx_ll_exti.h"

#include <rtc.h>
#include <EEPROM.h>
#include <STM32RTC.h>
#include <low_power.h>
#include <STM32LowPower.h>

#define RFM95_CS  PB0
#define RFM95_RST PB1
#define RFM95_EN  PA3   //PA3 is not physically conneced on board
#define RFM95_INT digitalPinToInterrupt(PA4)    //connected with LoRa G0 pin

#define TX_PIN PA9    //PA9 and PA10 are for configuration (set up with configurator)
#define RX_PIN PA10

#define GAS_INT PA0  //PA0 is used for G3 gas interrupt input

#define GAS_INT_EXTI LL_EXTI_LINE_11

#define RFM95_POWER 20
#define RFM95_FREQ 915.0
#define LTE_UART_BAUD 57600

bool SendLoraMessage(char *data);
void SetupNode();
void NodeLoop();
void SetupForwarder();
void SetupBase();
void BaseLoop();
void Sleep();
void SetupLowPower();
void GasPinInt();


// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);
//----- END STM32 CONFIG

// Class to manage message delivery and receipt, using the driver declared above
Arpa_RF95 lora(&driver, RFM95_RST, RFM95_FREQ, RFM95_POWER, RFM95_EN, EEPROM.read(1));
Configuration configuration;

void setup()
{
  // Setup STM32 Serial stuff
  Serial.setRx(RX_PIN);
  Serial.setTx(TX_PIN);

  pinMode(RX_PIN, INPUT);

  /* If the RX pin is HIGH, then the MCU automatically goes into configuration mode.
     This means any time a serial chip is connected, it will go into configuration mode.
     Comment this out to disable this feature. However, the chip will not be configurable 
     if this is commented out
  
     To manually program the NodeId, BaseId, and Node Type, a program can be flashed that calls
     the functions in the configuration.h file and sets the proper ids.
     Then, this program can be flashed back
   */
  
  /*if (digitalRead(RX_PIN) == HIGH)
  {
    // serial configuration
    configuration.StartConfiguration(RX_PIN);
  }*/

  Serial.begin(9600);

  auto nt = configuration.GetEEPromNodeType();
  switch (nt)
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
    while (true)
    {
      Serial.begin(9600);
      Serial.print("Unrecognized Node type: ");
      Serial.println(configuration.GetEEPromNodeType());
      delay(1000);
    }
  }
}

// Unused but here to satisfy arduino
void loop()
{
}

// Node stuff
char buf[ARPA_MAX_MSG_LENGTH];
uint8_t len = ARPA_MAX_MSG_LENGTH;
Arpa_msg_type msgType = ARPA_TYPE_ID_SYN;
bool hexanalDetected = false;

// Base stuff
int16_t currentConnectionId;

void SetupNode()
{
  SetupLowPower();

  Serial.begin(9600);
  Serial.print("SensorNode ");
  Serial.print(configuration.GetEEPromNodeId());
  Serial.println(" starting");

  lora.SetNodeId(configuration.GetEEPromNodeId());
  lora.SetBaseId(configuration.GetEEPromBaseId());

  while (!lora.InitModule())
  {
    Serial.println("LoRa couldn't be initialized");
    delay(5000); // If loRa can't be initialized, keep trying every 5 seconds
  }

  Serial.println("LoRa initialized successfully");
  Serial.println();
}

// This node loop will just sleep immediately.
// When it wakes up, if the hexanalDetected flag is set,
// then it will send a message over lora "gas=1"
void NodeLoop()
{
  while (1)
  {
    Sleep();
    
    if (hexanalDetected)
    {
      sprintf(buf, "gas=1");
      lora.SetSleepState(false); //wake up the LoRa module

      // Send the message and make sure it sent
      SendLoraMessage(buf);

      hexanalDetected = false;
    }
  }
}

bool SendLoraMessage(char *data)
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

  if (!lora.InitModule())
    Serial.println("LoRa couldn't be initialized");

  lora.SetNodeId(configuration.GetEEPromNodeId());
  lora.SetBaseId(configuration.GetEEPromBaseId());
}

void SetupBase()
{
  if (!Serial)
    Serial.begin(9600);

  if (!lora.InitModule())
  {
    while (true)
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
  while (true)
  {
    Serial.println("===== Waiting for syn =====");
    Serial.println("");
    currentConnectionId = lora.WaitForSyn();

    if (currentConnectionId >= 0)
    {
      Serial.print("===== Got syn from:  ");
      Serial.println(currentConnectionId);

      // Time to wait for messages
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

        if (msgType == ARPA_TYPE_ID_DATA)
        {
          // Send to LTE module
          Serial.print(currentConnectionId);
          Serial.print((char *)buf);
          Serial.print(';');
        }
      }
    }
  }
}

// Puts the MCU to sleep.
// When the gas pin goes high (RISING), it will wake up,
// and the GasPinInt() function is called.
//
// https://github.com/stm32duino/STM32LowPower for more details
void Sleep()
{
  lora.SetSleepState(true);//Set LoRa module to sleep mode
  LowPower.attachInterruptWakeup(GAS_INT, GasPinInt, RISING, DEEP_SLEEP_MODE);
  // ready to set the MCU to sleep mode
  LowPower.deepSleep();
  // GasPinInt() is called one time after the interrupt is triggered
}

void SetupLowPower()
{
  pinMode(GAS_INT, INPUT);
  LowPower.begin();
}

// When the MCU wakes up, this function is called.
// All it does is disables the interrupt (so it doesn't go off again until we put the processor to sleep)
// and clears the right flag in the processor
//
// The 'hexanalDetected' flag is set.
// When this function exits, it will exit back to the nodeLoop and
// from there, the code sends the lora message.
void GasPinInt()
{
  detachInterrupt(GAS_INT);
  LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_11);

  hexanalDetected = true;
}
