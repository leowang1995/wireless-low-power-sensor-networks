/*
 * ARPA-E Project wireless communication device code.
 * 
 * Microcontroller: Teensy3.6
 * Uses Arduino environment
 * 
 * Author: Bryan Hatasaka, Merek Goodrich, Sayali Tope
 * Some skeleton and setup code from PJRC Paul Stoffregen 
 * Libraries:
 * RadioHead supplied with Teensyduino - modified to work with the hardware
 * Snooze supplied with Teensyduino
 * Sparkfun TeensyView
 * 
 * Hardware:
 * Teensy 3.6
 * Adafruit RFM95W LoRa module breakout
 * Sparkfun TeensyView OLED
 * 
 */

#include <TeensyView.h>
#include <Snooze.h>
#include "Arpa_RF95.h"

#define THIS_NODE_ID 100

// RFM95W LoRa breakout pins
#define RFM95_RST 4
#define RFM95_EN 5
#define RFM95_CS 10
#define RFM95_INT digitalPinToInterrupt(3)
#define RFM95_POWER 23
#define RFM95_FREQ 915.0

// Oled Pins
#define OLED_RESET 25
#define OLED_DC    24
#define OLED_CS    9
#define OLED_SCK   13
#define OLED_MOSI  11

// ===== Function Declarations =====
void oled_init();
void oled_print(const char *text);
void oled_print_error(const char *text, int code);

void snooze_init();

bool SendLoraMessage(char* data);
bool SendLoraMessageWithDelayedRetry(char *data);
bool SendLoraGasNotif();

// ===== Global Variables =====
// OLED vars
TeensyView oled(OLED_RESET, OLED_DC, OLED_CS, OLED_SCK, OLED_MOSI);
char oled_buf[128];

// Snooze variables
const uint8_t sensor_wakeup_pin = 21;
const uint8_t button_wakeup_pin = 22;
SnoozeDigital digital;
SnoozeBlock config(digital);

// Lora configuration and vars
// Singleton instance of the lora radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);
// Class to manage message delivery and receipt, using the driver declared above
Arpa_RF95 lora(&driver, RFM95_RST, RFM95_FREQ, RFM95_POWER, RFM95_EN, THIS_NODE_ID);

void setup()
{ 
  // Init Snooze
  snooze_init();

  // Init OLED
  oled_init();
  oled_print("University of Utah");

  Serial.begin(9600);
  delay(1000); // Wait for Serial but don't require it

  // Init LoRa
  Serial.print("Node ");
  Serial.print(THIS_NODE_ID);
  Serial.println(" starting");
  if (!lora.InitModule())
  {
    Serial.println("LoRa couldn't be initialized");
    oled_print_error("LoRa fail init", 300);
    while(1) {}
  }
  else
  {
    Serial.println("LoRa initialized successfully");
  }

  lora.SetSleepState(true);
}

// LoRa loop vars
char buf[ARPA_MAX_MSG_LENGTH];
uint8_t len = ARPA_MAX_MSG_LENGTH;
Arpa_msg_type msgType = ARPA_TYPE_ID_SYN;

// ===== Superloop =====
void loop()
{
  oled.clear(ALL);
  Snooze.deepSleep(config); // Sleep until woken by either the button or sensor

  oled_print("Gas detected");
  delay(2000);

  // Send notification of gas
  if(lora.SetSleepState(true)) // Wake up module
  {
    oled_print("Gas detected\nSending data");
    if(SendLoraGasNotif())
    {
      oled_print("Gas detected\nData sent!");
    }
    else
    {
      oled_print_error("Data cant sent", 302);
      oled_print("Sleeping...");
    }
  }
  else
    oled_print_error("LoRa fail init", 300);

  lora.SetSleepState(false); // Sleep module

  delay(10000); // Delay to show message
}

bool SendLoraMessageWithDelayedRetry(char *data)
{
  while(true)
  {
    // Send the message and make sure it sent
    bool success = SendLoraMessage(data);
    
    if(success)
    {
      lora.ResetFailureToSendDelay();
      return true;
    }
    else
    {
      // Unsuccessful - delay according to protocol then resend
      if(!lora.FailureToSendDelay())
      {
        // Tried and failed to send too many times
        // give up
        lora.ResetFailureToSendDelay();
        return false;
      }
    }
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
      oled_print_error("Got NACK", 301);
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
    oled_print("Can't sync");
    delay(500);
    oled_print("Retrying");
    return false;
  }
  return true;
}

bool SendLoraGasNotif()
{
  sprintf(buf, "gas=1");
  return SendLoraMessageWithDelayedRetry(buf);
}

void oled_init()
{
  oled.begin();    // Initialize the OLED
  oled.clear(ALL); // Clear the display's internal memory
  oled.clear(PAGE); // Clear the buffer.
}

void oled_print(const char *text)
{
  oled.clear(PAGE);  // Clear the page
  oled.setFontType(1);  // Set font to type 1
  oled.setCursor(0, 0); // move cursor
  oled.print(text);  // Write a string

  oled.display();  // Send the PAGE to the OLED memory
}

// Prints out the error text for 5 seconds, then
// the code for another 5 seconds
void oled_print_error(const char *text, int code)
{
  sprintf(oled_buf, "Problem!\n%s", text);
  oled_print(oled_buf);
  delay(5000);
  sprintf(oled_buf, "Problem!\nError code %d", code);
  oled_print(oled_buf);
  delay(5000);
}

void snooze_init()
{
  digital.pinMode(sensor_wakeup_pin, INPUT, HIGH);
  digital.pinMode(button_wakeup_pin, INPUT_PULLUP, LOW);
}
