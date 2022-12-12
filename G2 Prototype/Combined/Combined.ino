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
#include <EEPROM.h>
#include "STM32LowPower.h"

#define RFM95_INT     digitalPinToInterrupt(PA4)
#define RFM95_CS      PB0
#define RFM95_RST     PB1
#define RFM95_EN PA3
#define RFM95_POWER 23
#define RFM95_FREQ 915.0

int gled = PA0;
int wled = PA12;
#define G_LED_ON (digitalWrite(gled, LOW))
#define G_LED_OFF (digitalWrite(gled, HIGH))
#define W_LED_ON (digitalWrite(wled, HIGH))
#define W_LED_OFF (digitalWrite(wled, LOW))

#define TX_PIN PA9
#define RX_PIN PA10
#define GAS_PIN PA11

#define LTE_UART_BAUD 57600

#define BASE_ID_0 0
#define BASE_ID_1 1

uint8_t THIS_NODE_ID = EEPROM.read(1);

bool SendLoraMessage(char* data);
void SetupLEDs();
void SetupInterrupts();

void gasDetected();

void SetupNode();
void NodeLoop();
void SetupForwarder();
void SetupBase();
void BaseLoop();

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);
//----- END TEENSY CONFIG

// Class to manage message delivery and receipt, using the driver declared above
Arpa_RF95 lora(&driver, RFM95_RST, RFM95_FREQ, RFM95_POWER, RFM95_EN, EEPROM.read(1));
Configuration configuration;

volatile int gasWakeup = -1;

void setup()
{
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
  SetupLEDs();
  LowPower.begin();
  SetupInterrupts();
  
  Serial.begin(9600);
  delay(1000); // Wait for Serial but don't require it
  Serial.print("Node ");
  Serial.print(THIS_NODE_ID);
  Serial.println(" starting");

  if (!lora.InitModule())
  {
    Serial.println("LoRa couldn't be initialized");
  }
  else
  {
    lora.SetNodeId(configuration.GetEEPromNodeId());
    lora.SetBaseId(configuration.GetEEPromBaseId());
    Serial.println("LoRa initialized successfully");
  }

  // delay(1000L * random(10, 15));
  gasWakeup = -1;
  delay(1000);
}

void NodeLoop()
{
  sprintf(buf, "gas=1");
  int prevGasWakeup = LOW;
  while(1)
  {
    // Sleep until we get an interrupt
    gasWakeup = digitalRead(GAS_PIN);
    // LowPower.sleep(0);
    if(gasWakeup == prevGasWakeup)
      continue;

    prevGasWakeup = gasWakeup;

    if(gasWakeup == HIGH)
    {
      Serial.println("Gas wakeup HIGH");
      G_LED_ON;
      W_LED_ON;

      // Send the message and ignore the return
      Serial.println("Sending LoRas");
      lora.SetBaseId(BASE_ID_0);
      SendLoraMessage(buf);
      lora.SetBaseId(BASE_ID_1);
      SendLoraMessage(buf);

      if(digitalRead(GAS_PIN) == LOW)
      {
        Serial.println("Gas wakeup LOW after sending");
        G_LED_OFF;
        W_LED_OFF;
      }

    }
    else // If the interrupt is low
    {
      Serial.println("Gas wakeup LOW");
      G_LED_OFF;
      W_LED_OFF;
    }

  }
}

bool SendLoraMessage(char* data)
{
  if (lora.Synchronize())
  {
    switch (lora.SendConnectedMessage(lora.GetBaseId(), ARPA_TYPE_ID_DATA, data))
    {
    case ARPA_TYPE_ID_ACK:
      // Got a good response from the base
      Serial.println("===== Data Success! =====");

      if (lora.Close())
      {
        return true;
      }

      break;

    case ARPA_TYPE_ID_NACK:
      // Got a nack - we don't have a current connection with the base
      return false;
      break;

    case ARPA_TYPE_ID_INVALID:
    default:
      return false;
      break;
    }
  }
  else
  {
    Serial.println("===== Error: Could not synchronize =====");
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
  pinMode(gled, OUTPUT); // Onboard
  pinMode(wled, OUTPUT); // External

  G_LED_ON;
  W_LED_ON;
  delay(100);
  G_LED_OFF;
  W_LED_OFF;
}

void gasDetected()
{
  gasWakeup = digitalRead(GAS_PIN);
}

void SetupInterrupts()
{
  pinMode(GAS_PIN, INPUT_PULLUP);
  // Attach a wakeup interrupt on pin, calling gasDetected when the device is woken up
  // LowPower.attachInterruptWakeup(GAS_PIN, gasDetected, RISING, SLEEP_MODE );
}